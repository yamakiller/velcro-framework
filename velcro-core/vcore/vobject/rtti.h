#ifndef V_FRAMEWORK_CORE_RTTI_RTTI_H
#define V_FRAMEWORK_CORE_RTTI_RTTI_H

#include <vcore/vobject/type.h>
#include <vcore/module/environment.h>
#include <vcore/std/typetraits/is_abstract.h>

namespace VStd {
    template<class T>
    class shared_ptr;
    template<class T>
    class intrusive_ptr;
}

namespace V {

    #define vobject_dynamic_cast        V::rtti_cast
    #define vobject_rtti_cast           V::rtti_cast
    #define vobject_rtti_istypeof       V::RttiIsTypeOf
    /// For instances vobject_rtti_typeid(instance) or vobject_rtti_typeid<Type>() for types
    #define vobject_rtti_typeid   V::RttiTypeId

    typedef void (*RTTI_EnumCallback)(const V::TypeId&, void*);

    
    #define VOBJECT_RTTI_COMMON()  \
    V_PUSH_DISABLE_WARNING(26433, "-Winconsistent-missing-override") \
    void RTTI_Enable(); \
    virtual inline const V::TypeId& RTTI_GetType() const { return RTTI_Type(); }\
    virtual inline const char*      RTTI_GetTypeName() const { return RTTI_TypeName();} \
    virtual inline bool             RTTI_IsTypeOf(const V::TypeId &typeId) const { return RTTI_IsContainType(typeId);} \
    virtual void                    RTTI_EnumTypes(V::RTTI_EnumCallback cb, void* userData) { RTTI_EnumHierarchy(cb, userData); } \
    static inline const char*       RTTI_Name() { return VOBJECT_Name(); } \
    static inline const V::TypeId   RTTI_Type() { return VOBJECT_Id();} \
    V_POP_DISABLE_WARNING

    /// VOBJECT_RTTI()
    #define VOBJECT_RTTI_1() VOBJECT_RTTI_COMMON() \
    static bool                     RTTI_IsContainType(const V::TypeId &typeId)  { return typeId == RTTI_Type(); } \
    static void                     RTTI_EnumHierarchy(V::RTTI_EnumCallback cb, void* userData) { cb(RTTI_Type(), userData); } \
    V_PUSH_DISABLE_WARNING(26433, "-Winconsistent-missing-override") \
    virtual inline const void*      RTTI_AddressOf(const V::TypeId& typeId) const { return (typeId == RTTI_Type()) ? this : nullptr; } \
    virtual inline void*            RTTI_AddressOf(const V::TypeId& typeId) { return (typeId == RTTI_Type()) ? this : nullptr; } \
    V_POP_DISABLE_WARNING

    /// VOBJECT_RTTI(BaseClass)
    #define VOBJECT_RTTI_2(_1) VOBJECT_RTTI_COMMON() \
    static bool                     RTTI_IsContainType(const V::TypeId &typeId)  { \
        if (typeId == RTTI_Type()) return true;  \
        return V::Internal::rtti_caller::Id<_1>::RTTI_IsContainType(typeId); \
    } \
    static void                     RTTI_EnumHierarchy(V::RTTI_EnumCallback cb, void* userData) {\
        cb(RTTI_Type(), userData); \
        V::Internal::rtti_caller::Id<_1>::RTTI_EnumHierarchy(cb, userData); \
    } \
    V_PUSH_DISABLE_WARNING(26433, "-Winconsistent-missing-override") \
    virtual inline const void*      RTTI_AddressOf(const V::TypeId& typeId) const { \
        if (typeId == RTTI_Type()) return this; \
        return V::Internal::rtti_caller::Id<_1>::RTTI_AddressOf(this, typeId); \
    } \
    virtual inline void*            RTTI_AddressOf(const V::TypeId& typeId) { \
        if (typeId == RTTI_Type()) return this; \
        return V::Internal::rtti_caller::Id<_1>::RTTI_AddressOf(this, typeId); \
    } \
    V_POP_DISABLE_WARNING

    /// VOBJECT_RTTI(BaseClass, BaseClass)
    #define VOBJECT_RTTI_3(_1, _2) VOBJECT_RTTI_COMMON() \
    static bool                     RTTI_IsContainType(const V::TypeId &typeId)  { \
        if (typeId == RTTI_Type()) return true;  \
        if (V::Internal::rtti_caller::Id<_1>::RTTI_IsContainType(typeId)) return true; \
        return V::Internal::rtti_caller::Id<_2>::RTTI_IsContainType(typeId); \
    } \
    static void                     RTTI_EnumHierarchy(V::RTTI_EnumCallback cb, void* userData) {\
        cb(RTTI_Type(), userData); \
        V::Internal::rtti_caller::Id<_1>::RTTI_EnumHierarchy(cb, userData); \
        V::Internal::rtti_caller::Id<_2>::RTTI_EnumHierarchy(cb, userData); \
    } \
    V_PUSH_DISABLE_WARNING(26433, "-Winconsistent-missing-override") \
    virtual inline const void*      RTTI_AddressOf(const V::TypeId& typeId) const { \
        if (typeId == RTTI_Type()) return this; \
        const void* r =  V::Internal::rtti_caller::Id<_1>::RTTI_AddressOf(this, typeId); \
        if (r) return r; \
        return V::Internal::rtti_caller::Id<_2>::RTTI_AddressOf(this, typeId); \
    } \
    virtual inline void*            RTTI_AddressOf(const V::TypeId& typeId) { \
        if (typeId == RTTI_Type()) return this; \
        void* r = V::Internal::rtti_caller::Id<_1>::RTTI_AddressOf(this, typeId); \
        if (r) return r; \
        return V::Internal::rtti_caller::Id<_2>::RTTI_AddressOf(this, typeId); \
    } \
    V_POP_DISABLE_WARNING

