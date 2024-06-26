#ifndef V_FRAMEWORK_CORE_EVENT_BUS_POLICIES_H
#define V_FRAMEWORK_CORE_EVENT_BUS_POLICIES_H

/**
 * @file
 * Header file for EventBus policies regarding addresses, handlers, connections, and storage.
 * These are internal policies. Do not include this file directly.
 */

// Includes for the event queue.
#include <vcore/std/functional.h>
#include <vcore/std/function/invoke.h>
#include <vcore/std/containers/queue.h>
#include <vcore/std/containers/intrusive_set.h>
#include <vcore/std/parallel/scoped_lock.h>


namespace V {
    /**
     * Defines how many addresses exist on the EventBus.
     */
    enum class EventBusAddressPolicy {
        /**
         * (Default) The EventBus has a single address.
         */
        Single,

        /**
         * The EventBus has multiple addresses; the order in which addresses
         * receive events is undefined.
         * Events that are addressed to an ID are received by handlers
         * that are connected to that ID.
         * Events that are broadcast without an ID are received by
         * handlers at all addresses.
         */
        ById,

        /**
         * The EventBus has multiple addresses; the order in which addresses
         * receive events is defined.
         * Events that are addressed to an ID are received by handlers
         * that are connected to that ID.
         * Events that are broadcast without an ID are received by
         * handlers at all addresses.
         * The order in which addresses receive events is defined by
         * V::EventBusTraits::BusIdOrderCompare.
         */
        ByIdAndOrdered,
    };

    /**
     * Defines how many handlers can connect to an address on the EventBus
     * and the order in which handlers at each address receive events.
     */
    enum class EventBusHandlerPolicy
    {
        /**
         * The EventBus supports one handler for each address.
         */
        Single,

        /**
         * (Default) Allows any number of handlers for each address;
         * handlers at an address receive events in the order
         * in which the handlers are connected to the EventBus.
         */
        Multiple,

        /**
         * Allows any number of handlers for each address; the order
         * in which each address receives an event is defined
         * by V::EventBusTraits::BusHandlerOrderCompare.
         */
        MultipleAndOrdered,
    };

    namespace Internal
    {
        struct NullBusMessageCall
        {
            template<class Function>
            NullBusMessageCall(Function) {}
            template<class Function, class Allocator>
            NullBusMessageCall(Function, const Allocator&) {}
        };
    } // namespace Internal

    /**
     * Defines the default connection policy that is used when
     * V::EventBusTraits::ConnectionPolicy is left unspecified.
     * Use this as a template for custom connection policies.
     * @tparam Bus A class that defines an EventBus.
     */
    template <class Bus>
    struct EventBusConnectionPolicy {
        /**
         * A pointer to a specific address on the EventBus.
         */
        typedef typename Bus::BusPtr BusPtr;

        /**
         * The type of ID that is used to address the EventBus.
         * This type is used when the address policy is EventBusAddressPolicy::ById
         * or EventBusAddressPolicy::ByIdAndOrdered only.
         */
        typedef typename Bus::BusIdType BusIdType;

        /**
         * A handler connected to the EventBus.
         */
        typedef typename Bus::HandlerNode HandlerNode;

        /**
         * Global data for the EventBus.
         * There is only one context for each EventBus type.
         */
        typedef typename Bus::Context Context;

        /**
        * Default MutexType used for connecting, disconnecting
        * and dispatch when enabled
        */
        typedef typename Bus::MutexType MutexType;

        /**
        * Lock type used for connecting / disconnecting to the bus.  When NullMutex isn't used as the
        * default mutex this will be a unique lock to allow for unlocking before connection
        * dispatches which some specialized policies perform
        *
        * E.g RequestConnect -> LockMutex -> ConnectInternal ->
        * UnlockMutex -> ExecuteHandlerMethod -> Return
        */
        typedef typename Bus::Context::ConnectLockGuard ConnectLockGuard;

        /**
         * Connects a handler to an EventBus address.
         * @param ptr[out] A pointer that will be bound to the EventBus address that
         * the handler will be connected to.
         * @param context Global data for the EventBus.
         * @param handler The handler to connect to the EventBus address.
         * @param id The ID of the EventBus address that the handler will be connected to.
         */
        static void Connect(BusPtr& ptr, Context& context, HandlerNode& handler, ConnectLockGuard& contextLock, const BusIdType& id = 0);

