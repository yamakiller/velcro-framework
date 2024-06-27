#include <vcore/std/allocator.h>
#include <vcore/memory/system_allocator.h>

namespace VStd {
    allocator::pointer_type
    allocator::allocate(size_type byteSize, size_type alignment, int flags) {
        return V::AllocatorInstance<V::SystemAllocator>::Get().Allocate(byteSize, alignment, flags, m_name, __FILE__, __LINE__, 1);
    }

    void
    allocator::deallocate(pointer_type ptr, size_type byteSize, size_type alignment) {
        V::AllocatorInstance<V::SystemAllocator>::Get().DeAllocate(ptr, byteSize, alignment);
    }

    allocator::size_type
    allocator::resize(pointer_type ptr, size_type newSize) {
        return V::AllocatorInstance<V::SystemAllocator>::Get().Resize(ptr, newSize);
    }

    auto allocator::max_size() const -> size_type {
        return V::AllocatorInstance<V::SystemAllocator>::Get().GetMaxContiguousAllocationSize();
    }

    allocator::size_type
    allocator::get_allocated_size() const {
        return V::AllocatorInstance<V::SystemAllocator>::Get().NumAllocatedBytes();
    }
}