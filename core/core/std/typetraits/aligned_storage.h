/*
 * Copyright (c) Contributors to the VelcroFramework.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 * 
 */
#ifndef V_FRAMEWORK_CORE_STD_TYPETRAITS_ALIGNED_STORAGE_H
#define V_FRAMEWORK_CORE_STD_TYPETRAITS_ALIGNED_STORAGE_H

#include <core/std/typetraits/config.h>

namespace VStd {
    using std::aligned_storage;
    using std::aligned_storage_t;

    template <typename T, size_t Alignment = alignof(T)>
    using aligned_storage_for_t = std::aligned_storage_t<sizeof(T), Alignment>;
}

#endif // V_FRAMEWORK_CORE_STD_TYPETRAITS_ALIGNED_STORAGE_H