    /// VOBJECT_RTTI(BaseClass, BaseClass, BaseClass)
    #define VOBJECT_RTTI_4(_1, _2, _3) VOBJECT_RTTI_COMMON() \
    static bool                     RTTI_IsContainType(const V::TypeId &typeId)  { \
        if (typeId == RTTI_Type()) return true;  \
        if (V::Internal::rtti_caller::Id<_1>::RTTI_IsContainType(typeId)) return true; \
        if (V::Internal::rtti_caller::Id<_2>::RTTI_IsContainType(typeId)) return true; \
        return V::Internal::rtti_caller::Id<_3>::RTTI_IsContainType(typeId); \
    } \
    static void                     RTTI_EnumHierarchy(V::RTTI_EnumCallback cb, void* userData) {\
        cb(RTTI_Type(), userData); \
        V::Internal::rtti_caller::Id<_1>::RTTI_EnumHierarchy(cb, userData); \
        V::Internal::rtti_caller::Id<_2>::RTTI_EnumHierarchy(cb, userData); \
        V::Internal::rtti_caller::Id<_3>::RTTI_EnumHierarchy(cb, userData); \
    } \
    V_PUSH_DISABLE_WARNING(26433, "-Winconsistent-missing-override") \
    virtual inline const void*      RTTI_AddressOf(const V::TypeId& typeId) const { \
        if (typeId == RTTI_Type()) return this; \
        const void* r =  V::Internal::rtti_caller::Id<_1>::RTTI_AddressOf(this, typeId); \
        if (r) return r; \
        r = V::Internal::rtti_caller::Id<_2>::RTTI_AddressOf(this, typeId); \
        if (r) return r; \
        return V::Internal::rtti_caller::Id<_3>::RTTI_AddressOf(this, typeId); \
    } \
    virtual inline void*            RTTI_AddressOf(const V::TypeId& typeId) { \
        if (typeId == RTTI_Type()) return this; \
        void* r = V::Internal::rtti_caller::Id<_1>::RTTI_AddressOf(this, typeId); \
        if (r) return r; \
        r = V::Internal::rtti_caller::Id<_2>::RTTI_AddressOf(this, typeId); \
        if (r) return r; \
        return V::Internal::rtti_caller::Id<_3>::RTTI_AddressOf(this, typeId); \
    } \
    V_POP_DISABLE_WARNING

    /// VOBJECT_RTTI(BaseClass, BaseClass, BaseClass, BaseClass)
    #define VOBJECT_RTTI_5(_1, _2, _3, _4) VOBJECT_RTTI_COMMON() \
    static bool                     RTTI_IsContainType(const V::TypeId &typeId)  { \
        if (typeId == RTTI_Type()) return true;  \
        if (V::Internal::rtti_caller::Id<_1>::RTTI_IsContainType(typeId)) return true; \
        if (V::Internal::rtti_caller::Id<_2>::RTTI_IsContainType(typeId)) return true; \
        if (V::Internal::rtti_caller::Id<_3>::RTTI_IsContainType(typeId)) return true; \
        return V::Internal::rtti_caller::Id<_4>::RTTI_IsContainType(typeId); \
    } \
    static void                     RTTI_EnumHierarchy(V::RTTI_EnumCallback cb, void* userData) {\
        cb(RTTI_Type(), userData); \
        V::Internal::rtti_caller::Id<_1>::RTTI_EnumHierarchy(cb, userData); \
        V::Internal::rtti_caller::Id<_2>::RTTI_EnumHierarchy(cb, userData); \
        V::Internal::rtti_caller::Id<_3>::RTTI_EnumHierarchy(cb, userData); \
        V::Internal::rtti_caller::Id<_4>::RTTI_EnumHierarchy(cb, userData); \
    } \
    V_PUSH_DISABLE_WARNING(26433, "-Winconsistent-missing-override") \
    virtual inline const void*      RTTI_AddressOf(const V::TypeId& typeId) const { \
        if (typeId == RTTI_Type()) return this; \
        const void* r =  V::Internal::rtti_caller::Id<_1>::RTTI_AddressOf(this, typeId); \
        if (r) return r; \
        r = V::Internal::rtti_caller::Id<_2>::RTTI_AddressOf(this, typeId); \
        if (r) return r; \
        r = V::Internal::rtti_caller::Id<_3>::RTTI_AddressOf(this, typeId); \
        if (r) return r; \
        return V::Internal::rtti_caller::Id<_4>::RTTI_AddressOf(this, typeId); \
    } \
    virtual inline void*            RTTI_AddressOf(const V::TypeId& typeId) { \
        if (typeId == RTTI_Type()) return this; \
        void* r = V::Internal::rtti_caller::Id<_1>::RTTI_AddressOf(this, typeId); \
        if (r) return r; \
        r = V::Internal::rtti_caller::Id<_2>::RTTI_AddressOf(this, typeId); \
        if (r) return r; \
        r = V::Internal::rtti_caller::Id<_3>::RTTI_AddressOf(this, typeId); \
        if (r) return r; \
        return V::Internal::rtti_caller::Id<_4>::RTTI_AddressOf(this, typeId); \
    } \
    V_POP_DISABLE_WARNING

