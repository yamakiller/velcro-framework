/*
 * Copyright (c) Contributors to the VelcroFramework.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 * 
 */
#ifndef V_FRAMEWORK_CORE_MEMORY_CONFIG_H
#define V_FRAMEWORK_CORE_MEMORY_CONFIG_H

#if !defined(_RELEASE)
    #define V_MEMORY_PROFILE(...) (__VA_ARGS__)
#else
    #define V_MEMORY_PROFILE(...)
#endif

#endif // V_FRAMEWORK_CORE_MEMORY_CONFIG_H