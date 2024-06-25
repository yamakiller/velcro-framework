#ifndef V_FRAMEWORK_CORE_STD_PARALLEL_LOCK2_H
#define V_FRAMEWORK_CORE_STD_PARALLEL_LOCK2_H

#include <vcore/std/parallel/config.h>
#include <vcore/std/createdestroy.h>
#include <vcore/std/parallel/thread.h>

namespace VStd {
    template<class Mutex>
    class upgrade_lock;

    struct defer_lock_t { };    // do not acquire ownership of the mutex
    struct try_to_lock_t { };   // try to acquire ownership of the mutex without blocking
    struct adopt_lock_t { };    // assume the calling thread has already

    constexpr defer_lock_t defer_lock{};
    constexpr try_to_lock_t try_to_lock{};
    constexpr adopt_lock_t adopt_lock{};

    template <class Mutex>
    class lock_guard {
    public:
        typedef Mutex mutex_type;
    public:
        V_FORCE_INLINE explicit lock_guard(mutex_type& mtx) : m_mutex(mtx) {
            m_mutex.lock();
        }
        V_FORCE_INLINE lock_guard(mutex_type& mtx, adopt_lock_t) : m_mutex(mtx) {
        }
        V_FORCE_INLINE ~lock_guard() {
            m_mutex.unlock();
        }
    private:
        V_FORCE_INLINE lock_guard(lock_guard const&) {}
        V_FORCE_INLINE lock_guard& operator=(lock_guard const&) {}
    private:
        mutex_type& m_mutex;
    }; 
    
    template <class Mutex>
    class unique_lock {
        template<class M>
        friend class upgrade_lock;
    public:
        typedef Mutex mutex_type;

        V_FORCE_INLINE unique_lock()
            : m_mutex(nullptr)
            , m_owns(false) {}
        V_FORCE_INLINE explicit unique_lock(mutex_type& mtx)
            : m_mutex(&mtx)
            , m_owns(false)
        {
            m_mutex->lock();
            m_owns = true;
        }
        V_FORCE_INLINE unique_lock(mutex_type& mtx, defer_lock_t)
            : m_mutex(&mtx)
            , m_owns(false) {}
        V_FORCE_INLINE unique_lock(mutex_type& mtx, try_to_lock_t)
            : m_mutex(&mtx)
        {
            m_owns = m_mutex->try_lock();
        }
        V_FORCE_INLINE unique_lock(mutex_type& mtx, adopt_lock_t)
            : m_mutex(&mtx)
            , m_owns(true) {}

         V_FORCE_INLINE unique_lock(unique_lock&& u)
            : m_mutex(u.m_mutex)
            , m_owns(u.m_owns)
        {
            u.m_mutex = nullptr;
            u.m_owns = false;
        }

        unique_lock(upgrade_lock<Mutex>&& other);

        V_FORCE_INLINE unique_lock& operator=(unique_lock&& u)
        {
            if (m_owns)
            {
                m_mutex->unlock();
            }

            m_mutex = u.m_mutex;
            m_owns = u.m_owns;
            u.m_mutex = nullptr;
            u.m_owns = false;
        }

        V_FORCE_INLINE ~unique_lock()
        {
            if (m_owns)
            {
                m_mutex->unlock();
            }
        }

        V_FORCE_INLINE void lock()
        {
            V_Assert(m_mutex != nullptr, "Mutex not set!");
            m_mutex->lock();
            m_owns = true;
        }
        V_FORCE_INLINE bool try_lock()
        {
            V_Assert(m_mutex != nullptr, "Mutex not set!");
            m_owns = m_mutex->try_lock();
            return m_owns;
        }

        V_FORCE_INLINE void unlock()
        {
            V_Assert(m_mutex != nullptr, "Mutex not set!");
            V_Assert(m_owns, "We must own the mutex to unlock!");
            m_mutex->unlock();
            m_owns = false;
        }

        // modifiers
        V_FORCE_INLINE void swap(unique_lock& rhs)
        {
            (void)rhs;
            // We need to solve that swap include
            V_Assert(false, "Todo");
        }
        V_FORCE_INLINE mutex_type* release()
        {
            mutex_type* mutex = m_mutex;
            //if( m_owns ) m_mutex->unlock();
            m_mutex = nullptr;
            m_owns = false;
            return mutex;
        }
        // observers
        V_FORCE_INLINE bool owns_lock() const { return m_owns; }
        V_FORCE_INLINE operator bool() const { return m_owns; }
        V_FORCE_INLINE mutex_type* mutex() const { return m_mutex; }
    private:
        unique_lock(unique_lock const&);
        unique_lock& operator=(unique_lock const&);
    private:
        mutex_type* m_mutex;
        bool        m_owns;
    };

    template <class Mutex>
    void swap(unique_lock<Mutex>& x, unique_lock<Mutex>& y)
    {
        x.swap(y);
    }

    template <class Mutex>
    class shared_lock
    {
    public:
        typedef Mutex mutex_type;
        V_FORCE_INLINE explicit shared_lock(mutex_type& mutex)
            : m_mutex(mutex)
        {
            m_mutex.lock_shared();
        }
        V_FORCE_INLINE shared_lock(mutex_type& mutex, adopt_lock_t)
            : m_mutex(mutex) {}
        V_FORCE_INLINE ~shared_lock() { m_mutex.unlock_shared(); }

