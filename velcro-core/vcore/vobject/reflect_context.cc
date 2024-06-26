#include <vcore/vobject/ReflectContext.h>


#include <vcore/std/functional.h>
#include <vcore/std/string/string.h>

namespace V
{
    //=========================================================================
    // OnDemandReflectionOwner
    //=========================================================================
    OnDemandReflectionOwner::OnDemandReflectionOwner(ReflectContext& context)
        : m_reflectContext(context)
    { }

    //=========================================================================
    // ~OnDemandReflectionOwner
    //=========================================================================
    OnDemandReflectionOwner::~OnDemandReflectionOwner()
    {
        m_reflectFunctions.clear();
        m_reflectFunctions.shrink_to_fit();
    }

    //=========================================================================
    // AddReflectFunction
    //=========================================================================
    void OnDemandReflectionOwner::AddReflectFunction(V::Uuid typeId, Internal::ReflectionFunctionRef reflectFunction)
    {
        auto& currentTypes = m_reflectContext.m_currentlyProcessingTypeIds;
        // If we're in process of reflecting this type already, don't store references to it
        if (VStd::find(currentTypes.begin(), currentTypes.end(), typeId) != currentTypes.end())
        {
            return;
        }

        auto reflectionIt = m_reflectContext.m_onDemandReflection.find(typeId);
        if (reflectionIt == m_reflectContext.m_onDemandReflection.end())
        {
            // Capture for lambda (in case this is gone when unreflecting)
            V::ReflectContext* reflectContext = &m_reflectContext;
            // If it's not already reflected, add it to the list, and capture a reference to it
            VStd::shared_ptr<Internal::ReflectionFunctionRef> reflectionPtr(reflectFunction, [reflectContext, typeId](Internal::ReflectionFunctionRef unreflectFunction)
            {
                bool isRemovingReflection = reflectContext->IsRemovingReflection();

                // Call the function in RemoveReflection mode
                reflectContext->EnableRemoveReflection();
                unreflectFunction(reflectContext);
                // Reset RemoveReflection mode
                reflectContext->m_isRemoveReflection = isRemovingReflection;

                // Remove the function from the central store (otherwise it stores an empty weak_ptr)
                reflectContext->m_onDemandReflection.erase(typeId);
            });

            // Capture reference to the reflection function
            m_reflectFunctions.emplace_back(reflectionPtr);

            // Tell the manager about the function
            m_reflectContext.m_onDemandReflection.emplace(typeId, reflectionPtr);
            m_reflectContext.m_toProcessOnDemandReflection.emplace_back(typeId, VStd::move(reflectFunction));
        }
        else
        {
            // If it is already reflected, just capture a reference to it
            auto reflectionPtr = reflectionIt->second.lock();
            AZ_Assert(reflectionPtr, "OnDemandReflection for typed %s is missing, despite being present in the reflect context", typeId.ToString<VStd::string>().c_str());
            if (reflectionPtr)
            {
                m_reflectFunctions.emplace_back(VStd::move(reflectionPtr));
            }
        }
    }

    //=========================================================================
    // ReflectContext
    //=========================================================================
    ReflectContext::ReflectContext()
        : m_isRemoveReflection(false)
    { }

    //=========================================================================
    // EnableRemoveReflection
    //=========================================================================
    void ReflectContext::EnableRemoveReflection()
    {
        m_isRemoveReflection = true;
    }

    //=========================================================================
    // DisableRemoveReflection
    //=========================================================================
    void ReflectContext::DisableRemoveReflection()
    {
        m_isRemoveReflection = false;
    }

    //=========================================================================
    // IsRemovingReflection
    //=========================================================================
    bool ReflectContext::IsRemovingReflection() const
    {
        return m_isRemoveReflection;
    }

    //=========================================================================
    // IsTypeReflected
    //=========================================================================
    bool ReflectContext::IsOnDemandTypeReflected(V::Uuid typeId)
    {
        return m_onDemandReflection.find(typeId) != m_onDemandReflection.end();
    }

    //=========================================================================
    // ExecuteQueuedReflections
    //=========================================================================
    void ReflectContext::ExecuteQueuedOnDemandReflections()
    {
        // Need to do move so recursive definitions don't result in reprocessing the same type
        auto toProcess = VStd::move(m_toProcessOnDemandReflection);
        m_toProcessOnDemandReflection.clear();
        for (const auto& reflectPair : toProcess)
        {
            m_currentlyProcessingTypeIds.emplace_back(reflectPair.first);
            reflectPair.second(this);
            m_currentlyProcessingTypeIds.pop_back();
        }
    }
}