        /**
         * Disconnects a handler from an EventBus address.
         * @param context Global data for the EventBus.
         * @param handler The handler to disconnect from the EventBus address.
         * @param ptr A pointer to a specific address on the EventBus.
         */
        static void Disconnect(Context& context, HandlerNode& handler, BusPtr& ptr);
    };

    /**
     * A choice of V::EventBusTraits::StoragePolicy that specifies
     * that EventBus data is stored in a global static variable.
     * With this policy, each module (DLL) has its own instance of the EventBus.
     * @tparam Context A class that contains EventBus data.
     */
    template <class Context>
    struct EventBusGlobalStoragePolicy
    {
        /**
         * Returns the EventBus data.
         * @return A pointer to the EventBus data.
         */
        static Context* Get()
        {
            // Because the context in this policy lives in static memory space, and
            // doesn't need to be allocated, there is no reason to defer creation.
            return &GetOrCreate();
        }

        /**
         * Returns the EventBus data.
         * @return A reference to the EventBus data.
         */
        static Context& GetOrCreate()
        {
            static Context s_context;
            return s_context;
        }
    };

    /**
     * A choice of V::EventBusTraits::StoragePolicy that specifies
     * that EventBus data is stored in a thread_local static variable.
     * With this policy, each thread has its own instance of the EventBus.
     * @tparam Context A class that contains EventBus data.
     */
    template <class Context>
    struct EventBusThreadLocalStoragePolicy
    {
        /**
         * Returns the EventBus data.
         * @return A pointer to the EventBus data.
         */
        static Context* Get()
        {
            // Because the context in this policy lives in static memory space, and
            // doesn't need to be allocated, there is no reason to defer creation.
            return &GetOrCreate();
        }

        /**
         * Returns the EventBus data.
         * @return A reference to the EventBus data.
         */
        static Context& GetOrCreate()
        {
            thread_local static Context s_context;
            return s_context;
        }
    };

    template <bool IsEnabled, class Bus, class MutexType>
    struct EventBusQueuePolicy
    {
        typedef V::Internal::NullBusMessageCall BusMessageCall;
        void Execute() {};
        void Clear() {};
        void SetActive(bool /*isActive*/) {};
        bool IsActive() { return false; }
        size_t Count() const { return 0; }
    };

    template <class Bus, class MutexType>
    struct EventBusQueuePolicy<true, Bus, MutexType>
    {
        typedef VStd::function<void()> BusMessageCall;

        typedef VStd::deque<BusMessageCall, typename Bus::AllocatorType> DequeType;
        typedef VStd::queue<BusMessageCall, DequeType > MessageQueueType;

        EventBusQueuePolicy() = default;

        bool                        m_isActive = Bus::Traits::EventQueueingActiveByDefault;
        MessageQueueType            m_messages;
        MutexType                   m_messagesMutex;        ///< Used to control access to the m_messages. Make sure you never interlock with the EventBus mutex. Otherwise, a deadlock can occur.

        void Execute()
        {
            V_Warning("System", m_isActive, "You are calling execute queued functions on a bus which has not activated its function queuing! Call YourBus::AllowFunctionQueuing(true)!");

            MessageQueueType localMessages;

            // Swap the current list of queue functions with a local instance
            {
                VStd::scoped_lock lock(m_messagesMutex);
                VStd::swap(localMessages, m_messages);
            }

            // Execute the queue functions safely now that are owned by the function
            while (!localMessages.empty())
            {
                const BusMessageCall& localMessage = localMessages.front();
                localMessage();
                localMessages.pop();
            }
        }

        void Clear()
        {
            VStd::lock_guard<MutexType> lock(m_messagesMutex);
            m_messages = {};
        }

        void SetActive(bool isActive)
        {
            VStd::lock_guard<MutexType> lock(m_messagesMutex);
            m_isActive = isActive;
            if (!m_isActive)
            {
                m_messages = {};
            }
        };

        bool IsActive()
        {
            return m_isActive;
        }

        size_t Count()
        {
            VStd::lock_guard<MutexType> lock(m_messagesMutex);
            return m_messages.size();
        }
    };

    /// @endcond

    ////////////////////////////////////////////////////////////
    // Implementations
    ////////////////////////////////////////////////////////////
    template <class Bus>
    void EventBusConnectionPolicy<Bus>::Connect(BusPtr&, Context&, HandlerNode&, ConnectLockGuard&, const BusIdType&)
    {
    }

