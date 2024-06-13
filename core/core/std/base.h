/*
 * Copyright (c) Contributors to the VelcroFramework.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 * 
 */
#ifndef V_FRAMEWORK_CORE_STD_BASE_H
#define V_FRAMEWORK_CORE_STD_BASE_H

#include <core/base.h>
#include <core/std/config.h>
#include <initializer_list>

#define VSTD_PRINTF printf

#if V_COMPILER_MSVC
#   define V_FORMAT_ATTRIBUTE(...)
#else
#   define V_FORMAT_ATTRIBUTE(STRING_INDEX, FIRST_TO_CHECK) __attribute__((__format__ (__printf__, STRING_INDEX, FIRST_TO_CHECK)))
#endif


namespace VStd {
    using ::size_t;
    using ::ptrdiff_t;

    using std::initializer_list;

    using std::nullptr_t;

    using sys_time_t = V::s64;
}

#endif // V_FRAMEWORK_CORE_STD_BASE_H