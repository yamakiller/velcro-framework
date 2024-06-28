#ifndef V_FRAMEWORK_CORE_DEBUG_EVENT_TRACE_DETECTOR_BUS_H
#define V_FRAMEWORK_CORE_DEBUG_EVENT_TRACE_DETECTOR_BUS_H

#include <vcore/detector/detector_bus.h>
#include <vcore/std/time.h>
#include <vcore/std/string/string.h>

namespace VStd
{
    struct thread_id;
}

namespace V
{
    namespace Debug
    {
        class EventTraceDetectorInterface
            : public DetectorEventBusTraits
        {
        public:
            //////////////////////////////////////////////////////////////////////////
            // EventBusTraits overrides
            static const V::EventBusHandlerPolicy HandlerPolicy = V::EventBusHandlerPolicy::Single;
            static const bool EnableEventQueue = true;
            static const bool EventQueueingActiveByDefault = false;
            //////////////////////////////////////////////////////////////////////////

            virtual ~EventTraceDetectorInterface() {}

            virtual void RecordSlice(
                const char* name,
                const char* category,
                const VStd::thread_id threadId,
                V::u64 timestamp,
                V::u32 duration) = 0;

            virtual void RecordInstantThread(
                const char* name,
                const char* category,
                const VStd::thread_id threadId,
                V::u64 timestamp) = 0;

            virtual void RecordInstantGlobal(
                const char* name,
                const char* category,
                V::u64 timestamp) = 0;
        };

        typedef V::EventBus<EventTraceDetectorInterface> EventTraceDetectorBus;

        class EventTraceDetectorSetupInterface
            : public DetectorEventBusTraits
        {
        public:
            //////////////////////////////////////////////////////////////////////////
            // EventBusTraits overrides
            static const V::EventBusHandlerPolicy HandlerPolicy = V::EventBusHandlerPolicy::Single;
            //////////////////////////////////////////////////////////////////////////

            virtual ~EventTraceDetectorSetupInterface() {}

            virtual void SetThreadName(const VStd::thread_id& threadId, const char* name) = 0;
        };

        typedef V::EventBus<EventTraceDetectorSetupInterface> EventTraceDetectorSetupBus;
    }
}

#define V_TRACE_INSTANT_GLOBAL_CATEGORY(name, category) \
    EBUS_QUEUE_EVENT(V::Debug::EventTraceDetectorBus, RecordInstantGlobal, name, category, VStd::GetTimeNowMicroSecond())
#define V_TRACE_INSTANT_GLOBAL(name) V_TRACE_INSTANT_GLOBAL_CATEGORY(name, "")

#define V_TRACE_INSTANT_THREAD_CATEGORY(name, category) \
    EBUS_QUEUE_EVENT(V::Debug::EventTraceDetectorBus, RecordInstantThread, name, category, VStd::this_thread::get_id(), VStd::GetTimeNowMicroSecond())
#define V_TRACE_INSTANT_THREAD(name) V_TRACE_INSTANT_THREAD_CATEGORY(name, "")

#endif // V_FRAMEWORK_CORE_DEBUG_EVENT_TRACE_DETECTOR_BUS_H