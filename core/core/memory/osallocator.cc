#include <core/memory/osallocator.h>

namespace V {
    //=========================================================================
    // OSAllocator
    //=========================================================================
    OSAllocator::OSAllocator()
        : AllocatorBase(this, "OSAllocator", "OS allocator, allocating memory directly from the OS (C heap)!")
        , m_custom(nullptr)
        , m_numAllocatedBytes(0) {
        DisableOverriding();
    }

    //=========================================================================
    // ~OSAllocator
    //=========================================================================
    OSAllocator::~OSAllocator() {
    }

    //=========================================================================
    // Create
    //=========================================================================
    bool OSAllocator::Create(const Descriptor& desc) {
        m_custom = desc.m_custom;
        m_numAllocatedBytes = 0;
        return true;
    }

    //=========================================================================
    // Destroy
    //=========================================================================
    void OSAllocator::Destroy() {
    }

    //=========================================================================
    // GetDebugConfig
    //=========================================================================
    AllocatorDebugConfig OSAllocator::GetDebugConfig() {
        return AllocatorDebugConfig().SetExcludeFromDebugging();
    }

    //=========================================================================
    // Allocate
    // [9/2/2009]
    //=========================================================================
    OSAllocator::pointer_type
    OSAllocator::Allocate(size_type byteSize, size_type alignment, int flags, const char* name, const char* fileName, int lineNum, unsigned int suppressStackRecord)
    {
        OSAllocator::pointer_type address;
        if (m_custom) {
            address = m_custom->Allocate(byteSize, alignment, flags, name, fileName, lineNum, suppressStackRecord);
        } else {
            address = V_OS_MALLOC(byteSize, alignment);
        }

        if (address == nullptr && byteSize > 0) {
            V_Printf("Memory", "======================================================\n");
            V_Printf("Memory", "OSAllocator run out of system memory!\nWe can't track the debug allocator, since it's used for tracking and pipes trought the OS... here are the other allocator status:\n");
            OnOutOfMemory(byteSize, alignment, flags, name, fileName, lineNum);
        }

        m_numAllocatedBytes += byteSize;

        return address;
    }

    //=========================================================================
    // DeAllocate
    //=========================================================================
    void OSAllocator::DeAllocate(pointer_type ptr, size_type byteSize, size_type alignment) {
        (void)alignment;
        if (m_custom) {
            m_custom->DeAllocate(ptr);
        } else {
            V_OS_FREE(ptr);
        }

        m_numAllocatedBytes -= byteSize;
    }

}