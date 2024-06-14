#ifndef V_FRAMEWORK_CORE_MEMORY_HPHA_SCHEMA_H
#define V_FRAMEWORK_CORE_MEMORY_HPHA_SCHEMA_H

#include <core/memory/memory.h>
#include <core/std/typetraits/aligned_storage.h>

namespace V {
    class HpAllocator;

    /**
    * Heap allocator schema, based on Dimitar Lazarov "High Performance Heap Allocator".
    */
    class HphaSchema
        : public IAllocatorAllocate
    {
    public:
        /**
        * Description - configure the heap allocator. By default
        * we will allocate system memory using system calls. You can
        * provide arena (memory block) with pre-allocated memory.
        */
        struct Descriptor
        {
            Descriptor()
                : m_fixedMemoryBlockAlignment(V_TRAIT_OS_DEFAULT_PAGE_SIZE)
                , m_pageSize(V_PAGE_SIZE)
                , m_poolPageSize(4*1024)
                , m_isPoolAllocations(true)
                , m_fixedMemoryBlockByteSize(0)
                , m_fixedMemoryBlock(nullptr)
                , m_subAllocator(nullptr)
                , m_systemChunkSize(0)
                , m_capacity(V_CORE_MAX_ALLOCATOR_SIZE)
            {}

            unsigned int            m_fixedMemoryBlockAlignment;
            unsigned int            m_pageSize;                             ///< Page allocation size must be 1024 bytes aligned.
            unsigned int            m_poolPageSize : 31;                    ///< Page size used to small memory allocations. Must be less or equal to m_pageSize and a multiple of it.
            unsigned int            m_isPoolAllocations : 1;                ///< True to allow allocations from pools, otherwise false.
            size_t                  m_fixedMemoryBlockByteSize;             ///< Memory block size, if 0 we use the OS memory allocation functions.
            void*                   m_fixedMemoryBlock;                     ///< Can be NULL if so the we will allocate memory from the subAllocator if m_memoryBlocksByteSize is != 0.
            IAllocatorAllocate*     m_subAllocator;                         ///< Allocator that m_memoryBlocks memory was allocated from or should be allocated (if NULL).
            size_t                  m_systemChunkSize;                      ///< Size of chunk to request from the OS when more memory is needed (defaults to m_pageSize)
            size_t                  m_capacity;                             ///< Max size this allocator can grow to
        };


        HphaSchema(const Descriptor& desc);
        virtual ~HphaSchema();

        pointer_type    Allocate(size_type byteSize, size_type alignment, int flags = 0, const char* name = 0, const char* fileName = 0, int lineNum = 0, unsigned int suppressStackRecord = 0) override;
        void            DeAllocate(pointer_type ptr, size_type byteSize = 0, size_type alignment = 0) override;
        pointer_type    ReAllocate(pointer_type ptr, size_type newSize, size_type newAlignment) override;
        /// Resizes allocated memory block to the size possible and returns that size.
        size_type       Resize(pointer_type ptr, size_type newSize) override;
        size_type       AllocationSize(pointer_type ptr) override;

        size_type       NumAllocatedBytes() const override;
        size_type       Capacity() const override;
        size_type       GetMaxAllocationSize() const override;
        size_type       GetMaxContiguousAllocationSize() const override;
        size_type       GetUnAllocatedMemory(bool isPrint = false) const override;
        IAllocatorAllocate* GetSubAllocator() override                       { return m_desc.m_subAllocator; }

        /// Return unused memory to the OS (if we don't use fixed block). Don't call this unless you really need free memory, it is slow.
        void            GarbageCollect() override;

    private:
        // [LY-84974][sconel@][2018-08-10] SliceStrike integration up to CL 671758
        // this must be at least the max size of HpAllocator (defined in the cpp) + any platform compiler padding
        static const int hpAllocatorStructureSize = 16584;
        // [LY][sconel@] end
        
        Descriptor          m_desc;
        int                 m_pad;      // pad the Descriptor to avoid C4355
        size_type           m_capacity;                 ///< Capacity in bytes.
        HpAllocator*        m_allocator;
        // [LY-84974][sconel@][2018-08-10] SliceStrike integration up to CL 671758
        VStd::aligned_storage<hpAllocatorStructureSize, 16>::type m_hpAllocatorBuffer;    ///< Memory buffer for HpAllocator
        // [LY][sconel@] end
        bool                m_ownMemoryBlock;
    };
} // namespace AZ

#endif // V_FRAMEWORK_CORE_MEMORY_HPHA_SCHEMA_H