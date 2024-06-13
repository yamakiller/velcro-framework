/*
 * Copyright (c) Contributors to the VelcroFramework.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 * 
 */
#ifndef V_FRAMEWORK_CORE_STD_TYPETRAITS_REMOVE_CONST_H
#define V_FRAMEWORK_CORE_STD_TYPETRAITS_REMOVE_CONST_H


#include <core/std/typetraits/config.h>

namespace VStd {
    using std::remove_const;
    template<class T>
    using remove_const_t = std::remove_const_t<T>;
}


#endif // V_FRAMEWORK_CORE_STD_TYPETRAITS_REMOVE_CONST_H