#include <core/memory/allocator_manager.h>
#include <core/memory/memory.h>
#include <core/memory/iallocator.h>

#include <core/memory/osallocator.h>
#include <core/memory/allocator_records.h>
#include <core/memory/allocator_override_shim.h>
#include <core/memory/malloc_schema.h>
//#include <core/memory/MemoryDrillerBus.h>

#include <core/std/parallel/lock.h>
#include <core/std/smart_ptr/make_shared.h>
#include <core/std/containers/array.h>

#include <core/math/crc.h>

using namespace V;

#if !defined(RELEASE) && !defined(VCORE_MEMORY_ENABLE_OVERRIDES)
#   define VCORE_MEMORY_ENABLE_OVERRIDES
#endif

namespace V {
    namespace Internal {
        struct AMStringHasher {
            using is_transparent = void;
            template<typename ConvertibleToStringView>
            size_t operator()(const ConvertibleToStringView& key) {
                return VStd::hash<VStd::string_view>{}(key);
            }
        };
        using AMString = VStd::basic_string<char, VStd::char_traits<char>, VStdIAllocator>;
        using AllocatorNameMap = VStd::unordered_map<AMString, IAllocator*, AMStringHasher, VStd::equal_to<>, VStdIAllocator>;
        using AllocatorRemappings = VStd::unordered_map<AMString, AMString, AMStringHasher, VStd::equal_to<>, VStdIAllocator>;

        // For allocators that are created before we have an environment, we keep some module-local data for them so that we can register them 
        // properly once the environment is attached.
        struct PreEnvironmentAttachData {
            static const int MAX_UNREGISTERED_ALLOCATORS = 8;
            VStd::mutex Mutex;
            MallocSchema MallocSchema;
            IAllocator* UnregisteredAllocators[MAX_UNREGISTERED_ALLOCATORS];
            int UnregisteredAllocatorCount = 0;
        };

    }
}

struct V::AllocatorManager::InternalData
{
    explicit InternalData(const VStdIAllocator& alloc)
        : m_allocatorMap(alloc)
        , m_remappings(alloc)
        , m_remappingsReverse(alloc)
    {}
    Internal::AllocatorNameMap m_allocatorMap;
    Internal::AllocatorRemappings m_remappings;
    Internal::AllocatorRemappings m_remappingsReverse;
};

static V::EnvironmentVariable<AllocatorManager> _allocManager = nullptr;
static AllocatorManager* _allocManagerDebug = nullptr;  // For easier viewing in crash dumps

/// Returns a module-local instance of data to use for allocators that are created before the environment is attached.
static V::Internal::PreEnvironmentAttachData& GetPreEnvironmentAttachData() {
    static V::Internal::PreEnvironmentAttachData _data;
    return _data;
}

/// Called to register an allocator before AllocatorManager::Instance() is available.
void AllocatorManager::PreRegisterAllocator(IAllocator* allocator)
{
    auto& data = GetPreEnvironmentAttachData();

#ifdef VCORE_MEMORY_ENABLE_OVERRIDES
    // All allocators must switch to an OverrideEnabledAllocationSource proxy if they are to support allocator overriding.
    if (allocator->CanBeOverridden())
    {
        auto shim = Internal::AllocatorOverrideShim::Create(allocator, &data.MallocSchema);
        allocator->SetAllocationSource(shim);
    }
#endif

    {
        VStd::lock_guard<VStd::mutex> lock(data.Mutex);
        V_Assert(data.UnregisteredAllocatorCount < Internal::PreEnvironmentAttachData::MAX_UNREGISTERED_ALLOCATORS, "Too many allocators trying to register before environment attached!");
        data.UnregisteredAllocators[data.UnregisteredAllocatorCount++] = allocator;
    }
}

IAllocator* AllocatorManager::CreateLazyAllocator(size_t size, size_t Alignment, IAllocator*(*creationFn)(void*)) {
    void* mem = GetPreEnvironmentAttachData().MallocSchema.Allocate(size, Alignment, 0);
    IAllocator* result = creationFn(mem);

    return result;
}

//////////////////////////////////////////////////////////////////////////
bool AllocatorManager::IsReady() {
    return _allocManager.IsConstructed();
}
//////////////////////////////////////////////////////////////////////////
void AllocatorManager::Destroy() {
    if (_allocManager) {
        _allocManager->InternalDestroy();
        _allocManager.Reset();
    }
}

