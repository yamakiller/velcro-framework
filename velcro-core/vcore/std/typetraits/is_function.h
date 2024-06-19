/*
 * Copyright (c) Contributors to the VelcroFramework.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 * 
 */
#ifndef V_FRAMEWORK_CORE_STD_TYPETRAITS_IS_FUNCTION_H
#define V_FRAMEWORK_CORE_STD_TYPETRAITS_IS_FUNCTION_H

#include <vcore/std/typetraits/conjunction.h>
#include <vcore/std/typetraits/is_pointer.h>
#include <vcore/std/typetraits/remove_pointer.h>

namespace VStd {
    using std::is_function;
    using std::is_function_v;

    template<class T>
    struct is_function_pointer :VStd::conjunction<VStd::is_pointer<T>, VStd::is_function<VStd::remove_pointer_t<T>>> {};

    template<class T>
    constexpr bool is_function_pointer_v = is_function_pointer<T>::value;
}


#endif // V_FRAMEWORK_CORE_STD_TYPETRAITS_IS_FUNCTION_H