    /// VOBJECT_RTTI(BaseClass, BaseClass, BaseClass, BaseClass, BaseClass)
    #define VOBJECT_RTTI_6(_1, _2, _3, _4, _5) VOBJECT_RTTI_COMMON() \
    static bool                     RTTI_IsContainType(const V::TypeId &typeId)  { \
        if (typeId == RTTI_Type()) return true;  \
        if (V::Internal::rtti_caller::Id<_1>::RTTI_IsContainType(typeId)) return true; \
        if (V::Internal::rtti_caller::Id<_2>::RTTI_IsContainType(typeId)) return true; \
        if (V::Internal::rtti_caller::Id<_3>::RTTI_IsContainType(typeId)) return true; \
        if (V::Internal::rtti_caller::Id<_4>::RTTI_IsContainType(typeId)) return true; \
        return V::Internal::rtti_caller::Id<_5>::RTTI_IsContainType(typeId); \
    } \
    static void                     RTTI_EnumHierarchy(V::RTTI_EnumCallback cb, void* userData) {\
        cb(RTTI_Type(), userData); \
        V::Internal::rtti_caller::Id<_1>::RTTI_EnumHierarchy(cb, userData); \
        V::Internal::rtti_caller::Id<_2>::RTTI_EnumHierarchy(cb, userData); \
        V::Internal::rtti_caller::Id<_3>::RTTI_EnumHierarchy(cb, userData); \
        V::Internal::rtti_caller::Id<_4>::RTTI_EnumHierarchy(cb, userData); \
        V::Internal::rtti_caller::Id<_5>::RTTI_EnumHierarchy(cb, userData); \
    } \
    V_PUSH_DISABLE_WARNING(26433, "-Winconsistent-missing-override") \
    virtual inline const void*      RTTI_AddressOf(const V::TypeId& typeId) const { \
        if (typeId == RTTI_Type()) return this; \
        const void* r =  V::Internal::rtti_caller::Id<_1>::RTTI_AddressOf(this, typeId); \
        if (r) return r; \
        r = V::Internal::rtti_caller::Id<_2>::RTTI_AddressOf(this, typeId); \
        if (r) return r; \
        r = V::Internal::rtti_caller::Id<_3>::RTTI_AddressOf(this, typeId); \
        if (r) return r; \
        r = V::Internal::rtti_caller::Id<_4>::RTTI_AddressOf(this, typeId); \
        if (r) return r; \
        return V::Internal::rtti_caller::Id<_5>::RTTI_AddressOf(this, typeId); \
    } \
    virtual inline void*            RTTI_AddressOf(const V::TypeId& typeId) { \
        if (typeId == RTTI_Type()) return this; \
        void* r = V::Internal::rtti_caller::Id<_1>::RTTI_AddressOf(this, typeId); \
        if (r) return r; \
        r = V::Internal::rtti_caller::Id<_2>::RTTI_AddressOf(this, typeId); \
        if (r) return r; \
        r = V::Internal::rtti_caller::Id<_3>::RTTI_AddressOf(this, typeId); \
        if (r) return r; \
        r = V::Internal::rtti_caller::Id<_4>::RTTI_AddressOf(this, typeId); \
        if (r) return r; \
        return V::Internal::rtti_caller::Id<_4>::RTTI_AddressOf(this, typeId); \
    } \
    V_POP_DISABLE_WARNING

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // MACRO specialization to allow optional parameters for template version of VOBJECT_RTTI
    #define VOBJECT_INTERNAL_EXTRACT(...) VOBJECT_INTERNAL_EXTRACT __VA_ARGS__
    #define VOBJECT_INTERNAL_NOTHING_VOBJECT_INTERNAL_EXTRACT
    #define VOBJECT_INTERNAL_PASTE(_x, ...)  _x ## __VA_ARGS__
    #define VOBJECT_INTERNAL_EVALUATING_PASTE(_x, ...) VOBJECT_INTERNAL_PASTE(_x, __VA_ARGS__)
    #define VOBJECT_INTERNAL_REMOVE_PARENTHESIS(_x) VOBJECT_INTERNAL_EVALUATING_PASTE(VOBJECT_INTERNAL_NOTHING_, VOBJECT_INTERNAL_EXTRACT _x)

    #define VOBJECT_INTERNAL_USE_FIRST_ELEMENT(_1, ...) _1
    #define VOBJECT_INTERNAL_SKIP_FIRST(_1, ...)  __VA_ARGS__

    #define VOBJECT_RTTI_MACRO_SPECIALIZE_II(MACRO_NAME, NPARAMS, PARAMS) MACRO_NAME##NPARAMS PARAMS
    #define VOBJECT_RTTI_MACRO_SPECIALIZE_I(MACRO_NAME, NPARAMS, PARAMS) VOBJECT_RTTI_MACRO_SPECIALIZE_II(MACRO_NAME, NPARAMS, PARAMS)
    #define VOBJECT_RTTI_MACRO_SPECIALIZE(MACRO_NAME, NPARAMS, PARAMS) VOBJECT_RTTI_MACRO_SPECIALIZE_I(MACRO_NAME, NPARAMS, PARAMS)

    #define VOBJECT_RTTI_I_MACRO_SPECIALIZE_II(MACRO_NAME, NPARAMS, PARAMS) MACRO_NAME##NPARAMS PARAMS
    #define VOBJECT_RTTI_I_MACRO_SPECIALIZE_I(MACRO_NAME, NPARAMS, PARAMS) VOBJECT_RTTI_I_MACRO_SPECIALIZE_II(MACRO_NAME, NPARAMS, PARAMS)
    #define VOBJECT_RTTI_I_MACRO_SPECIALIZE(MACRO_NAME, NPARAMS, PARAMS) VOBJECT_RTTI_I_MACRO_SPECIALIZE_I(MACRO_NAME, NPARAMS, PARAMS)

    #define VOBJECT_RTTI_HELPER_1(_Name, ...) VOBJECT_TYPE_INFO_INTERNAL_2(_Name, VOBJECT_INTERNAL_USE_FIRST_ELEMENT(__VA_ARGS__)); VOBJECT_RTTI_I_MACRO_SPECIALIZE(VOBJECT_RTTI_, V_VA_NUM_ARGS(__VA_ARGS__), (VOBJECT_INTERNAL_SKIP_FIRST(__VA_ARGS__)))

    #define VOBJECT_RTTI_HELPER_3(...)  VOBJECT_TYPE_INFO_INTERNAL(VOBJECT_INTERNAL_REMOVE_PARENTHESIS(VOBJECT_INTERNAL_USE_FIRST_ELEMENT(__VA_ARGS__))); VOBJECT_RTTI_I_MACRO_SPECIALIZE(VOBJECT_RTTI_, V_VA_NUM_ARGS(__VA_ARGS__), (VOBJECT_INTERNAL_SKIP_FIRST(__VA_ARGS__)))
    #define VOBJECT_RTTI_HELPER_2(...)  VOBJECT_RTTI_HELPER_3(__VA_ARGS__)
    #define VOBJECT_RTTI_HELPER_4(...)  VOBJECT_RTTI_HELPER_3(__VA_ARGS__)
    #define VOBJECT_RTTI_HELPER_5(...)  VOBJECT_RTTI_HELPER_3(__VA_ARGS__)
    #define VOBJECT_RTTI_HELPER_6(...)  VOBJECT_RTTI_HELPER_3(__VA_ARGS__)
    #define VOBJECT_RTTI_HELPER_7(...)  VOBJECT_RTTI_HELPER_3(__VA_ARGS__)
    #define VOBJECT_RTTI_HELPER_8(...)  VOBJECT_RTTI_HELPER_3(__VA_ARGS__)
    #define VOBJECT_RTTI_HELPER_9(...)  VOBJECT_RTTI_HELPER_3(__VA_ARGS__)
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // 检测类是否有 RTTI_Enable 成员函数
    V_HAS_MEMBER(VObjectRttiIntrusive, RTTI_Enable, void, ());

    enum class RttiKind {
        None,
        Intrusive,
        External
    };

    template<typename T>
    struct HasVObjectRttiExternal
    {
        static constexpr bool value = false;
        using type = VStd::false_type;
    };

    template<typename T>
    struct HasVObjectRtti
    {
        static constexpr bool value = HasVObjectRttiExternal<T>::value || HasVObjectRttiIntrusive<T>::value;
        using type = std::integral_constant<bool, value>;
        using kind_type = VStd::integral_constant<RttiKind, HasVObjectRttiIntrusive<T>::value ? RttiKind::Intrusive : (HasVObjectRttiExternal<T>::value ? RttiKind::External : RttiKind::None)>;
    };

