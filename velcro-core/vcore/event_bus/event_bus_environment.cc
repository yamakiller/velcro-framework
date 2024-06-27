/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <vcore/math/crc.h>
#include <vcore/event_bus/event_bus.h>

namespace V
{
    namespace Internal
    {
        //////////////////////////////////////////////////////////////////////////
        ContextBase::ContextBase()
        {
            // When we create Global context we store addition information for access 
            // to the TLS Environment and context index to support EBusEnvironmnets
            // hold a reference to the variable (even though we will never release it)
            static EnvironmentVariable<Internal::EventBusEnvironmentTLSAccessors> _tlsAccessor = nullptr;
            if (!_tlsAccessor)
            {
                _tlsAccessor = Environment::CreateVariable<Internal::EventBusEnvironmentTLSAccessors>(Internal::EventBusEnvironmentTLSAccessors::GetId());
            }

            m_ebusEnvGetter = _tlsAccessor->m_getter;
            m_ebusEnvTLSIndex = _tlsAccessor->NumUniqueEventBuses++;
        }

        //////////////////////////////////////////////////////////////////////////
        ContextBase::ContextBase(EventBusEnvironment* environment)
            : m_ebusEnvGetter(nullptr)
            , m_ebusEnvTLSIndex(-1)
        {
            // we need m_ebusEnvGetter and m_ebusEnvTLSIndex only in the global context
            (void)environment;
        }

        //////////////////////////////////////////////////////////////////////////
        V_THREAD_LOCAL EventBusEnvironment* EventBusEnvironmentTLSAccessors::_tlsCurrentEnvironment = nullptr;

        //////////////////////////////////////////////////////////////////////////
        EventBusEnvironmentTLSAccessors::EventBusEnvironmentTLSAccessors()
            : m_getter(&GetTLSEnvironment)
            , m_setter(&SetTLSEnvironment)
            , NumUniqueEventBuses(0)
        {
        }

        //////////////////////////////////////////////////////////////////////////
        u32 EventBusEnvironmentTLSAccessors::GetId()
        {
            return V_CRC_CE("EventBusEnvironmentTLSAccessors");
        }

        //////////////////////////////////////////////////////////////////////////
        EventBusEnvironment* EventBusEnvironmentTLSAccessors::GetTLSEnvironment()
        {
            return _tlsCurrentEnvironment;
        }

        //////////////////////////////////////////////////////////////////////////
        void EventBusEnvironmentTLSAccessors::SetTLSEnvironment(EventBusEnvironment* environment)
        {
            _tlsCurrentEnvironment = environment;
        }
    } // namespace Internal

    //////////////////////////////////////////////////////////////////////////
    EventBusEnvironment::EventBusEnvironment()
    {
        m_tlsAccessor = Environment::CreateVariable<Internal::EventBusEnvironmentTLSAccessors>(Internal::EventBusEnvironmentTLSAccessors::GetId());

#ifdef V_DEBUG_BUILD
        m_stackPrevEnvironment = reinterpret_cast<EventBusEnvironment*>(V_INVALID_POINTER);
#endif // V_DEBUG_BUILD
    }

    //////////////////////////////////////////////////////////////////////////
    EventBusEnvironment::~EventBusEnvironment()
    {
#ifdef V_DEBUG_BUILD
        V_Assert(m_stackPrevEnvironment == reinterpret_cast<EventBusEnvironment*>(V_INVALID_POINTER), "You can't destroy BusEvironment %p while it's active, make sure you PopBusEnvironment when not in use!", this);
#endif // V_DEBUG_BUILD

        if (!m_busContexts.empty())
        {
            for (auto& busContextIsOwnerPair : m_busContexts)
            {
                if (busContextIsOwnerPair.second)
                {
                    busContextIsOwnerPair.first->~ContextBase();
                    V_OS_FREE(busContextIsOwnerPair.first);
                }
            }
        }
    }

    //////////////////////////////////////////////////////////////////////////
    void EventBusEnvironment::ActivateOnCurrentThread()
    {
#ifdef V_DEBUG_BUILD
        V_Assert(m_stackPrevEnvironment == reinterpret_cast<EventBusEnvironment*>(V_INVALID_POINTER), "Environment %p is already active on another thread. This is illegal!", this);
#endif // V_DEBUG_BUILD

        m_stackPrevEnvironment = m_tlsAccessor->m_getter();
        m_tlsAccessor->m_setter(this);
    }

    //////////////////////////////////////////////////////////////////////////
    void EventBusEnvironment::DeactivateOnCurrentThread()
    {
#ifdef V_DEBUG_BUILD
        V_Assert(m_stackPrevEnvironment != reinterpret_cast<EventBusEnvironment*>(V_INVALID_POINTER), "Environment %p is not active you can't call Deactivate!", this);
#endif // V_DEBUG_BUILD

        m_tlsAccessor->m_setter(m_stackPrevEnvironment);

#ifdef V_DEBUG_BUILD
        m_stackPrevEnvironment = reinterpret_cast<EventBusEnvironment*>(V_INVALID_POINTER);
#endif // V_DEBUG_BUILD
    }

    //////////////////////////////////////////////////////////////////////////
    Internal::ContextBase* EventBusEnvironment::FindContext(int tlsKey)
    {
        if (tlsKey >= m_busContexts.size())
        {
            return nullptr;
        }
        return m_busContexts[tlsKey].first;
    }

    //////////////////////////////////////////////////////////////////////////
    bool EventBusEnvironment::InsertContext(int tlsKey, Internal::ContextBase* context, bool isTakeOwnership)
    {
        if (tlsKey >= m_busContexts.size())
        {
            m_busContexts.resize(tlsKey+1, VStd::make_pair(nullptr,false));
        }
        
        if (m_busContexts[tlsKey].first != nullptr)
        {
            return false; // we already context at this key
        }

        m_busContexts[tlsKey] = VStd::make_pair(context, isTakeOwnership);
        return true;
    }

    //////////////////////////////////////////////////////////////////////////
    EventBusEnvironment* EventBusEnvironment::Create()
    {
        return new(V_OS_MALLOC(sizeof(EventBusEnvironment), VStd::alignment_of<EventBusEnvironment>::value)) EventBusEnvironment;
    }

    //////////////////////////////////////////////////////////////////////////
    void EventBusEnvironment::Destroy(EventBusEnvironment* environment)
    {
        if (environment)
        {
            environment->~EventBusEnvironment();
            V_OS_FREE(environment);
        }
    }

} // namespace V