//////////////////////////////////////////////////////////////////////////
// The only allocator manager instance.
AllocatorManager& AllocatorManager::Instance()
{
    if (!_allocManager)
    {
        V_Assert(Environment::IsReady(), "Environment must be ready before calling Instance()");
        _allocManager = V::Environment::CreateVariable<AllocatorManager>(V_CRC("V::AllocatorManager::_allocManager", 0x6bdd908c));

        // Register any allocators that were created in this module before we attached to the environment
        auto& data = GetPreEnvironmentAttachData();

        {
            VStd::lock_guard<VStd::mutex> lock(data.Mutex);
            for (int idx = 0; idx < data.UnregisteredAllocatorCount; idx++)
            {
                _allocManager->RegisterAllocator(data.UnregisteredAllocators[idx]);
            }

            data.UnregisteredAllocatorCount = 0;
        
        }

        _allocManagerDebug = &(*_allocManager);
    }

    return *_allocManager;
}
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
// Create malloc schema using custom V_OS_MALLOC allocator.
V::MallocSchema* AllocatorManager::CreateMallocSchema()
{
    return static_cast<V::MallocSchema*>(new(V_OS_MALLOC(sizeof(V::MallocSchema), alignof(V::MallocSchema))) V::MallocSchema());
}


//=========================================================================
// AllocatorManager
//=========================================================================
AllocatorManager::AllocatorManager()
    : m_profilingRefcount(0)
    , m_mallocSchema(CreateMallocSchema(), [](V::MallocSchema* schema) {
        if (schema) {
            schema->~MallocSchema();
            V_OS_FREE(schema);
        }
    }
)
{
    m_overrideSource = nullptr;
    m_numAllocators = 0;
    m_isAllocatorLeaking = false;
    m_configurationFinalized = false;
    m_defaultTrackingRecordMode = V::Debug::AllocationRecords::RECORD_NO_RECORDS;
    m_data = new (m_mallocSchema->Allocate(sizeof(InternalData), VStd::alignment_of<InternalData>::value, 0)) InternalData(VStdIAllocator(m_mallocSchema.get()));
}

//=========================================================================
// ~AllocatorManager
//=========================================================================
AllocatorManager::~AllocatorManager()
{
    InternalDestroy();
}

//=========================================================================
// RegisterAllocator
//=========================================================================
void
AllocatorManager::RegisterAllocator(class IAllocator* alloc)
{
    VStd::lock_guard<VStd::mutex> lock(m_allocatorListMutex);
    V_Assert(m_numAllocators < m_maxNumAllocators, "Too many allocators %d! Max is %d", m_numAllocators, m_maxNumAllocators);

    for (size_t i = 0; i < m_numAllocators; i++)
    {
        V_Assert(m_allocators[i] != alloc, "Allocator %s (%s) registered twice!", alloc->GetName(), alloc->GetDescription());
    }

    alloc->SetProfilingActive(m_profilingRefcount.load() > 0);

    m_allocators[m_numAllocators++] = alloc;

#ifdef VCORE_MEMORY_ENABLE_OVERRIDES
    ConfigureAllocatorOverrides(alloc);
#endif
}

//=========================================================================
// InternalDestroy
//=========================================================================
void
AllocatorManager::InternalDestroy() {
    while (m_numAllocators > 0) {
        IAllocator* allocator = m_allocators[m_numAllocators - 1];
        (void)allocator;
        V_Assert(allocator->IsLazilyCreated(), "Manually created allocator '%s (%s)' must be manually destroyed before shutdown", allocator->GetName(), allocator->GetDescription());
        m_allocators[--m_numAllocators] = nullptr;
        // Do not actually destroy the lazy allocator as it may have work to do during non-deterministic shutdown
    }

    if (m_data) {
        m_data->~InternalData();
        m_mallocSchema->DeAllocate(m_data);
        m_data = nullptr;
    }

    if (!m_isAllocatorLeaking) {
        V_Assert(m_numAllocators == 0, "There are still %d registered allocators!", m_numAllocators);
    }
}