    // VOBJECT_RTTI( ( (ClassName<TemplateArg1, TemplateArg2, ...>), ClassUuid, TemplateArg1, TemplateArg2, ...), BaseClass1...)
    #define VOBJECT_RTTI(...)  VOBJECT_RTTI_MACRO_SPECIALIZE(VOBJECT_RTTI_HELPER_, V_VA_NUM_ARGS(VOBJECT_INTERNAL_REMOVE_PARENTHESIS(VOBJECT_INTERNAL_USE_FIRST_ELEMENT(__VA_ARGS__))), (__VA_ARGS__))

    // Adds RTTI support for <Type> without modifying the class
    // Variadic parameters should be the base class(es) if there are any
    #define VOBJECT_EXTERNAL_RTTI_SPECIALIZE(Type, ...) \
        template<> \
        struct HasVObjectRttiExternal<Type> { \
            static const bool value = true; \
            using type = VStd::true_type; \
        }; \
         \
        template<>  \
            struct V::Internal::RttiHelper<Type> \
            : public V::Internal::ExternalVariadicRttiHelper<Type, ##__VA_ARGS__>  \
        { \
        };

    
    class IRttiHelper
    {
    public:
        virtual ~IRttiHelper() = default;
        virtual const V::TypeId&  GetActualUuid(const void* instance) const = 0;
        virtual const char*       GetActualTypeName(const void* instance) const = 0;
        virtual const void*       Cast(const void* instance, const V::TypeId& asType) const = 0;
        virtual void*             Cast(void* instance, const V::TypeId& asType) const = 0;
        virtual const V::TypeId&  GetTypeId() const = 0;
        // If the type is an instance of a template this will return the type id of the template instead of the instance.
        // otherwise it return the regular type id.
        virtual const V::TypeId& GetGenericTypeId() const = 0;
        virtual bool              IsTypeOf(const V::TypeId& id) const = 0;
        virtual bool              IsAbstract() const = 0;
        virtual bool              ProvidesFullRtti() const = 0;
        virtual void              EnumHierarchy(RTTI_EnumCallback callback, void* userData = nullptr) const = 0;
        virtual TypeTraits        GetTypeTraits() const = 0;
        virtual size_t            GetTypeSize() const = 0;

        // Template helpers
        template <typename TargetType>
        V_FORCE_INLINE const TargetType* Cast(const void* instance)
        {
            return reinterpret_cast<const TargetType*>(Cast(instance, V::VObject<TargetType>::Id()()));
        }

        template <typename TargetType>
        V_FORCE_INLINE TargetType* Cast(void* instance)
        {
            return reinterpret_cast<TargetType*>(Cast(instance, V::VObject<TargetType>::Id()));
        }

        template <typename QueryType>
        V_FORCE_INLINE bool IsTypeOf()
        {
            return IsTypeOf(V::VObject<QueryType>::Id());
        }
    };

     namespace Internal
    {
        /*
         * Helper class to retrieve V::TypeIds and perform AZRtti queries.
         * This helper uses AZRtti when available, and does what it can when not.
         * It automatically resolves pointer-to-pointers to their value types
         * and supports queries without type information through the IRttiHelper
         * interface.
         * Call GetRttiHelper<T>() to retrieve an IRttiHelper interface
         * for AZRtti-enabled types while type info is still available, so it
         * can be used for queries after type info is lost.
         */
        template<typename T>
        struct RttiHelper
            : public IRttiHelper
        {
        public:
            using ValueType = T;
            static_assert(HasVObjectRtti<ValueType>::value, "Type parameter for RttiHelper must support VObject RTTI.");

            //////////////////////////////////////////////////////////////////////////
            // IRttiHelper
            const V::TypeId& GetActualUuid(const void* instance) const override
            {
                return instance
                    ? reinterpret_cast<const T*>(instance)->RTTI_GetType()
                    : T::RTTI_Type();
            }
            const char* GetActualTypeName(const void* instance) const override
            {
                return instance
                    ? reinterpret_cast<const T*>(instance)->RTTI_GetTypeName()
                    : T::RTTI_TypeName();
            }
            const void* Cast(const void* instance, const V::TypeId& asType) const override
            {
                return instance
                    ? reinterpret_cast<const T*>(instance)->RTTI_AddressOf(asType)
                    : nullptr;
            }
            void* Cast(void* instance, const V::TypeId& asType) const override
            {
                return instance
                    ? reinterpret_cast<T*>(instance)->RTTI_AddressOf(asType)
                    : nullptr;
            }
            const V::TypeId& GetTypeId() const override
            {
                return T::RTTI_Type();
            }
            const V::TypeId& GetGenericTypeId() const override
            {
                return VObject<T>::template Id<V::GenericTypeIdTag>();
            }
            bool IsTypeOf(const V::TypeId& id) const override
            {
                return T::RTTI_IsContainType(id);
            }
            bool IsAbstract() const override
            {
                return VStd::is_abstract<T>::value;
            }
            bool ProvidesFullRtti() const override
            {
                return true;
            }
            void EnumHierarchy(RTTI_EnumCallback callback, void* userData = nullptr) const override
            {
                T::RTTI_EnumHierarchy(callback, userData);
            }
            TypeTraits GetTypeTraits() const override
            {
                return VObject<T>::GetTypeTraits();
            }
            size_t GetTypeSize() const override
            {
                return VObject<T>::Size();
            }
            //////////////////////////////////////////////////////////////////////////
        };

        template<typename T>
        struct TypeInfoOnlyRttiHelper
            : public IRttiHelper
        {
        public:
            using ValueType = T;
            static_assert(V::Internal::HasVObject<ValueType>::value, "Type parameter for RttiHelper must support VObject Type Info.");

            //////////////////////////////////////////////////////////////////////////
            // IRttiHelper
            const V::TypeId& GetActualUuid(const void*) const override
            {
                return V::VObject<ValueType>::Id();
            }
            const char* GetActualTypeName(const void*) const override
            {
                return V::VObject<ValueType>::Name();
            }
            const void* Cast(const void* instance, const V::TypeId& asType) const override
            {
                return asType == GetTypeId() ? instance : nullptr;
            }
            void* Cast(void* instance, const V::TypeId& asType) const override
            {
                return asType == GetTypeId() ? instance : nullptr;
            }
            const V::TypeId& GetTypeId() const override
            {
                return V::VObject<ValueType>::Id();
            }
            const V::TypeId& GetGenericTypeId() const override
            {
                return V::VObject<ValueType>::template Id<V::GenericTypeIdTag>();
            }
            bool IsTypeOf(const V::TypeId& id) const override
            {
                return id == GetTypeId();
            }
            bool IsAbstract() const override
            {
                return false;
            }
            bool ProvidesFullRtti() const override
            {
                return false;
            }
            void EnumHierarchy(RTTI_EnumCallback callback, void* instance) const override
            {
                callback(GetTypeId(), instance);
            }
            TypeTraits GetTypeTraits() const override
            {
                return VObject<T>::GetTypeTraits();
            }
            size_t GetTypeSize() const override
            {
                return VObject<T>::Size();
            }
            //////////////////////////////////////////////////////////////////////////
        };

