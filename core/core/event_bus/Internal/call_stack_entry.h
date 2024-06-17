
#ifndef V_FRAMEWORK_CORE_EVENT_BUS_INTERNAL_CALL_STACK_ENTRY_H
#define V_FRAMEWORK_CORE_EVENT_BUS_INTERNAL_CALL_STACK_ENTRY_H

#include <core/event_bus/internal/debug.h>

#include <core/std/parallel/lock.h>

namespace V
{
    template <typename Interface, typename Traits>
    class EventBus;

    namespace Internal
    {
        // Common implementation between single and multithreaded versions
        template <typename Interface, typename Traits>
        struct CallstackEntryBase
        {
            using BusType = EventBus<Interface, Traits>;

            CallstackEntryBase(const typename Traits::BusIdType* id)
                : m_busId(id)
            { }

            virtual ~CallstackEntryBase() = default;

            virtual void OnRemoveHandler(Interface* handler)
            {
                if (m_prev)
                {
                    m_prev->OnRemoveHandler(handler);
                }
            }

            virtual void OnPostRemoveHandler()
            {
                if (m_prev)
                {
                    m_prev->OnPostRemoveHandler();
                }
            }

            virtual void SetRouterProcessingState(typename BusType::RouterProcessingState /*state*/) { }

            virtual bool IsRoutingQueuedEvent() const { return false; }

            virtual bool IsRoutingReverseEvent() const { return false; }

            const typename Traits::BusIdType* m_busId;
            CallstackEntryBase<Interface, Traits>* m_prev = nullptr;
        };

        // Single threaded callstack entry
        template <typename Interface, typename Traits>
        struct CallstackEntry
            : public CallstackEntryBase<Interface, Traits>
        {
            using BusType = EventBus<Interface, Traits>;
            using BusContextPtr = typename BusType::Context*;

            CallstackEntry(BusContextPtr context, const typename Traits::BusIdType* busId)
                : CallstackEntryBase<Interface, Traits>(busId)
                , m_threadId(VStd::this_thread::get_id().m_id)
            {
                EBUS_ASSERT(context, "Internal error: context deleted while execution still in progress.");
                m_context = context;

                this->m_prev = m_context->s_callstack->m_prev;

                // We don't use the V_Assert macro here because it places the assert call (unlikely) before the
                // actual work to do (likely) which results in inlining shenanigans and icache misses.
                if (!this->m_prev || static_cast<CallstackEntry*>(this->m_prev)->m_threadId == m_threadId)
                {
                    m_context->s_callstack->m_prev = this;

                    m_context->m_dispatches++;
                }
                else
                {
                    AZ::Debug::Trace::Instance().Assert(__FILE__, __LINE__, AZ_FUNCTION_SIGNATURE,
                        "Bus %s has multiple threads in its callstack records. Configure MutexType on the bus, or don't send to it from multiple threads", BusType::GetName());
                }
            }

            CallstackEntry(CallstackEntry&&) = default;

            CallstackEntry(const CallstackEntry&) = delete;
            CallstackEntry& operator=(const CallstackEntry&) = delete;
            CallstackEntry& operator=(CallstackEntry&&) = delete;

            ~CallstackEntry() override
            {
                m_context->m_dispatches--;

                m_context->s_callstack->m_prev = this->m_prev;
            }

            BusContextPtr m_context = nullptr;
            VStd::native_thread_id_type m_threadId;
        };

        // One of these will be allocated per thread. It acts as the bottom of any callstack during dispatch within
        // that thread. It has to be stored in the context so that it is shared across DLLs. We accelerate this by
        // caching the root into a thread_local pointer (Context::s_callstack) on first access. Since global bus contexts
        // never die, the TLS pointer does not need to be lifetime managed.
        template <typename Interface, typename Traits>
        struct CallstackEntryRoot
            : public CallstackEntryBase<Interface, Traits>
        {
            using BusType = EventBus<Interface, Traits>;

            CallstackEntryRoot()
                : CallstackEntryBase<Interface, Traits>(nullptr)
            {}

            void OnRemoveHandler(Interface*) override { V_Assert(false, "Callstack root should never attempt to handle the removal of a bus handler"); }
            void OnPostRemoveHandler() override { V_Assert(false, "Callstack root should never attempt to handle the removal of a bus handler"); }
            void SetRouterProcessingState(typename BusType::RouterProcessingState) override { V_Assert(false, "Callstack root should never attempt to alter router processing state"); }
            bool IsRoutingQueuedEvent() const override { return false; }
            bool IsRoutingReverseEvent() const override { return false; }
        };

        template <class C, bool UseTLS /*= false*/>
        struct EventBusCallstackStorage
        {
            C* m_entry = nullptr;

            EventBusCallstackStorage() = default;
            ~EventBusCallstackStorage() = default;
            EventBusCallstackStorage(const EventBusCallstackStorage&) = delete;
            EventBusCallstackStorage(EventBusCallstackStorage&&) = delete;

            C* operator->() const
            {
                return m_entry;
            }

            C& operator*() const
            {
                return *m_entry;
            }

            C* operator=(C* entry)
            {
                m_entry = entry;
                return m_entry;
            }

            operator C*() const
            {
                return m_entry;
            }
        };

        template <class C>
        struct EventBusCallstackStorage<C, true>
        {
            static V_THREAD_LOCAL C* _entry;

            EventBusCallstackStorage() = default;
            ~EventBusCallstackStorage() = default;
            EventBusCallstackStorage(const EventBusCallstackStorage&) = delete;
            EventBusCallstackStorage(EventBusCallstackStorage&&) = delete;

            C* operator->() const
            {
                return _entry;
            }

            C& operator*() const
            {
                return *_entry;
            }

            C* operator=(C* entry)
            {
                _entry = entry;
                return _entry;
            }

            operator C*() const
            {
                return _entry;
            }
        };
    }
}

#endif // V_FRAMEWORK_CORE_EVENT_BUS_INTERNAL_CALL_STACK_ENTRY_H