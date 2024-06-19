/*
 * Copyright (c) Contributors to the VelcroFramework.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 * 
 */
#ifndef V_FRAMEWORK_CORE_STD_TYPETRAITS_REMOVE_REFERENCE_H
#define V_FRAMEWORK_CORE_STD_TYPETRAITS_REMOVE_REFERENCE_H

#include <vcore/std/typetraits/config.h>

namespace VStd {
    using std::remove_reference;
    template< class T >
    using remove_reference_t = typename remove_reference<T>::type;
}


#endif // V_FRAMEWORK_CORE_STD_TYPETRAITS_REMOVE_REFERENCE_H