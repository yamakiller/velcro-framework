/*
 * Copyright (c) Contributors to the VelcroFramework.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 * 
 */
#ifndef V_FRAMEWORK_CORE_STD_PARALLEL_SCOPED_LOCK_H
#define V_FRAMEWORK_CORE_STD_PARALLEL_SCOPED_LOCK_H

//#include <vcore/std/parallel/lock.h>
#include <vcore/std/tuple.h>

namespace VStd {
    /**
     * scoped_lock is a mutex wrapper that provides RAII style mechanism for
     * owning multiple mutexes in a scoped block
     */
    template<class... Mutexes>
    class scoped_lock;

    template<>
    class scoped_lock<>
    {
    public:
        scoped_lock() = default;
        explicit scoped_lock(adopt_lock_t) {
        }
        ~scoped_lock() = default;

    private:
        scoped_lock(scoped_lock const&) = delete;
        scoped_lock& operator=(scoped_lock const&) = delete;
    };

    template<class Mutex>
    class scoped_lock<Mutex> {
    public:
        using mutex_type = Mutex;
        explicit scoped_lock(Mutex& mutex)
            : m_mutex(mutex) {
            m_mutex.lock();
        }
        scoped_lock(adopt_lock_t, Mutex& mutex)
            : m_mutex(mutex) {
        }
        ~scoped_lock() {
            m_mutex.unlock();
        }

    private:
        scoped_lock(scoped_lock const&) = delete;
        scoped_lock& operator=(scoped_lock const&) = delete;
        Mutex& m_mutex;
    };

    template<class... Mutexes>
    class scoped_lock {
    public:
        static_assert(sizeof...(Mutexes) >= 2, "variadic scoped_lock specialization requires at least 2 mutexes");

        explicit scoped_lock(Mutexes&... mutexes)
            : m_mutexes(mutexes...) {
            VStd::apply(VStd::lock<Mutexes...>, m_mutexes);
        }
        scoped_lock(adopt_lock_t, Mutexes&... mutexes)
            : m_mutexes(mutexes...) {
        }
        ~scoped_lock() {
            VStd::apply(VStd::unlock<Mutexes...>, m_mutexes);
        }

    private:
        scoped_lock(scoped_lock const&) = delete;
        scoped_lock& operator=(scoped_lock const&) = delete;
        VStd::tuple<Mutexes&...> m_mutexes;
    };
}

#endif // V_FRAMEWORK_CORE_STD_PARALLEL_SCOPED_LOCK_H