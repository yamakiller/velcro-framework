#ifndef V_FRAMEWORK_CORE_DEBUG_EVENT_TRACE_DETECTOR_H
#define V_FRAMEWORK_CORE_DEBUG_EVENT_TRACE_DETECTOR_H

#include <vcore/detector/detector.h>
//#include <vcore/component/tick_bus.h>
#include <vcore/std/parallel/thread_bus.h>
#include <vcore/std/containers/array.h>
#include <vcore/debug/event_trace_detector_bus.h>

namespace V
{
    namespace Debug
    {
        class EventTraceDetector
            : public Detector
            , public EventTraceDetectorBus::Handler
            , public EventTraceDetectorSetupBus::Handler
            , public VStd::ThreadDetectorEventBus::Handler
            //, public V::TickBus::Handler
        {
        public:
            V_CLASS_ALLOCATOR(EventTraceDetector, OSAllocator, 0)

            EventTraceDetector();
            virtual ~EventTraceDetector();

        private:
            // Driller
            //////////////////////////////////////////////////////////////////////////
            const char*  GroupName() const override { return "SystemDetectors"; }
            const char*  GetName() const override { return "EventTraceDetector"; }
            const char*  GetDescription() const override { return "Handles timed events for a Chrome Tracing."; }
            void         Start(const Param* params = NULL, int numParams = 0) override;
            void         Stop() override;

            // ThreadBus
            //////////////////////////////////////////////////////////////////////////
            void OnThreadEnter(const VStd::thread::id& id, const VStd::thread_desc* desc) override;
            void OnThreadExit(const VStd::thread::id& id) override;

            // TickBus
            //////////////////////////////////////////////////////////////////////////
            //void OnTick(float deltaTime, ScriptTimePoint time) override;

            // EventTraceDetectorSetupBus
            //////////////////////////////////////////////////////////////////////////
            void SetThreadName(const VStd::thread_id& threadId, const char* name) override;

            // EventTraceDetectorBus
            //////////////////////////////////////////////////////////////////////////
            void RecordSlice(
                const char* name,
                const char* category,
                const VStd::thread_id threadId,
                V::u64 timestamp,
                V::u32 duration) override;

            void RecordInstantGlobal(
                const char* name,
                const char* category,
                V::u64 timestamp) override;

            void RecordInstantThread(
                const char* name,
                const char* category,
                const VStd::thread_id threadId,
                V::u64 timestamp) override;

            void RecordThreads();

            struct ThreadData
            {
                VStd::string name;
            };

            VStd::recursive_mutex m_ThreadMutex;
            VStd::unordered_map<size_t, ThreadData, VStd::hash<size_t>, VStd::equal_to<size_t>, OSStdAllocator> m_Threads;
        };
    }
} // namespace V


#endif // V_FRAMEWORK_CORE_DEBUG_EVENT_TRACE_DETECTOR_H