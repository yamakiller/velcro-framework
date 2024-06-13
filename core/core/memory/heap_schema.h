#ifndef V_FRAMEWORK_CORE_MEMORY_HEAP_SCHEMA_H
#define V_FRAMEWORK_CORE_MEMORY_HEAP_SCHEMA_H

#include <core/memory/memory.h>

namespace V {
    /**
     * Heap allocator scheme
     * Internally uses use dlmalloc or version of it (nedmalloc, ptmalloc3).
     */
    class HeapSchema
        : public IAllocatorAllocate {
    public:
        typedef void*       pointer_type;
        typedef size_t      size_type;
        typedef ptrdiff_t   difference_type;

        /**
         * Description - configure the heap allocator. By default
         * we will allocate system memory using system calls. You can
         * provide arenas (spaces) with pre-allocated memory, and use the
         * flag to specify which arena you want to allocate from.
         */
        struct Descriptor {
            Descriptor()
                : NumMemoryBlocks(0)
                , IsMultithreadAlloc(true)
            {}

            static const int        MemoryBlockAlignment = 64 * 1024;
            static const int        MaxNumBlocks = 5;
            int                     NumMemoryBlocks;                        ///< Number of memory blocks to use.
            void*                   MemoryBlocks[MaxNumBlocks];             ///< Pointers to provided memory blocks or NULL if you want the system to allocate them for you with the System Allocator.
            size_t                  MemoryBlocksByteSize[MaxNumBlocks];     ///< Sizes of different memory blocks, if MemoryBlock is 0 the block will be allocated for you with the System Allocator.
            bool                    IsMultithreadAlloc;                     ///< Set to true to enable multi threading safe allocation.
        };

        HeapSchema(const Descriptor& desc);
        virtual ~HeapSchema();

        pointer_type    Allocate(size_type byteSize, size_type alignment, int flags, const char* name = 0, const char* fileName = 0, int lineNum = 0, unsigned int suppressStackRecord = 0) override;
        void            DeAllocate(pointer_type ptr, size_type byteSize = 0, size_type alignment = 0) override;
        pointer_type    ReAllocate(pointer_type ptr, size_type newSize, size_type newAlignment) override { (void)ptr; (void)newSize; (void)newAlignment; return NULL; }
        size_type       Resize(pointer_type ptr, size_type newSize) override                  { (void)ptr; (void)newSize; return 0; }
        size_type       AllocationSize(pointer_type ptr) override;

        size_type       NumAllocatedBytes() const override               { return m_used; }
        size_type       Capacity() const override                        { return m_capacity; }
        size_type       GetMaxAllocationSize() const override;
        size_type       GetMaxContiguousAllocationSize() const override;
        IAllocatorAllocate* GetSubAllocator() override                   { return m_subAllocator; }
        void GarbageCollect() override                                   {}

    private:
        V_FORCE_INLINE size_type ChunckSize(pointer_type ptr);

        void*           m_memSpaces[Descriptor::MaxNumBlocks];
        Descriptor      m_desc;
        size_type       m_capacity;         ///< Capacity in bytes.
        size_type       m_used;             ///< Number of bytes in use.
        IAllocatorAllocate* m_subAllocator;
        bool            m_ownMemoryBlock[Descriptor::MaxNumBlocks];
    };
}

#endif // V_FRAMEWORK_CORE_MEMORY_HEAP_SCHEMA_H