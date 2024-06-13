/*
 * Copyright (c) Contributors to the VelcroFramework.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 * 
 */
#ifndef V_FRAMEWORK_CORE_STD_TYPETRAITS_VOID_T_H
#define V_FRAMEWORK_CORE_STD_TYPETRAITS_VOID_T_H

namespace VStd {
    // Implementation of the C++ standard is void_t meta function
    // http://en.cppreference.com/w/cpp/types/void_t
    // This utility function maps any type to void
    // It can be used to detect ill-formed which are not mappable to void
    template<typename... Args> struct make_void { using type = void; };
    template<typename... Args> using void_t = typename make_void<Args...>::type;
}

#endif // V_FRAMEWORK_CORE_STD_TYPETRAITS_VOID_T_H