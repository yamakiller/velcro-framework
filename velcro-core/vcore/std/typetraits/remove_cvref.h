/*
 * Copyright (c) Contributors to the VelcroFramework.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 * 
 */
#ifndef V_FRAMEWORK_CORE_STD_TYPETRAITS_REMOVE_CVREF_H
#define V_FRAMEWORK_CORE_STD_TYPETRAITS_REMOVE_CVREF_H


#include <vcore/std/typetraits/remove_cv.h>
#include <vcore/std/typetraits/remove_reference.h>

namespace VStd {
    template <typename T>
    struct remove_cvref
    {
        using type = remove_cv_t<remove_reference_t<T>>;
    };

    template <typename  T>
    using remove_cvref_t = typename remove_cvref<T>::type;
}


#endif // V_FRAMEWORK_CORE_STD_TYPETRAITS_REMOVE_CVREF_H