/*
 * Copyright (c) Contributors to the VelcroFramework.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 * 
 */
#ifndef V_FRAMEWORK_CORE_STD_PARALLEL_SEMAPHORE_H
#define V_FRAMEWORK_CORE_STD_PARALLEL_SEMAPHORE_H

#include <vcore/std/parallel/config.h>
#include <vcore/std/chrono/types.h>

namespace VStd {
     /**
     * Semaphore synchronization primitive. This is not
     * part of the \ref C++0x standard.
     */
    class semaphore {
    public:
        enum {
            // we prevent semaphores from having a MAX of more than this
            // because some platforms don't check and overflow
            MAXIMUM_COUNT = 0x7fff
        };

        typedef native_semaphore_handle_type native_handle_type;

        semaphore(unsigned int initialCount = 0, unsigned int maximumCount = MAXIMUM_COUNT);
        semaphore(const char* name, unsigned int initialCount = 0, unsigned int maximumCount = MAXIMUM_COUNT);
        ~semaphore();
        void acquire();
        template <class Rep, class Period>
        bool try_acquire_for(const chrono::duration<Rep, Period>& rel_time);
        template <class Clock, class Duration>
        bool try_acquire_until(const chrono::time_point<Clock, Duration>& abs_time);
        void release(unsigned int releaseCount = 1);

        native_handle_type native_handle();
    private:
        semaphore(const semaphore&) {}
        semaphore& operator=(const semaphore&) { return *this; }

        native_semaphore_data_type m_semaphore;

#if !V_TRAIT_SEMAPHORE_HAS_NATIVE_MAX_COUNT
        //Unlike Windows, these platforms do not natively support a semaphore max count. So we use a second mutex to implement
        //the producer-consumer pattern which gives us the same behaviour.
        native_semaphore_data_type m_maxCountSemaphore;
#endif
    };
}


#endif // V_FRAMEWORK_CORE_STD_PARALLEL_SEMAPHORE_H