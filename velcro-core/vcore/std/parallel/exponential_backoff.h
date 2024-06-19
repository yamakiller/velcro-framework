/*
 * Copyright (c) Contributors to the VelcroFramework.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 * 
 */
#ifndef V_FRAMEWORK_CORE_STD_PARALLEL_EXPONENTIAL_BACKOFF_H
#define V_FRAMEWORK_CORE_STD_PARALLEL_EXPONENTIAL_BACKOFF_H

#include <vcore/std/parallel/thread.h>

namespace VStd
{
    class exponential_backoff {
    public:
        exponential_backoff()
            : m_count(1) { }

        void wait() {
            if (m_count <= MAX_PAUSE_LOOPS) {
                VStd::this_thread::pause(m_count);
                m_count <<= 1;
            } else {
                VStd::this_thread::yield();
            }
        }

        void reset()
        {
            m_count = 1;
        }

    private:
        static const int MAX_PAUSE_LOOPS = 32;
        int m_count;
    };
}


#endif // V_FRAMEWORK_CORE_STD_PARALLEL_EXPONENTIAL_BACKOFF_H