//=========================================================================
// ConfigureAllocatorOverrides
//=========================================================================
void 
AllocatorManager::ConfigureAllocatorOverrides(IAllocator* alloc) {
    auto record = m_data->m_allocatorMap.emplace(VStd::piecewise_construct, VStd::forward_as_tuple(alloc->GetName(), VStdIAllocator(m_mallocSchema.get())), VStd::forward_as_tuple(alloc));

    // We only need to keep going if the allocator supports overrides.
    if (!alloc->CanBeOverridden()) {
        return;
    }

    if (!alloc->IsAllocationSourceChanged()) {
        // All allocators must switch to an OverrideEnabledAllocationSource proxy if they are to support allocator overriding.
        auto overrideEnabled = Internal::AllocatorOverrideShim::Create(alloc, m_mallocSchema.get());
        alloc->SetAllocationSource(overrideEnabled);
    }

    auto itr = m_data->m_remappings.find(record.first->first);

    if (itr != m_data->m_remappings.end()){
        auto remapTo = m_data->m_allocatorMap.find(itr->second);

        if (remapTo != m_data->m_allocatorMap.end()) {
            static_cast<Internal::AllocatorOverrideShim*>(alloc->GetAllocationSource())->SetOverride(remapTo->second->GetOriginalAllocationSource());
        }
    }

    itr = m_data->m_remappingsReverse.find(record.first->first);

    if (itr != m_data->m_remappingsReverse.end()){
        auto remapFrom = m_data->m_allocatorMap.find(itr->second);

        if (remapFrom != m_data->m_allocatorMap.end()) {
            V_Assert(!m_configurationFinalized, "Allocators may only remap to allocators that have been created before configuration finalization");
            static_cast<Internal::AllocatorOverrideShim*>(remapFrom->second->GetAllocationSource())->SetOverride(alloc->GetOriginalAllocationSource());
        }
    }

    if (m_overrideSource) {
        static_cast<Internal::AllocatorOverrideShim*>(alloc->GetAllocationSource())->SetOverride(m_overrideSource);
    }

    if (m_configurationFinalized) {
        // We can get rid of the intermediary if configuration won't be changing any further.
        // (The creation of it at the top of this function was superflous, but it made it easier to set things up going through a single code path.)
        auto shim = static_cast<Internal::AllocatorOverrideShim*>(alloc->GetAllocationSource());
        alloc->SetAllocationSource(shim->GetOverride());
        Internal::AllocatorOverrideShim::Destroy(shim);
    }
}

//=========================================================================
// UnRegisterAllocator
//=========================================================================
void
AllocatorManager::UnRegisterAllocator(class IAllocator* alloc){
    VStd::lock_guard<VStd::mutex> lock(m_allocatorListMutex);

    if (alloc->GetRecords()){
        //EBUS_EVENT(Debug::MemoryDrillerBus, UnregisterAllocator, alloc);
    }

    for (int i = 0; i < m_numAllocators; ++i) {
        if (m_allocators[i] == alloc) {
            --m_numAllocators;
            m_allocators[i] = m_allocators[m_numAllocators];
        }
    }
}



//=========================================================================
// LockAllocators
//=========================================================================
AllocatorManager::AllocatorLock::~AllocatorLock() {
}

VStd::shared_ptr<AllocatorManager::AllocatorLock>
AllocatorManager::LockAllocators() {
    class AllocatorLockImpl : public AllocatorLock {
    public:
        AllocatorLockImpl(VStd::mutex& mutex) : m_lock(mutex) {
        }

        VStd::unique_lock<VStd::mutex> m_lock;
    };

    VStd::shared_ptr<AllocatorLock> result = VStd::allocate_shared<AllocatorLockImpl>(OSStdAllocator(), m_allocatorListMutex);

    return result;
}

//=========================================================================
// GarbageCollect
//=========================================================================
void
AllocatorManager::GarbageCollect()
{
    VStd::lock_guard<VStd::mutex> lock(m_allocatorListMutex);

    for (int i = 0; i < m_numAllocators; ++i) {
        m_allocators[i]->GetAllocationSource()->GarbageCollect();
    }
}

//=========================================================================
// AddOutOfMemoryListener
//=========================================================================
bool
AllocatorManager::AddOutOfMemoryListener(const OutOfMemoryCBType& cb)
{
    V_Warning("Memory", !m_outOfMemoryListener, "Out of memory listener was already installed!");
    if (!m_outOfMemoryListener)
    {
        m_outOfMemoryListener = cb;
        return true;
    }

    return false;
}

//=========================================================================
// RemoveOutOfMemoryListener
//=========================================================================
void
AllocatorManager::RemoveOutOfMemoryListener()
{
    m_outOfMemoryListener = nullptr;
}

