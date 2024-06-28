#ifndef V_FRAMEWORK_CORE_EVENT_BUS_EVENT_BUS_H
#define V_FRAMEWORK_CORE_EVENT_BUS_EVENT_BUS_H

#include <vcore/event_bus/bus_impl.h>
#include <vcore/event_bus/environment.h>
#include <vcore/event_bus/results.h>
#include <vcore/event_bus/internal/debug.h>

 // Included for backwards compatibility purposes
#include <vcore/std/typetraits/typetraits.h>
#include <vcore/std/smart_ptr/unique_ptr.h>
#include <vcore/std/containers/unordered_set.h>

#include <vcore/std/typetraits/is_same.h>

#include <vcore/std/utils.h>
#include <vcore/std/parallel/scoped_lock.h>
#include <vcore/std/parallel/shared_mutex.h>

namespace VStd
{
    class recursive_mutex; // Forward declare for a static assert.
}

namespace V
{
    /**
     * %EventBusTraits are properties that you use to configure an EventBus.
     *
     * The key %EventBusTraits to understand are #AddressPolicy, which defines
     * how many addresses the EventBus contains, #HandlerPolicy, which
     * describes how many handlers can connect to each address, and
     * `BusIdType`, which is the type of ID that is used to address the EventBus
     * if addresses are used.
     *
     * For example, if you want an EventBus that makes requests of game objects
     * that each have a unique integer identifier, then define an EventBus with
     * the following traits:
     * @code{.cpp}
     *      // The EventBus has multiple addresses and each event is addressed to a
     *      // specific ID (the game object's ID), which corresponds to an address
     *      // on the bus.
     *      static const EventBusAddressPolicy AddressPolicy = EventBusAddressPolicy::ById;
     *
     *      // Each event is received by a single handler (the game object).
     *      static const EventBusHandlerPolicy HandlerPolicy = EventBusHandlerPolicy::Single;
     *
     *      // Events are addressed by this type of ID (the game object's ID).
     *      using BusIdType = int;
     * @endcode
     *
     */
    struct EventBusTraits
    {
    protected:

        /**
         * Note - the destructor is intentionally not virtual to avoid adding vtable overhead to every EventBusTraits derived class.
         */
        ~EventBusTraits() = default;

    public:
        /**
         * Allocator used by the EventBus.
         * The default setting is Internal EventBusEnvironmentAllocator
         * EventBus code stores their Context instances in static memory
         * Therfore the configured allocator must last as long as the EventBus in a module
         */
        using AllocatorType = V::Internal::EventBusEnvironmentAllocator;

        /**
         * Defines how many handlers can connect to an address on the EventBus
         * and the order in which handlers at each address receive events.
         * For available settings, see V::EventBusHandlerPolicy.
         * By default, an EventBus supports any number of handlers.
         */
        static constexpr EventBusHandlerPolicy HandlerPolicy = EventBusHandlerPolicy::Multiple;

        /**
         * Defines how many addresses exist on the EventBus.
         * For available settings, see V::EventBusAddressPolicy.
         * By default, an EventBus uses a single address.
         */
        static constexpr EventBusAddressPolicy AddressPolicy = EventBusAddressPolicy::Single;

        /**
         * The type of ID that is used to address the EventBus.
         * Used only when the #AddressPolicy is V::EventBusAddressPolicy::ById
         * or V::EventBusAddressPolicy::ByIdAndOrdered.
         * The type must support `VStd::hash<ID>` and
         * `bool operator==(const ID&, const ID&)`.
         */
        using BusIdType = NullBusId;

        /**
         * Sorting function for EventBus address IDs.
         * Used only when the #AddressPolicy is V::EventBusAddressPolicy::ByIdAndOrdered.
         * If an event is dispatched without an ID, this function determines
         * the order in which each address receives the event.
         *
         * The following example shows a sorting function that meets these requirements.
         * @code{.cpp}
         * using BusIdOrderCompare = VStd::less<BusIdType>; // Lesser IDs first.
         * @endcode
         */
        using BusIdOrderCompare = NullBusIdCompare;

        /**
         * Sorting function for EventBus event handlers.
         * Used only when the #HandlerPolicy is V::EventBusHandlerPolicy::MultipleAndOrdered.
         * This function determines the order in which handlers at an address receive an event.
         *
         * By default, the function requires the handler to implement the following comparison
         * function.
         * @code{.cpp}
         * // Returns whether 'this' should precede 'other'.
         * bool Compare(const Interface* other) const;
         * @endcode
         */
        using BusHandlerOrderCompare = BusHandlerCompareDefault;

        /**
         * Locking primitive that is used when connecting handlers to the EventBus or executing events.
         * By default, all access is assumed to be single threaded and no locking occurs.
         * For multithreaded access, specify a mutex of the following type.
         * - For simple multithreaded cases, use VStd::mutex.
         * - For multithreaded cases where an event handler sends a new event on the same bus
         *   or connects/disconnects while handling an event on the same bus, use VStd::recursive_mutex.
         * - For specialized multithreading cases, such as allowing events to execute in parallel with each other but not with
         *   connects / disconnects, use custom mutex types along with custom LockGuard policies to control the specific locking
         *   requirements for each mutex use case (connection, dispatch, binding, callstack tracking).
         */
        using MutexType = NullMutex;

        /**
         * Specifies whether the EventBus supports an event queue.
         * You can use the event queue to execute events at a later time.
         * To execute the queued events, you must call
         * `<BusName>::ExecuteQueuedEvents()`.
         * By default, the event queue is disabled.
         */
        static constexpr bool EnableEventQueue = false;

        /**
         * Specifies whether the bus should accept queued messages by default or not.
         * If set to false, Bus::AllowFunctionQueuing(true) must be called before events are accepted.
         * Used only when #EnableEventQueue is true.
         */
        static constexpr bool EventQueueingActiveByDefault = true;

        /**
         * Specifies whether the EventBus supports queueing functions which take reference
         * arguments. This means that the sender is responsible for the lifetime of the
         * arguments (they should be static or class members or otherwise persistently stored).
         * You should only use this if you know that the data being passed as arguments will
         * outlive the dispatch of the queued event.
         */
        static constexpr bool EnableQueuedReferences = false;

        /**
         * Locking primitive that is used when adding and removing
         * events from the queue.
         * Not used for connection or event execution.
         * Used only when #EnableEventQueue is true.
         * If left unspecified, it will use the #MutexType.
         */
        using EventQueueMutexType = NullMutex;

        /**
         * Enables custom logic to run when a handler connects or
         * disconnects from the EventBus.
         * For example, you can make a handler execute an event
         * immediately upon connecting to the EventBus by modifying the
         * EventBusConnectionPolicy of the bus.
         * By default, no extra logic is run.
         */
        template <class Bus>
        using ConnectionPolicy = EventBusConnectionPolicy<Bus>;

        /**
        * Determines whether the bus will lock during dispatch
        * On buses where handlers are attached at startup and removed at shutdown,
        * or where connect/disconnect are not done from within handlers, this is safe
        * to do.
        * By default, the standard policy is used, which locks around all dispatches
        */
        static constexpr bool LocklessDispatch = false;

        /**
         * Specifies where EventBus data is stored.
         * This drives how many instances of this EventBus exist at runtime.
         * Available storage policies include the following:
         * - (Default) EventBusEnvironmentStoragePolicy - %EventBus data is stored
         * in the V::Environment. With this policy, a single %EventBus instance
         * is shared across all modules (DLLs) that attach to the V::Environment. It also
         * supports multiple EventBus environments.
         * - EventBusGlobalStoragePolicy - %EventBus data is stored in a global static variable.
         * With this policy, each module (DLL) has its own instance of the %EventBus.
         * - EventBusThreadLocalStoragePolicy - %EventBus data is stored in a thread_local static
         * variable. With this policy, each thread has its own instance of the %EventBus.
         *
         * \note Make sure you carefully consider the implication of switching this policy. If your code use EventBusEnvironments and your storage policy is not
         * complaint in the best case you will cause contention and unintended communication across environments, separation is a goal of environments. In the worst
         * case when you have listeners, you can receive messages when you environment is NOT active, potentially causing all kinds of havoc especially if you execute
         * environments in parallel.
         */
        template <class Context>
        using StoragePolicy = EventBusEnvironmentStoragePolicy<Context>;

        /**
         * Controls the flow of EventBus events.
         * Enables an event to be forwarded, and possibly stopped, before reaching
         * the normal event handlers.
         * Use cases for routing include tracing, debugging, and versioning an %EventBus.
         * The default `EventBusRouterPolicy` forwards the event to each connected
         * `EventBusRouterNode` before sending the event to the normal handlers. Each
         * node can stop the event or let it continue.
         */
        template <class Bus>
        using RouterPolicy = EventBusRouterPolicy<Bus>;

        /*
        * Performs the actual call on an Ebus handler.
        * Enables custom code to be run on a per-callee basis.
        * Use cases include debugging systems and profiling that need to run custom
        * code before or after an event.
        */
        using EventProcessingPolicy = EventBusEventProcessingPolicy;

        /**
         * The following Lock Guard classes are exposed so that it's possible to redefine them with custom lock/unlock functionality
         * when using custom types for the EventBus MutexType.
         */

       /**
        * Template Lock Guard class to use during event dispatches.
        * By default it will use a scoped_lock, but IsLocklessDispatch=true will cause it to use a NullLockGuard.
        * The IsLocklessDispatch bool is there to defer evaluation of the LocklessDispatch constant
        * Otherwise the value above in EventBusTraits.h is always used and not the value
        * that the derived trait class sets.
        */
        template <typename DispatchMutex, bool IsLocklessDispatch>
        using DispatchLockGuard = VStd::conditional_t<IsLocklessDispatch, V::Internal::NullLockGuard<DispatchMutex>, VStd::scoped_lock<DispatchMutex>>;

