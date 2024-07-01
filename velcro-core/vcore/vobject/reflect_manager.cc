#include <vcore/vobject/reflect_manager.h>

namespace V {
    //=========================================================================
    // ~ReflectionManager
    //=========================================================================
    ReflectionManager::~ReflectionManager()
    {
        Clear();
    }

      //=========================================================================
    // Clear
    //=========================================================================
    void ReflectionManager::Clear()
    {
        // Reset current typeid
        while (!m_contexts.empty())
        {
            RemoveReflectContext(vobject_rtti_typeid(m_contexts.back().get()));
        }

        // Clear the entry points
        m_entryPoints.clear();
    }

     //=========================================================================
    // Reflect
    //=========================================================================
    void ReflectionManager::Reflect(V::TypeId typeId, const ReflectionFunction& reflectEntryPoint)
    {
        // Early out if this type id is not unique
        if (m_typedEntryPoints.find(typeId) != m_typedEntryPoints.end())
        {
            return;
        }

        // Place the reflect function in the collection
        auto entryIt = m_entryPoints.emplace(m_entryPoints.end(), typeId, reflectEntryPoint);
        m_typedEntryPoints.emplace(typeId, VStd::move(entryIt));

        // Call the new entry point with all known contexts
        for (const auto& context : m_contexts)
        {
            reflectEntryPoint(context.get());
        }
    }

     //=========================================================================
    // Unreflect
    //=========================================================================
    void ReflectionManager::Unreflect(V::TypeId typeId)
    {
        auto entryIt = m_typedEntryPoints.find(typeId);
        if (entryIt != m_typedEntryPoints.end())
        {
            // Call unreflect on everything in reverse context order
            for (auto contextIt = m_contexts.rbegin(); contextIt != m_contexts.rend(); ++contextIt)
            {
                (*contextIt)->EnableRemoveReflection();
                (*entryIt->second)(contextIt->get());
                (*contextIt)->DisableRemoveReflection();
            }

            // Remove and destroy entry point
            m_entryPoints.erase(entryIt->second);
            m_typedEntryPoints.erase(entryIt);
        }
    }

      //=========================================================================
    // Reflect
    //=========================================================================
    void ReflectionManager::Reflect(StaticReflectionFunctionPtr reflectEntryPoint)
    {
        // Early out if this entry point is not unique
        if (m_nonTypedEntryPoints.find(reflectEntryPoint) != m_nonTypedEntryPoints.end())
        {
            return;
        }

        // Place the reflect function in the collection
        auto entryIt = m_entryPoints.emplace(m_entryPoints.end(), reflectEntryPoint);
        m_nonTypedEntryPoints.emplace(reflectEntryPoint, VStd::move(entryIt));

        // Call the new entry point with all known contexts
        for (const auto& contextIt : m_contexts)
        {
            reflectEntryPoint(contextIt.get());
        }
    }

     //=========================================================================
    // Unreflect
    //=========================================================================
    void ReflectionManager::Unreflect(StaticReflectionFunctionPtr reflectEntryPoint)
    {
        auto entryIt = m_nonTypedEntryPoints.find(reflectEntryPoint);
        if (entryIt != m_nonTypedEntryPoints.end())
        {
            // Call unreflect on everything (order doesn't matter, the contexts are disparate)
            for (auto contextIt = m_contexts.rbegin(); contextIt != m_contexts.rend(); ++contextIt)
            {
                (*contextIt)->EnableRemoveReflection();
                (*entryIt->second)(contextIt->get());
                (*contextIt)->DisableRemoveReflection();
            }

            // Remove and destroy entry point
            m_entryPoints.erase(entryIt->second);
            m_nonTypedEntryPoints.erase(entryIt);
        }
    }

    ReflectionManager::EntryPoint::EntryPoint(StaticReflectionFunctionPtr staticEntry)
        : m_nonTyped(staticEntry)
    { }

    ReflectionManager::EntryPoint::EntryPoint(V::TypeId typeId, const ReflectionFunction& entry)
        : m_typeId(typeId)
        , m_typed(entry)
    { }

    void ReflectionManager::EntryPoint::operator()(ReflectContext* context) const
    {
        if (m_nonTyped)
        {
            m_nonTyped(context);
        }
        else
        {
            m_typed(context);
        }
    }

    bool ReflectionManager::EntryPoint::operator==(const EntryPoint& other) const
    {
        return m_nonTyped == other.m_nonTyped && m_typeId == other.m_typeId;
    }

      //=========================================================================
    // AddReflectContext
    //=========================================================================
    void ReflectionManager::AddReflectContext(VStd::unique_ptr<ReflectContext>&& context)
    {
        // Early out if the context is already registered
        if (GetReflectContext(vobject_rtti_typeid(context.get())) != nullptr)
        {
            return;
        }

        for (const auto& entry : m_entryPoints)
        {
            entry(context.get());
        }
        m_contexts.emplace_back(VStd::move(context));
    }

    //=========================================================================
    // GetReflectContext
    //=========================================================================
    ReflectContext* ReflectionManager::GetReflectContext(V::TypeId contextTypeId)
    {
        for (const auto& context : m_contexts)
        {
            if (vobject_rtti_typeid(context.get()) == contextTypeId)
            {
                return context.get();
            }
        }

        return nullptr;
    }

    //=========================================================================
    // RemoveReflectContext
    //=========================================================================
    void ReflectionManager::RemoveReflectContext(V::TypeId contextTypeId)
    {
        for (auto contextIt = m_contexts.begin(); contextIt != m_contexts.end(); ++contextIt)
        {
            ReflectContext* context = contextIt->get();
            if (vobject_rtti_typeid(context) == contextTypeId)
            {
                // Unreflect everything from the context
                context->EnableRemoveReflection();
                for (auto entryIt = m_entryPoints.rbegin(); entryIt != m_entryPoints.rend(); ++entryIt)
                {
                    (*entryIt)(context);
                }
                context->DisableRemoveReflection();

                m_contexts.erase(contextIt);
                return;
            }
        }
    }
}