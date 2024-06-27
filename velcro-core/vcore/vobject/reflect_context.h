#ifndef V_FRAMEWORK_CORE_VOBJECT_REFLECT_CONTEXT_H
#define V_FRAMEWORK_CORE_VOBJECT_REFLECT_CONTEXT_H

#include <vcore/vobject/vobject.h>

// For attributes
#include <vcore/memory/system_allocator.h>
#include <vcore/std/containers/vector.h>
#include <vcore/std/containers/unordered_map.h>
#include <vcore/std/containers/deque.h>
#include <vcore/std/smart_ptr/weak_ptr.h>
#include <vcore/std/smart_ptr/unique_ptr.h>
#include <vcore/std/typetraits/decay.h>
#include <vcore/std/typetraits/function_traits.h>
#include <vcore/std/typetraits/is_function.h>
#include <vcore/std/typetraits/is_member_function_pointer.h>
#include <vcore/std/typetraits/is_same.h>

namespace V
{
    class ReflectContext;

    /// Function type to called for on demand reflection within methods, properties, etc.
    /// OnDemandReflection functions must be static, so we can optimize by just using function pointer instead of object
    using StaticReflectionFunctionPtr = void(*)(ReflectContext* context);

    /// Must be function object, as these are typically virtual member functions (see ComponentDescriptor::Reflect)
    using ReflectionFunction = VStd::function<void(ReflectContext* context)>;

    namespace Internal
    {
        // This needs to be not a pointer, because we're jamming it into shared_ptr, and we don't want a pointer to a pointer
        // #MSVC2013 does not allow using of a function signature.
        typedef void reflection_function_ref(ReflectContext* context);
    }

     class OnDemandReflectionOwner
    {
    public:
        virtual ~OnDemandReflectionOwner();

        // Disallow all copy-move operations, as well as default construct
        OnDemandReflectionOwner() = delete;
        OnDemandReflectionOwner(const OnDemandReflectionOwner&) = delete;
        OnDemandReflectionOwner& operator=(const OnDemandReflectionOwner&) = delete;
        OnDemandReflectionOwner(OnDemandReflectionOwner&&) = delete;
        OnDemandReflectionOwner& operator=(OnDemandReflectionOwner&&) = delete;

        /// Register an OnDemandReflection function
        void AddReflectFunction(V::Uuid typeId, StaticReflectionFunctionPtr reflectFunction);

    protected:
        /// Constructor to be called by child class
        explicit OnDemandReflectionOwner(ReflectContext& context);

    private:
        /// Reflection functions this instance owns references to
        VStd::vector<VStd::shared_ptr<Internal::reflection_function_ref>> m_reflectFunctions;
        ReflectContext& m_reflectContext;
    };

    template<class T>
    struct OnDemandReflection
    {
        void NoSpecializationFunction();
    };

    V_HAS_MEMBER(NoOnDemandReflection, NoSpecializationFunction, void, ())

    class ReflectContext
    {
    public:
        VOBJECT_RTTI(V::ReflectContext, "{B18D903B-7FAD-4A53-918A-3967B3198224}")

        ReflectContext();
        virtual ~ReflectContext() = default;

        void EnableRemoveReflection();
        void DisableRemoveReflection();
        bool IsRemovingReflection() const;

        /// Check if an OnDemandReflection type's typeid is already reflected
        bool IsOnDemandTypeReflected(V::Uuid typeId);

        // Derived classes should override this if they want users to be able to query whether something has already been reflected or not
        virtual bool IsTypeReflected(V::Uuid /*typeId*/) const { return false; }

        /// Execute all queued OnDemandReflection calls
        void ExecuteQueuedOnDemandReflections();

    protected:
        /// True if all calls in the context should be considered to remove not to add reflection.
        bool m_isRemoveReflection;

