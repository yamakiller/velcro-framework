/*
 * Copyright (c) Contributors to the VelcroFramework.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 * 
 */
#ifndef V_FRAMEWORK_CORE_STD_TYPETRAITS_NEGATION_H
#define V_FRAMEWORK_CORE_STD_TYPETRAITS_NEGATION_H


#include <vcore/std/typetraits/intrinsics.h>

namespace VStd {
    template<class B>
    struct negation : integral_constant<bool, !B::value> {};

    template<class B>
    constexpr bool negation_v = negation<B>::value;
}


#endif // V_FRAMEWORK_CORE_STD_TYPETRAITS_NEGATION_H