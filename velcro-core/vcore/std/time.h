/*
 * Copyright (c) Contributors to the VelcroFramework.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef V_FRAMEWORK_CORE_STD_TIME_H
#define V_FRAMEWORK_CORE_STD_TIME_H

#include <vcore/std/base.h>

namespace VStd {
    VStd::sys_time_t GetTimeTicksPerSecond();
    VStd::sys_time_t GetTimeNowTicks();
    VStd::sys_time_t GetTimeNowMicroSecond();
    VStd::sys_time_t GetTimeNowSecond();
    // return time in millisecond since 1970/01/01 00:00:00 UTC.
    V::u64           GetTimeUTCMilliSecond();
}

#endif // V_FRAMEWORK_CORE_STD_TIME_H