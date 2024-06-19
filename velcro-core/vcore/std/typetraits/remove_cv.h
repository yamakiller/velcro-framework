/*
 * Copyright (c) Contributors to the VelcroFramework.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 * 
 */
#ifndef V_FRAMEWORK_CORE_STD_TYPETRAITS_REMOVE_CV_H
#define V_FRAMEWORK_CORE_STD_TYPETRAITS_REMOVE_CV_H


#include <vcore/std/typetraits/config.h>

namespace VStd {
    using std::remove_cv;
    template<class T>
    using remove_cv_t = std::remove_cv_t<T>;
}


#endif // V_FRAMEWORK_CORE_STD_TYPETRAITS_REMOVE_CV_H