#ifndef V_FRAMEWORK_CORE_STD_PARALLEL_SPIN_MUTEX2_H
#define V_FRAMEWORK_CORE_STD_PARALLEL_SPIN_MUTEX2_H

#include <vcore/std/parallel/atomic.h>
#include <vcore/std/parallel/exponential_backoff.h>

namespace VStd {
    class spin_mutex
    {
    public:
        spin_mutex(bool isLocked = false) {
            m_flag.store(isLocked, VStd::memory_order_release);
        }

        void lock() {
            bool expected = false;
            if (!m_flag.compare_exchange_weak(expected, true,  VStd::memory_order_acq_rel,  VStd::memory_order_acquire)) {
                VStd::exponential_backoff backoff;
                for (;; ) {
                    expected = false;
                    if (m_flag.compare_exchange_weak(expected, true,  VStd::memory_order_acq_rel,  VStd::memory_order_acquire)) {
                        break;
                    }
                    backoff.wait();
                }
            }
        }

        bool try_lock() {
            bool expected = false;
            return m_flag.compare_exchange_weak(expected, true, VStd::memory_order_acq_rel, VStd::memory_order_acquire);
        }

        void unlock() {
            m_flag.store(false, VStd::memory_order_release);
        }

    private:
        spin_mutex(const spin_mutex&);
        spin_mutex& operator=(const spin_mutex&);

        VStd::atomic<bool> m_flag;
    };
}
#endif // V_FRAMEWORK_CORE_STD_PARALLEL_SPIN_MUTEX2_H