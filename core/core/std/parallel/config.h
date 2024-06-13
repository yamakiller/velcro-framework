/*
 * Copyright (c) Contributors to the VelcroFramework.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 * 
 */
#ifndef V_FRAMEWORK_CORE_STD_PARALLEL_CONFIG_H
#define V_FRAMEWORK_CORE_STD_PARALLEL_CONFIG_H

#include <core/std/base.h>
#include <core/platform_default.h>
#include <core/std/parallel/config_platform.h>


namespace VStd {
    struct thread_id {
        thread_id()
            : m_id(native_thread_invalid_id) {}
        thread_id(native_thread_id_type threadId)
            : m_id(threadId) {}
        native_thread_id_type m_id;
    };

    inline bool operator==(VStd::thread_id x, VStd::thread_id y)     { return x.m_id == y.m_id; }
    inline bool operator!=(VStd::thread_id x, VStd::thread_id y)     { return x.m_id != y.m_id; }
    inline bool operator<(VStd::thread_id  x, VStd::thread_id y)     { return x.m_id < y.m_id; }
    inline bool operator<=(VStd::thread_id x, VStd::thread_id y)     { return x.m_id <= y.m_id; }
    inline bool operator>(VStd::thread_id  x, VStd::thread_id y)     { return x.m_id > y.m_id; }
    inline bool operator>=(VStd::thread_id x, VStd::thread_id y)     { return x.m_id >= y.m_id; }
}

#endif // V_FRAMEWORK_CORE_STD_PARALLEL_CONFIG_H