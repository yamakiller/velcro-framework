/*
 * Copyright (c) Contributors to the VelcroFramework.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 * 
 */
#ifndef V_FRAMEWORK_CORE_STD_TYPETRAITS_CONJUNCTION_H
#define V_FRAMEWORK_CORE_STD_TYPETRAITS_CONJUNCTION_H

#include <vcore/std/typetraits/conditional.h>
#include <vcore/std/typetraits/integral_constant.h>

namespace VStd {
    template<class... Bases>
    struct conjunction
        : true_type {
    };

    template<class B1>
    struct conjunction<B1>
        : B1 {
    };

    template<class B1, class... Bases>
    struct conjunction<B1, Bases...>
        : conditional_t<!B1::value, B1, conjunction<Bases...>> {
    };

    template<class... Bases>
    constexpr bool conjunction_v = conjunction<Bases...>::value;
}

#endif // V_FRAMEWORK_CORE_STD_TYPETRAITS_CONJUNCTION_H