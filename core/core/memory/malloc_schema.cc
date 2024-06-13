#include <core/memory/malloc_schema.h>

#include <core/memory/osallocator.h>
#include <core/std/algorithm.h>

namespace V {
    namespace Internal {
        struct Header {
            uint32_t offset;
            uint32_t size;
        };
    }
}

//---------------------------------------------------------------------
// MallocSchema methods
//---------------------------------------------------------------------

V::MallocSchema::MallocSchema(const Descriptor& desc) : 
    m_bytesAllocated(0) {
    if (desc.UseAZMalloc) {
        static const int DEFAULT_ALIGNMENT = sizeof(void*) * 2;  // Default malloc alignment

        m_mallocFn = [](size_t byteSize) { return V_OS_MALLOC(byteSize, DEFAULT_ALIGNMENT); };
        m_freeFn = [](void* ptr) { V_OS_FREE(ptr); };
    }
    else
    {
        m_mallocFn = &malloc;
        m_freeFn = &free;
    }
}

V::MallocSchema::~MallocSchema()
{
}

V::MallocSchema::pointer_type V::MallocSchema::Allocate(size_type byteSize, size_type alignment, int flags, const char* name, const char* fileName, int lineNum, unsigned int suppressStackRecord)
{
    (void)flags;
    (void)name;
    (void)fileName;
    (void)lineNum;
    (void)suppressStackRecord;

    if (!byteSize)
    {
        return nullptr;
    }

    if (alignment == 0)
    {
        alignment = sizeof(void*) * 2;  // Default malloc alignment
    }

    V_Assert(byteSize < 0x100000000ull, "Malloc allocator only allocates up to 4GB");

    size_type required = byteSize + sizeof(Internal::Header) + ((alignment > sizeof(double)) ? alignment : 0);  // Malloc will align to a minimum boundary for native objects, so we only pad if aligning to a large value
    void* data = (*m_mallocFn)(required);
    void* result = PointerAlignUp(reinterpret_cast<void*>(reinterpret_cast<size_t>(data) + sizeof(Internal::Header)), alignment);
    Internal::Header* header = PointerAlignDown<Internal::Header>((Internal::Header*)(reinterpret_cast<size_t>(result) - sizeof(Internal::Header)), VStd::alignment_of<Internal::Header>::value);

    header->offset = static_cast<uint32_t>(reinterpret_cast<size_type>(result) - reinterpret_cast<size_type>(data));
    header->size = static_cast<uint32_t>(byteSize);
    m_bytesAllocated += byteSize;

    return result;
}

void V::MallocSchema::DeAllocate(pointer_type ptr, size_type byteSize, size_type alignment)
{
    (void)byteSize;
    (void)alignment;

    if (!ptr) return;

    Internal::Header* header = PointerAlignDown(reinterpret_cast<Internal::Header*>(reinterpret_cast<size_t>(ptr) - sizeof(Internal::Header)), VStd::alignment_of<Internal::Header>::value);
    void* freePtr = reinterpret_cast<void*>(reinterpret_cast<size_t>(ptr) - static_cast<size_t>(header->offset));

    m_bytesAllocated -= header->size;
    (*m_freeFn)(freePtr);
}

V::MallocSchema::pointer_type V::MallocSchema::ReAllocate(pointer_type ptr, size_type newSize, size_type newAlignment)
{
    void* newPtr = Allocate(newSize, newAlignment, 0);
    size_t oldSize = AllocationSize(ptr);

    memcpy(newPtr, ptr, VStd::min(oldSize, newSize));
    DeAllocate(ptr, 0, 0);

    return newPtr;
}

V::MallocSchema::size_type V::MallocSchema::Resize(pointer_type ptr, size_type newSize)
{
    (void)ptr;
    (void)newSize;

    return 0;
}

V::MallocSchema::size_type V::MallocSchema::AllocationSize(pointer_type ptr)
{
    size_type result = 0;

    if (ptr) {
        Internal::Header* header = PointerAlignDown(reinterpret_cast<Internal::Header*>(reinterpret_cast<size_t>(ptr) - sizeof(Internal::Header)), VStd::alignment_of<Internal::Header>::value);
        result = header->size;
    }

    return result;
}

V::MallocSchema::size_type V::MallocSchema::NumAllocatedBytes() const {
    return m_bytesAllocated;
}

V::MallocSchema::size_type V::MallocSchema::Capacity() const {
    return 0;
}

V::MallocSchema::size_type V::MallocSchema::GetMaxAllocationSize() const {
    return 0xFFFFFFFFull;
}

V::MallocSchema::size_type V::MallocSchema::GetMaxContiguousAllocationSize() const {
    return V_CORE_MAX_ALLOCATOR_SIZE;
}

V::IAllocatorAllocate* V::MallocSchema::GetSubAllocator() {
    return nullptr;
}

void V::MallocSchema::GarbageCollect() {
}