        /**
         * Template Lock Guard class to use during connection / disconnection.
         * By default it will use a unique_lock if the ContextMutex is anything but a NullMutex.
         * This can be overridden to provide a different locking policy with custom EventBus MutexType settings.
         * Also, some specialized policies execute handler methods which can cause unnecessary delays holding
         * the context mutex, such as performing blocking waits. These methods must unlock the context mutex before
         * doing so to prevent deadlocks, especially when the wait is for an event in another thread which is trying
         * to connect to the same bus before it can complete.
         */
        template<typename ContextMutex>
        using ConnectLockGuard = VStd::conditional_t<
            VStd::is_same_v<ContextMutex, V::NullMutex>,
            V::Internal::NullLockGuard<ContextMutex>,
            VStd::unique_lock<ContextMutex>>;

        /**
         * Template Lock Guard class to use for EventBus bind calls.
         * By default it will use a scoped_lock.
         * This can be overridden to provide a different locking policy with custom EventBus MutexType settings.
         */
        template<typename ContextMutex>
        using BindLockGuard = VStd::scoped_lock<ContextMutex>;

        /**
         * Template Lock Guard class to use for EventBus callstack tracking.
         * By default it will use a unique_lock if the ContextMutex is anything but a NullMutex.
         * This can be overridden to provide a different locking policy with custom EventBus MutexType settings.
         */
        template<typename ContextMutex>
        using CallstackTrackerLockGuard = VStd::conditional_t<
            VStd::is_same_v<ContextMutex, V::NullMutex>,
            V::Internal::NullLockGuard<ContextMutex>,
            VStd::unique_lock<ContextMutex>>;
    };

    namespace Internal
    {
        template<class EventBus>
        class EventBusRouter;

        template<class EventBus>
        class EventBusNestedVersionRouter;
    }

