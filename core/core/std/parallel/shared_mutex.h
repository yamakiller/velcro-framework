/*
 * Copyright (c) Contributors to the VelcroFramework.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 * 
 */
#ifndef V_FRAMEWORK_CORE_STD_PARALLEL_SHARED_MUTEX_H
#define V_FRAMEWORK_CORE_STD_PARALLEL_SHARED_MUTEX_H

#include <core/std/parallel/atomic.h>
#include <core/std/parallel/exponential_backoff.h>

namespace VStd {
      /**
    * Light weight Read Write spin lock (based on C++17 standard std::shared_mutex)
    * TODO: Since this mode uses relaxed more if we care about debug performance we can handcraft using the intrinsics
    * here instead of the function overhead that atomics have.
    */
    class shared_mutex {
        VStd::atomic<V::u32>  m_value; // last bit for exclusive lock, the rest shared locks counter.
        static const V::u32 m_exclusiveLockBit = 0x80000000;
        shared_mutex& operator=(const shared_mutex&) = delete;
    public:
        explicit shared_mutex()
            : m_value(0)  {
        }

        void lock_shared() {
            if ((m_value.fetch_add(1, memory_order_acquire) & m_exclusiveLockBit) != 0) {
                exponential_backoff backoff; // this is optional (to be more efficient)
                while ((m_value.load(memory_order_acquire) & m_exclusiveLockBit) != 0)    // if there is a exclusive lock, wait
                {
                    backoff.wait();
                }
            }
        }
        void unlock_shared() {
            m_value.fetch_sub(1, memory_order_release);
        }
        bool try_lock_shared() {
            if ((m_value.fetch_add(1, memory_order_acquire) & m_exclusiveLockBit) == 0) {
                return true;
            }
            m_value.fetch_sub(1, memory_order_relaxed);
            return false;
        }

        void lock() {
            exponential_backoff backoff; // this is optional (to be more efficient)
            while (true) {
                V::u32 expected = 0; // no locks of any kind
                if (m_value.compare_exchange_weak(expected, m_exclusiveLockBit, memory_order_acquire)) {
                    break;
                }
                backoff.wait();
            }
        }
        void unlock() {
            m_value.fetch_sub(m_exclusiveLockBit, memory_order_release);
        }
        bool try_lock() {
            V::u32 expected = 0;
            return m_value.compare_exchange_weak(expected, m_exclusiveLockBit, memory_order_acquire, memory_order_relaxed);
        }
    };
}

#endif // V_FRAMEWORK_CORE_STD_PARALLEL_SHARED_MUTEX_H