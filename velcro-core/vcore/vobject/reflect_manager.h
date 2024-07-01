#ifndef V_FRAMEWORK_CORE_VOBJECT_REFLECT_MANAGER_H
#define V_FRAMEWORK_CORE_VOBJECT_REFLECT_MANAGER_H

#include <vcore/vobject/reflect_context.h>

#include <vcore/std/containers/vector.h>
#include <vcore/std/functional.h>
#include <vcore/std/smart_ptr/unique_ptr.h>
#include <vcore/std/typetraits/is_same.h>

namespace V {
    class ReflectionManager {
    private:
        // Reusable check for whether or not a type is a reflect context
        template <typename ReflectContextT>
        using IsReflectContextT = VStd::enable_if_t<VStd::is_base_of<ReflectContext, ReflectContextT>::value>;
    public:
        V_CLASS_ALLOCATOR(ReflectionManager, SystemAllocator, 0);

         ReflectionManager() = default;
        ~ReflectionManager();

         /// Unreflect all entry points and contexts
        void Clear();

        /// Add a reflect function with an associated typeid
        void Reflect(V::TypeId typeId, const ReflectionFunction& reflectEntryPoint);
        /// Call unreflect on the entry point associated with the typeId
        void Unreflect(V::TypeId typeId);

          /// Add a static reflect function
        void Reflect(StaticReflectionFunctionPtr reflectEntryPoint);
        /// Unreflect a static reflect function
        void Unreflect(StaticReflectionFunctionPtr reflectEntryPoint);

        /// Creates a reflect context, and reflects all registered entry points
        template <typename ReflectContextT, typename = IsReflectContextT<ReflectContextT>>
        void AddReflectContext() { AddReflectContext(VStd::make_unique<ReflectContextT>()); }

          /// Gets a reflect context of the requested type
        template <typename ReflectContextT, typename = IsReflectContextT<ReflectContextT>>
        ReflectContextT* GetReflectContext() { return vobject_rtti_cast<ReflectContextT*>(GetReflectContext(vobject_rtti_typeid<ReflectContextT>())); }

        /// Destroy a reflect context (unreflects all entry points)
        template <typename ReflectContextT, typename = IsReflectContextT<ReflectContextT>>
        void RemoveReflectContext() { RemoveReflectContext(vobject_rtti_typeid<ReflectContextT>()); }
    protected:
        void AddReflectContext(VStd::unique_ptr<ReflectContext>&& context);
        ReflectContext* GetReflectContext(V::TypeId contextTypeId);
        void RemoveReflectContext(V::TypeId contextTypeId);
    protected:
        struct EntryPoint
        {
            // Only m_nonTyped OR m_typeId and m_entry will be set
            StaticReflectionFunctionPtr m_nonTyped = nullptr;
            V::TypeId m_typeId = V::TypeId::CreateNull();
            ReflectionFunction m_typed;

            EntryPoint(StaticReflectionFunctionPtr staticEntry);
            EntryPoint(V::TypeId typeId, const ReflectionFunction& entry);

            void operator()(ReflectContext* context) const;
            bool operator==(const EntryPoint& other) const;
        };

        using EntryPointList = VStd::list<EntryPoint>;
        
    protected:
        VStd::vector<VStd::unique_ptr<ReflectContext>> m_contexts;
        EntryPointList m_entryPoints;
        VStd::unordered_map<TypeId, EntryPointList::iterator> m_typedEntryPoints;
        VStd::unordered_map<StaticReflectionFunctionPtr, EntryPointList::iterator> m_nonTypedEntryPoints;
    };
}

#endif // V_FRAMEWORK_CORE_VOBJECT_REFLECT_MANAGER_H