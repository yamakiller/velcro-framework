/*
 * Copyright (c) Contributors to the VelcroFramework.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 * 
 */
#ifndef V_FRAMEWORK_CORE_STD_TYPETRAITS_IS_CONVERTIBLE_H
#define V_FRAMEWORK_CORE_STD_TYPETRAITS_IS_CONVERTIBLE_H

#include <core/std/typetraits/intrinsics.h>

namespace VStd {
    using std::is_convertible;
    template<typename From, typename To>
    constexpr bool is_convertible_v = std::is_convertible_v<From, To>;
}


#endif // V_FRAMEWORK_CORE_STD_TYPETRAITS_IS_CONVERTIBLE_H