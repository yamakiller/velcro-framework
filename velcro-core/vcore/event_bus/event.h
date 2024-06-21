/*
 * Copyright (c) Contributors to the VelcroFramework Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <vcore/base.h>
#include <vcore/casting/numeric_cast.h>
#include <vcore/memory/system_allocator.h>
#include <vcore/std/containers/vector.h>
#include <vcore/std/containers/stack.h>
#include <vcore/std/function/function_template.h>

namespace V
{
    //! A specialized type of EventBus useful for eventing on a specific event type
    //! Whereas with EventBus you would have to define a bus, implement the bus, and manage connecting and disconnecting from the bus
    //! Event only requires you declare an Event<>, and then connect Event<>::Handlers
    //! Note that this system does not provide *any* thread safety, Handler Connect and Disconnect must happen on the same thread dispatching Events
    //! It is safe to connect or disconnect handlers during a signal, in this case we don't guarantee the signal will be dispatched to the disconnected handler
    //! Example Usage:
    //! @code{.cpp}
    //!      {
    //!          Event<int32_t> event; // An Event that can event a single int32_t
    //!          Event<int32_t>::Handler handler([](int32_t value) { DO_SOMETHING_WITH_VALUE(value); });
    //!          handler.Connect(event); // Our handler is now connected to the Event
    //!          event.Signal(1); // Our handlers lambda will now get invoked with the value 1
    //!      };
    //! @endcode

    template <typename... Params>
    class Event;

    template <typename... Params>
    class EventHandler;

    template <typename... Params>
    class Event final
    {
        friend class EventHandler<Params...>; // This is required in order to allow for relocating the handle pointers on a move

    public:

        using Callback = VStd::function<void(Params...)>;
        using Handler = EventHandler<Params...>;       

        VELCRO_TYPE_INFO(Event, "{c77781c1-ec05-4e69-b664-e8151c7b9fe6}");
        V_CLASS_ALLOCATOR(Event<Params...>, V::SystemAllocator, 0);

        Event() = default;
        Event(Event&& rhs);
        
        ~Event();

        Event& operator=(Event&& rhs);

        //! Take the handlers registered with the other event
        //! and move them to this event. The other will event
        //! will be cleared after call
        //! @param other event to move handlers
        Event& ClaimHandlers(Event&& other);

        //! Returns true if at least one handler is connected to this event.
        bool HasHandlerConnected() const;

        //! Disconnects all connected handlers.
        void DisconnectAllHandlers();

        //! Signal an event.
        //! @param params variadic set of event parameters
        void Signal(const Params&... params) const;

    private:

        //! Used internally to rebind all handlers from an old event to the current event instance
        void BindHandlerEventPointers();

        void Connect(Handler& handler) const;
        void Disconnect(Handler& handler) const;

    private:

        // Note that these are mutable because we want Signal() to be const, but we do a bunch of book-keeping during Signal()
        mutable VStd::vector<Handler*> m_handlers; //< Active handlers
        mutable VStd::vector<Handler*> m_addList; //< Handlers added during a Signal, pending being added to the active handlers
        mutable VStd::stack<size_t> m_freeList; //< Set of unused handler indices

        mutable bool m_updating = false; //< Raised during a Signal, false otherwise, used to guard m_handlers during handler iteration
    };

    //! A handler class that can connect to an Event
    template <typename... Params>
    class EventHandler final
    {
        friend class Event<Params...>;

    public:
        using Callback = VStd::function<void(Params...)>;

        VELCRO_TYPE_INFO(EventHandler, "{f82884fa-12c0-4333-b126-c213c5ff941d}");
        V_CLASS_ALLOCATOR(EventHandler<Params...>, V::SystemAllocator, 0);

        // We support default constructing of event handles (with no callback function being bound) to allow for better usage with container types
        // An unbound event handle cannot be added to an event and we do not support dynamically binding the callback post construction
        // (except for on assignment since that will also add the handle to the event; i.e. there is no way to unbind the callback after being added to an event)
        EventHandler() = default;
        explicit EventHandler(std::nullptr_t);
        explicit EventHandler(Callback callback);
        EventHandler(const EventHandler& rhs);
        EventHandler(EventHandler&& rhs);

        ~EventHandler();

        EventHandler& operator=(const EventHandler& rhs);
        EventHandler& operator=(EventHandler&& rhs);

        //! Connects the handler to the provided event.
        //! @param event the Event to connect to
        void Connect(Event<Params...>& event);

        //! Disconnects the handler from its connected event, does nothing if the event is not connected.
        void Disconnect();

        //! Returns true if this handler is connected to an event.
        //! @return boolean true if this handler is connected to an event
        bool IsConnected() const;

    private:

        //! Swaps the event handler pointers from the from instance to this instance
        //! @param from the handler instance we are replacing on the attached event
        void SwapEventHandlerPointers(const EventHandler& from);

        const Event<Params...>* m_event = nullptr; //< The connected event
        int32_t m_index = 0; //< Index into the add or handler vectors (negative means pending add)
        Callback m_callback; //< The lambda to invoke during events
    };
}

#include <vcore/event_bus/event.inl>
