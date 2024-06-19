/*
 * Copyright (c) Contributors to the VelcroFramework.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 * 
 */
#ifndef V_FRAMEWORK_CORE_STD_TYPETRAITS_IS_POINTER_H
#define V_FRAMEWORK_CORE_STD_TYPETRAITS_IS_POINTER_H

#include <vcore/std/typetraits/config.h>

namespace VStd {
    using std::is_pointer;
    template<typename T>
    inline constexpr bool is_pointer_v = std::is_pointer_v<T>;
}


#endif // V_FRAMEWORK_CORE_STD_TYPETRAITS_IS_POINTER_H