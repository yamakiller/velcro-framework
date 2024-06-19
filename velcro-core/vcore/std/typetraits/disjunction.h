/*
 * Copyright (c) Contributors to the VelcroFramework Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 * 
 */
#ifndef V_FRAMEWORK_CORE_STD_TYPETRAITS_DISJUNCTION_H
#define V_FRAMEWORK_CORE_STD_TYPETRAITS_DISJUNCTION_H

#include <vcore/std/typetraits/integral_constant.h>
#include <vcore/std/typetraits/conditional.h>

namespace VStd {
    template<class... Bases>
    struct disjunction : false_type {};

    template<class B1>
    struct disjunction<B1> : B1 {};

    template<class B1, class... Bases>
    struct disjunction<B1, Bases...> : conditional_t<B1::value, B1, disjunction<Bases...>> {};

    template<class... Bases>
    constexpr bool disjunction_v = disjunction<Bases...>::value;
}


#endif // V_FRAMEWORK_CORE_STD_TYPETRAITS_DISJUNCTION_H