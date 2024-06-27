/*
 * Copyright (c) Contributors to the VelcroFramework.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 * 
 */
#ifndef V_FRAMEWORK_CORE_MEMORY_PLATFORM_MEMORY_INSTRUMENTATION_H
#define V_FRAMEWORK_CORE_MEMORY_PLATFORM_MEMORY_INSTRUMENTATION_H

#include <vcore/vcore_traits_platform.h>

#if V_TRAIT_OS_MEMORY_INSTRUMENTATION && !defined(_RELEASE)
#define PLATFORM_MEMORY_INSTRUMENTATION_ENABLED 1
#else
#define PLATFORM_MEMORY_INSTRUMENTATION_ENABLED 0
#endif

#if PLATFORM_MEMORY_INSTRUMENTATION_ENABLED
#include <vcore/base.h>
#include <vcore/std/parallel/mutex.h>
namespace V {
    /// @brief 平台特定内存检测的抽象层.
    class PlatformMemoryInstrumentation {
        public:
            static uint16_t GetNextGroupId() { return NextGroupId++; };
            static void RegisterGroup(uint16_t id, const char* name, uint16_t parentGroup);
            static void Alloc(const void* ptr, uint64_t size, uint32_t padding, uint16_t group);
            static void Free(const void* ptr);
            static void ReallocBegin(const void* origPtr, uint64_t size, uint16_t group);
            static void ReallocEnd(const void* newPtr, uint64_t size, uint32_t padding);
        public:
            static const uint16_t GroupRoot;
            static uint16_t NextGroupId;
    };
}
#endif // PLATFORM_MEMORY_INSTRUMENTATION_ENABLED

#endif // V_FRAMEWORK_CORE_MEMORY_PLATFORM_MEMORY_INSTRUMENTATION_H