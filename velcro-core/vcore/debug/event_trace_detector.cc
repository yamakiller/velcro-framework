#include <vcore/debug/event_trace_detector.h>
#include <vcore/debug/event_trace.h>
#include <vcore/std/containers/array.h>
#include <algorithm>

namespace V
{
    namespace Debug
    {
        namespace Crc
        {
            const u32 EventTraceDetector = V_CRC("EventTraceDetector", 0xf7aeae55);
            const u32 Slice = V_CRC("Slice", 0x3dae78a5);
            const u32 ThreadInfo = V_CRC("ThreadInfo", 0x89bf78be);
            const u32 Name = V_CRC("Name", 0x5e237e06);
            const u32 Category = V_CRC("Category", 0x064c19c1);
            const u32 ThreadId = V_CRC("ThreadId", 0xd0fd9043);
            const u32 Timestamp = V_CRC("Timestamp", 0xa5d6e63e);
            const u32 Duration = V_CRC("Duration", 0x865f80c0);
            const u32 Instant = V_CRC("Instant", 0x0e9047ad);
        }

        EventTraceDetector::EventTraceDetector()
        {
            EventTraceDetectorSetupBus::Handler::BusConnect();
            VStd::ThreadDetectorEventBus::Handler::BusConnect();
        }

        EventTraceDetector::~EventTraceDetector()
        {
            VStd::ThreadDetectorEventBus::Handler::BusDisconnect();
            EventTraceDetectorSetupBus::Handler::BusDisconnect();
        }

        void EventTraceDetector::Start(const Param* params, int numParams)
        {
            (void)params;
            (void)numParams;

            EventTraceDetectorBus::Handler::BusConnect();
            //TickBus::Handler::BusConnect();

            EventTraceDetectorBus::AllowFunctionQueuing(true);
        }

        void EventTraceDetector::Stop()
        {
            EventTraceDetectorBus::AllowFunctionQueuing(false);
            EventTraceDetectorBus::ClearQueuedEvents();

            EventTraceDetectorBus::Handler::BusDisconnect();
            //TickBus::Handler::BusDisconnect();
        }

        /*void EventTraceDetector::OnTick(float deltaTime, ScriptTimePoint time)
        {
            (void)deltaTime;
            (void)time;

            V_TRACE_METHOD();
            RecordThreads();
            EventTraceDetectorBus::ExecuteQueuedEvents();
        }*/

        void EventTraceDetector::SetThreadName(const VStd::thread_id& id, const char* name)
        {
            VStd::lock_guard<VStd::recursive_mutex> lock(m_ThreadMutex);
            m_Threads[(size_t)id.m_id] = ThreadData{ name };
        }

        void EventTraceDetector::OnThreadEnter(const VStd::thread::id& id, const VStd::thread_desc* desc)
        {
            if (desc && desc->m_name)
            {
                SetThreadName(id, desc->m_name);
            }
        }

        void EventTraceDetector::OnThreadExit(const VStd::thread::id& id)
        {
            VStd::lock_guard<VStd::recursive_mutex> lock(m_ThreadMutex);
            m_Threads.erase((size_t)id.m_id);
        }

        void EventTraceDetector::RecordThreads()
        {
            if (m_output && m_Threads.size())
            {
                // Main bus mutex guards m_output.
                auto& context = EventTraceDetectorBus::GetOrCreateContext();

                VStd::scoped_lock<decltype(context.m_contextMutex), decltype(m_ThreadMutex)> lock(context.m_contextMutex, m_ThreadMutex);
                for (const auto& keyValue : m_Threads)
                {
                    m_output->BeginTag(Crc::EventTraceDetector);
                    m_output->BeginTag(Crc::ThreadInfo);
                    m_output->Write(Crc::ThreadId, keyValue.first);
                    m_output->Write(Crc::Name, keyValue.second.name);
                    m_output->EndTag(Crc::ThreadInfo);
                    m_output->EndTag(Crc::EventTraceDetector);
                }
            }
        }

        void EventTraceDetector::RecordSlice(
            const char* name,
            const char* category,
            const VStd::thread_id threadId,
            V::u64 timestamp,
            V::u32 duration)
        {
            m_output->BeginTag(Crc::EventTraceDetector);
            m_output->BeginTag(Crc::Slice);
            m_output->Write(Crc::Name, name);
            m_output->Write(Crc::Category, category);
            m_output->Write(Crc::ThreadId, (size_t)threadId.m_id);
            m_output->Write(Crc::Timestamp, timestamp);
            m_output->Write(Crc::Duration, std::max(duration, 1u));
            m_output->EndTag(Crc::Slice);
            m_output->EndTag(Crc::EventTraceDetector);
        }

        void EventTraceDetector::RecordInstantGlobal(
            const char* name,
            const char* category,
            V::u64 timestamp)
        {
            m_output->BeginTag(Crc::EventTraceDetector);
            m_output->BeginTag(Crc::Instant);
            m_output->Write(Crc::Name, name);
            m_output->Write(Crc::Category, category);
            m_output->Write(Crc::Timestamp, timestamp);
            m_output->EndTag(Crc::Instant);
            m_output->EndTag(Crc::EventTraceDetector);
        }

        void EventTraceDetector::RecordInstantThread(
            const char* name,
            const char* category,
            const VStd::thread_id threadId,
            V::u64 timestamp)
        {
            m_output->BeginTag(Crc::EventTraceDetector);
            m_output->BeginTag(Crc::Instant);
            m_output->Write(Crc::Name, name);
            m_output->Write(Crc::Category, category);
            m_output->Write(Crc::ThreadId, (size_t)threadId.m_id);
            m_output->Write(Crc::Timestamp, timestamp);
            m_output->EndTag(Crc::Instant);
            m_output->EndTag(Crc::EventTraceDetector);
        }
    }
}