//=========================================================================
// SetTrackingMode
//=========================================================================
void
AllocatorManager::SetTrackingMode(V::Debug::AllocationRecords::Mode mode)
{
    VStd::lock_guard<VStd::mutex> lock(m_allocatorListMutex);
    for (int i = 0; i < m_numAllocators; ++i)
    {
        V::Debug::AllocationRecords* records = m_allocators[i]->GetRecords();
        if (records)
        {
            records->SetMode(mode);
        }
    }
}

//=========================================================================
// SetOverrideSchema
//=========================================================================
void
AllocatorManager::SetOverrideAllocatorSource(IAllocatorAllocate* source, bool overrideExistingAllocators){
    (void)source;
    (void)overrideExistingAllocators;

#ifdef VCORE_MEMORY_ENABLE_OVERRIDES
    V_Assert(!m_configurationFinalized, "You cannot set an allocator source after FinalizeConfiguration() has been called.");
    m_overrideSource = source;

    if (overrideExistingAllocators) {
        VStd::lock_guard<VStd::mutex> lock(m_allocatorListMutex);
        for (int i = 0; i < m_numAllocators; ++i){
            if (m_allocators[i]->CanBeOverridden()){
                auto shim = static_cast<Internal::AllocatorOverrideShim*>(m_allocators[i]->GetAllocationSource());
                shim->SetOverride(source);
            }
        }
    }
#endif
}

//=========================================================================
// AddAllocatorRemapping
//=========================================================================
void
AllocatorManager::AddAllocatorRemapping(const char* fromName, const char* toName){
    (void)fromName;
    (void)toName;

#ifdef VCORE_MEMORY_ENABLE_OVERRIDES
    V_Assert(!m_configurationFinalized, "You cannot set an allocator remapping after FinalizeConfiguration() has been called.");
    m_data->m_remappings.emplace(VStd::piecewise_construct, VStd::forward_as_tuple(fromName, m_mallocSchema.get()), VStd::forward_as_tuple(toName, m_mallocSchema.get()));
    m_data->m_remappingsReverse.emplace(VStd::piecewise_construct, VStd::forward_as_tuple(toName, m_mallocSchema.get()), VStd::forward_as_tuple(fromName, m_mallocSchema.get()));
#endif
}

void 
AllocatorManager::FinalizeConfiguration() {
    if (m_configurationFinalized) {
        return;
    }

#ifdef VCORE_MEMORY_ENABLE_OVERRIDES
    {
        VStd::lock_guard<VStd::mutex> lock(m_allocatorListMutex);

        for (int i = 0; i < m_numAllocators; ++i) {
            if (!m_allocators[i]->CanBeOverridden()) {
                continue;
            }

            auto shim = static_cast<Internal::AllocatorOverrideShim*>(m_allocators[i]->GetAllocationSource());

            if (!shim->IsOverridden()) {
                m_allocators[i]->ResetAllocationSource();
                Internal::AllocatorOverrideShim::Destroy(shim);
            } else if (!shim->HasOrphanedAllocations()) {
                m_allocators[i]->SetAllocationSource(shim->GetOverride());
                Internal::AllocatorOverrideShim::Destroy(shim);
            } else {
                shim->SetFinalizedConfiguration();
            }
        }
    }
#endif

    m_configurationFinalized = true;
}

void
AllocatorManager::EnterProfilingMode() {
    if (!m_profilingRefcount++) {
        // We were at 0, so enable profiling
        VStd::lock_guard<VStd::mutex> lock(m_allocatorListMutex);

        for (int i = 0; i < m_numAllocators; ++i) {
            m_allocators[i]->SetProfilingActive(true);
        }
    }
}

void
AllocatorManager::ExitProfilingMode() {
    if (!--m_profilingRefcount) {
        // We've gone down to 0, so disable profiling
        VStd::lock_guard<VStd::mutex> lock(m_allocatorListMutex);

        for (int i = 0; i < m_numAllocators; ++i)
        {
            m_allocators[i]->SetProfilingActive(false);
        }
    }

    V_Assert(m_profilingRefcount.load() >= 0, "ExitProfilingMode called without matching EnterProfilingMode");
}

