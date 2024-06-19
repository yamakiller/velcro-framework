#include <vcore/event_bus/scheduled_event_handle.h>
#include <vcore/event_bus/scheduled_event.h>
//#include <vcore/console/ILogger.h>

namespace V
{
    ScheduledEventHandle::ScheduledEventHandle(TimeMs executeTimeMs, TimeMs durationTimeMs, ScheduledEvent* scheduledEvent, bool ownsScheduledEvent)
        : m_executeTimeMs(executeTimeMs)
        , m_durationMs(durationTimeMs)
        , m_event(scheduledEvent)
        , m_ownsScheduledEvent(ownsScheduledEvent)
    {
        ;
    }

    bool ScheduledEventHandle::operator >(const ScheduledEventHandle& rhs) const
    {
        return m_executeTimeMs > rhs.m_executeTimeMs;
    }

    bool ScheduledEventHandle::Notify()
    {
        if (m_event)
        {
            if (m_event->m_handle == this)
            {
                m_event->Notify();

                // Check whether or not the event was deleted during the callback
                if (m_event != nullptr)
                {
                    if (m_event->m_autoRequeue)
                    {
                        m_event->Requeue();
                        return true;
                    }
                    else // Not configured to auto-requeue, so remove the handle
                    {
                        m_event->ClearHandle();
                    }
                }
            }
            else
            {
                //VLOG_WARN("ScheduledEventHandle event pointer doesn't match to the pointer of handle to the event.");
            }
        }
        return false; // Event has been deleted, so the handle class must be deleted after this function.
    }

    TimeMs ScheduledEventHandle::GetExecuteTimeMs() const
    {
        return m_executeTimeMs;
    }

    TimeMs ScheduledEventHandle::GetDurationTimeMs() const
    {
        return m_durationMs;
    }

    bool ScheduledEventHandle::GetOwnsScheduledEvent() const
    {
        return m_ownsScheduledEvent;
    }

    ScheduledEvent* ScheduledEventHandle::GetScheduledEvent() const
    {
        return m_event;
    }
}