/*
 * Copyright (c) Contributors to the VelcroFramework.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 * 
 */

#ifndef V_FRAMEWORK_CORE_DEBUG_MEMORY_PROFILER_H
#define V_FRAMEWORK_CORE_DEBUG_MEMORY_PROFILER_H

#ifndef V_PROFILE_MEMORY_ALLOC
// 没有其他分析器定义了性能标记 V_PROFILE_MEMORY_ALLOC, 回退到 Driller 实现（当前为空）
#   define V_PROFILE_MEMORY_ALLOC(category, address, size, context)
#   define V_PROFILE_MEMORY_ALLOC_EX(category, filename, lineNumber, address, size, context)
#   define V_PROFILE_MEMORY_FREE(category, address)
#   define V_PROFILE_MEMORY_FREE_EX(category, filename, lineNumber, address)
#endif

#endif 