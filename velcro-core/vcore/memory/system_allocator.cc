#include <vcore/memory/system_allocator.h>
#include <vcore/memory/allocator_manager.h>

#include <vcore/memory/osallocator.h>
//#include <vcore/memory/allocator_records.h>

#include <vcore/std/functional.h>

//#include <vcore/debug/profiler.h>

#define VCORE_SYSTEM_ALLOCATOR_HPHA 1
#define VCORE_SYSTEM_ALLOCATOR_MALLOC 2
#define VCORE_SYSTEM_ALLOCATOR_HEAP 3

#if !defined(VCORE_SYSTEM_ALLOCATOR)
    // define the default
    #define VCORE_SYSTEM_ALLOCATOR VCORE_SYSTEM_ALLOCATOR_HPHA
#endif

#if VCORE_SYSTEM_ALLOCATOR == VCORE_SYSTEM_ALLOCATOR_HPHA
    #include <vcore/memory/hpha_schema.h>
#elif VCORE_SYSTEM_ALLOCATOR == VCORE_SYSTEM_ALLOCATOR_MALLOC
    #include <vcore/memory/malloc_schema.h>
#elif VCORE_SYSTEM_ALLOCATOR == VCORE_SYSTEM_ALLOCATOR_HEAP
    #include <vcore/memory/heap_schema.h>
#else
    #error "Invalid allocator selected for SystemAllocator"
#endif

using namespace V;

//////////////////////////////////////////////////////////////////////////
// Globals - we use global storage for the first memory schema, since we can't use dynamic memory!
static bool g_isSystemSchemaUsed = false;
#if VCORE_SYSTEM_ALLOCATOR == VCORE_SYSTEM_ALLOCATOR_HPHA
    static VStd::aligned_storage<sizeof(HphaSchema), VStd::alignment_of<HphaSchema>::value>::type g_systemSchema;
#elif VCORE_SYSTEM_ALLOCATOR == VCORE_SYSTEM_ALLOCATOR_MALLOC
    static VStd::aligned_storage<sizeof(MallocSchema), VStd::alignment_of<MallocSchema>::value>::type g_systemSchema;
#elif VCORE_SYSTEM_ALLOCATOR == VCORE_SYSTEM_ALLOCATOR_HEAP
    static VStd::aligned_storage<sizeof(HeapSchema), VStd::alignment_of<HeapSchema>::value>::type g_systemSchema;
#endif

//////////////////////////////////////////////////////////////////////////

//=========================================================================
// SystemAllocator
//=========================================================================
SystemAllocator::SystemAllocator()
    : AllocatorBase(this, "SystemAllocator", "Fundamental generic memory allocator")
    , m_isCustom(false)
    , m_allocator(nullptr)
    , m_ownsOSAllocator(false) {
}

//=========================================================================
// ~SystemAllocator
//=========================================================================
SystemAllocator::~SystemAllocator() {
    if (IsReady()) {
        Destroy();
    }
}