        /// Store the on demand reflect functions so we can avoid double-reflecting something
        VStd::unordered_map<V::Uuid, VStd::weak_ptr<Internal::reflection_function_ref>> m_onDemandReflection;
        /// OnDemandReflection functions that need to be called
        VStd::vector<VStd::pair<V::Uuid, StaticReflectionFunctionPtr>> m_toProcessOnDemandReflection;
        /// The type ids of the currently reflecting type. Used to prevent circular references. Is a set to prevent recursive circular references.
        VStd::deque<V::Uuid> m_currentlyProcessingTypeIds;

        friend OnDemandReflectionOwner;
    };

     // Attributes to be used by reflection contexts

    /**
    * Base abstract class for all attributes. Use azrtti to get the
    * appropriate version. Of course if NULL there is a data mismatch of attributes.
    */
    class Attribute
    {
    public:
        using ContextDeleter = void(*)(void* contextData);

        VOBJECT_RTTI(V::Attribute, "{2C656E00-12B0-476E-9225-5835B92209CC}");
        Attribute()
            : m_contextData(nullptr, &DefaultDelete)
        { }
        virtual ~Attribute() = default;

        void SetContextData(void* contextData, ContextDeleter destroyer)
        {
            m_contextData = VStd::unique_ptr<void, ContextDeleter>(contextData, destroyer);
        }

        void* GetContextData() const
        {
            return m_contextData.get();
        }

        bool m_describesChildren = false;
        bool m_childClassOwned = false;

    private:
        VStd::unique_ptr<void, ContextDeleter> m_contextData; ///< a generic value you can use to store extra data associated with the attribute

        static void DefaultDelete(void*) { }
    };

    typedef V::u32 AttributeId;
    typedef VStd::pair<AttributeId, Attribute*> AttributePair;
    typedef VStd::vector<AttributePair> AttributeArray;

    template<typename ContainerType>
    inline Attribute* FindAttribute(AttributeId id, const ContainerType& attrArray)
    {
        for (const auto& attrPair : attrArray)
        {
            if (attrPair.first == id)
            {
                return attrPair.second ? &*attrPair.second : nullptr;
            }
        }
        return nullptr;
    }


    /**
    * Generic attribute by value data container. This is the most common attribute.
    */
    template<class T>
    class AttributeData
        : public Attribute
    {
    public:
        VOBJECT_RTTI((AttributeData<T>, "{24248937-86FB-406C-8DD5-023B10BD0B60}", T), Attribute);
        V_CLASS_ALLOCATOR(AttributeData<T>, SystemAllocator, 0);
        template<class U>
        explicit AttributeData(U&& data)
            : m_data(VStd::forward<U>(data)) {}
        virtual const T& Get(void* instance) const { (void)instance; return m_data; }
        T& operator = (T& data) { m_data = data; return m_data; }
        T& operator = (const T& data) { m_data = data; return m_data; }
    private:
        T   m_data;
    };

    /**
    * Generic attribute for class member data, we use the object instance to access member data.
    * In general since the AttributeData::Get function already takes the instance there is no reason to cast (azrtti)
    * to this class (unless you really want to check/know).
    */
    template<class T>
    class AttributeMemberData;

    template<class T, class C>
    class AttributeMemberData<T C::*>
        : public AttributeData<T>
    {
    public:
        VOBJECT_RTTI((V::AttributeMemberData<T C::*>, "{00E5F991-6B96-43CC-9869-F371548581D9}", T, C), AttributeData<T>);
        V_CLASS_ALLOCATOR(AttributeMemberData<T C::*>, SystemAllocator, 0);
        typedef T C::* DataPtr;
        explicit AttributeMemberData(DataPtr p)
            : AttributeData<T>(T())
            , m_dataPtr(p) {}
        const T& Get(void* instance) const override { return (reinterpret_cast<C*>(instance)->*m_dataPtr); }
        DataPtr GetMemberDataPtr() const { return m_dataPtr; }
    private:
        DataPtr m_dataPtr;
    };

    /**
    * Generic attribute global function pointer container. All function must return non void result (we can implement this)
    * as this is attribute and assumed to return something.
    */
    template<class F>
    class AttributeFunction;

