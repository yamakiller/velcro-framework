/*
 * Copyright (c) Contributors to the VelcroFramework.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 * 
 */
#ifndef V_FRAMEWORK_CORE_STD_TYPETRAITS_IS_DESTRUCTIBLE_H
#define V_FRAMEWORK_CORE_STD_TYPETRAITS_IS_DESTRUCTIBLE_H

#include <type_traits>

namespace VStd {
    using std::is_destructible;
    using std::is_trivially_destructible;
    using std::is_nothrow_destructible;
    template<class T>
    constexpr bool is_default_destructible_v = std::is_destructible<T>::value;
    template<class T>
    constexpr bool is_trivially_destructible_v = std::is_trivially_destructible<T>::value;
    template<class T>
    constexpr bool is_nothrow_destructible_v = std::is_nothrow_destructible<T>::value;
}


#endif // V_FRAMEWORK_CORE_STD_TYPETRAITS_IS_DESTRUCTIBLE_H