        template<typename T, typename... TArgs>
        struct ExternalVariadicRttiHelper
            : public IRttiHelper
        {
            const V::TypeId& GetActualUuid(const void*) const override
            {
                return V::VObject<T>::Id();
            }
            const char* GetActualTypeName(const void*) const override
            {
                return V::VObject<T>::Name();
            }

            const void* Cast(const void* instance, const V::TypeId& asType) const override
            {
                const void* result = GetTypeId() == asType ? instance : nullptr;

                using dummy = bool[];
                [[maybe_unused]] dummy d { true, (CastInternal<TArgs>(result, instance, asType), true)... };
                return result;
            }

            void* Cast(void* instance, const V::TypeId& asType) const override
            {
                return const_cast<void*>(Cast(const_cast<const void*>(instance), asType));
            }

            const V::TypeId& GetTypeId() const override
            {
                return V::VObject<T>::Id();
            }

            const V::TypeId& GetGenericTypeId() const override
            {
                return V::VObject<T>::template Id<V::GenericTypeIdTag>();
            }

            bool IsTypeOf(const V::TypeId& id) const override
            {
                bool result = GetTypeId() == id;

                using dummy = bool[];
                [[maybe_unused]] dummy d = { true, (IsTypeOfInternal<TArgs>(result, id), true)... };

                return result;
            }
            bool IsAbstract() const override
            {
                return VStd::is_abstract<T>::value;
            }
            bool ProvidesFullRtti() const override
            {
                return true;
            }
            void EnumHierarchy(RTTI_EnumCallback callback, void* instance) const override
            {
                callback(GetActualUuid(instance), instance);

                using dummy = bool[];
                [[maybe_unused]] dummy d = { true, (RttiHelper<TArgs>{}.EnumHierarchy(callback, instance), true)... };
            }
            TypeTraits GetTypeTraits() const override
            {
                return VObject<T>::GetTypeTraits();
            }
            size_t GetTypeSize() const override
            {
                return VObject<T>::Size();
            }

        private:
            template<typename T2>
            static void CastInternal(const void*& outPointer, const void* instance, const V::TypeId& asType)
            {
                if (outPointer == nullptr)
                {
                    // For classes with Intrusive RTTI IsTypeOf is used instead as this External RTTI class does not have 
                    // virtual RTTI_AddressOf functions for looking up the correct address
                    if (HasVObjectRttiExternal<T2>::value)
                    {
                        outPointer = RttiHelper<T2>{}.Cast(instance, asType);
                    }
                    else if(RttiHelper<T2>{}.IsTypeOf(asType))
                    {
                        outPointer = static_cast<const T2*>(instance);
                    }
                }
            }

            template<typename T2>
            static void IsTypeOfInternal(bool& result, const V::TypeId& id)
            {
                if (!result)
                {
                    result = RttiHelper<T2>{}.IsTypeOf(id);
                }
            }
        };

        namespace RttiHelperTags
        {
            struct Unavailable {};
            struct Standard {};
            struct TypeInfoOnly {};
        }

        // Overload for VObjectRtti
        // Named _Internal so that GetRttiHelper() calls from inside namespace Internal don't resolve here
        template<typename T>
        IRttiHelper* GetRttiHelper_Internal(RttiHelperTags::Standard)
        {
            static RttiHelper<T> _instance;
            return &_instance;
        }

        template<typename T>
        IRttiHelper* GetRttiHelper_Internal(RttiHelperTags::TypeInfoOnly)
        {
            static TypeInfoOnlyRttiHelper<T> _instance;
            return &_instance;
        }
        // Overload for no typeinfo available
        template<typename>
        IRttiHelper* GetRttiHelper_Internal(RttiHelperTags::Unavailable)
        {
            return nullptr;
        }
    } // namespace Internal

    template<typename T>
    IRttiHelper* GetRttiHelper()
    {
        using ValueType = std::remove_const_t<std::remove_pointer_t<T>>;
        static constexpr bool hasRtti =  HasVObjectRtti<ValueType>::value;
        static constexpr bool hasTypeInfo = V::Internal::HasVObject<ValueType>::value;

        using TypeInfoTag = VStd::conditional_t<hasTypeInfo, V::Internal::RttiHelperTags::TypeInfoOnly, V::Internal::RttiHelperTags::Unavailable>;
        using Tag = VStd::conditional_t<hasRtti, V::Internal::RttiHelperTags::Standard, TypeInfoTag>;

        static_assert(!HasVObjectRttiIntrusive<ValueType>::value || !V::Internal::HasVObjectSpecialized<ValueType>::value,
            "Types cannot use VOBJECT_SPECIALIZE macro externally with the VOBJECT_RTTI macro"
            " VOBJECT_RTTI adds virtual functions to the type in order to find the concrete class when looking up a typeid"
            " Specializing VObject for a type would cause it not be used");

        return V::Internal::GetRttiHelper_Internal<ValueType>(Tag{});
    }

    namespace Internal
    {
        template<class T, class U>
        inline T        rtti_cast_helper(U ptr, const VStd::integral_constant<RttiKind, RttiKind::Intrusive>& /* HasAZRttiIntrusive<U> */)
        {
            typedef typename VStd::remove_pointer<T>::type CastType;
            return ptr ? reinterpret_cast<T>(ptr->RTTI_AddressOf(VObject<CastType>::Id())) : nullptr;
        }

        template<class T, class U>
        inline T        rtti_cast_helper(U ptr, const VStd::integral_constant<RttiKind, RttiKind::External>& /* HasAZRttiExternal<U> */)
        {
            typedef typename VStd::remove_pointer<T>::type CastType;
            return ptr ? reinterpret_cast<T>(RttiHelper<VStd::remove_pointer_t<U>>().Cast(ptr, VObject<CastType>::Id())) : nullptr;
        }