void
AllocatorManager::DumpAllocators() {
    static const char TAG[] = "mem";

    VStd::lock_guard<VStd::mutex> lock(m_allocatorListMutex);
    size_t totalUsedBytes = 0;
    size_t totalReservedBytes = 0;
    size_t totalConsumedBytes = 0;

    memset(m_dumpInfo, 0, sizeof(m_dumpInfo));
    void* sourceList[m_maxNumAllocators];

    V_Printf(TAG, "%d allocators active\n", m_numAllocators);
    V_Printf(TAG, "Index,Name,Used kb,Reserved kb,Consumed kb\n");

    for (int i = 0; i < m_numAllocators; i++) {
        auto allocator = m_allocators[i];
        auto source = allocator->GetAllocationSource();
        const char* name = allocator->GetName();
        size_t usedBytes = source->NumAllocatedBytes();
        size_t reservedBytes = source->Capacity();
        size_t consumedBytes = reservedBytes;

        // Very hacky and inefficient check to see if this allocator obtains its memory from another allocator
        sourceList[i] = source;
        if (VStd::find(sourceList, sourceList + i, allocator->GetSchema()) != sourceList + i) {
            consumedBytes = 0;
        }

        totalUsedBytes += usedBytes;
        totalReservedBytes += reservedBytes;
        totalConsumedBytes += consumedBytes;
        m_dumpInfo[i].Name = name;
        m_dumpInfo[i].Used = usedBytes;
        m_dumpInfo[i].Reserved = reservedBytes;
        m_dumpInfo[i].Consumed = consumedBytes;
        V_Printf(TAG, "%d,%s,%.2f,%.2f,%.2f\n", i, name, usedBytes / 1024.0f, reservedBytes / 1024.0f, consumedBytes / 1024.0f);
    }

    V_Printf(TAG, "-,Totals,%.2f,%.2f,%.2f\n", totalUsedBytes / 1024.0f, totalReservedBytes / 1024.0f, totalConsumedBytes / 1024.0f);
}
void AllocatorManager::GetAllocatorStats(size_t& allocatedBytes, size_t& capacityBytes, VStd::vector<AllocatorStats>* outStats)
{
    allocatedBytes = 0;
    capacityBytes = 0;

    VStd::lock_guard<VStd::mutex> lock(m_allocatorListMutex);
    const int allocatorCount = GetNumAllocators();
    VStd::unordered_map<V::IAllocatorAllocate*, V::IAllocator*> existingAllocators;
    VStd::unordered_map<V::IAllocatorAllocate*, V::IAllocator*> sourcesToAllocators;

    // Build a mapping of original allocator sources to their allocators
    for (int i = 0; i < allocatorCount; ++i) {
        V::IAllocator* allocator = GetAllocator(i);
        sourcesToAllocators.emplace(allocator->GetOriginalAllocationSource(), allocator);
    }

    for (int i = 0; i < allocatorCount; ++i) {
        V::IAllocator* allocator = GetAllocator(i);
        V::IAllocatorAllocate* source = allocator->GetAllocationSource();
        V::IAllocatorAllocate* originalSource = allocator->GetOriginalAllocationSource();
        V::IAllocatorAllocate* schema = allocator->GetSchema();
        V::IAllocator* alias = (source != originalSource) ? sourcesToAllocators[source] : nullptr;

        if (schema && !alias) {
            // Check to see if this allocator's source maps to another allocator
            // Need to check both the schema and the allocator itself, as either one might be used as the alias depending on how it's implemented
            VStd::array<V::IAllocatorAllocate*, 2> checkAllocators = { { schema, allocator->GetAllocationSource() } };

            for (V::IAllocatorAllocate* check : checkAllocators) {
                auto existing = existingAllocators.emplace(check, allocator);

                if (!existing.second) {
                    alias = existing.first->second;
                    // Do not break out of the loop as we need to add to the map for all entries
                }
            }
        }

        static const V::IAllocator* OS_ALLOCATOR = &V::AllocatorInstance<V::OSAllocator>::GetAllocator();
        size_t sourceAllocatedBytes = source->NumAllocatedBytes();
        size_t sourceCapacityBytes = source->Capacity();

        if (allocator == OS_ALLOCATOR) {
            // Need to special case the OS allocator because its capacity is a made-up number. Better to just use the allocated amount, it will hopefully be small anyway.
            sourceCapacityBytes = sourceAllocatedBytes;
        }

        if (outStats) {
            outStats->emplace(outStats->end(), allocator->GetName(), alias ? alias->GetName() : allocator->GetDescription(), sourceAllocatedBytes, sourceCapacityBytes, alias != nullptr);
        }

        if (!alias) {
            allocatedBytes += sourceAllocatedBytes;
            capacityBytes += sourceCapacityBytes;
        }
    }
}

