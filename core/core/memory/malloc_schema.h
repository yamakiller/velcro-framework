#ifndef V_FRAMEWORK_CORE_MEMORY_MALLOC_SCHEMA_H
#define V_FRAMEWORK_CORE_MEMORY_MALLOC_SCHEMA_H

#include <core/memory/memory.h>

namespace V {
    /**
     * Malloc allocator scheme
     * Uses malloc internally. Mainly intended for debugging using host operating system features.
     */
    class MallocSchema
        : public IAllocatorAllocate {
    public:
       
        typedef void*       pointer_type;
        typedef size_t      size_type;
        typedef ptrdiff_t   difference_type;

        struct Descriptor {
            Descriptor(bool useAZMalloc = true)
                : UseAZMalloc(useAZMalloc)
            {
            }

            bool UseAZMalloc;
        };

        MallocSchema(const Descriptor& desc = Descriptor());
        virtual ~MallocSchema();

        //---------------------------------------------------------------------
        // IAllocatorAllocate
        //---------------------------------------------------------------------
        pointer_type Allocate(size_type byteSize, size_type alignment, int flags, const char* name = 0, const char* fileName = 0, int lineNum = 0, unsigned int suppressStackRecord = 0) override;
        void DeAllocate(pointer_type ptr, size_type byteSize = 0, size_type alignment = 0) override;
        pointer_type ReAllocate(pointer_type ptr, size_type newSize, size_type newAlignment) override;
        size_type Resize(pointer_type ptr, size_type newSize) override;
        size_type AllocationSize(pointer_type ptr) override;

        size_type NumAllocatedBytes() const override;
        size_type Capacity() const override;
        size_type GetMaxAllocationSize() const override;
        size_type GetMaxContiguousAllocationSize() const override;
        IAllocatorAllocate* GetSubAllocator() override;
        void GarbageCollect() override;

    private:
        typedef void* (*MallocFn)(size_t);
        typedef void (*FreeFn)(void*);

        VStd::atomic<size_t> m_bytesAllocated;
        MallocFn m_mallocFn;
        FreeFn m_freeFn;
    };
}

#endif // V_FRAMEWORK_CORE_MEMORY_MALLOC_SCHEMA_H