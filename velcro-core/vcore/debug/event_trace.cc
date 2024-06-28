#include <vcore/debug/event_trace.h>
#include <vcore/debug/event_trace_detector_bus.h>
#include <vcore/std/time.h>
#include <vcore/std/parallel/thread.h>

namespace V
{
    namespace Debug
    {
        EventTrace::ScopedSlice::ScopedSlice(const char* name, const char* category)
            : m_Name(name)
            , m_Category(category)
            , m_Time(VStd::GetTimeNowMicroSecond())
        {}

        EventTrace::ScopedSlice::~ScopedSlice()
        {
            EventTraceDetectorBus::TryQueueBroadcast(&EventTraceDetectorInterface::RecordSlice, m_Name, m_Category, VStd::this_thread::get_id(), m_Time, (uint32_t)(VStd::GetTimeNowMicroSecond() - m_Time));
        }
    }
}