//=========================================================================
// MemoryBreak
//=========================================================================
AllocatorManager::MemoryBreak::MemoryBreak()
{
    AddressStart = nullptr;
    AddressEnd = nullptr;
    ByteSize = 0;
    Alignment = static_cast<size_t>(0xffffffff);
    Name = nullptr;

    FileName = nullptr;
    LineNum = -1;
}

//=========================================================================
// SetMemoryBreak
//=========================================================================
void
AllocatorManager::SetMemoryBreak(int slot, const MemoryBreak& mb)
{
    V_Assert(slot < MaxNumMemoryBreaks, "Invalid slot index");
    m_activeBreaks |= 1 << slot;
    m_memoryBreak[slot] = mb;
}

//=========================================================================
// ResetMemoryBreak
//=========================================================================
void
AllocatorManager::ResetMemoryBreak(int slot) {
    if (slot == -1) {
        m_activeBreaks = 0;
    } else {
        V_Assert(slot < MaxNumMemoryBreaks, "Invalid slot index");
        m_activeBreaks &= ~(1 << slot);
    }
}

//=========================================================================
// DebugBreak
//=========================================================================
void
AllocatorManager::DebugBreak(void* address, const Debug::AllocationInfo& info) {
    (void)address;
    (void)info;
    if (m_activeBreaks) {
        void* AddressEnd = (char*)address + info.ByteSize;
        (void)AddressEnd;
        for (int i = 0; i < MaxNumMemoryBreaks; ++i) {
            if ((m_activeBreaks & (1 << i)) != 0) {
                // check address range
                V_Assert(!((address <= m_memoryBreak[i].AddressStart && m_memoryBreak[i].AddressStart < AddressEnd) ||
                            (address < m_memoryBreak[i].AddressEnd && m_memoryBreak[i].AddressEnd <= AddressEnd) ||
                            (address >= m_memoryBreak[i].AddressStart && AddressEnd <= m_memoryBreak[i].AddressEnd)),
                    "User triggered breakpoint - address overlap [%p,%p] with [%p,%p]", address, AddressEnd, m_memoryBreak[i].AddressStart, m_memoryBreak[i].AddressEnd);

                V_Assert(!(m_memoryBreak[i].AddressStart <= address && address < m_memoryBreak[i].AddressEnd), "User triggered breakpoint - address overlap [%p,%p] with [%p,%p]", address, AddressEnd, m_memoryBreak[i].AddressStart, m_memoryBreak[i].AddressEnd);
                V_Assert(!(m_memoryBreak[i].AddressStart < AddressEnd && AddressEnd <= m_memoryBreak[i].AddressEnd), "User triggered breakpoint - address overlap [%p,%p] with [%p,%p]", address, AddressEnd, m_memoryBreak[i].AddressStart, m_memoryBreak[i].AddressEnd);


                V_Assert(!(m_memoryBreak[i].Alignment == info.Alignment), "User triggered breakpoint - Alignment (%d)", info.Alignment);
                V_Assert(!(m_memoryBreak[i].ByteSize == info.ByteSize), "User triggered breakpoint - allocation size (%d)", info.ByteSize);
                V_Assert(!(info.Name != nullptr && m_memoryBreak[i].Name != nullptr && strcmp(m_memoryBreak[i].Name, info.Name) == 0), "User triggered breakpoint - name \"%s\"", info.Name);
                if (m_memoryBreak[i].LineNum != 0) {
                    V_Assert(!(info.FileName != nullptr && m_memoryBreak[i].FileName != nullptr && strcmp(m_memoryBreak[i].FileName, info.FileName) == 0 && m_memoryBreak[i].LineNum == info.LineNum), "User triggered breakpoint - file/line number : %s(%d)", info.FileName, info.LineNum);
                } else {
                    V_Assert(!(info.FileName != nullptr && m_memoryBreak[i].FileName != nullptr && strcmp(m_memoryBreak[i].FileName, info.FileName) == 0), "User triggered breakpoint - file name \"%s\"", info.FileName);
                }
            }
        }
    }
}