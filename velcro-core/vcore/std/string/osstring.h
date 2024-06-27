/*
 * Copyright (c) Contributors to the VelcroFramework.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 * 
 */
#ifndef V_FRAMEWORK_CORE_STD_STRING_OSSTRING_H
#define V_FRAMEWORK_CORE_STD_STRING_OSSTRING_H

#include <vcore/std/string/string.h>
#include <vcore/memory/osallocator.h>

namespace V
{
    // ASCII string type that allocates from the OS allocator rather than the system allocator.
    // This should be used very sparingly, and only in application bootstrap situations where
    // the system allocator is not yet available.
    typedef VStd::basic_string<char, VStd::char_traits<char>, OSStdAllocator> OSString;
} // namespace V


#endif // V_FRAMEWORK_CORE_STD_STRING_OSSTRING_H