    template<class Interface, class BusTraits = Interface>
    class EventBus
        : public BusInternal::EventBusImpl<V::EventBus<Interface, BusTraits>, BusInternal::EventBusImplTraits<Interface, BusTraits>, typename BusTraits::BusIdType>
    {
    public:
        class Context;

        /**
         * Contains data about EventBusTraits.
         */
        using ImplTraits = BusInternal::EventBusImplTraits<Interface, BusTraits>;

        /**
         * Represents an %EventBus with certain broadcast, event, and routing functionality.
         */
        using BaseImpl = BusInternal::EventBusImpl<V::EventBus<Interface, BusTraits>, BusInternal::EventBusImplTraits<Interface, BusTraits>, typename BusTraits::BusIdType>;

        /**
         * Alias for EventBusTraits.
         */
        using Traits = typename ImplTraits::Traits;

        /**
         * The type of %EventBus, which is defined by the interface and the EventBusTraits.
         */
        using ThisType = V::EventBus<Interface, Traits>;

        /**
         * Allocator used by the %EventBus.
         * The default setting is VStd::allocator, which uses V::SystemAllocator.
         */
        using AllocatorType = typename ImplTraits::AllocatorType;

        /**
         * The class that defines the interface of the %EventBus.
         */
        using InterfaceType = typename ImplTraits::InterfaceType;

        /**
         * The events that are defined by the %EventBus interface.
         */
        using Events = typename ImplTraits::Events;

        /**
         * The type of ID that is used to address the %EventBus.
         * Used only when the address policy is V::EventBusAddressPolicy::ById
         * or V::EventBusAddressPolicy::ByIdAndOrdered.
         * The type must support `VStd::hash<ID>` and
         * `bool operator==(const ID&, const ID&)`.
         */
        using BusIdType = typename ImplTraits::BusIdType;

        /**
         * Sorting function for %EventBus address IDs.
         * Used only when the address policy is V::EventBusAddressPolicy::ByIdAndOrdered.
         * If an event is dispatched without an ID, this function determines
         * the order in which each address receives the event.
         *
         * The following example shows a sorting function that meets these requirements.
         * @code{.cpp}
         * using BusIdOrderCompare = VStd::less<BusIdType>; // Lesser IDs first.
         * @endcode
         */
        using BusIdOrderCompare = typename ImplTraits::BusIdOrderCompare;

        /**
         * Locking primitive that is used when connecting handlers to the EventBus or executing events.
         * By default, all access is assumed to be single threaded and no locking occurs.
         * For multithreaded access, specify a mutex of the following type.
         * - For simple multithreaded cases, use VStd::mutex.
         * - For multithreaded cases where an event handler sends a new event on the same bus
         *   or connects/disconnects while handling an event on the same bus, use VStd::recursive_mutex.
         */
        using MutexType = typename ImplTraits::MutexType;

        /**
         * Contains all of the addresses on the %EventBus.
         */
        using BusesContainer = typename ImplTraits::BusesContainer;

        /**
         * Locking primitive that is used when executing events in the event queue.
         */
        using EventQueueMutexType = typename ImplTraits::EventQueueMutexType;

        /**
         * Pointer to an address on the bus.
         */
        using BusPtr = typename ImplTraits::BusPtr;

        /**
         * Pointer to a handler node.
         */
        using HandlerNode = typename ImplTraits::HandlerNode;

        /**
         * Policy for the function queue.
         */
        using QueuePolicy = EventBusQueuePolicy<Traits::EnableEventQueue, ThisType, EventQueueMutexType>;

        /**
         * Enables custom logic to run when a handler connects to
         * or disconnects from the %EventBus.
         * For example, you can make a handler execute an event
         * immediately upon connecting to the %EventBus.
         * For available settings, see V::EventBusConnectionPolicy.
         * By default, no extra logic is run.
         */
        using ConnectionPolicy = typename Traits::template ConnectionPolicy<ThisType>;

        /**
         * Used to manually create a callstack entry for a call (often used by ConnectionPolicy)
         */
        using CallstackEntry = V::Internal::CallstackEntry<Interface, Traits>;

        /**
         * Specifies whether the %EventBus supports an event queue.
         * You can use the event queue to execute events at a later time.
         * To execute the queued events, you must call
         * `<BusName>::ExecuteQueuedEvents()`.
         * By default, the event queue is disabled.
         */
        static const bool EnableEventQueue = ImplTraits::EnableEventQueue;

        /**
         * Class that implements %EventBus routing functionality.
         */
        using Router = V::Internal::EventBusRouter<ThisType>;

        /**
         * Class that implements an %EventBus version router.
         */
        using NestedVersionRouter = V::Internal::EventBusNestedVersionRouter<ThisType>;

        /**
         * Controls the flow of %EventBus events.
         * Enables an event to be forwarded, and possibly stopped, before reaching
         * the normal event handlers.
         * Use cases for routing include tracing, debugging, and versioning an %EventBus.
         * The default `EventBusRouterPolicy` forwards the event to each connected
         * `EventBusRouterNode` before sending the event to the normal handlers. Each
         * node can stop the event or let it continue.
         */
        using RouterPolicy = typename Traits::template RouterPolicy<ThisType>;

        /**
         * State that indicates whether to continue routing the event, skip all
         * handlers but notify other routers, or stop processing the event.
         */
        using RouterProcessingState = typename RouterPolicy::EventProcessingState;

        /**
         * True if the %EventBus supports more than one address. Otherwise, false.
         */
        static const bool HasId = Traits::AddressPolicy != EventBusAddressPolicy::Single;

        /**
        * Template Lock Guard class that wraps around the Mutex
        * The EventBus uses for Dispatching Events.
        * This is not EventBus Context Mutex when LocklessDispatch is set
        */
        template <typename DispatchMutex>
        using DispatchLockGuardTemplate = typename ImplTraits::template DispatchLockGuard<DispatchMutex>;

        /**
         * Template Lock Guard class that wraps around the Mutex the EventBus uses for Bus Connects / Disconnects.
         */
        template<typename ContextMutexType>
        using ConnectLockGuardTemplate = typename ImplTraits::template ConnectLockGuard<ContextMutexType>;

        /**
         * Template Lock Guard class that wraps around the Mutex the EventBus uses for Bus Bind calls.
         */
        template<typename ContextMutexType>
        using BindLockGuardTemplate = typename ImplTraits::template BindLockGuard<ContextMutexType>;

        /**
         * Template Lock Guard class that wraps around the Mutex the EventBus uses for Bus callstack tracking.
         */
        template<typename ContextMutexType>
        using CallstackTrackerLockGuardTemplate = typename ImplTraits::template CallstackTrackerLockGuard<ContextMutexType>;

        //////////////////////////////////////////////////////////////////////////
        // Check to help identify common mistakes
        /// @cond EXCLUDE_DOCS
        static_assert((HasId || VStd::is_same<BusIdType, NullBusId>::value),
            "When you use EventBusAddressPolicy::Single there is no need to define BusIdType!");
        static_assert((!HasId || !VStd::is_same<BusIdType, NullBusId>::value),
            "You must provide a valid BusIdType when using EventBusAddressPolicy::ById or EventBusAddressPolicy::ByIdAndOrdered! (ex. using BusIdType = int;");
        static_assert((BusTraits::AddressPolicy == EventBusAddressPolicy::ByIdAndOrdered || VStd::is_same<BusIdOrderCompare, NullBusIdCompare>::value),
            "When you use EventBusAddressPolicy::Single or EventBusAddressPolicy::ById there is no need to define BusIdOrderCompare!");
        static_assert((BusTraits::AddressPolicy != EventBusAddressPolicy::ByIdAndOrdered || !VStd::is_same<BusIdOrderCompare, NullBusIdCompare>::value),
            "When you use EventBusAddressPolicy::ByIdAndOrdered you must define BusIdOrderCompare (ex. using BusIdOrderCompare = VStd::less<BusIdType>)");
        /// @endcond
        /// //////////////////////////////////////////////////////////////////////////

        /// @cond EXCLUDE_DOCS

        /**
         * Connects a handler to an EventBus address.
         * A handler will not receive EventBus events until it is connected to the bus.
         * @param[out] ptr A pointer that will be bound to the EventBus address that
         * the handler will be connected to.
         * @param handler The handler to connect to the EventBus address.
         * @param id The ID of the EventBus address that the handler will be connected to.
        */
        static void Connect(HandlerNode& handler, const BusIdType& id = 0);

         /**
         * Disconnects a handler from an EventBus address.
         * @param handler The handler to disconnect from the EventBus address.
         */
        static void Disconnect(HandlerNode& handler);

        /**
         * Disconnects a handler from an EventBus address without locking the mutex
         * Only call this if the context mutex is held already
         * @param handler The handler to disconnect from the EventBus address.
         */
        static void DisconnectInternal(Context& context, HandlerNode& handler);
        /// @endcond

        /**
         * Returns the total number of handlers that are connected to the EventBus.
         * @return The total number of handlers that are connected to the EventBus.
         */
        static size_t GetTotalNumOfEventHandlers();

        /**
         * Returns whether any handlers are connected to the EventBus.
         * @return True if there are any handlers connected to the
         * EventBus. Otherwise, false.
         */
        static bool HasHandlers();

        /**
         * Returns whether handlers are connected to this specific address.
         * @return True if there are any handlers connected at the address.
         * Otherwise, false.
         */
        static bool HasHandlers(const BusIdType& id);

        /**
         * Returns whether handlers are connected to the specific cached address.
         * @return True if there are any handlers connected at the cached address.
         * Otherwise, false.
         */
        static bool HasHandlers(const BusPtr& ptr);

        /**
         * Gets the ID of the address that is currently receiving an event.
         * You can use this function while handling an event to determine which ID
         * the event concerns.
         * This is especially useful for handlers that connect to multiple address IDs.
         * @return Pointer to the address ID that is currently receiving an event.
         * Returns a null pointer if the EventBus is not currently sending an event
         * or the EventBus does not use an V::EventBusAddressPolicy that has multiple addresses.
         */
        static const BusIdType* GetCurrentBusId();

        /**
         * Checks to see if an EventBus with a given Bus ID appears twice in the callstack.
         * This can be used to detect infinite recursive loops and other reentrancy problems.
         * This method only checks EventBus and ID, not the specific EventBus event, so two different
         * nested event calls on the same EventBus and ID will still return true.
         * @param busId The bus ID to check for reentrancy on this thread.
         * @return true if the EventBus has been called more than once on this thread's callstack, false if not.
         */
        static bool HasReentrantEventBusUseThisThread(const BusIdType* busId = GetCurrentBusId());

        /// @cond EXCLUDE_DOCS
        /**
         * Sets the current event processing state. This function has an
         * effect only when it is called within a router event.
         * @param state A new processing state.
         */
        static void SetRouterProcessingState(RouterProcessingState state);

        /**
         * Determines whether the current event is being routed as a queued event.
         * This function has an effect only when it is called within a router event.
         * @return True if the current event is being routed as a queued event.
         * Otherwise, false.
         */
        static bool IsRoutingQueuedEvent();

        /**
         * Determines whether the current event is being routed in reverse order.
         * This function has an effect only when it is called within a router event.
         * @return True if the current event is being routed in reverse order.
         * Otherwise, false.
         */
        static bool IsRoutingReverseEvent();
        /// @endcond

        /**
         * Returns a unique signature for the EventBus.
         * @return A unique signature for the EventBus.
         */
        static const char* GetName();

        /// @cond EXCLUDE_DOCS
        class Context : public V::Internal::ContextBase
        {
            friend ThisType;
            friend Router;
        public:
            /**
             * The mutex type to use during broadcast/event dispatch.
             * When LocklessDispatch is set on the EventBus and a NullMutex is supplied a shared_mutex is used to protect the context otherwise the supplied MutexType is used
             * The reason why a recursive_mutex is used in this situation, is that specifying LocklessDispatch is implies that the EventBus will be used across multiple threads
             * @see EventBusTraits::LocklessDispatch
             */
            using ContextMutexType = VStd::conditional_t<BusTraits::LocklessDispatch && VStd::is_same_v<MutexType, V::NullMutex>, VStd::shared_mutex, MutexType>;

            /**
             * The scoped lock guard to use
             * during broadcast/event dispatch.
             * @see EventBusTraits::LocklessDispatch
             */
            using DispatchLockGuard = DispatchLockGuardTemplate<ContextMutexType>;

            /**
            * The scoped lock guard to use during connection / disconnection.  Some specialized policies execute handler methods which
            * can cause unnecessary delays holding the context mutex or in some cases perform blocking waits and
            * must unlock the context mutex before doing so to prevent deadlock when the wait is for
            * an event in another thread which is trying to connect to the same bus before it can complete
            */
            using ConnectLockGuard = ConnectLockGuardTemplate<ContextMutexType>;

            /**
             * The scoped lock guard to use for bind calls.
             */
            using BindLockGuard = BindLockGuardTemplate<ContextMutexType>;

            /**
             * The scoped lock guard to use for callstack tracking.
             */
            using CallstackTrackerLockGuard = CallstackTrackerLockGuardTemplate<ContextMutexType>;

            BusesContainer          m_buses;         ///< The actual bus container, which is a static map for each bus type.
            ContextMutexType        m_contextMutex;  ///< Mutex to control access when modifying the context
            QueuePolicy             m_queue;
            RouterPolicy            m_routing;

            Context();
            Context(EventBusEnvironment* environment);
            ~Context() override;

            // Disallow all copying/moving
            Context(const Context&) = delete;
            Context(Context&&) = delete;
            Context& operator=(const Context&) = delete;
            Context& operator=(Context&&) = delete;

        private:
            using CallstackEntryBase = V::Internal::CallstackEntryBase<Interface, Traits>;
            using CallstackEntryRoot = V::Internal::CallstackEntryRoot<Interface, Traits>;
            using CallstackEntryStorageType = V::Internal::EventBusCallstackStorage<CallstackEntryBase, !VStd::is_same_v<ContextMutexType, V::NullMutex>>;

            mutable VStd::unordered_map<VStd::native_thread_id_type, CallstackEntryRoot, VStd::hash<VStd::native_thread_id_type>, VStd::equal_to<VStd::native_thread_id_type>, V::Internal::EventBusEnvironmentAllocator> m_callstackRoots;
            CallstackEntryStorageType _callstack;    ///< Linked list of other bus calls to this bus on the stack, per thread if MutexType is defined
            VStd::atomic_uint m_dispatches;   ///< Number of active dispatches in progress

            friend CallstackEntry;
        };
        /// @endcond

        /**
         * Specifies where %EventBus data is stored.
         * This drives how many instances of this %EventBus exist at runtime.
         * Available storage policies include the following:
         * - (Default) EventBusEnvironmentStoragePolicy - %EventBus data is stored
         * in the V::Environment. With this policy, a single %EventBus instance
         * is shared across all modules (DLLs) that attach to the V::Environment.
         * - EventBusGlobalStoragePolicy - %EventBus data is stored in a global static variable.
         * With this policy, each module (DLL) has its own instance of the %EventBus.
         * - EventBusThreadLocalStoragePolicy - %EventBus data is stored in a thread_local static
         * variable. With this policy, each thread has its own instance of the %EventBus.
         */
        using StoragePolicy = typename Traits::template StoragePolicy<Context>;

        /**
         * Returns the global bus data (if it was created).
         * Depending on the storage policy, there might be one or multiple instances
         * of the bus data.
         * @return A reference to the bus context.
         */
        static Context* GetContext(bool trackCallstack=true);

        using ConnectLockGuard = typename Context::ConnectLockGuard;
        /**
         * Connects a handler to an EventBus address without locking the mutex
         * Only call this if the context mutex is held already
         * A handler will not receive EventBus events until it is connected to the bus.
         * the handler will be connected to.
         * @param handler The handler to connect to the EventBus address.
         * @param id The ID of the EventBus address that the handler will be connected to.
        */
        static void ConnectInternal(Context& context, HandlerNode& handler, ConnectLockGuard& contextLock, const BusIdType& id = 0);

        /**
         * Returns the global bus data. Creates it if it wasn't already created.
         * Depending on the storage policy, there might be one or multiple instances
         * of the bus data.
         * @return A reference to the bus context.
         */
        static Context& GetOrCreateContext(bool trackCallstack=true);

        static bool IsInDispatch(Context* context = GetContext(false));

        /**
         * Returns whether the EventBus context is in the middle of a dispatch on the current thread
        */
        static bool IsInDispatchThisThread(Context* context = GetContext(false));
        /// @cond EXCLUDE_DOCS
        struct RouterCallstackEntry
            : public CallstackEntry
        {
            typedef typename RouterPolicy::Container::iterator Iterator;

            RouterCallstackEntry(Iterator it, const BusIdType* busId, bool isQueued, bool isReverse);

            ~RouterCallstackEntry() override = default;

            void SetRouterProcessingState(RouterProcessingState state) override;

            bool IsRoutingQueuedEvent() const override;

            bool IsRoutingReverseEvent() const override;

            Iterator m_iterator;
            RouterProcessingState m_processingState;
            bool m_isQueued;
            bool m_isReverse;
        };
        /// @endcond
    };

