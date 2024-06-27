#ifndef V_FRAMEWORK_CORE_EVENT_BUS_EVENT_BUS_SHARED_DISPATCH_TRAITS_H
#define V_FRAMEWORK_CORE_EVENT_BUS_EVENT_BUS_SHARED_DISPATCH_TRAITS_H

#include <vcore/event_bus/event_bus.h>
#include <vcore/std/parallel/mutex.h>
#include <vcore/std/parallel/shared_mutex.h>

namespace V
{
    /**
     * EventBusSharedDispatchTraits is a custom mutex type and lock guards that can be used with an EventBus to allow for
     * parallel dispatch calls, but still prevents connects / disconnects during a dispatch.
     *
     * Features:
     *   - Event dispatches can execute in parallel when called on separate threads
     *   - Bus connects / disconnects will only execute when no event dispatches are executing
     *   - Event dispatches can call other event dispatches on the same bus recursively
     *
     * Limitations:
     *   - If the bus contains custom connect / disconnect logic, it must not call any event dispatches on the same bus.
     *   - Bus connects / disconnects cannot happen within event dispatches on the same bus.
     *
     * Usage:
     *   To use the traits, inherit from EventBusSharedDispatchTraits<BusType>:
     *      class MyBus : public V::EventBusSharedDispatchTraits<MyBus>
     * 
     *   Alternatively, you can directly define the specific traits via the following:
     *      using MutexType = V::EventBusSharedDispatchMutex;
     *
     *      template <typename MutexType, bool IsLocklessDispatch>
     *      using DispatchLockGuard = V::EventBusSharedDispatchMutexDispatchLockGuard<V::EventBus<MyBus>>;
     *
     *      template<typename MutexType>
     *      using ConnectLockGuard = V::EventBusSharedDispatchMutexConnectLockGuard<V::EventBus<MyBus>>;
     *
     *      template<typename MutexType>
     *      using CallstackTrackerLockGuard = V::EventBusSharedDispatchMutexCallstackLockGuard<V::EventBus<MyBus>>;
     */


    // Simple custom mutex class that contains a shared_mutex for use with connects / disconnects / event dispatches
    // and a separate mutex for callstack tracking thread protection.
    class EventBusSharedDispatchMutex
    {
    public:
        EventBusSharedDispatchMutex() = default;
        ~EventBusSharedDispatchMutex() = default;

        void CallstackMutexLock()
        {
            m_callstackMutex.lock();
        }

        void CallstackMutexUnlock()
        {
            m_callstackMutex.unlock();
        }

        void EventMutexLockExclusive()
        {
            m_eventMutex.lock();
        }

        void EventMutexUnlockExclusive()
        {
            m_eventMutex.unlock();
        }

        void EventMutexLockShared()
        {
            m_eventMutex.lock_shared();
        }

        void EventMutexUnlockShared()
        {
            m_eventMutex.unlock_shared();
        }

    private:
        VStd::shared_mutex m_eventMutex;
        VStd::mutex m_callstackMutex;

        // This custom mutex type should only be used with the lock guards below since it needs additional context to know which
        // mutex to lock and what type of lock to request. If you get a compile error due to these methods being private, the EventBus
        // declaration is likely missing one or more of the lock guards below.
        void lock(){}
        void unlock(){}
    };

    // Custom lock guard to handle Connection lock management.
    // This will lock/unlock the event mutex with an exclusive lock. It will assert and disallow
    // exclusive locks if currently inside of a shared lock.
    template<class EventBusType>
    class EventBusSharedDispatchMutexConnectLockGuard
    {
    public:
        EventBusSharedDispatchMutexConnectLockGuard(EventBusSharedDispatchMutex& mutex, VStd::adopt_lock_t)
            : m_mutex(mutex)
        {
        }

        explicit EventBusSharedDispatchMutexConnectLockGuard(EventBusSharedDispatchMutex& mutex)
            : m_mutex(mutex)
        {
            V_AssertV(!EventBusType::IsInDispatchThisThread(), "Can't connect/disconnect while inside an event dispatch.");
            m_mutex.EventMutexLockExclusive();
        }

        ~EventBusSharedDispatchMutexConnectLockGuard()
        {
            m_mutex.EventMutexUnlockExclusive();
        }

    private:
        EventBusSharedDispatchMutexConnectLockGuard(EventBusSharedDispatchMutexConnectLockGuard const&) = delete;
        EventBusSharedDispatchMutexConnectLockGuard& operator=(EventBusSharedDispatchMutexConnectLockGuard const&) = delete;
        EventBusSharedDispatchMutex& m_mutex;
    };

