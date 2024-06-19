/*
 * Copyright (c) Contributors to the VelcroFramework.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 * 
 */
#ifndef V_FRAMEWORK_CORE_MEMORY_ALLOCATOR_BASE_H
#define V_FRAMEWORK_CORE_MEMORY_ALLOCATOR_BASE_H

#include <vcore/memory/iallocator.h>
#include <vcore/memory/platform_memory_instrumentation.h>

namespace V {
    namespace Debug {
        struct AllocationInfo;
    }

    /**
    * AllocatorBase - all Velcro-compatible allocators should inherit from this implementation of IAllocator.
    */
    class AllocatorBase : public IAllocator {
    protected:
        AllocatorBase(IAllocatorAllocate* allocationSource, const char* name, const char* desc);
        ~AllocatorBase();

    public:

        //---------------------------------------------------------------------
        // IAllocator implementation
        //---------------------------------------------------------------------
        const char* GetName() const override;
        const char* GetDescription() const override;
        IAllocatorAllocate* GetSchema() override;
        Debug::AllocationRecords* GetRecords() final;
        void SetRecords(Debug::AllocationRecords* records) final;
        bool IsReady() const final;
        bool CanBeOverridden() const final;
        void PostCreate() override;
        void PreDestroy() final;
        void SetLazilyCreated(bool lazy) final;
        bool IsLazilyCreated() const final;
        void SetProfilingActive(bool active) final;
        bool IsProfilingActive() const final;
        //---------------------------------------------------------------------

    protected:
        /// 返回调整跟踪后的内存分配大小.
        V_FORCE_INLINE size_t MemorySizeAdjustedUp(size_t byteSize) const {
            if (m_records && byteSize > 0) {
                byteSize += m_memoryGuardSize;
            }

            return byteSize;
        }

        /// 返回内存分配的大小, 去除任何跟踪开销.
        V_FORCE_INLINE size_t MemorySizeAdjustedDown(size_t byteSize) const {
            if (m_records && byteSize > 0) {
                byteSize -= m_memoryGuardSize;
            }

            return byteSize;
        }

        /// 调用以禁止覆盖此分配器.
        /// 只有那些被覆盖会特别成问题的内核级分配器才应该这样做.
        void DisableOverriding();

        /// 调用以禁止此分配器向 AllocatorManager 注册.
        /// 只有在向 AllocatorManager 注册时会出现特别问题的内核级分配器才应执行此操作.
        void DisableRegistration();

        /// 记录分配以供分析.
        void ProfileAllocation(void* ptr, size_t byteSize, size_t alignment, const char* name, const char* fileName, int lineNum, int suppressStackRecord);

        /// 记录释放以供分析.
        void ProfileDeallocation(void* ptr, size_t byteSize, size_t alignment, Debug::AllocationInfo* info);

        /// 记录重新分配以进行分析.
        void ProfileReallocationBegin(void* ptr, size_t newSize);

        /// 记录重新分配的开始以进行分析.
        void ProfileReallocationEnd(void* ptr, void* newPtr, size_t newSize, size_t newAlignment);

        /// 记录调整大小以进行分析.
        void ProfileResize(void* ptr, size_t newSize);

        /// 用户分配器在内存不足时应调用此函数!
        bool OnOutOfMemory(size_t byteSize, size_t alignment, int flags, const char* name, const char* fileName, int lineNum);

    private:

        const char* m_name = nullptr;
        const char* m_desc = nullptr;
        // 指向分配记录的缓存指针. 与 MemoryDriller 一起使用.
        Debug::AllocationRecords* m_records = nullptr;  
        size_t m_memoryGuardSize = 0;
        bool m_isLazilyCreated = false;
        bool m_isProfilingActive = false;
        bool m_isReady = false;
        bool m_canBeOverridden = true;
        bool m_registrationEnabled = true;
#if PLATFORM_MEMORY_INSTRUMENTATION_ENABLED
        uint16_t m_platformMemoryInstrumentationGroupId = 0;
#endif
    };

    namespace Internal  {
        struct AllocatorDummy{};
    }
} // namespace V

#endif // V_FRAMEWORK_CORE_MEMORY_ALLOCATOR_BASE_H