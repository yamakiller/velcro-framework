#include <utility>
#include <vcore/casting/numeric_cast.h>
#include <platforms/common/winapi/vcore/io/Streamer/streamer_context_winapi.h>
#include <vcore/debug/profiler.h>

namespace V::Platform {
    StreamerContextThreadSync::StreamerContextThreadSync() {
        for (size_t i = 0; i < MAXIMUM_WAIT_OBJECTS; ++i)
        {
            HANDLE event = CreateEvent(nullptr, true, false, nullptr);
            if (event) {
                m_events[i] = event;
            } else {
                V_Assert(event, "Failed to create a required event for IO Scheduler (Error: %u).", ::GetLastError());
            }
        }
    }

    StreamerContextThreadSync::~StreamerContextThreadSync()
    {
        for (size_t i = 0; i < MAXIMUM_WAIT_OBJECTS; ++i)
        {
            HANDLE event = m_events[i];
            if (event)
            {
                if (!::CloseHandle(event))
                {
                    V_Assert(false, "Failed to close an event handle for IO Scheduler (Error: %u)", ::GetLastError());
                }
            }
        }
    }

    void StreamerContextThreadSync::Suspend()
    {
        V_Assert(m_events[0], "There is no synchronization event created for the main streamer thread to use to suspend.");

        DWORD result = ::WaitForMultipleObjects(m_handleCount, m_events, false, INFINITE);
        if (result < WAIT_OBJECT_0 + m_handleCount)
        {
            DWORD index = result - WAIT_OBJECT_0;
            ::ResetEvent(m_events[index]);
        }
        else
        {
            V_Assert(false, "Unexpected wait result: %u.", result);
        }
    }

    void StreamerContextThreadSync::Resume()
    {
        V_Assert(m_events[0], "There is no synchronization event created for the main streamer thread to use to resume.");
        ::SetEvent(m_events[0]);
    }

    HANDLE StreamerContextThreadSync::CreateEventHandle()
    {
        V_Assert(m_handleCount < MAXIMUM_WAIT_OBJECTS, "There are no more slots available to allocate a new IO event in.");
        return m_events[m_handleCount++];
    }

    void StreamerContextThreadSync::DestroyEventHandle(HANDLE event)
    {
        V_Assert(m_handleCount > 1, "There are no more IO events that can be destroyed.");

        for (size_t i = 1; i < m_handleCount; ++i)
        {
            if (m_events[i] == event)
            {
                m_handleCount--;
                VStd::swap(m_events[i], m_events[m_handleCount]);
                return;
            }
        }

        V_Assert(false, "IO event couldn't be destroyed as it wasn't found.");
    }

    DWORD StreamerContextThreadSync::GetEventHandleCount() const
    {
        return m_handleCount - 1;
    }

    bool StreamerContextThreadSync::AreEventHandlesAvailable() const
    {
        return m_handleCount < MAXIMUM_WAIT_OBJECTS;
    }
}