    template <class Bus>
    void EventBusConnectionPolicy<Bus>::Disconnect(Context&, HandlerNode&, BusPtr&)
    {
    }

    /**
     * This is the default bus event compare operator. If not overridden in your bus traits and you
     * use ordered handlers (HandlerPolicy = EventBusHandlerPolicy::MultipleAndOrdered), you will need
     * to declare the following function in your interface: 'bool Compare(const Interface* rhs) const'.
     */
    struct BusHandlerCompareDefault;

    //////////////////////////////////////////////////////////////////////////
    // Router Policy
    /// @cond EXCLUDE_DOCS
    template <class Interface>
    struct EventBusRouterNode
        : public VStd::intrusive_multiset_node<EventBusRouterNode<Interface>>
    {
        Interface*  m_handler = nullptr;
        int m_order = 0;

        EventBusRouterNode& operator=(Interface* handler);

        Interface* operator->() const;

        operator Interface*() const;

        bool operator<(const EventBusRouterNode& rhs) const;
    };

    template <class Bus>
    struct EventBusRouterPolicy
    {
        using RouterNode = EventBusRouterNode<typename Bus::InterfaceType>;
        typedef VStd::intrusive_multiset<RouterNode, VStd::intrusive_multiset_base_hook<RouterNode>> Container;

        // We have to share this with a general processing event if we want to support stopping messages in progress.
        enum class EventProcessingState : int
        {
            ContinueProcess, ///< Continue with the event process as usual (default).
            SkipListeners, ///< Skip all listeners but notify all other routers.
            SkipListenersAndRouters, ///< Skip everybody. Nobody should receive the event after this.
        };

        template <class Function, class... InputArgs>
        inline bool RouteEvent(const typename Bus::BusIdType* busIdPtr, bool isQueued, bool isReverse, Function&& func, InputArgs&&... args);

        Container m_routers;
    };

    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    template <class Interface>
    inline EventBusRouterNode<Interface>& EventBusRouterNode<Interface>::operator=(Interface* handler)
    {
        m_handler = handler;
        return *this;
    }

    //////////////////////////////////////////////////////////////////////////
    template <class Interface>
    inline Interface* EventBusRouterNode<Interface>::operator->() const
    {
        return m_handler;
    }

    //////////////////////////////////////////////////////////////////////////
    template <class Interface>
    inline EventBusRouterNode<Interface>::operator Interface*() const
    {
        return m_handler;
    }

    //////////////////////////////////////////////////////////////////////////
    template <class Interface>
    bool EventBusRouterNode<Interface>::operator<(const EventBusRouterNode& rhs) const
    {
        return m_order < rhs.m_order;
    }

    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    template <class Bus>
    template <class Function, class... InputArgs>
    inline bool EventBusRouterPolicy<Bus>::RouteEvent(const typename Bus::BusIdType* busIdPtr, bool isQueued, bool isReverse, Function&& func, InputArgs&&... args)
    {
        auto rtLast = m_routers.end();
        typename Bus::RouterCallstackEntry rtCurrent(m_routers.begin(), busIdPtr, isQueued, isReverse);
        while (rtCurrent.m_iterator != rtLast)
        {
            VStd::invoke(func, (*rtCurrent.m_iterator++), args...);

            if (rtCurrent.m_processingState == EventProcessingState::SkipListenersAndRouters)
            {
                return true;
            }
        }

        if (rtCurrent.m_processingState != EventProcessingState::ContinueProcess)
        {
            return true;
        }

        return false;
    }

    /// @endcond
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////

    struct EventBusEventProcessingPolicy
    {
        template<class Results, class Function, class Interface, class... InputArgs>
        static void CallResult(Results& results, Function&& func, Interface&& iface, InputArgs&&... args)
        {
            results = VStd::invoke(VStd::forward<Function>(func), VStd::forward<Interface>(iface), VStd::forward<InputArgs>(args)...);
        }

        template<class Function, class Interface, class... InputArgs>
        static void Call(Function&& func, Interface&& iface, InputArgs&&... args)
        {
            VStd::invoke(VStd::forward<Function>(func), VStd::forward<Interface>(iface), VStd::forward<InputArgs>(args)...);
        }
    };


} // namespace V


#endif // V_FRAMEWORK_CORE_EVENT_BUS_POLICIES_H