        template<class T, class U>
        struct RttiIsSameCast
        {
            static inline T Cast(U)         { return nullptr; }
        };

        template<class T>
        struct RttiIsSameCast<T, T>
        {
            static inline T Cast(T ptr)     { return ptr; }
        };

        template<class T, class U>
        inline T        rtti_cast_helper(U ptr, const VStd::integral_constant<RttiKind, RttiKind::None>& /* !HasVObjectRtti<U> */)
        {
            static_assert(HasVObject<U>::value, "VOBJECT is required to perform an vobject_rtti_cast");
            return RttiIsSameCast<T, U>::Cast(ptr);
        }

        template<class T, bool IsConst = VStd::is_const<VStd::remove_pointer_t<T>>::value >
        struct AddressTypeHelper
        {
            typedef const void* type;
        };

        template<class T>
        struct AddressTypeHelper<T, false>
        {
            typedef void* type;
        };

        template<class U>
        inline typename AddressTypeHelper<U>::type RttiAddressOfHelper(U ptr, const V::TypeId& id, const VStd::integral_constant<RttiKind, RttiKind::Intrusive>& /* HasAZRttiIntrusive<U> */)
        {
            return ptr ? ptr->RTTI_AddressOf(id) : nullptr;
        }

        template<class U>
        inline typename AddressTypeHelper<U>::type RttiAddressOfHelper(U ptr, const V::TypeId& id, const VStd::integral_constant<RttiKind, RttiKind::External>& /* HasAZRttiExternal<U> */)
        {
            return ptr ? RttiHelper<VStd::remove_pointer_t<U>>().Cast(ptr, id) : nullptr;
        }

        template<class U>
        inline typename AddressTypeHelper<U>::type  RttiAddressOfHelper(U ptr, const V::TypeId& id, const VStd::integral_constant<RttiKind, RttiKind::None>& /* !HasVObjectRtti<U> */)
        {
            typedef typename VStd::remove_pointer<U>::type CastType;
            return (id == VObject<CastType>::Id()) ? ptr : nullptr;
        }

        template<class T>
        struct RttiRemoveQualifiers
        {
            typedef typename VStd::remove_cv<typename VStd::remove_reference<typename VStd::remove_pointer<T>::type>::type>::type type;
        };

        template<class T, class U>
        struct RttiIsTypeOfHelper
        {
            static inline bool  Check(const U& ref, const VStd::integral_constant<RttiKind, RttiKind::Intrusive>& /* HasAZRttiIntrusive<U> */)
            {
                typedef typename RttiRemoveQualifiers<T>::type CheckType;
                return ref.RTTI_IsTypeOf(VObject<CheckType>::Id());
            }

            static inline bool  Check(const U&, const VStd::integral_constant<RttiKind, RttiKind::External>& /* HasAZRttiExternal<U> */)
            {
                typedef typename RttiRemoveQualifiers<T>::type CheckType;
                return RttiHelper<U>().IsTypeOf(VObject<CheckType>::Id());
            }

            static inline bool  Check(const U&, const VStd::integral_constant<RttiKind, RttiKind::None>& /* !HasVObjectRtti<U> */)
            {
                typedef typename RttiRemoveQualifiers<T>::type CheckType;
                typedef typename RttiRemoveQualifiers<U>::type SrcType;
                return VObject<CheckType>::Id() == VObject<SrcType>::Id();
            }
        };
        template<class T, class U>
        struct RttiIsTypeOfHelper<T, U*>
        {
            static inline bool  Check(const U* ptr, const VStd::integral_constant<RttiKind, RttiKind::Intrusive>& /* HasAZRttiIntrusive<U> */)
            {
                typedef typename RttiRemoveQualifiers<T>::type CheckType;
                return ptr && ptr->RTTI_IsTypeOf(VObject<CheckType>::Id());
            }

            static inline bool  Check(const U* ptr, const VStd::integral_constant<RttiKind, RttiKind::External>& /* HasAZRttiExternal<U> */)
            {
                typedef typename RttiRemoveQualifiers<T>::type CheckType;
                return ptr ? RttiHelper<U>().IsTypeOf(VObject<CheckType>::Id()) : false;
            }

            static inline bool  Check(const U*, const VStd::integral_constant<RttiKind, RttiKind::None>& /* !HasVObjectRtti<U> */)
            {
                typedef typename RttiRemoveQualifiers<T>::type CheckType;
                typedef typename RttiRemoveQualifiers<U>::type SrcType;
                return VObject<CheckType>::Id() == VObject<SrcType>::Id();
            }
        };

        template<class U>
        struct rtti_is_type_of_id_helper
        {
            static inline bool  Check(const V::TypeId& id, const U& ref, const VStd::integral_constant<RttiKind, RttiKind::Intrusive>& /* HasAZRttiIntrusive<U> */)
            {
                return ref.RTTI_IsTypeOf(id);
            }

            static inline bool  Check(const V::TypeId& id, const U&, const VStd::integral_constant<RttiKind, RttiKind::External>& /* HasAZRttiExternal<U> */)
            {
                return RttiHelper<U>().IsTypeOf(id);
            }

            static inline bool  Check(const V::TypeId& id, const U&, const VStd::integral_constant<RttiKind, RttiKind::None>& /* !HasVObjectRtti<U> */)
            {
                typedef typename RttiRemoveQualifiers<U>::type SrcType;
                return id == VObject<SrcType>::Id();
            }

            static inline const V::TypeId& Type(const U& ref, const VStd::integral_constant<RttiKind, RttiKind::Intrusive>& /* HasAZRttiIntrusive<U> */)
            {
                return ref.RTTI_GetType();
            }

            static inline const V::TypeId& Type(const U&, const VStd::integral_constant<RttiKind, RttiKind::External>& /* HasVObjectRttiExternal<U> */)
            {
                return RttiHelper<U>().GetTypeId();
            }

            static inline const V::TypeId& Type(const U&, const VStd::integral_constant<RttiKind, RttiKind::None>& /* !HasVObjectRtti<U> */)
            {
                typedef typename RttiRemoveQualifiers<U>::type SrcType;
                return VObject<SrcType>::Id();
            }
        };

        template<class U>
        struct rtti_is_type_of_id_helper<U*>
        {
            static inline bool Check(const V::TypeId& id, U* ptr, const VStd::integral_constant<RttiKind, RttiKind::Intrusive>& /* HasVObjectRttiIntrusive<U> */)
            {
                return ptr && ptr->RTTI_IsTypeOf(id);
            }

