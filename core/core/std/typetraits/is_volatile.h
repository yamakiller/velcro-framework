/*
 * Copyright (c) Contributors to the VelcroFramework.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 * 
 */
#ifndef V_FRAMEWORK_CORE_STD_TYPETRAITS_IS_VOLATILE_H
#define V_FRAMEWORK_CORE_STD_TYPETRAITS_IS_VOLATILE_H


#include <core/std/typetraits/config.h>

namespace VStd {
    using std::is_volatile;

    template<class T>
    constexpr bool is_volatile_v = std::is_volatile<T>::value;
}


#endif // V_FRAMEWORK_CORE_STD_TYPETRAITS_IS_VOLATILE_H