//=========================================================================
// ~Create
//=========================================================================
bool SystemAllocator::Create(const Descriptor& desc) {
    V_Assert(IsReady() == false, "System allocator was already created!");
    if (IsReady()) {
        return false;
    }

    m_desc = desc;

    if (!AllocatorInstance<OSAllocator>::IsReady()) {
        m_ownsOSAllocator = true;
        AllocatorInstance<OSAllocator>::Create();
    }
    bool isReady = false;
    if (desc.Custom) {
        m_isCustom = true;
        m_allocator = desc.Custom;
        isReady = true;
    } else {
        m_isCustom = false;
#if VCORE_SYSTEM_ALLOCATOR == VCORE_SYSTEM_ALLOCATOR_HPHA
        HphaSchema::Descriptor heapDesc;
        heapDesc.PageSize = desc.m_heap.PageSize;
        heapDesc.PoolPageSize = desc.m_heap.PoolPageSize;
        V_Assert(desc.m_heap.NumFixedMemoryBlocks <= 1, "We support max1 memory block at the moment!");
        if (desc.m_heap.NumFixedMemoryBlocks > 0)
        {
            heapDesc.FixedMemoryBlock = desc.m_heap.FixedMemoryBlocks[0];
            heapDesc.FixedMemoryBlockByteSize = desc.m_heap.FixedMemoryBlocksByteSize[0];
        }
        heapDesc.SubAllocator = desc.m_heap.SubAllocator;
        heapDesc.IsPoolAllocations = desc.m_heap.IsPoolAllocations;
        // Fix SystemAllocator from growing in small chunks
        heapDesc.SystemChunkSize = desc.m_heap.SystemChunkSize;
        
#elif VCORE_SYSTEM_ALLOCATOR == VCORE_SYSTEM_ALLOCATOR_MALLOC
        MallocSchema::Descriptor heapDesc;
#elif VCORE_SYSTEM_ALLOCATOR == VCORE_SYSTEM_ALLOCATOR_HEAP
        // TODO: 内存拷贝溢出heapDesc.MemoryBlocks == 5, desc.m_heap.FixedMemoryBlocks == 3
        HeapSchema::Descriptor heapDesc;
        memcpy(heapDesc.MemoryBlocks, desc.m_heap.FixedMemoryBlocks, sizeof(heapDesc.MemoryBlocks));
        memcpy(heapDesc.MemoryBlocksByteSize, desc.m_heap.FixedMemoryBlocksByteSize, sizeof(heapDesc.MemoryBlocksByteSize));
        heapDesc.NumMemoryBlocks = desc.m_heap.NumFixedMemoryBlocks;
        // TODO: 需要修改
#endif
        // if we are the system allocator
        if (&AllocatorInstance<SystemAllocator>::Get() == this) {
            V_Assert(!g_isSystemSchemaUsed, "V::SystemAllocator MUST be created first! It's the source of all allocations!");

#if VCORE_SYSTEM_ALLOCATOR == VCORE_SYSTEM_ALLOCATOR_HPHA
            m_allocator = new(&g_systemSchema)HphaSchema(heapDesc);
#elif VCORE_SYSTEM_ALLOCATOR == VCORE_SYSTEM_ALLOCATOR_MALLOC
            m_allocator = new(&g_systemSchema)MallocSchema(heapDesc);
#elif VCORE_SYSTEM_ALLOCATOR == VCORE_SYSTEM_ALLOCATOR_HEAP
            m_allocator = new(&g_systemSchema)HeapSchema(heapDesc);
#endif
            g_isSystemSchemaUsed = true;
            isReady = true;
        } else {
            // this class should be inheriting from SystemAllocator
            V_Assert(AllocatorInstance<SystemAllocator>::IsReady(), "System allocator must be created before any other allocator! They allocate from it.");

#if VCORE_SYSTEM_ALLOCATOR == VCORE_SYSTEM_ALLOCATOR_HPHA
            m_allocator = vcreate(HphaSchema, (heapDesc), SystemAllocator);
#elif VCORE_SYSTEM_ALLOCATOR == VCORE_SYSTEM_ALLOCATOR_MALLOC
            m_allocator = vcreate(MallocSchema, (heapDesc), SystemAllocator);
#elif VCORE_SYSTEM_ALLOCATOR == VCORE_SYSTEM_ALLOCATOR_HEAP
            m_allocator = vcreate(HeapSchema, (heapDesc), SystemAllocator);
#endif
            if (m_allocator == nullptr) {
                isReady = false;
            } else {
                isReady = true;
            }
        }
    }

    return isReady;
}

//=========================================================================
// Allocate
//=========================================================================
void SystemAllocator::Destroy() {
    if (g_isSystemSchemaUsed) {
        int dummy;
        (void)dummy;
    }

    if (!m_isCustom) {
        if ((void*)m_allocator == (void*)&g_systemSchema) {
#if VCORE_SYSTEM_ALLOCATOR == VCORE_SYSTEM_ALLOCATOR_HPHA
            static_cast<HphaSchema*>(m_allocator)->~HphaSchema();
#elif VCORE_SYSTEM_ALLOCATOR == VCORE_SYSTEM_ALLOCATOR_MALLOC
            static_cast<MallocSchema*>(m_allocator)->~MallocSchema();
#elif VCORE_SYSTEM_ALLOCATOR == VCORE_SYSTEM_ALLOCATOR_HEAP
            static_cast<HeapSchema*>(m_allocator)->~HeapSchema();
#endif
            g_isSystemSchemaUsed = false;
        } else {
            vdestroy(m_allocator);
        }
    }

    if (m_ownsOSAllocator) {
        AllocatorInstance<OSAllocator>::Destroy();
        m_ownsOSAllocator = false;
    }
}