            static inline bool  Check(const V::TypeId& id, const U* ptr, const VStd::integral_constant<RttiKind, RttiKind::External>& /* HasVObjetRttiExternal<U> */)
            {
                return ptr && GetRttiHelper<U>()->IsTypeOf(id);
            }

            static inline bool  Check(const V::TypeId& id, U*, const VStd::integral_constant<RttiKind, RttiKind::None>& /* !HasVObjectRtti<U> */)
            {
                typedef typename RttiRemoveQualifiers<U>::type SrcType;
                return id == VObject<SrcType>::Id();
            }

            static inline const V::TypeId& Type(U* ptr, const VStd::integral_constant<RttiKind, RttiKind::Intrusive>& /* HasAZRttiIntrusive<U> */)
            {
                static V::TypeId s_invalidUuid = V::TypeId::CreateNull();
                return ptr ? ptr->RTTI_GetType() : s_invalidUuid;
            }

            static inline const V::TypeId& Type(U* ptr, const VStd::integral_constant<RttiKind, RttiKind::External>& /* HasAZRttiExternal<U> */)
            {
                static V::TypeId s_invalidUuid = V::TypeId::CreateNull();
                return ptr ? RttiHelper<U>().GetTypeId() : s_invalidUuid;
            }

            static inline const V::TypeId& Type(U*, const VStd::integral_constant<RttiKind, RttiKind::None>& /* !HasVObjectRtti<U> */)
            {
                typedef typename RttiRemoveQualifiers<U>::type SrcType;
                return VObject<SrcType>::Id();
            }
        };

        template<class T>
        void rtti_enum_hierarchy_helper(RTTI_EnumCallback cb, void* userData, const VStd::true_type& /* HasVObjectRtti<T> */)
        {
            RttiHelper<T>().EnumHierarchy(cb, userData);
        }

        template<class T>
        void rtti_enum_hierarchy_helper(RTTI_EnumCallback cb, void* userData, const VStd::false_type& /* HasVObjectRtti<T> */)
        {
            cb(VObject<T>::Id(), userData);
        }

        // Helper to prune types that are base class, but don't support RTTI
        template<class T, RttiKind rttiKind = HasVObjectRtti<T>::kind_type::value>
        struct rtti_caller
        {
            V_FORCE_INLINE static bool RTTI_IsContainType(const V::TypeId& id)
            {
                return T::RTTI_IsContainType(id);
            }

            V_FORCE_INLINE static void RTTI_EnumHierarchy(V::RTTI_EnumCallback cb, void* userData)
            {
                T::RTTI_EnumHierarchy(cb, userData);
            }

            V_FORCE_INLINE static const void* RTTI_AddressOf(const T* obj, const V::TypeId& id)
            {
                return obj->T::RTTI_AddressOf(id);
            }

            V_FORCE_INLINE static void* RTTI_AddressOf(T* obj, const V::TypeId& id)
            {
                return obj->T::RTTI_AddressOf(id);
            }
        };

        template<class T>
        struct rtti_caller<T, RttiKind::External>
        {
            V_FORCE_INLINE static bool RTTI_IsContainType(const V::TypeId& id)
            {
                return RttiHelper<T>().IsTypeOf(id);
            }

            V_FORCE_INLINE static void RTTI_EnumHierarchy(V::RTTI_EnumCallback cb, void* userData)
            {
                RttiHelper<T>().EnumHierarchy(cb, userData);
            }

            V_FORCE_INLINE static const void* RTTI_AddressOf(const T* obj, const V::TypeId& id)
            {
                return RttiHelper<T>().Cast(obj, id);
            }

            V_FORCE_INLINE static void* RTTI_AddressOf(T* obj, const V::TypeId& id)
            {
                return RttiHelper<T>().Cast(obj, id);
            }
        };

        // Specialization for classes that don't have intrusive RTTI implemented
        template<class T>
        struct rtti_caller<T, RttiKind::None>
        {
            V_FORCE_INLINE static bool RTTI_IsContainType(const V::TypeId&)
            {:
                return false;
            }

            V_FORCE_INLINE static void RTTI_EnumHierarchy(V::RTTI_EnumCallback, void*)
            {
            }

            V_FORCE_INLINE static const void* RTTI_AddressOf(const T*, const V::TypeId&)
            {
                return nullptr;
            }

            V_FORCE_INLINE static void* RTTI_AddressOf(T*, const V::TypeId&)
            {
                return nullptr;
            }
        };

        template<class U, typename TypeIdResolverTag, typename VStd::enable_if_t<VStd::is_same_v<TypeIdResolverTag, VStd::false_type>>* = nullptr>
        inline const V::TypeId& RttiTypeId()
        {
            return VObject<U>::Id();
        }

        template<class U, typename TypeIdResolverTag, typename VStd::enable_if_t<!VStd::is_same_v<TypeIdResolverTag, VStd::false_type>>* = nullptr>
        inline const V::TypeId& RttiTypeId()
        {
            return VObject<U>::template Id<TypeIdResolverTag>();
        }
    } // namespace Internal

    /// Enumerate class hierarchy (static) based on input type. Safe to call for type not supporting VObjectRtti 
    /// returns the VObject<T>::Id only.
    template<class T>
    inline void      rtti_enum_hierarchy(RTTI_EnumCallback cb, void* userData)
    {
        V::Internal::rtti_enum_hierarchy_helper<T>(cb, userData, typename HasVObjectRtti<VStd::remove_pointer_t<T>>::type());
    }

    /// Performs a rtti_cast when possible otherwise return NULL. Safe to call for type not supporting VObjectRtti 
    /// (returns NULL unless type fully match).
    template<class T, class U>
    inline T        rtti_cast(U ptr)
    {
        // We do support only pointer types, because we don't use exceptions. So
        // if we have a reference and we can't convert we can't really check if the returned
        // reference is correct.
        static_assert(VStd::is_pointer<T>::value, "vobject_rtti_cast supports only pointer types");
        return V::Internal::rtti_cast_helper<T>(ptr, typename HasVObjectRtti<VStd::remove_pointer_t<U>>::kind_type());
    }

    /// Specialization for nullptr_t, it's convertible to anything
    template <class T>
    inline T        rtti_cast(VStd::nullptr_t)
    {
        static_assert(VStd::is_pointer<T>::value, "azrtti_cast supports only pointer types");
        return nullptr;
    }