    private:
        V_FORCE_INLINE shared_lock(shared_lock const&);
        V_FORCE_INLINE shared_lock& operator=(shared_lock const&);
    private:
        mutex_type& m_mutex; // exposition only
    };


    template<class Lockable1, class Lockable2>
    int try_lock(Lockable1& lockable1, Lockable2& lockable2)
    {
        VStd::unique_lock<Lockable1> firstLock(lockable1, VStd::try_to_lock);
        if (firstLock.owns_lock())
        {
            if (lockable2.try_lock())
            {
                firstLock.release();
                return -1;
            }
            else
            {
                return 1;
            }
        }
        return 0;
    }

     template<class Lockable1, class Lockable2, class Lockable3, class... LockableN>
    int try_lock(Lockable1& lockable1, Lockable2& lockable2, Lockable3& lockable3, LockableN&... lockableN)
    {
        int mutexWhichFailedToLockIndex = 0;
        VStd::unique_lock<Lockable1> firstLock(lockable1, try_to_lock);
        if (firstLock.owns_lock())
        {
            mutexWhichFailedToLockIndex = VStd::try_lock(lockable2, lockable3, lockableN...);
            if (mutexWhichFailedToLockIndex == -1)
            {
                firstLock.release();
            }
            else
            {
                // Must increment the mutexWhichFailedToLockIndex by 1 to take into account that lockable1 was not part of the try_lock call
                ++mutexWhichFailedToLockIndex;
            }
        }

        return mutexWhichFailedToLockIndex;
    }

    template<class Lockable>
    void lock(Lockable& lockable)
    {
        lockable.lock();
    }

    template<class Lockable1, class Lockable2>
    void lock(Lockable1& lockable1, Lockable2& lockable2)
    {
        while (true)
        {
            {
                VStd::unique_lock<Lockable1> firstLock(lockable1);
                if (lockable2.try_lock())
                {
                    firstLock.release();
                    break;
                }
            }
            // Yield the thread to allow other threads to unlock the two lockable mutexes
            VStd::this_thread::yield();

            {
                VStd::unique_lock<Lockable2> secondLock(lockable2);
                if (lockable1.try_lock())
                {
                    secondLock.release();
                    break;
                }
            }
            VStd::this_thread::yield();
        }
    }

    template<class Lockable1, class Lockable2, class Lockable3, class... LockableN>
    void lock_helper(int32_t mutexWhichFailedToLockIndex, Lockable1& lockable1, Lockable2& lockable2, Lockable3& lockable3, LockableN&... lockableN)
    {
        while (true)
        {
            switch (mutexWhichFailedToLockIndex)
            {
            case 0:
            {
                VStd::unique_lock<Lockable1> firstLock(lockable1);
                mutexWhichFailedToLockIndex = VStd::try_lock(lockable2, lockable3, lockableN...);
                if (mutexWhichFailedToLockIndex == -1)
                {
                    firstLock.release();
                    return;
                }

                // Must increment the mutexhWhicFailedToLockIndex by 1 to take into account that lockable1 was not part of the try_lock call
                ++mutexWhichFailedToLockIndex;
            }
            // Yield the thread to allow other threads to unlock the two lockable mutexes
            VStd::this_thread::yield();
            break;
            case 1:
            {
                // Attempt to lock the second lockable first
                VStd::unique_lock<Lockable2> secondLock(lockable2);
                // Pass in the first lockable as the last parameter in order to detect if it has not been locked
                constexpr int32_t firstLocklableIndex = 1 + sizeof...(lockableN);
                mutexWhichFailedToLockIndex = VStd::try_lock(lockable3, lockableN..., lockable1);
                if (mutexWhichFailedToLockIndex == -1)
                {
                    secondLock.release();
                    return;
                }
                mutexWhichFailedToLockIndex = (mutexWhichFailedToLockIndex == firstLocklableIndex) ? 0 : mutexWhichFailedToLockIndex + 2;
            }
            VStd::this_thread::yield();
            break;
            default:
                lock_helper(mutexWhichFailedToLockIndex - 2, lockable3, lockableN..., lockable1, lockable2);
                return;
            }
        }
    }

    template<class Lockable1, class Lockable2, class Lockable3, class... LockableN>
    void lock(Lockable1& lockable1, Lockable2& lockable2, Lockable3& lockable3, LockableN&... lockableN)
    {
        int32_t mutexWhichFailedToLockIndex = 0; // Initialized to 0 to start with locking the first mutex
        lock_helper(mutexWhichFailedToLockIndex, lockable1, lockable2, lockable3, lockableN...);
    }

    template<class Lockable>
    void unlock(Lockable& lockable)
    {
        lockable.unlock();
    }

    template<class Lockable1, class Lockable2>
    void unlock(Lockable1& lockable1, Lockable2& lockable2)
    {
        lockable1.unlock();
        lockable2.unlock();
    }

    template<class Lockable1, class Lockable2, class Lockable3, class... LockableN>
    void unlock(Lockable1& lockable1, Lockable2& lockable2, Lockable3& lockable3, LockableN&... lockableN)
    {
        lockable1.unlock();
        lockable2.unlock();
        unlock(lockable3, lockableN...);
    }
} // namespace VStd

#endif // V_FRAMEWORK_CORE_STD_PARALLEL_LOCK2_H