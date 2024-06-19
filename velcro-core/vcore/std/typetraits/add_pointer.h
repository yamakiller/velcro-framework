/*
 * Copyright (c) Contributors to the VelcroFramework.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 * 
 */
#ifndef V_FRAMEWORK_CORE_STD_TYPETRAITS_ADD_POINTER_H
#define V_FRAMEWORK_CORE_STD_TYPETRAITS_ADD_POINTER_H


#include <vcore/std/typetraits/config.h>

namespace VStd {
    using std::add_pointer;
    template<class Type>
    using add_pointer_t = std::add_pointer_t<Type>;
}

#endif // V_FRAMEWORK_CORE_STD_TYPETRAITS_ADD_POINTER_H