    /// rtti_cast specialization for shared_ptr.
    template<class T, class U>
    inline VStd::shared_ptr<typename VStd::remove_pointer<T>::type>       rtti_cast(const VStd::shared_ptr<U>& ptr)
    {
        using DestType = typename VStd::remove_pointer<T>::type;
        // We do support only pointer types, because we don't use exceptions. So
        // if we have a reference and we can't convert we can't really check if the returned
        // reference is correct.
        static_assert(VStd::is_pointer<T>::value, "azrtti_cast supports only pointer types");
        T castPtr = V::Internal::rtti_cast_helper<T>(ptr.get(), typename HasVObjectRtti<U>::kind_type());
        if (castPtr)
        {
            return VStd::shared_ptr<DestType>(ptr, castPtr);
        }
        return VStd::shared_ptr<DestType>();
    }

    // rtti_cast specialization for intrusive_ptr.
    template<class T, class U>
    inline VStd::intrusive_ptr<typename VStd::remove_pointer<T>::type>    rtti_cast(const VStd::intrusive_ptr<U>& ptr)
    {
        // We do support only pointer types, because we don't use exceptions. So
        // if we have a reference and we can't convert we can't really check if the returned
        // reference is correct.
        static_assert(VStd::is_pointer<T>::value, "rtti_cast supports only pointer types");
        return V::Internal::rtti_cast_helper<T>(ptr.get(), typename HasVObjectRtti<U>::kind_type());
    }

    // Gets address of a contained type or NULL. Safe to call for type not supporting AZRtti (returns 0 unless type fully match).
    template<class T>
    inline typename V::Internal::AddressTypeHelper<T>::type RttiAddressOf(T ptr, const V::TypeId& id)
    {
        // we can support references (as not exception is needed), but pointer should be sufficient when it comes to addresses!
        static_assert(VStd::is_pointer<T>::value, "RttiAddressOf supports only pointer types");
        return V::Internal::RttiAddressOfHelper(ptr, id, typename HasVObjectRtti<VStd::remove_pointer_t<T>>::kind_type());
    }

    // TypeIdResolverTag is one of the TypeIdResolverTags such as CanonicalTypeIdTag. If false_type is provided the default on assigned
    // by the type will be used.
    template<class U, typename TypeIdResolverTag = VStd::false_type>
    inline const V::TypeId& RttiTypeId()
    {
        return V::Internal::RttiTypeId<U, TypeIdResolverTag>();
    }
    
    template<template<typename...> class U, typename = void>
    inline const V::TypeId& RttiTypeId()
    {
        return GenericVObject::Id<U>();
    }
    
    template<template<VStd::size_t...> class U, typename = void>
    inline const V::TypeId& RttiTypeId()
    {
        return GenericVObject::Id<U>();
    }
    
    template<template<typename, VStd::size_t> class U, typename = void>
    inline const V::TypeId& RttiTypeId()
    {
        return GenericVObject::Id<U>();
    }
    
    template<template<typename, typename, VStd::size_t> class U, typename = void>
    inline const V::TypeId& RttiTypeId()
    {
        return GenericVObject::Id<U>();
    }
    
    template<template<typename, typename, typename, VStd::size_t> class U, typename = void>
    inline const V::TypeId& RttiTypeId()
    {
        return GenericVObject::Id<U>();
    }
    
    template<class U>
    inline const V::TypeId& RttiTypeId(const U& data)
    {
        return V::Internal::rtti_is_type_of_id_helper<U>::Type(data, typename HasVObjectRtti<VStd::remove_pointer_t<U>>::kind_type());
    }


    // Returns true if the type is contained, otherwise false. Safe to call for type not supporting AZRtti (returns false unless type fully match).
    template<class T, class U>
    inline bool     RttiIsTypeOf(const U& data)
    {
        return V::Internal::RttiIsTypeOfHelper<T, U>::Check(data, typename HasVObjectRtti<VStd::remove_pointer_t<U>>::kind_type());
    }

    // Since it's not possible to create an instance of a non-type template, only the T has to be overloaded.

    // Returns true if the type is contained, otherwise false. Safe to call for type not supporting AZRtti (returns false unless type fully match).
    template<template<typename...> class T, class U>
    inline bool     RttiIsTypeOf(const U&)
    {
        using CheckType = typename V::Internal::RttiRemoveQualifiers<U>::type;
        return GenericVObject::Id<T>() == RttiTypeId<CheckType, V::GenericTypeIdTag>();
    }

    // Returns true if the type is contained, otherwise false. Safe to call for type not supporting AZRtti (returns false unless type fully match).
    template<template<VStd::size_t...> class T, class U>
    inline bool     RttiIsTypeOf(const U&)
    {
        using CheckType = typename V::Internal::RttiRemoveQualifiers<U>::type;
        return GenericVObject::Id<T>() == RttiTypeId<CheckType, V::GenericTypeIdTag>();
    }

    // Returns true if the type is contained, otherwise false. Safe to call for type not supporting AZRtti (returns false unless type fully match).
    template<template<typename, VStd::size_t> class T, class U>
    inline bool     RttiIsTypeOf(const U&)
    {
        using CheckType = typename V::Internal::RttiRemoveQualifiers<U>::type;
        return GenericVObject::Id<T>() == RttiTypeId<CheckType, V::GenericTypeIdTag>();
    }

    // Returns true if the type is contained, otherwise false.Safe to call for type not supporting AZRtti(returns false unless type fully match).
    template<template<typename, typename, VStd::size_t> class T, class U>
    inline bool     RttiIsTypeOf(const U&)
    {
        using CheckType = typename V::Internal::RttiRemoveQualifiers<U>::type;
        return GenericVObject::Id<T>() == RttiTypeId<CheckType, V::GenericTypeIdTag>();
    }

    // Returns true if the type is contained, otherwise false.Safe to call for type not supporting AZRtti(returns false unless type fully match).
    template<template<typename, typename, typename, VStd::size_t> class T, class U>
    inline bool     RttiIsTypeOf(const U&)
    {
        using CheckType = typename V::Internal::RttiRemoveQualifiers<U>::type;
        return GenericVObject::Id<T>() == RttiTypeId<CheckType, V::GenericTypeIdTag>();
    }

    // Returns true if the type (by id) is contained, otherwise false. Safe to call for type not supporting AZRtti (returns false unless type fully match).
    template<class U>
    inline bool     RttiIsTypeOf(const V::TypeId& id, U data)
    {
        return V::Internal::rtti_is_type_of_id_helper<U>::Check(id, data, typename HasVObjectRtti<VStd::remove_pointer_t<U>>::kind_type());
    }

} // namespace V

#endif // V_FRAMEWORK_CORE_RTTI_RTTI_H