    template<class R, class... Args>
    class AttributeFunction<R(Args...)> : public Attribute
    {
    public:
        VOBJECT_RTTI((V::AttributeFunction<R(Args...)>, "{EE535A42-940C-42DE-848D-9C6CE57D8A62}", R, Args...), Attribute);
        V_CLASS_ALLOCATOR(AttributeFunction<R(Args...)>, SystemAllocator, 0);
        typedef R(*FunctionPtr)(Args...);
        explicit AttributeFunction(FunctionPtr f)
                : m_function(f)
        {}

        virtual R Invoke(void* instance, const Args&... args)
        {
            (void)instance;
            return m_function(args...);
        }

        virtual V::Uuid GetInstanceType() const
        {
            return V::Uuid::CreateNull();
        }

        FunctionPtr m_function;
    };

    // Wraps a type that implements the C++ callable concept(i.e has either an overloaded operator() or a function pointer,
    // pointer to member data or pointer to member function)
    // If the type doesn't implement the callable concept stores the type as is
    template<typename Invocable>
    class AttributeInvocable
        : public Attribute
    {
        using Callable = VStd::conditional_t<VStd::function_traits<Invocable>::value, VStd::function<typename VStd::function_traits<Invocable>::function_type>, Invocable>;
    public:
        VOBJECT_RTTI((AttributeInvocable<Invocable>, "{60D5804F-9AF4-4EB1-8F5A-62AFB4883F9D}", Invocable), V::Attribute);
        V_CLASS_ALLOCATOR(AttributeInvocable<Invocable>, SystemAllocator, 0);
        template<typename CallableType>
        explicit AttributeInvocable(CallableType&& invocable)
            : m_callable(VStd::forward<CallableType>(invocable))
        {
        }

        template<typename... FuncArgs>
        decltype(auto) operator()(FuncArgs&&... args)
        {
            if constexpr (VStd::function_traits<Invocable>::value)
            {
                static_assert(VStd::is_invocable_v<Callable, FuncArgs...>, "Attribute stores Invocable type, but cannot be invoked with supplied arguments");
                return VStd::invoke(m_callable, VStd::forward<FuncArgs>(args)...);
            }
            else
            {
                return m_callable;
            }
        }
    private:
        Callable m_callable;
    };

    // Deduction guide for AttributeInvocable type
    // If the type has valid function traits(i.e it fits the callable concept, then the function_type is retrieved from the function type),
    // otherwise the type is used without modification
    template<typename Invocable>
    AttributeInvocable(Invocable&&) -> AttributeInvocable<VStd::conditional_t<VStd::function_traits<Invocable>::value, typename VStd::function_traits<Invocable>::function_type, Invocable>>;

    /**
    * Generic attribute member function pointer container. All function must return non void result (we can implement this)
    * as this is attribute and assumed to return something. The instance to the class will provided vie the invoke function.
    * In general (unless you care) you should use this class vie the AttributeFunction class.
    */
    template<class T>
    class AttributeMemberFunction;

    template<class R, class C, class... Args>
    class AttributeMemberFunction<R(C::*)(Args...)>
        : public AttributeFunction<R(Args...)>
    {
    public:
        VOBJECT_RTTI((V::AttributeMemberFunction<R(C::*)(Args...)>, "{F41F655D-87F7-4A87-9412-9AF4B528B142}", R, C, Args...), AttributeFunction<R(Args...)>);
        V_CLASS_ALLOCATOR(AttributeMemberFunction<R(C::*)(Args...)>, SystemAllocator, 0);
        typedef R(C::* FunctionPtr)(Args...);

        explicit AttributeMemberFunction(FunctionPtr f)
                : AttributeFunction<R(Args...)>(nullptr)
                , m_memFunction(f)
        {}

        R Invoke(void* instance, const Args&... args) override
        {
            return (reinterpret_cast<C*>(instance)->*m_memFunction)(args...);
        }

        V::Uuid GetInstanceType() const override
        {
            return VObject<C>::Id();
        }

        FunctionPtr GetMemberFunctionPtr() const
        {
            return m_memFunction;
        }
    private:
        FunctionPtr m_memFunction;
    };