    // Custom lock guard to handle Dispatch lock management.
    // This will lock/unlock the event mutex with a shared lock. It also allows for recursive shared locks by only holding the
    // shared lock at the top level of the recursion.
    // Here's how this works:
    //  - Each thread that has an EventBus call ends up creating a lock guard.
    //  - The lock guard checks (via EventBusType::IsInDispatchThisThread) whether or not it is the first EventBus call to occur on this thread. 
    //  - If it is, it sets the boolean and share locks the shared_mutex.
    //  - If not, it doesn't, and accepts that something higher up the callstack has a share lock on the shared_mutex.
    //  - If a recursive call to the EventBus occurs on the thread, it does _not_ grab the share lock.
    // This is required because shared_mutex by itself doesn't support recursion. Attempting to call lock_shared() twice in the same
    // thread will result in a deadlock.
    template<class EventBusType>
    class EventBusSharedDispatchMutexDispatchLockGuard
    {
    public:
        EventBusSharedDispatchMutexDispatchLockGuard(EventBusSharedDispatchMutex& mutex, VStd::adopt_lock_t)
            : m_mutex(mutex)
        {
        }

        explicit EventBusSharedDispatchMutexDispatchLockGuard(EventBusSharedDispatchMutex& mutex)
            : m_mutex(mutex)
        {
            if (!EventBusType::IsInDispatchThisThread())
            {
                m_ownSharedLockOnThread = true;
                m_mutex.EventMutexLockShared();
            }
        }

        ~EventBusSharedDispatchMutexDispatchLockGuard()
        {
            if (m_ownSharedLockOnThread)
            {
                m_mutex.EventMutexUnlockShared();
            }
        }

    private:
        EventBusSharedDispatchMutexDispatchLockGuard(EventBusSharedDispatchMutexDispatchLockGuard const&) = delete;
        EventBusSharedDispatchMutexDispatchLockGuard& operator=(EventBusSharedDispatchMutexDispatchLockGuard const&) = delete;
        EventBusSharedDispatchMutex& m_mutex;
        bool m_ownSharedLockOnThread = false;
    };

    // Custom lock guard to handle callstack tracking lock management.
    // This uses a separate always-exclusive lock for the callstack tracking, regardless of whether or not we're in a shared lock
    // for event dispatches.
    template<class EventBusType>
    class EventBusSharedDispatchMutexCallstackLockGuard
    {
    public:
        EventBusSharedDispatchMutexCallstackLockGuard(EventBusSharedDispatchMutex& mutex, VStd::adopt_lock_t)
            : m_mutex(mutex)
        {
        }

        explicit EventBusSharedDispatchMutexCallstackLockGuard(EventBusSharedDispatchMutex& mutex)
            : m_mutex(mutex)
        {
            m_mutex.CallstackMutexLock();
        }

        ~EventBusSharedDispatchMutexCallstackLockGuard()
        {
            m_mutex.CallstackMutexUnlock();
        }

    private:
        EventBusSharedDispatchMutexCallstackLockGuard(EventBusSharedDispatchMutexCallstackLockGuard const&) = delete;
        EventBusSharedDispatchMutexCallstackLockGuard& operator=(EventBusSharedDispatchMutexCallstackLockGuard const&) = delete;
        EventBusSharedDispatchMutex& m_mutex;
    };

    // The EventBusTraits that can be inherited from to automatically set up the MutexType and LockGuards.
    // To inherit, use "class MyBus : public V::EventBusSharedDispatchTraits<MyBus>"
    template<class BusType>
    struct EventBusSharedDispatchTraits : EventBusTraits
    {
        using MutexType = V::EventBusSharedDispatchMutex;

        template<typename MutexType, bool IsLocklessDispatch>
        using DispatchLockGuard = V::EventBusSharedDispatchMutexDispatchLockGuard<V::EventBus<BusType>>;

        template<typename MutexType>
        using ConnectLockGuard = V::EventBusSharedDispatchMutexConnectLockGuard<V::EventBus<BusType>>;

        template<typename MutexType>
        using CallstackTrackerLockGuard = V::EventBusSharedDispatchMutexCallstackLockGuard<V::EventBus<BusType>>;
    };

} // namespace V

#endif // V_FRAMEWORK_CORE_EVENT_BUS_EVENT_BUS_SHARED_DISPATCH_TRAITS_H

