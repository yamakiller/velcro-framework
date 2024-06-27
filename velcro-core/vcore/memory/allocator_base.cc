#include <vcore/memory/allocator_base.h>
#include <vcore/Memory/allocator_manager.h>

using namespace V;

AllocatorBase::AllocatorBase(IAllocatorAllocate* allocationSource, const char* name, const char* desc) :
    IAllocator(allocationSource),
    m_name(name),
    m_desc(desc) {}

AllocatorBase::~AllocatorBase() {
    V_Assert(!m_isReady, "Allocator %s (%s) is being destructed without first having gone through proper calls to PreDestroy() and Destroy(). Use AllocatorInstance<> for global allocators or AllocatorWrapper<> for local allocators.", m_name, m_desc);
}

const char* AllocatorBase::GetName() const
{
    return m_name;
}

const char* AllocatorBase::GetDescription() const
{
    return m_desc;
}

IAllocatorAllocate* AllocatorBase::GetSchema()
{
    return nullptr;
}

Debug::AllocationRecords* AllocatorBase::GetRecords()
{
    return m_records;
}

void AllocatorBase::SetRecords(Debug::AllocationRecords* records)
{
    m_records = records;
    m_memoryGuardSize = records ? records->MemoryGuardSize() : 0;
}

bool AllocatorBase::IsReady() const
{
    return m_isReady;
}

bool AllocatorBase::CanBeOverridden() const
{
    return m_canBeOverridden;
}

void AllocatorBase::PostCreate()
{
    if (m_registrationEnabled)
    {
        if (V::Environment::IsReady())
        {
            AllocatorManager::Instance().RegisterAllocator(this);
        }
        else
        {
            AllocatorManager::PreRegisterAllocator(this);
        }
    }

#if PLATFORM_MEMORY_INSTRUMENTATION_ENABLED
    m_platformMemoryInstrumentationGroupId = V::PlatformMemoryInstrumentation::GetNextGroupId();
    V::PlatformMemoryInstrumentation::RegisterGroup(m_platformMemoryInstrumentationGroupId, GetDescription(), V::PlatformMemoryInstrumentation::m_groupRoot);
#endif

    m_isReady = true;
}

void AllocatorBase::PreDestroy() {
    if (m_registrationEnabled && V::AllocatorManager::IsReady()) {
        AllocatorManager::Instance().UnRegisterAllocator(this);
    }

    m_isReady = false;
}

void AllocatorBase::SetLazilyCreated(bool lazy)
{
    m_isLazilyCreated = lazy;
}

bool AllocatorBase::IsLazilyCreated() const
{
    return m_isLazilyCreated;
}

void AllocatorBase::SetProfilingActive(bool active)
{
    m_isProfilingActive = active;
}

bool AllocatorBase::IsProfilingActive() const
{
    return m_isProfilingActive;
}

void AllocatorBase::DisableOverriding()
{
    m_canBeOverridden = false;
}

void AllocatorBase::DisableRegistration()
{
    m_registrationEnabled = false;
}

void AllocatorBase::ProfileAllocation(void* ptr, size_t byteSize, size_t alignment, const char* name, const char* fileName, int lineNum, int suppressStackRecord)
{
#if defined(V_HAS_VARIADIC_TEMPLATES) && defined(V_DEBUG_BUILD)
    ++suppressStackRecord; // one more for the fact the ebus is a function
#endif // V_HAS_VARIADIC_TEMPLATES

    if (m_isProfilingActive)
    {
#if PLATFORM_MEMORY_INSTRUMENTATION_ENABLED
        V::PlatformMemoryInstrumentation::Alloc(ptr, byteSize, 0, m_platformMemoryInstrumentationGroupId);
#else
        //EBUS_EVENT(V::Debug::MemoryDrillerBus, RegisterAllocation, this, ptr, byteSize, alignment, name, fileName, lineNum, suppressStackRecord);
#endif
    }
}

void AllocatorBase::ProfileDeallocation(void* ptr, size_t byteSize, size_t alignment, Debug::AllocationInfo* info)
{
    if (m_isProfilingActive)
    {
#if PLATFORM_MEMORY_INSTRUMENTATION_ENABLED
        V::PlatformMemoryInstrumentation::Free(ptr);
#else
        //EBUS_EVENT(V::Debug::MemoryDrillerBus, UnregisterAllocation, this, ptr, byteSize, alignment, info);
#endif
    }
}

void AllocatorBase::ProfileReallocationBegin(void* ptr, size_t newSize)
{
    if (m_isProfilingActive)
    {
#if PLATFORM_MEMORY_INSTRUMENTATION_ENABLED
        V::PlatformMemoryInstrumentation::ReallocBegin(ptr, newSize, m_platformMemoryInstrumentationGroupId);
#else
        // Driller API intensionally not called, only End is required.
        V_UNUSED(ptr);
        V_UNUSED(newSize);
#endif
    }
}

void AllocatorBase::ProfileReallocationEnd(void* ptr, void* newPtr, size_t newSize, size_t newAlignment)
{
    if (m_isProfilingActive)
    {
#if PLATFORM_MEMORY_INSTRUMENTATION_ENABLED
        V::PlatformMemoryInstrumentation::ReallocEnd(newPtr, newSize, 0);
#else
        //EBUS_EVENT(V::Debug::MemoryDrillerBus, ReallocateAllocation, this, ptr, newPtr, newSize, newAlignment);
#endif
    }
}

void AllocatorBase::ProfileResize(void* ptr, size_t newSize)
{
    if (newSize && m_isProfilingActive)
    {
        //EBUS_EVENT(V::Debug::MemoryDrillerBus, ResizeAllocation, this, ptr, newSize);
    }
}

bool AllocatorBase::OnOutOfMemory(size_t byteSize, size_t alignment, int flags, const char* name, const char* fileName, int lineNum)
{
    if (AllocatorManager::IsReady() && AllocatorManager::Instance().m_outOfMemoryListener)
    {
        AllocatorManager::Instance().m_outOfMemoryListener(this, byteSize, alignment, flags, name, fileName, lineNum);
        return true;
    }
    return false;
}