    // const versions
    template<class R, class C, class... Args>
    class AttributeMemberFunction<R(C::*)(Args...) const>
        : public AttributeFunction<R(Args...)>
    {
    public:
        VOBJECT_RTTI((V::AttributeMemberFunction<R(C::*)(Args...) const>, "{4E21155A-0FB0-4F11-999A-B946B5954A0A}", R, C, Args...), AttributeFunction<R(Args...)>);
        V_CLASS_ALLOCATOR(AttributeMemberFunction<R(C::*)(Args...) const>, SystemAllocator, 0);
        typedef R(C::* FunctionPtr)(Args...) const;

        explicit AttributeMemberFunction(FunctionPtr f)
            : AttributeFunction<R(Args...)>(nullptr)
            , m_memFunction(f)
        {}

        R Invoke(void* instance, const Args&... args) override
        {
            return (reinterpret_cast<const C*>(instance)->*m_memFunction)(args...);
        }

        V::Uuid GetInstanceType() const override
        {
            return VObject<C>::Id();
        }

        FunctionPtr GetMemberFunctionPtr() const
        {
            return m_memFunction;
        }
    private:
        FunctionPtr m_memFunction;
    };

    template <typename T>
    using AttributeContainerType = VStd::conditional_t<VStd::is_member_pointer<T>::value,
        VStd::conditional_t<VStd::is_member_function_pointer<T>::value, AttributeMemberFunction<T>, AttributeMemberData<T> >,
        VStd::conditional_t<VStd::is_function<VStd::remove_pointer_t<T>>::value, AttributeFunction<VStd::remove_pointer_t<T>>,
        VStd::conditional_t<VStd::function_traits<T>::value, AttributeInvocable<typename VStd::function_traits<T>::function_type>, AttributeData<T>>>
    >;

    namespace Internal
    {
        template<class T>
        struct AttributeValueTypeClassChecker
        {
            // By default any value is OK
            static bool Check(const Uuid&, IRttiHelper*)
            {
                return true;
            }
        };

        template<class T, class C>
        struct AttributeValueTypeClassChecker<T C::*>
        {
            static bool Check(const Uuid& classId, IRttiHelper* rtti)
            {
                if (classId == VObject<C>::Id())
                {
                    return true;
                }
                return rtti ? rtti->IsTypeOf(VObject<C>::Id()) : false;
            }
        };

        template<class T, class C>
        struct AttributeValueTypeClassChecker<T(C::*)()>
        {
            static bool Check(const Uuid& classId, IRttiHelper* rtti)
            {
                if (classId == VObject<C>::Id())
                {
                    return true;
                }
                return rtti ? rtti->IsTypeOf(VObject<C>::Id()) : false;
            }
        };

        template<class T, class C>
        struct AttributeValueTypeClassChecker<T(C::*)() const>
        {
            static bool Check(const Uuid& classId, IRttiHelper* rtti)
            {
                if (classId == VObject<C>::Id())
                {
                    return true;
                }
                return rtti ? rtti->IsTypeOf(VObject<C>::Id()) : false;
            }
        };

        template<class T, bool IsNoSpecialization = HasNoOnDemandReflection<OnDemandReflection<T>>::value>
        struct OnDemandReflectHook
        {
            static StaticReflectionFunctionPtr Get()
            {
                return nullptr;
            }
        };

        template<class T>
        struct OnDemandReflectHook<T, false>
        {
            static StaticReflectionFunctionPtr Get()
            {
                return &OnDemandReflection<T>::Reflect;
            }
        };
    }
}
#endif // V_FRAMEWORK_CORE_VOBJECT_REFLECT_CONTEXT_H