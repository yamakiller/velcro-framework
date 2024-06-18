#ifndef V_FRAMEWORK_CORE_EVENT_BUS_IEVENT_SCHEDULER_H
#define V_FRAMEWORK_CORE_EVENT_BUS_IEVENT_SCHEDULER_H

#include <core/interface/interface.h>
#include <core/time/itime.h>
#include <core/event_bus/event_bus.h>
#include <core/std/smart_ptr/unique_ptr.h>

namespace V
{
    class Name;
    class ScheduledEvent;
    class ScheduledEventHandle;

    //! @class IEventScheduler
    //! @brief This is an V::Interface<> for managing scheduled events.
    //! Users should not require any direct interaction with this interface, ScheduledEvent is a self contained abstraction.
    class IEventScheduler
    {
    public:

        using ScheduledEventHandlePtr = VStd::unique_ptr<ScheduledEventHandle>;

        IEventScheduler() = default;
        virtual ~IEventScheduler() = default;

        //! Adds a scheduled event to run in durationMs.
        //! Actual duration is not guaranteed but will not be less than the value provided.
        //! @param scheduledEvent a scheduled event to add
        //! @param durationMs a millisecond interval to run the scheduled event
        //! @return pointer to the handle for this scheduled event, IEventScheduler maintains ownership
        virtual ScheduledEventHandle* AddEvent(ScheduledEvent* scheduledEvent, TimeMs durationMs) = 0;

        //! Schedules a callback to run in durationMs.
        //! Actual duration is not guaranteed but will not be less than the value provided.
        //! @param callback a callback to invoke after durationMs
        //! @param eventName a text descriptor of the callback
        //! @param durationMs a millisecond interval to run the scheduled callback
        virtual void AddCallback(const VStd::function<void()>& callback, const Name& eventName, TimeMs durationMs) = 0;

        V_DISABLE_COPY_MOVE(IEventScheduler);
    };

    // EBus wrapper for ScriptCanvas
    class IEventSchedulerRequests : public V::EventBusTraits
    {
    public:
        static const V::EventBusHandlerPolicy HandlerPolicy = V::EventBusHandlerPolicy::Single;
        static const V::EventBusAddressPolicy AddressPolicy = V::EventBusAddressPolicy::Single;
    };
    using IEventSchedulerRequestBus = V::EventBus<IEventScheduler, IEventSchedulerRequests>;
}

#endif // V_FRAMEWORK_CORE_EVENT_BUS_IEVENT_SCHEDULER_H