AllocatorDebugConfig SystemAllocator::GetDebugConfig() {
    return AllocatorDebugConfig()
    .SetStackRecordLevels(m_desc.StackRecordLevels)
    .SetUsesMemoryGuards(!m_isCustom)
    .SetMarksUnallocatedMemory(!m_isCustom)
    .SetExcludeFromDebugging(!m_desc.AllocationRecords);
}

IAllocatorAllocate* SystemAllocator::GetSchema() {
    return m_allocator;
}

//=========================================================================
// Allocate
//=========================================================================
SystemAllocator::pointer_type
SystemAllocator::Allocate(size_type byteSize, size_type alignment, int flags, const char* name, const char* fileName, int lineNum, unsigned int suppressStackRecord)
{
    if (byteSize == 0) {
        return nullptr;
    }
    V_Assert(byteSize > 0, "You can not allocate 0 bytes!");
    V_Assert((alignment & (alignment - 1)) == 0, "Alignment must be power of 2!");

    byteSize = MemorySizeAdjustedUp(byteSize);
    SystemAllocator::pointer_type address = m_allocator->Allocate(byteSize, alignment, flags, name, fileName, lineNum, suppressStackRecord + 1);

    if (address == nullptr) {
        // Free all memory we can and try again!
        AllocatorManager::Instance().GarbageCollect();

        address = m_allocator->Allocate(byteSize, alignment, flags, name, fileName, lineNum, suppressStackRecord + 1);
    }

    if (address == nullptr) {
        byteSize = MemorySizeAdjustedDown(byteSize);  // restore original size

        if (!OnOutOfMemory(byteSize, alignment, flags, name, fileName, lineNum)) {
            if (GetRecords()) {
                //EBUS_EVENT(Debug::MemoryDrillerBus, DumpAllAllocations);
            }
        }
    }

    V_Assert(address != nullptr, "SystemAllocator: Failed to allocate %d bytes aligned on %d (flags: 0x%08x) %s : %s (%d)!", byteSize, alignment, flags, name ? name : "(no name)", fileName ? fileName : "(no file name)", lineNum);

    //V_PROFILE_MEMORY_ALLOC_EX(MemoryReserved, fileName, lineNum, address, byteSize, name);
    V_MEMORY_PROFILE(ProfileAllocation(address, byteSize, alignment, name, fileName, lineNum, suppressStackRecord + 1));

    return address;
}

//=========================================================================
// DeAllocate
//=========================================================================
void SystemAllocator::DeAllocate(pointer_type ptr, size_type byteSize, size_type alignment) {
    byteSize = MemorySizeAdjustedUp(byteSize);
    //V_PROFILE_MEMORY_FREE(MemoryReserved, ptr);
    V_MEMORY_PROFILE(ProfileDeallocation(ptr, byteSize, alignment, nullptr));
    m_allocator->DeAllocate(ptr, byteSize, alignment);
}

//=========================================================================
// ReAllocates
//=========================================================================
SystemAllocator::pointer_type
SystemAllocator::ReAllocate(pointer_type ptr, size_type newSize, size_type newAlignment) {
    newSize = MemorySizeAdjustedUp(newSize);

    V_MEMORY_PROFILE(ProfileReallocationBegin(ptr, newSize));
    //V_PROFILE_MEMORY_FREE(MemoryReserved, ptr);
    pointer_type newAddress = m_allocator->ReAllocate(ptr, newSize, newAlignment);
    //V_PROFILE_MEMORY_ALLOC(MemoryReserved, newAddress, newSize, "SystemAllocator realloc");
    V_MEMORY_PROFILE(ProfileReallocationEnd(ptr, newAddress, newSize, newAlignment));

    return newAddress;
}

//=========================================================================
// Resize
//=========================================================================
SystemAllocator::size_type
SystemAllocator::Resize(pointer_type ptr, size_type newSize)
{
    newSize = MemorySizeAdjustedUp(newSize);
    size_type resizedSize = m_allocator->Resize(ptr, newSize);

    V_MEMORY_PROFILE(ProfileResize(ptr, resizedSize));

    return MemorySizeAdjustedDown(resizedSize);
}

//=========================================================================
// AllocationSize
//=========================================================================
SystemAllocator::size_type
SystemAllocator::AllocationSize(pointer_type ptr) {
    size_type allocSize = MemorySizeAdjustedDown(m_allocator->AllocationSize(ptr));

    return allocSize;
}