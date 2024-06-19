#ifndef V_FRAMEWORK_CORE_MEMORY_SYSTEM_ALLOCATOR_H
#define V_FRAMEWORK_CORE_MEMORY_SYSTEM_ALLOCATOR_H

#include <vcore/memory/memory.h>

namespace V {
     class SystemAllocator
        : public AllocatorBase
        , public IAllocatorAllocate
    {
    public:
    
        SystemAllocator();
        ~SystemAllocator() override;

        /**
         * Description - configure the system allocator. By default
         * we will allocate system memory using system calls. You can
         * provide arenas (spaces) with pre-allocated memory, and use the
         * flag to specify which arena you want to allocate from.
         * You are also allowed to supply IAllocatorAllocate, but if you do
         * so you will need to take care of all allocations, we will not use
         * the default HeapSchema.
         * \ref HeapSchema::Descriptor
         */
        struct Descriptor
        {
            Descriptor()
                : Custom(0)
                , AllocationRecords(true)
                , StackRecordLevels(5)
            {}
            IAllocatorAllocate*         Custom;   ///< You can provide our own allocation scheme. If NULL a HeapScheme will be used with the provided Descriptor.

            struct Heap
            {
                Heap()
                    : PageSize(DefaultPageSize)
                    , PoolPageSize(DefaultPoolPageSize)
                    , IsPoolAllocations(true)
                    , NumFixedMemoryBlocks(0)
                    , SubAllocator(nullptr)
                    , SystemChunkSize(0) {}
                static const int        DefaultPageSize = V_TRAIT_OS_DEFAULT_PAGE_SIZE;
                static const int        DefaultPoolPageSize = 4 * 1024;
                static const int        MemoryBlockAlignment = DefaultPageSize;
                static const int        MaxNumFixedBlocks = 3;
                unsigned int            PageSize;                                 ///< Page allocation size must be 1024 bytes aligned. (default m_defaultPageSize)
                unsigned int            PoolPageSize;                             ///< Page size used to small memory allocations. Must be less or equal to m_pageSize and a multiple of it. (default m_defaultPoolPageSize)
                bool                    IsPoolAllocations;                        ///< True (default) if we use pool for small allocations (< 256 bytes), otherwise false. IMPORTANT: Changing this to false will degrade performance!
                int                     NumFixedMemoryBlocks;                     ///< Number of memory blocks to use.
                void*                   FixedMemoryBlocks[MaxNumFixedBlocks];   ///< Pointers to provided memory blocks or NULL if you want the system to allocate them for you with the System Allocator.
                size_t                  FixedMemoryBlocksByteSize[MaxNumFixedBlocks]; ///< Sizes of different memory blocks (MUST be multiple of m_pageSize), if m_memoryBlock is 0 the block will be allocated for you with the System Allocator.
                IAllocatorAllocate*     SubAllocator;                             ///< Allocator that m_memoryBlocks memory was allocated from or should be allocated (if NULL).
                size_t                  SystemChunkSize;                          ///< Size of chunk to request from the OS when more memory is needed (defaults to m_pageSize)
            }                           m_heap;
            bool                        AllocationRecords;    ///< True if we want to track memory allocations, otherwise false.
            unsigned char               StackRecordLevels;    ///< If stack recording is enabled, how many stack levels to record.
        };

        bool Create(const Descriptor& desc);

        void Destroy() override;

        //////////////////////////////////////////////////////////////////////////
        // IAllocator
        AllocatorDebugConfig GetDebugConfig() override;
        IAllocatorAllocate* GetSchema() override;

        //////////////////////////////////////////////////////////////////////////
        // IAllocatorAllocate

        pointer_type    Allocate(size_type byteSize, size_type alignment, int flags = 0, const char* name = 0, const char* fileName = 0, int lineNum = 0, unsigned int suppressStackRecord = 0) override;
        void            DeAllocate(pointer_type ptr, size_type byteSize = 0, size_type alignment = 0) override;
        pointer_type    ReAllocate(pointer_type ptr, size_type newSize, size_type newAlignment) override;
        size_type       Resize(pointer_type ptr, size_type newSize) override;
        size_type       AllocationSize(pointer_type ptr) override;
        void            GarbageCollect() override                 { m_allocator->GarbageCollect(); }

        size_type       NumAllocatedBytes() const override       { return m_allocator->NumAllocatedBytes(); }
        size_type       Capacity() const override                { return m_allocator->Capacity(); }
        /// Keep in mind this operation will execute GarbageCollect to make sure it returns, max allocation. This function WILL be slow.
        size_type       GetMaxAllocationSize() const override    { return m_allocator->GetMaxAllocationSize(); }
        size_type       GetMaxContiguousAllocationSize() const override { return m_allocator->GetMaxContiguousAllocationSize(); }
        size_type       GetUnAllocatedMemory(bool isPrint = false) const override    { return m_allocator->GetUnAllocatedMemory(isPrint); }
        IAllocatorAllocate*  GetSubAllocator() override          { return m_isCustom ? m_allocator : m_allocator->GetSubAllocator(); }

        //////////////////////////////////////////////////////////////////////////

    protected:
        SystemAllocator(const SystemAllocator&);
        SystemAllocator& operator=(const SystemAllocator&);

        Descriptor                  m_desc;
        bool                        m_isCustom;
        IAllocatorAllocate*         m_allocator;
        bool                        m_ownsOSAllocator;
    };
}
#endif // V_FRAMEWORK_CORE_MEMORY_SYSTEM_ALLOCATOR_H