    /// Helper macro to deprecate the helper typedef EventBus<_Interface> _BusName
    /// Where _Interface is a deprecated  EventBus API class
#   define DEPRECATE_EVENTBUS(_Interface, _BusName, _message) DEPRECATE_EBUS_WITH_TRAITS(_Interface, _Interface, _BusName, _message)
    /// Helper macro to deprecate the helper typedef EventBus<_Interface, _BusTraits> _BusName
    /// Where _Interface is a deprecated EventBus API class and/or _BusTraits is a deprecated EventBusTraits class
#   define DEPRECATE_EBUS_WITH_TRAITS(_Interface, _BusTraits, _BusName, _message)       \
    V_PUSH_DISABLE_WARNING(4996, "-Wdeprecated-declarations")                          \
    typedef V::EventBus<_Interface, _BusTraits> DeprecatedBus_##_Interface##_BusTraits;    \
    V_POP_DISABLE_WARNING                                                              \
    V_DEPRECATED(typedef DeprecatedBus_##_Interface##_BusTraits _BusName, _message);

    // The macros below correspond to functions in BusImpl.h.
    // The macros enable you to write shorter code, but don't work as well for code completion.

    /// Dispatches an event to handlers at a cached address.
#   define EBUS_EVENT_PTR(_BusPtr, _EVENTBUS, /*EventName,*/ ...)  _EVENTBUS::Event(_BusPtr, &_EVENTBUS::Events::__VA_ARGS__)

    /// Dispatches an event to handlers at a cached address and receives results.
#   define EBUS_EVENT_PTR_RESULT(_Result, _BusPtr, _EVENTBUS, /*EventName,*/ ...) _EVENTBUS::EventResult(_Result, _BusPtr, &_EVENTBUS::Events::__VA_ARGS__)

    /// Dispatches an event to handlers at a specific address.
#   define EBUS_EVENT_ID(_BusId, _EVENTBUS, /*EventName,*/ ...)    _EVENTBUS::Event(_BusId, &_EVENTBUS::Events::__VA_ARGS__)

    /// Dispatches an event to handlers at a specific address and receives results.
#   define EBUS_EVENT_ID_RESULT(_Result, _BusId, _EVENTBUS, /*EventName,*/ ...) _EVENTBUS::EventResult(_Result, _BusId, &_EVENTBUS::Events::__VA_ARGS__)

    /// Dispatches an event to all handlers.
#   define EBUS_EVENT(_EVENTBUS, /*EventName,*/ ...) _EVENTBUS::Broadcast(&_EVENTBUS::Events::__VA_ARGS__)

    /// Dispatches an event to all handlers and receives results.
#   define EBUS_EVENT_RESULT(_Result, _EVENTBUS, /*EventName,*/ ...) _EVENTBUS::BroadcastResult(_Result, &_EVENTBUS::Events::__VA_ARGS__)

    /// Dispatches an event to handlers at a cached address in reverse order.
#   define EBUS_EVENT_PTR_REVERSE(_BusPtr, _EVENTBUS, /*EventName,*/ ...)  _EVENTBUS::EventReverse(_BusPtr, &_EVENTBUS::Events::__VA_ARGS__)

    /// Dispatches an event to handlers at a cached address in reverse order and receives results.
#   define EBUS_EVENT_PTR_RESULT_REVERSE(_Result, _BusPtr, _EVENTBUS, /*EventName,*/ ...) _EVENTBUS::EventResultReverse(_Result, _BusPtr, &_EVENTBUS::Events::__VA_ARGS__)

    /// Dispatches an event to handlers at a specific address in reverse order.
#   define EBUS_EVENT_ID_REVERSE(_BusId, _EVENTBUS, /*EventName,*/ ...) _EVENTBUS::EventReverse(_BusId, &_EVENTBUS::Events::__VA_ARGS__)

    /// Dispatches an event to handlers at a specific address in reverse order and receives results.
#   define EBUS_EVENT_ID_RESULT_REVERSE(_Result, _BusId, _EVENTBUS, /*EventName,*/ ...) _EVENTBUS::EventReverse(_Result, _BusId, &_EVENTBUS::Events::__VA_ARGS__)

    /// Dispatches an event to all handlers in reverse order.
#   define EBUS_EVENT_REVERSE(_EVENTBUS, /*EventName,*/ ...) _EVENTBUS::BroadcastReverse(&_EVENTBUS::Events::__VA_ARGS__)

    /// Dispatches an event to all handlers in reverse order and receives results.
#   define EBUS_EVENT_RESULT_REVERSE(_Result, _EVENTBUS, /*EventName,*/ ...) _EVENTBUS::BroadcastResultReverse(_Result, &_EVENTBUS::Events::__VA_ARGS__)

    /// Enqueues an asynchronous event to dispatch to all handlers.
#   define EBUS_QUEUE_EVENT(_EVENTBUS, /*EventName,*/ ...)                _EVENTBUS::QueueBroadcast(&_EVENTBUS::Events::__VA_ARGS__)

    /// Enqueues an asynchronous event to dispatch to handlers at a cached address.
#   define EBUS_QUEUE_EVENT_PTR(_BusPtr, _EVENTBUS, /*EventName,*/ ...)    _EVENTBUS::QueueEvent(_BusPtr, &_EVENTBUS::Events::__VA_ARGS__)

    /// Enqueues an asynchronous event to dispatch to handlers at a specific address.
#   define EBUS_QUEUE_EVENT_ID(_BusId, _EVENTBUS, /*EventName,*/ ...)      _EVENTBUS::QueueEvent(_BusId, &_EVENTBUS::Events::__VA_ARGS__)

    /// Enqueues an asynchronous event to dispatch to all handlers in reverse order.
#   define EBUS_QUEUE_EVENT_REVERSE(_EVENTBUS, /*EventName,*/ ...)                _EVENTBUS::QueueBroadcastReverse(&_EVENTBUS::Events::__VA_ARGS__)

    /// Enqueues an asynchronous event to dispatch to handlers at a cached address in reverse order.
#   define EBUS_QUEUE_EVENT_PTR_REVERSE(_BusPtr, _EVENTBUS, /*EventName,*/ ...)    _EVENTBUS::QueueEventReverse(_BusPtr, &_EVENTBUS::Events::__VA_ARGS__)

    /// Enqueues an asynchronous event to dispatch to handlers at a specific address in reverse order.
#   define EBUS_QUEUE_EVENT_ID_REVERSE(_BusId, _EVENTBUS, /*EventName,*/ ...)      _EVENTBUS::QueueEventReverse(_BusId, &_EVENTBUS::Events::__VA_ARGS__)

    /// Enqueues an arbitrary callable function to be executed asynchronously.
#   define EBUS_QUEUE_FUNCTION(_EVENTBUS, /*Function pointer, params*/ ...)       _EVENTBUS::QueueFunction(__VA_ARGS__)

    //////////////////////////////////////////////////////////////////////////
    // Debug events active only when V_DEBUG_BUILD is defined
#if defined(V_DEBUG_BUILD)

    /// Dispatches an event to handlers at a cached address.
#   define EBUS_DBG_EVENT_PTR(_BusPtr, _EVENTBUS, /*EventName,*/ ...)   EBUS_EVENT_PTR(_BusPtr, _EVENTBUS, __VA_ARGS__)

    /// Dispatches an event to handlers at a cached address and receives results.
#   define EBUS_DBG_EVENT_PTR_RESULT(_Result, _BusPtr, _EVENTBUS, /*EventName,*/ ...) EBUS_EVENT_PTR_RESULT(_Result, _BusPtr, _EVENTBUS, __VA_ARGS__)

    /// Dispatches an event to handlers at a specific address.
#   define EBUS_DBG_EVENT_ID(_BusId, _EVENTBUS, /*EventName,*/ ...) EBUS_EVENT_ID(_BusId, _EVENTBUS, __VA_ARGS__)

    /// Dispatches an event to handlers at a specific address and receives results.
#   define EBUS_DBG_EVENT_ID_RESULT(_Result, _BusId, _EVENTBUS, /*EventName,*/ ...) EBUS_EVENT_ID_RESULT(_Result, _BusId, _EVENTBUS, __VA_ARGS__)

    /// Dispatches an event to all handlers.
#   define EBUS_DBG_EVENT(_EVENTBUS, /*EventName,*/ ...) EBUS_EVENT(_EVENTBUS, __VA_ARGS__)

    /// Dispatches an event to all handlers and receives results.
#   define EBUS_DBG_EVENT_RESULT(_Result, _EVENTBUS, /*EventName,*/ ...)  EBUS_EVENT_RESULT(_Result, _EVENTBUS, __VA_ARGS__)

    /// Dispatches an event to handlers at a cached address in reverse order.
#   define EBUS_DBG_EVENT_PTR_REVERSE(_BusPtr, _EVENTBUS, /*EventName,*/ ...) EBUS_EVENT_PTR_REVERSE(_BusPtr, _EVENTBUS, __VA_ARGS__)

    /// Dispatches an event to handlers at a cached address in reverse order and receives results.
#   define EBUS_DBG_EVENT_PTR_RESULT_REVERSE(_Result, _BusPtr, _EVENTBUS, /*EventName,*/ ...) EBUS_EVENT_PTR_RESULT_REVERSE(_Result, _BusPtr, _EVENTBUS, __VA_ARGS__)

    /// Dispatches an event to handlers at a specific address in reverse order.
#   define EBUS_DBG_EVENT_ID_REVERSE(_BusId, _EVENTBUS, /*EventName,*/ ...) EBUS_EVENT_ID_REVERSE(_BusId, _EVENTBUS, __VA_ARGS__)

    /// Dispatches an event to handlers at a specific address in reverse order and receives results.
#   define EBUS_DBG_EVENT_ID_RESULT_REVERSE(_Result, _BusId, _EVENTBUS, /*EventName,*/ ...) EBUS_EVENT_ID_RESULT_REVERSE(_Result, _BusId, _EVENTBUS, __VA_ARGS__)

    /// Dispatches an event to all handlers in reverse order.
#   define EBUS_DBG_EVENT_REVERSE(_EVENTBUS, /*EventName,*/ ...) EBUS_EVENT_REVERSE(_EVENTBUS, __VA_ARGS__)

    /// Dispatches an event to all handlers in reverse order and receives results.
#   define EBUS_DBG_EVENT_RESULT_REVERSE(_Result, _EVENTBUS, /*EventName,*/ ...) EBUS_EVENT_RESULT_REVERSE(_Result, _EVENTBUS, __VA_ARGS__)

    /// Enqueues an asynchronous event to dispatch to all handlers.
#   define EBUS_DBG_QUEUE_EVENT(_EVENTBUS, /*EventName,*/ ...)                EBUS_QUEUE_EVENT(_EVENTBUS, __VA_ARGS__)

    /// Enqueues an asynchronous event to dispatch to handlers at a cached address.
#   define EBUS_DBG_QUEUE_EVENT_PTR(_BusPtr, _EVENTBUS, /*EventName,*/ ...)    EBUS_QUEUE_EVENT_PTR(BusPtr, _EVENTBUS, __VA_ARGS__)

    /// Enqueues an asynchronous event to dispatch to handlers at a specific address.
#   define EBUS_DBG_QUEUE_EVENT_ID(_BusId, _EVENTBUS, /*EventName,*/ ...)      EBUS_QUEUE_EVENT_ID(_BusId, _EVENTBUS, __VA_ARGS__)

    /// Enqueues an asynchronous event to dispatch to all handlers in reverse order.
#   define EBUS_DBG_QUEUE_EVENT_REVERSE(_EVENTBUS, /*EventName,*/ ...)                EBUS_QUEUE_EVENT_REVERSE(_EVENTBUS, __VA_ARGS__)

    /// Enqueues an asynchronous event to dispatch to handlers at a cached address in reverse order.
#   define EBUS_DBG_QUEUE_EVENT_PTR_REVERSE(_BusPtr, _EVENTBUS, /*EventName,*/ ...)    EBUS_QUEUE_EVENT_PTR_REVERSE(_BusPtr, _EVENTBUS, __VA_ARGS__)

    /// Enqueues an asynchronous event to dispatch to handlers at a specific address in reverse order.
#   define EBUS_DBG_QUEUE_EVENT_ID_REVERSE(_BusId, _EVENTBUS, /*EventName,*/ ...)      EBUS_QUEUE_EVENT_ID_REVERSE(_BusId, _EVENTBUS, __VA_ARGS__)

    /// Enqueues an arbitrary callable function to be executed asynchronously.
#   define EBUS_DBG_QUEUE_FUNCTION(_EVENTBUS, /*Function pointer, params*/ ...)   EBUS_QUEUE_FUNCTION(_EVENTBUS, __VA_ARGS__)

#else // !V_DEBUG_BUILD

    /// Dispatches an event to handlers at a cached address.
#   define EBUS_DBG_EVENT_PTR(_BusPtr, _EVENTBUS, /*EventName,*/ ...)

    /// Dispatches an event to handlers at a cached address and receives results.
#   define EBUS_DBG_EVENT_PTR_RESULT(_Result, _BusPtr, _EVENTBUS, /*EventName,*/ ...)

    /// Dispatches an event to handlers at a specific address.
#   define EBUS_DBG_EVENT_ID(_BusId, _EVENTBUS, /*EventName,*/ ...)

    /// Dispatches an event to handlers at a specific address and receives results.
#   define EBUS_DBG_EVENT_ID_RESULT(_Result, _BusId, _EVENTBUS, /*EventName,*/ ...)

    /// Dispatches an event to all handlers.
#   define EBUS_DBG_EVENT(_EVENTBUS, /*EventName,*/ ...)

    /// Dispatches an event to all handlers and receives results.
#   define EBUS_DBG_EVENT_RESULT(_Result, _EVENTBUS, /*EventName,*/ ...)

    /// Dispatches an event to handlers at a cached address in reverse order.
#   define EBUS_DBG_EVENT_PTR_REVERSE(_BusPtr, _EVENTBUS, /*EventName,*/ ...)

    /// Dispatches an event to handlers at a cached address in reverse order and receives results.
#   define EBUS_DBG_EVENT_PTR_RESULT_REVERSE(_Result, _BusPtr, _EVENTBUS, /*EventName,*/ ...)

    /// Dispatches an event to handlers at a specific address in reverse order.
#   define EBUS_DBG_EVENT_ID_REVERSE(_BusId, _EVENTBUS, /*EventName,*/ ...)

    /// Dispatches an event to handlers at a specific address in reverse order and receives results.
#   define EBUS_DBG_EVENT_ID_RESULT_REVERSE(_Result, _BusId, _EVENTBUS, /*EventName,*/ ...)

    /// Dispatches an event to all handlers in reverse order.
#   define EBUS_DBG_EVENT_REVERSE(_EVENTBUS, /*EventName,*/ ...)

    /// Dispatches an event to all handlers in reverse order and receives results.
#   define EBUS_DBG_EVENT_RESULT_REVERSE(_Result, _EVENTBUS, /*EventName,*/ ...)

    /// Enqueues an asynchronous event to dispatch to all handlers.
#   define EBUS_DBG_QUEUE_EVENT(_EVENTBUS, /*EventName,*/ ...)

    /// Enqueues an asynchronous event to dispatch to handlers at a cached address.
#   define EBUS_DBG_QUEUE_EVENT_PTR(_BusPtr, _EVENTBUS, /*EventName,*/ ...)

    /// Enqueues an asynchronous event to dispatch to handlers at a specific address.
#   define EBUS_DBG_QUEUE_EVENT_ID(_BusId, _EVENTBUS, /*EventName,*/ ...)

    /// Enqueues an asynchronous event to dispatch to all handlers in reverse order.
#   define EBUS_DBG_QUEUE_EVENT_REVERSE(_EVENTBUS, /*EventName,*/ ...)

    /// Enqueues an asynchronous event to dispatch to handlers at a cached address in reverse order.
#   define EBUS_DBG_QUEUE_EVENT_PTR_REVERSE(_BusPtr, _EVENTBUS, /*EventName,*/ ...)

    /// Enqueues an asynchronous event to dispatch to handlers at a specific address in reverse order.
#   define EBUS_DBG_QUEUE_EVENT_ID_REVERSE(_BusId, _EVENTBUS, /*EventName,*/ ...)

    /// Enqueues an arbitrary callable function to be executed asynchronously.
#   define EBUS_DBG_QUEUE_FUNCTION(_EVENTBUS, /*Function name*/ ...)

#endif // !V_DEBUG BUILD

    // Debug events
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////
    // EventBus implementations

    namespace Internal
    {
        template <class C>
        V_THREAD_LOCAL C* EventBusCallstackStorage<C, true>::_entry = nullptr;
    }

    //=========================================================================
    // Context::Context
    //=========================================================================
    template<class Interface, class Traits>
    EventBus<Interface, Traits>::Context::Context()
        : m_dispatches(0)
    {
        _callstack = nullptr;
    }

    //=========================================================================
    // Context::Context
    //=========================================================================
    template<class Interface, class Traits>
    EventBus<Interface, Traits>::Context::Context(EventBusEnvironment* environment)
        : V::Internal::ContextBase(environment)
        , m_dispatches(0)
    {
        _callstack = nullptr;
    }

    template <class Interface, class Traits>
    EventBus<Interface, Traits>::Context::~Context()
    {
        // Clear the callstack in this thread. It is expected that most buses will be lifetime managed
        // by the thread that creates them (almost certainly the main thread). This allows a bus
        // to be re-entrant within the same main thread (useful for unit tests and code reloading).
        _callstack = nullptr;
    }

V_PUSH_DISABLE_WARNING(4127, "-Wunknown-warning-option")

    //=========================================================================
    // Connect
    //=========================================================================
    template<class Interface, class Traits>
    inline void EventBus<Interface, Traits>::Connect(HandlerNode& handler, const BusIdType& id)
    {
        Context& context = GetOrCreateContext();
        // scoped lock guard in case of exception / other odd situation
        // Context mutex is separate from the Dispatch lock guard and therefore this is safe to lock this mutex while in the middle of a dispatch
        ConnectLockGuard lock(context.m_contextMutex);
        ConnectInternal(context, handler, lock, id);
    }

    //=========================================================================
    // ConnectInternal
    //=========================================================================
    template<class Interface, class Traits>
    inline void EventBus<Interface, Traits>::ConnectInternal(Context& context, HandlerNode& handler, ConnectLockGuard& contextLock, const BusIdType& id)
    {
        // To call this while executing a message, you need to make sure this mutex is VStd::recursive_mutex. Otherwise, a deadlock will occur.
        V_Assert(!Traits::LocklessDispatch || !IsInDispatch(&context), "It is not safe to connect during dispatch on a lockless dispatch EventBus");

        // Do the actual connection
        context.m_buses.Connect(handler, id);

        BusPtr ptr;
        if constexpr (EventBus::HasId)
        {
            ptr = handler.m_holder;
        }
        CallstackEntry entry(&context, &id);
        ConnectionPolicy::Connect(ptr, context, handler, contextLock, id);
    }

    //=========================================================================
    // Disconnect
    //=========================================================================
    template<class Interface, class Traits>
    inline void EventBus<Interface, Traits>::Disconnect(HandlerNode& handler)
    {
        // To call Disconnect() from a message while being thread safe, you need to make sure the context.m_contextMutex is VStd::recursive_mutex. Otherwise, a deadlock will occur.
        if (Context* context = GetContext())
        {
            // scoped lock guard in case of exception / other odd situation
            ConnectLockGuard lock(context->m_contextMutex);
            DisconnectInternal(*context, handler);
        }
    }

    //=========================================================================
    // DisconnectInternal
    //=========================================================================
    template<class Interface, class Traits>
    inline void EventBus<Interface, Traits>::DisconnectInternal(Context& context, HandlerNode& handler)
    {
        // To call this while executing a message, you need to make sure this mutex is VStd::recursive_mutex. Otherwise, a deadlock will occur.
        V_Assert(!Traits::LocklessDispatch || !IsInDispatch(&context), "It is not safe to disconnect during dispatch on a lockless dispatch EventBus");

        auto callstack = context._callstack->m_prev;
        if (callstack)
        {
            callstack->OnRemoveHandler(handler);
        }

        BusPtr ptr;
        if constexpr (EventBus::HasId)
        {
            ptr = handler.m_holder;
        }
        ConnectionPolicy::Disconnect(context, handler, ptr);

        CallstackEntry entry(&context, nullptr);

        // Do the actual disconnection
        context.m_buses.Disconnect(handler);

        if (callstack)
        {
            callstack->OnPostRemoveHandler();
        }

        handler = nullptr;
    }

V_POP_DISABLE_WARNING

    //=========================================================================
    // GetTotalNumOfEventHandlers
    //=========================================================================
    template<class Interface, class Traits>
    size_t  EventBus<Interface, Traits>::GetTotalNumOfEventHandlers()
    {
        size_t size = 0;
        BaseImpl::EnumerateHandlers([&size](Interface*)
        {
            ++size;
            return true;
        });
        return size;
    }

    //=========================================================================
    // HasHandlers
    //=========================================================================
    template<class Interface, class Traits>
    inline bool EventBus<Interface, Traits>::HasHandlers()
    {
        bool hasHandlers = false;
        auto findFirstHandler = [&hasHandlers](InterfaceType*)
        {
            hasHandlers = true;
            return false;
        };
        BaseImpl::EnumerateHandlers(findFirstHandler);
        return hasHandlers;
    }

    //=========================================================================
    // HasHandlers
    //=========================================================================
    template<class Interface, class Traits>
    inline bool EventBus<Interface, Traits>::HasHandlers(const BusIdType& id)
    {
        return BaseImpl::FindFirstHandler(id) != nullptr;
    }

    //=========================================================================
    // HasHandlers
    //=========================================================================
    template<class Interface, class Traits>
    inline bool EventBus<Interface, Traits>::HasHandlers(const BusPtr& ptr)
    {
        return BaseImpl::FindFirstHandler(ptr) != nullptr;
    }

    //=========================================================================
    // GetCurrentBusId
    //=========================================================================
    template<class Interface, class Traits>
    const typename EventBus<Interface, Traits>::BusIdType * EventBus<Interface, Traits>::GetCurrentBusId()
    {
        Context* context = GetContext();
        if (IsInDispatchThisThread(context))
        {
            return context->_callstack->m_prev->m_busId;
        }
        return nullptr;
    }

    //=========================================================================
    // HasReentrantEventBusUseThisThread
    //=========================================================================
    template<class Interface, class Traits>
    bool EventBus<Interface, Traits>::HasReentrantEventBusUseThisThread(const BusIdType* busId)
    {
        Context* context = GetContext();

        if (busId && IsInDispatchThisThread(context))
        {
            bool busIdInCallstack = false;

            // If we're in a dispatch, callstack->m_prev contains the entry for the current bus call. Start the search for the given
            // bus ID and look upwards. If we find the given ID more than once in the callstack, we've got a reentrant call.
            for (auto callstackEntry = context->_callstack->m_prev; callstackEntry != nullptr; callstackEntry = callstackEntry->m_prev)
            {
                if ((*busId) == (*callstackEntry->m_busId))
                {
                    if (busIdInCallstack)
                    {
                        return true;
                    }

                    busIdInCallstack = true;
                }
            }
        }

        return false;
    }

    //=========================================================================
    // SetRouterProcessingState
    //=========================================================================
    template<class Interface, class Traits>
    void EventBus<Interface, Traits>::SetRouterProcessingState(RouterProcessingState state)
    {
        Context* context = GetContext();
        if (IsInDispatch(context))
        {
            context->_callstack->m_prev->SetRouterProcessingState(state);
        }
    }

    //=========================================================================
    // IsRoutingQueuedEvent
    //=========================================================================
    template<class Interface, class Traits>
    bool EventBus<Interface, Traits>::IsRoutingQueuedEvent()
    {
        Context* context = GetContext();
        if (IsInDispatch(context))
        {
            return context->_callstack->m_prev->IsRoutingQueuedEvent();
        }

        return false;
    }

    //=========================================================================
    // IsRoutingReverseEvent
    //=========================================================================
    template<class Interface, class Traits>
    bool EventBus<Interface, Traits>::IsRoutingReverseEvent()
    {
        Context* context = GetContext();
        if (IsInDispatch(context))
        {
            return context->_callstack->m_prev->IsRoutingReverseEvent();
        }

        return false;
    }

    //=========================================================================
    // GetName
    //=========================================================================
    template<class Interface, class Traits>
    const char* EventBus<Interface, Traits>::GetName()
    {
        return V_FUNCTION_SIGNATURE;
    }

    //=========================================================================
    // GetContext
    //=========================================================================
    template<class Interface, class Traits>
    typename EventBus<Interface, Traits>::Context* EventBus<Interface, Traits>::GetContext(bool trackCallstack)
    {
        Context* context = StoragePolicy::Get();
        if (trackCallstack && context && !context->_callstack)
        {
            // Cache the callstack root into this thread/dll. Even though _callstack is thread-local, we need a mutex lock
            // for the modifications to m_callstackRoots, which is NOT thread-local.
            typename Context::CallstackTrackerLockGuard lock(context->m_contextMutex);
            context->_callstack = &context->m_callstackRoots[VStd::this_thread::get_id().m_id];
        }
        return context;
    }

    //=========================================================================
    // GetContext
    //=========================================================================
    template<class Interface, class Traits>
    typename EventBus<Interface, Traits>::Context& EventBus<Interface, Traits>::GetOrCreateContext(bool trackCallstack)
    {
        Context& context = StoragePolicy::GetOrCreate();
        if (trackCallstack && !context._callstack)
        {
            // Cache the callstack root into this thread/dll. Even though _callstack is thread-local, we need a mutex lock
            // for the modifications to m_callstackRoots, which is NOT thread-local.
            typename Context::CallstackTrackerLockGuard lock(context.m_contextMutex);
            context._callstack = &context.m_callstackRoots[VStd::this_thread::get_id().m_id];
        }
        return context;
    }

    //=========================================================================
    // IsInDispatch
    //=========================================================================
    template<class Interface, class Traits>
    bool EventBus<Interface, Traits>::IsInDispatch(Context* context)
    {
        return context != nullptr && context->m_dispatches > 0;
    }

    template<class Interface, class Traits>
    bool EventBus<Interface, Traits>::IsInDispatchThisThread(Context* context)
    {
        return context != nullptr && context->_callstack != nullptr
            && context->_callstack->m_prev != nullptr;
    }

    //=========================================================================
    template<class Interface, class Traits>
    EventBus<Interface, Traits>::RouterCallstackEntry::RouterCallstackEntry(Iterator it, const BusIdType* busId, bool isQueued, bool isReverse)
        : CallstackEntry(EventBus::GetContext(), busId)
        , m_iterator(it)
        , m_processingState(RouterPolicy::EventProcessingState::ContinueProcess)
        , m_isQueued(isQueued)
        , m_isReverse(isReverse)
    {
    }

    //=========================================================================
    template<class Interface, class Traits>
    void EventBus<Interface, Traits>::RouterCallstackEntry::SetRouterProcessingState(RouterProcessingState state)
    {
        m_processingState = state;
    }

    //=========================================================================
    template<class Interface, class Traits>
    bool EventBus<Interface, Traits>::RouterCallstackEntry::IsRoutingQueuedEvent() const
    {
        return m_isQueued;
    }

    //=========================================================================
    template<class Interface, class Traits>
    bool EventBus<Interface, Traits>::RouterCallstackEntry::IsRoutingReverseEvent() const
    {
        return m_isReverse;
    }

    namespace Internal
    {
        //////////////////////////////////////////////////////////////////////////
        // NonIdHandler
        template <typename Interface, typename Traits, typename ContainerType>
        void NonIdHandler<Interface, Traits, ContainerType>::BusConnect()
        {
            typename BusType::Context& context = BusType::GetOrCreateContext();
            typename BusType::Context::ConnectLockGuard contextLock(context.m_contextMutex);
            if (!BusIsConnected())
            {
                typename Traits::BusIdType id;
                m_node = this;
                BusType::ConnectInternal(context, m_node, contextLock, id);
            }
        }
        template <typename Interface, typename Traits, typename ContainerType>
        void NonIdHandler<Interface, Traits, ContainerType>::BusDisconnect()
        {
            if (typename BusType::Context* context = BusType::GetContext())
            {
                typename BusType::Context::ConnectLockGuard contextLock(context->m_contextMutex);
                if (BusIsConnected())
                {
                    BusType::DisconnectInternal(*context, m_node);
                }
            }
        }

        //////////////////////////////////////////////////////////////////////////
        // IdHandler
        template <typename Interface, typename Traits, typename ContainerType>
        void IdHandler<Interface, Traits, ContainerType>::BusConnect(const IdType& id)
        {
            typename BusType::Context& context = BusType::GetOrCreateContext();
            typename BusType::Context::ConnectLockGuard contextLock(context.m_contextMutex);
            if (BusIsConnected())
            {
                // Connecting on the BusId that is already connected is a no-op
                if (m_node.GetBusId() == id)
                {
                    return;
                }
                V_Assert(false, "Connecting to a different id on this bus without disconnecting first! Please ensure you call BusDisconnect before calling BusConnect again, or if multiple connections are desired you must use a MultiHandler instead.");
                BusType::DisconnectInternal(context, m_node);
            }

            m_node = this;
            BusType::ConnectInternal(context, m_node, contextLock, id);
        }
        template <typename Interface, typename Traits, typename ContainerType>
        void IdHandler<Interface, Traits, ContainerType>::BusDisconnect(const IdType& id)
        {
            if (typename BusType::Context* context = BusType::GetContext())
            {
                typename BusType::Context::ConnectLockGuard contextLock(context->m_contextMutex);
                if (BusIsConnectedId(id))
                {
                    BusType::DisconnectInternal(*context, m_node);
                }
            }
        }
        template <typename Interface, typename Traits, typename ContainerType>
        void IdHandler<Interface, Traits, ContainerType>::BusDisconnect()
        {
            if (typename BusType::Context* context = BusType::GetContext())
            {
                typename BusType::Context::ConnectLockGuard contextLock(context->m_contextMutex);
                if (BusIsConnected())
                {
                    BusType::DisconnectInternal(*context, m_node);
                }
            }
        }

        //////////////////////////////////////////////////////////////////////////
        // MultiHandler
        template <typename Interface, typename Traits, typename ContainerType>
        void MultiHandler<Interface, Traits, ContainerType>::BusConnect(const IdType& id)
        {
            typename BusType::Context& context = BusType::GetOrCreateContext();
            typename BusType::Context::ConnectLockGuard contextLock(context.m_contextMutex);
            if (m_handlerNodes.find(id) == m_handlerNodes.end())
            {
                void* handlerNodeAddr = m_handlerNodes.get_allocator().allocate(sizeof(HandlerNode), VStd::alignment_of<HandlerNode>::value);
                auto handlerNode = new(handlerNodeAddr) HandlerNode(this);
                m_handlerNodes.emplace(id, VStd::move(handlerNode));
                BusType::ConnectInternal(context, *handlerNode, contextLock, id);
            }
        }
        template <typename Interface, typename Traits, typename ContainerType>
        void MultiHandler<Interface, Traits, ContainerType>::BusDisconnect(const IdType& id)
        {
            if (typename BusType::Context* context = BusType::GetContext())
            {
                typename BusType::Context::ConnectLockGuard contextLock(context->m_contextMutex);
                auto nodeIt = m_handlerNodes.find(id);
                if (nodeIt != m_handlerNodes.end())
                {
                    HandlerNode* handlerNode = nodeIt->second;
                    BusType::DisconnectInternal(*context, *handlerNode);
                    m_handlerNodes.erase(nodeIt);
                    handlerNode->~HandlerNode();
                    m_handlerNodes.get_allocator().deallocate(handlerNode, sizeof(HandlerNode), alignof(HandlerNode));
                }
            }
        }
        template <typename Interface, typename Traits, typename ContainerType>
        void MultiHandler<Interface, Traits, ContainerType>::BusDisconnect()
        {
            decltype(m_handlerNodes) handlerNodesToDisconnect;
            if (typename BusType::Context* context = BusType::GetContext())
            {
                typename BusType::Context::ConnectLockGuard contextLock(context->m_contextMutex);
                handlerNodesToDisconnect = VStd::move(m_handlerNodes);

                for (const auto& nodePair : handlerNodesToDisconnect)
                {
                    BusType::DisconnectInternal(*context, *nodePair.second);

                    nodePair.second->~HandlerNode();
                    handlerNodesToDisconnect.get_allocator().deallocate(nodePair.second, sizeof(HandlerNode), VStd::alignment_of<HandlerNode>::value);
                }
            }
        }

        template <class EventBus, class TargetEventBus, class BusIdType>
        struct EventBusRouterQueueEventForwarder
        {
            static_assert((VStd::is_same<BusIdType, typename EventBus::BusIdType>::value), "Routers may only route between buses with the same id/traits");
            static_assert((VStd::is_same<BusIdType, typename TargetEventBus::BusIdType>::value), "Routers may only route between buses with the same id/traits");

            template<class Event, class... Args>
            static void ForwardEvent(Event event, Args&&... args);

            template <class Event, class... Args>
            static void ForwardEventResult(Event event, Args&&... args);
        };

        //////////////////////////////////////////////////////////////////////////
        template <class EventBus, class TargetEventBus, class BusIdType>
        template<class Event, class... Args>
        void EventBusRouterQueueEventForwarder<EventBus, TargetEventBus, BusIdType>::ForwardEvent(Event event, Args&&... args)
        {
            const BusIdType* busId = EventBus::GetCurrentBusId();
            if (busId == nullptr)
            {
                // Broadcast
                if (EventBus::IsRoutingQueuedEvent())
                {
                    // Queue broadcast
                    if (EventBus::IsRoutingReverseEvent())
                    {
                        // Queue broadcast reverse
                        TargetEventBus::QueueBroadcastReverse(event, args...);
                    }
                    else
                    {
                        // Queue broadcast forward
                        TargetEventBus::QueueBroadcast(event, args...);
                    }
                }
                else
                {
                    // In place broadcast
                    if (EventBus::IsRoutingReverseEvent())
                    {
                        // In place broadcast reverse
                        TargetEventBus::BroadcastReverse(event, args...);
                    }
                    else
                    {
                        // In place broadcast forward
                        TargetEventBus::Broadcast(event, args...);
                    }
                }
            }
            else
            {
                // Event with an ID
                if (EventBus::IsRoutingQueuedEvent())
                {
                    // Queue event
                    if (EventBus::IsRoutingReverseEvent())
                    {
                        // Queue event reverse
                        TargetEventBus::QueueEventReverse(*busId, event, args...);
                    }
                    else
                    {
                        // Queue event forward
                        TargetEventBus::QueueEvent(*busId, event, args...);
                    }
                }
                else
                {
                    // In place event
                    if (EventBus::IsRoutingReverseEvent())
                    {
                        // In place event reverse
                        TargetEventBus::EventReverse(*busId, event, args...);
                    }
                    else
                    {
                        // In place event forward
                        TargetEventBus::Event(*busId, event, args...);
                    }
                }
            }
        }

        //////////////////////////////////////////////////////////////////////////
        template <class EventBus, class TargetEventBus, class BusIdType>
        template <class Event, class... Args>
        void EventBusRouterQueueEventForwarder<EventBus, TargetEventBus, BusIdType>::ForwardEventResult(Event event, Args&&... args)
        {

        }

        template <class EventBus, class TargetEventBus>
        struct EventBusRouterQueueEventForwarder<EventBus, TargetEventBus, NullBusId>
        {
            static_assert((VStd::is_same<NullBusId, typename EventBus::BusIdType>::value), "Routers may only route between buses with the same id/traits");
            static_assert((VStd::is_same<NullBusId, typename TargetEventBus::BusIdType>::value), "Routers may only route between buses with the same id/traits");

            template<class Event, class... Args>
            static void ForwardEvent(Event event, Args&&... args);

            template <class Event, class... Args>
            static void ForwardEventResult(Event event, Args&&... args);
        };

        //////////////////////////////////////////////////////////////////////////
        template <class EventBus, class TargetEventBus>
        template<class Event, class... Args>
        void EventBusRouterQueueEventForwarder<EventBus, TargetEventBus, NullBusId>::ForwardEvent(Event event, Args&&... args)
        {
            // Broadcast
            if (EventBus::IsRoutingQueuedEvent())
            {
                // Queue broadcast
                if (EventBus::IsRoutingReverseEvent())
                {
                    // Queue broadcast reverse
                    TargetEventBus::QueueBroadcastReverse(event, args...);
                }
                else
                {
                    // Queue broadcast forward
                    TargetEventBus::QueueBroadcast(event, args...);
                }
            }
            else
            {
                // In place broadcast
                if (EventBus::IsRoutingReverseEvent())
                {
                    // In place broadcast reverse
                    TargetEventBus::BroadcastReverse(event, args...);
                }
                else
                {
                    // In place broadcast forward
                    TargetEventBus::Broadcast(event, args...);
                }
            }
        }

        //////////////////////////////////////////////////////////////////////////
        template <class EventBus, class TargetEventBus>
        template <class Event, class... Args>
        void EventBusRouterQueueEventForwarder<EventBus, TargetEventBus, NullBusId>::ForwardEventResult(Event event, Args&&... args)
        {
        }

        template <class EventBus, class TargetEventBus, class BusIdType>
        struct EventBusRouterEventForwarder
        {
            template<class Event, class... Args>
            static void ForwardEvent(Event event, Args&&... args);

            template<class Result, class Event, class... Args>
            static void ForwardEventResult(Result& result, Event event, Args&&... args);
        };

        //////////////////////////////////////////////////////////////////////////
        template <class EventBus, class TargetEventBus, class BusIdType>
        template<class Event, class... Args>
        void EventBusRouterEventForwarder<EventBus, TargetEventBus, BusIdType>::ForwardEvent(Event event, Args&&... args)
        {
            const BusIdType* busId = EventBus::GetCurrentBusId();
            if (busId == nullptr)
            {
                // Broadcast
                if (EventBus::IsRoutingReverseEvent())
                {
                    // Broadcast reverse
                    TargetEventBus::BroadcastReverse(event, args...);
                }
                else
                {
                    // Broadcast forward
                    TargetEventBus::Broadcast(event, args...);
                }
            }
            else
            {
                // Event.
                if (EventBus::IsRoutingReverseEvent())
                {
                    // Event reverse
                    TargetEventBus::EventReverse(*busId, event, args...);
                }
                else
                {
                    // Event forward
                    TargetEventBus::Event(*busId, event, args...);
                }
            }
        }

        //////////////////////////////////////////////////////////////////////////
        template <class EventBus, class TargetEventBus, class BusIdType>
        template<class Result, class Event, class... Args>
        void EventBusRouterEventForwarder<EventBus, TargetEventBus, BusIdType>::ForwardEventResult(Result& result, Event event, Args&&... args)
        {
            const BusIdType* busId = EventBus::GetCurrentBusId();
            if (busId == nullptr)
            {
                // Broadcast
                if (EventBus::IsRoutingReverseEvent())
                {
                    // Broadcast reverse
                    TargetEventBus::BroadcastResultReverse(result, event, args...);
                }
                else
                {
                    // Broadcast forward
                    TargetEventBus::BroadcastResult(result, event, args...);
                }
            }
            else
            {
                // Event
                if (EventBus::IsRoutingReverseEvent())
                {
                    // Event reverse
                    TargetEventBus::EventResultReverse(result, *busId, event, args...);
                }
                else
                {
                    // Event forward
                    TargetEventBus::EventResult(result, *busId, event, args...);
                }
            }
        }

        template <class EventBus, class TargetEventBus>
        struct EventBusRouterEventForwarder<EventBus, TargetEventBus, NullBusId>
        {
            template<class Event, class... Args>
            static void ForwardEvent(Event event, Args&&... args);

            template<class Result, class Event, class... Args>
            static void ForwardEventResult(Result& result, Event event, Args&&... args);
        };

        //////////////////////////////////////////////////////////////////////////
        template <class EventBus, class TargetEventBus>
        template<class Event, class... Args>
        void EventBusRouterEventForwarder<EventBus, TargetEventBus, NullBusId>::ForwardEvent(Event event, Args&&... args)
        {
            // Broadcast
            if (EventBus::IsRoutingReverseEvent())
            {
                // Broadcast reverse
                TargetEventBus::BroadcastReverse(event, args...);
            }
            else
            {
                // Broadcast forward
                TargetEventBus::Broadcast(event, args...);
            }
        }

        //////////////////////////////////////////////////////////////////////////
        template <class EventBus, class TargetEventBus>
        template<class Result, class Event, class... Args>
        void EventBusRouterEventForwarder<EventBus, TargetEventBus, NullBusId>::ForwardEventResult(Result& result, Event event, Args&&... args)
        {
            // Broadcast
            if (EventBus::IsRoutingReverseEvent())
            {
                // Broadcast reverse
                TargetEventBus::BroadcastResultReverse(result, event, args...);
            }
            else
            {
                // Broadcast forward
                TargetEventBus::BroadcastResult(result, event, args...);
            }
        }

        template<class EventBus, class TargetEventBus, bool allowQueueing = EventBus::EnableEventQueue>
        struct EventBusRouterForwarderHelper
        {
            template<class Event, class... Args>
            static void ForwardEvent(Event event, Args&&... args)
            {
                EventBusRouterQueueEventForwarder<EventBus, TargetEventBus, typename EventBus::BusIdType>::ForwardEvent(event, args...);
            }

            template<class Result, class Event, class... Args>
            static void ForwardEventResult(Result&, Event, Args&&...)
            {

            }
        };

        template<class EventBus, class TargetEventBus>
        struct EventBusRouterForwarderHelper<EventBus, TargetEventBus, false>
        {
            template<class Event, class... Args>
            static void ForwardEvent(Event event, Args&&... args)
            {
                EventBusRouterEventForwarder<EventBus, TargetEventBus, typename EventBus::BusIdType>::ForwardEvent(event, args...);
            }

            template<class Result, class Event, class... Args>
            static void ForwardEventResult(Result& result, Event event, Args&&... args)
            {
                EventBusRouterEventForwarder<EventBus, TargetEventBus, typename EventBus::BusIdType>::ForwardEventResult(result, event, args...);
            }
        };

        /**
        * EventBus router helper class. Inherit from this class the same way
        * you do with EventBus::Handlers, to implement router functionality.
        *
        */
        template<class EventBus>
        class EventBusRouter
            : public EventBus::InterfaceType
        {
            EventBusRouterNode<typename EventBus::InterfaceType> m_routerNode;
            bool m_isConnected;
        public:
            EventBusRouter();
            virtual ~EventBusRouter();

            void BusRouterConnect(int order = 0);

            void BusRouterDisconnect();

            bool BusRouterIsConnected() const;

            template<class TargetEventBus, class Event, class... Args>
            static void ForwardEvent(Event event, Args&&... args);

            template<class Result, class TargetEventBus, class Event, class... Args>
            static void ForwardEventResult(Result& result, Event event, Args&&... args);
        };

        /**
         * Helper class for when we are writing an EventBus version router that will be part
         * of an EventBusRouter policy (that is, active the entire time the bus is used).
         * It will be created when a bus context is created.
         */
        template<class EventBus>
        class EventBusNestedVersionRouter
                : public EventBus::InterfaceType
        {
            EventBusRouterNode<typename EventBus::InterfaceType> m_routerNode;
        public:
            virtual ~EventBusNestedVersionRouter() = default;
            template<class Container>
            void BusRouterConnect(Container& container, int order = 0);

            template<class Container>
            void BusRouterDisconnect(Container& container);

            template<class TargetEventBus, class Event, class... Args>
            static void ForwardEvent(Event event, Args&&... args);

            template<class Result, class TargetEventBus, class Event, class... Args>
            static void ForwardEventResult(Result& result, Event event, Args&&... args);
        };

        //////////////////////////////////////////////////////////////////////////
        //////////////////////////////////////////////////////////////////////////
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        template<class EventBus>
        EventBusRouter<EventBus>::EventBusRouter()
            : m_isConnected(false)
        {
            m_routerNode.m_handler = this;
        }

        //////////////////////////////////////////////////////////////////////////
        template<class EventBus>
        EventBusRouter<EventBus>::~EventBusRouter()
        {
            BusRouterDisconnect();
        }

        //////////////////////////////////////////////////////////////////////////
        template<class EventBus>
        void EventBusRouter<EventBus>::BusRouterConnect(int order)
        {
            if (!m_isConnected)
            {
                m_routerNode.m_order = order;
                auto& context = EventBus::GetOrCreateContext();
                // We could support connection/disconnection while routing a message, but it would require a call to a fix
                // function because there is already a stack entry. This is typically not a good pattern because routers are
                // executed often. If time is not important to you, you can always queue the connect/disconnect functions
                // on the TickBus or another safe bus.
                V_Assert(context._callstack->m_prev == nullptr, "Current we don't allow router connect while in a message on the bus!");
                {
                    VStd::scoped_lock<decltype(context.m_contextMutex)> lock(context.m_contextMutex);
                    context.m_routing.m_routers.insert(&m_routerNode);
                }
                m_isConnected = true;
            }
        }

        //////////////////////////////////////////////////////////////////////////
        template<class EventBus>
        void EventBusRouter<EventBus>::BusRouterDisconnect()
        {
            if (m_isConnected)
            {
                auto* context = EventBus::GetContext();
                EBUS_ASSERT(context, "Internal error: context deleted while router attached.");
                {
                    VStd::scoped_lock<decltype(context->m_contextMutex)> lock(context->m_contextMutex);
                    // We could support connection/disconnection while routing a message, but it would require a call to a fix
                    // function because there is already a stack entry. This is typically not a good pattern because routers are
                    // executed often. If time is not important to you, you can always queue the connect/disconnect functions
                    // on the TickBus or another safe bus.
                    V_Assert(context->_callstack->m_prev == nullptr, "Current we don't allow router disconnect while in a message on the bus!");
                    context->m_routing.m_routers.erase(&m_routerNode);
                }
                m_isConnected = false;
            }
        }

        //////////////////////////////////////////////////////////////////////////
        template<class EventBus>
        bool EventBusRouter<EventBus>::BusRouterIsConnected() const
        {
            return m_isConnected;
        }

        //////////////////////////////////////////////////////////////////////////
        template<class EventBus>
        template<class TargetEventBus, class Event, class... Args>
        void EventBusRouter<EventBus>::ForwardEvent(Event event, Args&&... args)
        {
            EventBusRouterForwarderHelper<EventBus, TargetEventBus>::ForwardEvent(event, args...);
        }

        //////////////////////////////////////////////////////////////////////////
        template<class EventBus>
        template<class Result, class TargetEventBus, class Event, class... Args>
        void EventBusRouter<EventBus>::ForwardEventResult(Result& result, Event event, Args&&... args)
        {
            EventBusRouterForwarderHelper<EventBus, TargetEventBus>::ForwardEventResult(result, event, args...);
        }

        //////////////////////////////////////////////////////////////////////////
        template<class EventBus>
        template<class Container>
        void EventBusNestedVersionRouter<EventBus>::BusRouterConnect(Container& container, int order)
        {
            m_routerNode.m_handler = this;
            m_routerNode.m_order = order;
            // We don't need to worry about removing this because we
            // will be alive as long as the container is.
            container.insert(&m_routerNode);
        }

        //////////////////////////////////////////////////////////////////////////
        template<class EventBus>
        template<class Container>
        void EventBusNestedVersionRouter<EventBus>::BusRouterDisconnect(Container& container)
        {
            container.erase(&m_routerNode);
        }

        //////////////////////////////////////////////////////////////////////////
        template<class EventBus>
        template<class TargetEventBus, class Event, class... Args>
        void EventBusNestedVersionRouter<EventBus>::ForwardEvent(Event event, Args&&... args)
        {
            EventBusRouterForwarderHelper<EventBus, TargetEventBus>::ForwardEvent(event, args...);
        }

        //////////////////////////////////////////////////////////////////////////
        template<class EventBus>
        template<class Result, class TargetEventBus, class Event, class... Args>
        void EventBusNestedVersionRouter<EventBus>::ForwardEventResult(Result& result, Event event, Args&&... args)
        {
            EventBusRouterForwarderHelper<EventBus, TargetEventBus>::ForwardEventResult(result, event, args...);
        }
    } // namespace Internal
}

// The following allow heavily-used busses to be declared extern, in order to speed up compile time where the same header
// with the same bus is included in many different source files.
// to use it, declare the EventBus extern using DECLARE_EBUS_EXTERN or DECLARE_EBUS_EXTERN_WITH_TRAITS in the header file
// and then use DECLARE_EBUS_INSTANTIATION or DECLARE_EBUS_INSTANTIATION_WITH_TRAITS in a file that everything that includes the header
// will link to (for example, in a static library, dynamic library with export library, or .inl that everyone must include in a compile unit).

// The following must be declared AT GLOBAL SCOPE and the namespace V is assumed due to the rule that extern template declarations must occur
// in their enclosing scope.

//! Externs an EventBus class template with both the interface and bus traits arguments
#define DECLARE_EBUS_EXTERN_WITH_TRAITS(a,b) \
namespace V \
{ \
   extern template class EventBus<a, b>; \
}

//! Externs an EventBus class template using only the interface argument
//! for both the EventBus Interface and BusTraits template parameters
#define DECLARE_EBUS_EXTERN(a) \
namespace V \
{ \
   extern template class EventBus<a, a>; \
}

//! Instantiates an EventBus class template with both the interface and bus traits arguments
#define DECLARE_EBUS_INSTANTIATION_WITH_TRAITS(a,b) \
namespace V \
{ \
   template class EventBus<a, b>; \
}
//! Instantiates an EventBus class template using only the interface argument
//! for both the EventBus Interface and BusTraits template parameters

#define DECLARE_EBUS_INSTANTIATION(a) \
namespace V \
{ \
   template class EventBus<a, a>; \
}


#endif //V_FRAMEWORK_CORE_EVENT_BUS_EVENT_BUS_H