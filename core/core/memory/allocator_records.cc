#include <core/platform_incl.h>
#include <core/memory/allocator_records.h>
#include <core/memory/allocator_manager.h>


#include <core/std/time.h>
#include <core/std/parallel/mutex.h>


#include <core/debug/stack_tracer.h>


using namespace V;
using namespace V::Debug;

// Many PC tools break with alloc/free size mismatches when the memory guard is enabled.  Disable for now
//#define ENABLE_MEMORY_GUARD


//=========================================================================
// AllocationRecords
//=========================================================================
AllocationRecords::AllocationRecords(unsigned char stackRecordLevels, bool isMemoryGuard, bool isMarkUnallocatedMemory, const char* allocatorName)
    : m_mode(AllocatorManager::Instance().m_defaultTrackingRecordMode)
    , m_isAutoIntegrityCheck(false)
    , m_isMarkUnallocatedMemory(isMarkUnallocatedMemory)
    , m_saveNames(false)
    , m_decodeImmediately(false)
    , m_numStackLevels(stackRecordLevels)
    , m_requestedAllocs(0)
    , m_requestedBytes(0)
    , m_requestedBytesPeak(0)
    , m_allocatorName(allocatorName)
{
#if defined(ENABLE_MEMORY_GUARD)
    m_memoryGuardSize = isMemoryGuard ? sizeof(Debug::GuardValue) : 0;
#else
    (void)isMemoryGuard;
    m_memoryGuardSize = 0;
#endif
#if V_TRAIT_OS_HAS_CRITICAL_SECTION_SPIN_COUNT
    SetCriticalSectionSpinCount(m_recordsMutex.native_handle(), 4000);
#endif
    // preallocate some buckets
};

//=========================================================================
// ~AllocationRecords
//=========================================================================
AllocationRecords::~AllocationRecords() {
    if (!AllocatorManager::Instance().m_isAllocatorLeaking) {
        // dump all allocation (we should not have any at this point).
        bool includeNameAndFilename = (m_saveNames || m_mode == RECORD_FULL);
        EnumerateAllocations(PrintAllocationsCB(true, includeNameAndFilename));
        V_Error("Memory", m_records.empty(), "We still have %d allocations on record! They must be freed prior to destroy!", m_records.size());
    }
}

//=========================================================================
// lock
//=========================================================================
void AllocationRecords::lock() {
    m_recordsMutex.lock();
}

//=========================================================================
// try_lock
//=========================================================================
bool AllocationRecords::try_lock() {
    m_recordsMutex.try_lock();
}

//=========================================================================
// unlock
//=========================================================================
void
AllocationRecords::unlock() {
    m_recordsMutex.unlock();
}

//=========================================================================
// RegisterAllocation
//=========================================================================
const AllocationInfo*
AllocationRecords::RegisterAllocation(void* address, size_t byteSize, size_t alignment, const char* name, const char* fileName, int lineNum, unsigned int stackSuppressCount)
{
    (void)stackSuppressCount;
    if (m_mode == RECORD_NO_RECORDS)
    {
        return nullptr;
    }
    if (address == nullptr)
    {
        return nullptr;
    }

    // memory guard
    if (m_memoryGuardSize == sizeof(Debug::GuardValue))
    {
        if (m_isAutoIntegrityCheck)
        {
            IntegrityCheckNoLock();
        }

        V_Assert(byteSize>sizeof(Debug::GuardValue), "Did you forget to add the extra MemoryGuardSize() bytes?");
        byteSize -= sizeof(Debug::GuardValue);
        new(reinterpret_cast<char*>(address)+byteSize) Debug::GuardValue();
    }

    Debug::AllocationRecordsType::pair_iter_bool iterBool = m_records.insert_key(address);
    
    if (!iterBool.second)
    {
        // If that memory address was already registered, print the stack trace of the previous registration
        PrintAllocationsCB(true, (m_saveNames || m_mode == RECORD_FULL))(address, iterBool.first->second, m_numStackLevels);
        V_Assert(iterBool.second, "Memory address 0x%p is already allocated and in the records!", address);
    }

    Debug::AllocationInfo& ai = iterBool.first->second;
    ai.ByteSize =  byteSize;
    ai.Alignment = static_cast<unsigned int>(alignment);
    if ((m_saveNames || m_mode == RECORD_FULL) && name && fileName)
    {
        // In RECORD_FULL mode or when specifically enabled in app descriptor with 
        // m_allocationRecordsSaveNames, we allocate our own memory to save off name and fileName.
        // When testing for memory leaks, on process shutdown AllocationRecords::~AllocationRecords 
        // gets called to enumerate the remaining (leaked) allocations. Unfortunately, any names
        // referenced in dynamic module memory whose modules are unloaded won't be valid
        // references anymore and we won't get useful information from the enumeration print.
        // This code block ensures we keep our name/fileName valid for when we need it.
        const size_t nameLength = strlen(name);
        const size_t fileNameLength = strlen(fileName);
        const size_t totalLength = nameLength + fileNameLength + 2; // + 2 for terminating null characters
        ai.NamesBlock = m_records.get_allocator().allocate(totalLength, 1);
        ai.NamesBlockSize = totalLength;
        char* savedName = reinterpret_cast<char*>(ai.NamesBlock);
        char* savedFileName = savedName + nameLength + 1;
        memcpy(reinterpret_cast<void*>(savedName), reinterpret_cast<const void*>(name), nameLength + 1);
        memcpy(reinterpret_cast<void*>(savedFileName), reinterpret_cast<const void*>(fileName), fileNameLength + 1);
        ai.Name = savedName;
        ai.FileName = savedFileName;
    }
    else
    {
        ai.Name = name;
        ai.FileName = fileName;
        ai.NamesBlock = nullptr;
        ai.NamesBlockSize = 0;
    }
    ai.LineNum = lineNum;
    ai.TimeStamp = VStd::GetTimeNowMicroSecond();

    // if we don't have a fileName,lineNum record the stack or if the user requested it.
    if ((fileName == nullptr && m_mode == RECORD_STACK_IF_NO_FILE_LINE) || m_mode == RECORD_FULL)
    {
        ai.StackFrames = m_numStackLevels ? reinterpret_cast<V::Debug::StackFrame*>(m_records.get_allocator().allocate(sizeof(V::Debug::StackFrame)*m_numStackLevels, 1)) : nullptr;
        if (ai.StackFrames)
        {
            Debug::StackRecorder::Record(ai.StackFrames, m_numStackLevels, stackSuppressCount + 1);

            if (m_decodeImmediately)
            {
                // OPTIONAL DEBUGGING CODE - enable in app descriptor m_allocationRecordsAttemptDecodeImmediately
                // This is optionally-enabled code for tracking down memory allocations
                // that fail to be decoded. DecodeFrames() typically runs at the end of
                // your application when leaks were found. Sometimes you have stack prints
                // full of "(module-name not available)" and "(function-name not available)"
                // that are not actionable. If you have those, enable this code. It'll slow
                // down your process significantly because for every allocation recorded
                // we get the stack trace on the spot. Put a breakpoint in DecodeFrames()
                // at the "(module-name not available)" and "(function-name not available)"
                // locations and now at the moment those allocations happen you'll have the
                // full stack trace available and the ability to debug what could be causing it
                {
                    const unsigned char decodeStep = 40;
                    Debug::SymbolStorage::StackLine lines[decodeStep];
                    unsigned char iFrame = 0;
                    unsigned char numStackLevels = m_numStackLevels;
                    while (numStackLevels > 0)
                    {
                        unsigned char numToDecode = VStd::GetMin(decodeStep, numStackLevels);
                        Debug::SymbolStorage::DecodeFrames(&ai.StackFrames[iFrame], numToDecode, lines);
                        numStackLevels -= numToDecode;
                        iFrame += numToDecode;
                    }
                }
            }
        }
    }

    AllocatorManager::Instance().DebugBreak(address, ai);

    // statistics
    m_requestedBytes += byteSize;
    m_requestedBytesPeak = VStd::GetMax(m_requestedBytesPeak, m_requestedBytes);
    ++m_requestedAllocs;

    return &ai;
}

//=========================================================================
// UnregisterAllocation
//=========================================================================
void
AllocationRecords::UnregisterAllocation(void* address, size_t byteSize, size_t alignment, AllocationInfo* info)
{
    if (m_mode == RECORD_NO_RECORDS)
    {
        return;
    }
    if (address == nullptr)
    {
        return;
    }

    Debug::AllocationRecordsType::iterator iter = m_records.find(address);

    // We cannot assert if an allocation does not exist because our allocators start up way before the driller is started and the Allocator Records would be created.
    // It is currently impossible to actually track all allocations that happen before a certain point
    //V_Assert(iter!=m_records.end(), "Could not find address 0x%p in the allocator!", address);
    if (iter == m_records.end())
    {
        return;
    }
    AllocatorManager::Instance().DebugBreak(address, iter->second);

    (void)byteSize;
    (void)alignment;
    V_Assert(byteSize==0||byteSize==iter->second.ByteSize, "Mismatched byteSize at deallocation! You supplied an invalid value!");
    V_Assert(alignment==0||alignment==iter->second.Alignment, "Mismatched alignment at deallocation! You supplied an invalid value!");

    // statistics
    m_requestedBytes -= iter->second.ByteSize;
    
#if defined(ENABLE_MEMORY_GUARD)
    // memory guard
    if (m_memoryGuardSize == sizeof(Debug::GuardValue))
    {
        if (m_isAutoIntegrityCheck)
        {
            // full integrity check
            IntegrityCheckNoLock();
        }
        else
        {
            // check current allocation
            char* guardAddress = reinterpret_cast<char*>(address)+iter->second.ByteSize;
            Debug::GuardValue* guard = reinterpret_cast<Debug::GuardValue*>(guardAddress);
            if (!guard->Validate())
            {
                V_Printf("Memory", "Memory stomp located at address %p, part of allocation:", guardAddress);
                PrintAllocationsCB printAlloc(true);
                printAlloc(address, iter->second, m_numStackLevels);
                V_Assert(false, "MEMORY STOMP DETECTED!!!");
            }
            guard->~GuardValue();
        }
    }
#endif

    // delete allocation record
    if (iter->second.NamesBlock) {
        m_records.get_allocator().deallocate(iter->second.NamesBlock, iter->second.NamesBlockSize, 1);
        iter->second.NamesBlock = nullptr;
        iter->second.NamesBlockSize = 0;
        iter->second.Name = nullptr;
        iter->second.FileName = nullptr;
    }
    if (iter->second.StackFrames) {
        m_records.get_allocator().deallocate(iter->second.StackFrames, sizeof(V::Debug::StackFrame)*m_numStackLevels, 1);
        iter->second.StackFrames = nullptr;
    }

    if (info) {
        *info = iter->second;
    }

    m_records.erase(iter);

    // try to be more aggressive and keep the memory footprint low.
    // \todo store the load factor at the last rehash to avoid unnecessary rehash
    if (m_records.load_factor()<0.9f) {
        m_records.rehash(0);
    }

    // if requested set memory to a specific value.
    if (m_isMarkUnallocatedMemory) {
        memset(address, GetUnallocatedMarkValue(), byteSize);
    }
}

//=========================================================================
// ResizeAllocation
//=========================================================================
void
AllocationRecords::ResizeAllocation(void* address, size_t newSize) {
    if (m_mode == RECORD_NO_RECORDS) {
        return;
    }

    Debug::AllocationRecordsType::iterator iter = m_records.find(address);
    V_Assert(iter!=m_records.end(), "Could not find address 0x%p in the allocator!", address);
    AllocatorManager::Instance().DebugBreak(address, iter->second);
    
#if defined(ENABLE_MEMORY_GUARD)
    if (m_memoryGuardSize == sizeof(Debug::GuardValue)) {
        if (m_isAutoIntegrityCheck) {
            // full integrity check
            IntegrityCheckNoLock();
        } else {
            // check memory guard
            char* guardAddress = reinterpret_cast<char*>(address)+iter->second.ByteSize;
            Debug::GuardValue* guard = reinterpret_cast<Debug::GuardValue*>(guardAddress);
            if (!guard->Validate()) {
                V_Printf("Memory", "Memory stomp located at address %p, part of allocation:", guardAddress);
                PrintAllocationsCB printAlloc(true);
                printAlloc(address, iter->second, m_numStackLevels);
                V_Assert(false, "MEMORY STOMP DETECTED!!!");
            }
            guard->~GuardValue();
        }
        // init the new memory guard
        newSize -= sizeof(Debug::GuardValue);
        new(reinterpret_cast<char*>(address)+newSize) Debug::GuardValue();
    }
#endif

    // statistics
    m_requestedBytes -= iter->second.ByteSize;
    m_requestedBytes += newSize;
    m_requestedBytesPeak = VStd::GetMax(m_requestedBytesPeak, m_requestedBytes);
    ++m_requestedAllocs;

    // update allocation size
    iter->second.ByteSize = newSize;
}

//=========================================================================
// EnumerateAllocations
//=========================================================================
void
AllocationRecords::SetMode(Mode mode)
{
    m_recordsMutex.lock();

    if (mode==RECORD_NO_RECORDS)
    {
        m_records.clear();
        m_requestedBytes = 0;
        m_requestedBytesPeak = 0;
        m_requestedAllocs = 0;
    }

    V_Warning("Memory", m_mode!=RECORD_NO_RECORDS||mode==RECORD_NO_RECORDS, "Records recording was disabled and now it's enabled! You might get assert when you free memory, if a you have allocations which were not recorded!");

    m_mode = mode;

   m_recordsMutex.unlock();
}

//=========================================================================
// EnumerateAllocations
//=========================================================================
void
AllocationRecords::EnumerateAllocations(AllocationInfoCBType cb)
{
    m_recordsMutex.lock();
    // enumerate all allocations and stop if requested.
    // Since allocations can change during the iteration (code that prints out the records could allocate, which will
    // mutate m_records), we are going to make a copy and iterate the copy.
    const Debug::AllocationRecordsType recordsCopy = m_records;
    for (Debug::AllocationRecordsType::const_iterator iter = recordsCopy.begin(); iter != recordsCopy.end(); ++iter)
    {
        if (!cb(iter->first, iter->second, m_numStackLevels))
        {
            break;
        }
    }
    m_recordsMutex.unlock();
}

//=========================================================================
// IntegrityCheck
//=========================================================================
void
AllocationRecords::IntegrityCheck() const
{
    if (m_memoryGuardSize == sizeof(Debug::GuardValue))
    {
        m_recordsMutex.lock();

        IntegrityCheckNoLock();

        m_recordsMutex.unlock();
    }
}

//=========================================================================
// IntegrityCheckNoLock
//=========================================================================
void
AllocationRecords::IntegrityCheckNoLock() const {
#if defined(ENABLE_MEMORY_GUARD)
    for (Debug::AllocationRecordsType::const_iterator iter = m_records.begin(); iter != m_records.end(); ++iter) {
        // check memory guard
        const char* guardAddress = reinterpret_cast<const char*>(iter->first)+ iter->second.ByteSize;
        if (!reinterpret_cast<const Debug::GuardValue*>(guardAddress)->Validate()) {
            // We have to turn off the integrity check at this point if we want to succesfully report the memory
            // stomp we just found. If we don't turn this off, the printf just winds off the stack as each memory
            // allocation done therein recurses this same code.
            *const_cast<bool*>(&m_isAutoIntegrityCheck) = false;
            V_Printf("Memory", "Memory stomp located at address %p, part of allocation:", guardAddress);
            PrintAllocationsCB printAlloc(true);
            printAlloc(iter->first, iter->second, m_numStackLevels);
            V_Error("Memory", false, "MEMORY STOMP DETECTED!!!");
        }
    }
#endif
}

//=========================================================================
// operator()
//=========================================================================
bool
PrintAllocationsCB::operator()(void* address, const AllocationInfo& info, unsigned char numStackLevels) {
    if (m_includeNameAndFilename && info.Name) {
        V_Printf("Memory", "Allocation Name: \"%s\" Addr: 0%p Size: %d Alignment: %d\n", info.Name, address, info.ByteSize, info.Alignment);
    } else {
        V_Printf("Memory", "Allocation Addr: 0%p Size: %d Alignment: %d\n", address, info.ByteSize, info.Alignment);
    }

    if (m_isDetailed) {
        if (!info.StackFrames) {
            V_Printf("Memory", " %s (%d)\n", info.FileName, info.LineNum);
        } else {
            // Allocation callstack
            const unsigned char decodeStep = 40;
            Debug::SymbolStorage::StackLine lines[decodeStep];
            unsigned char iFrame = 0;
            while (numStackLevels>0) {
                unsigned char numToDecode = VStd::GetMin(decodeStep, numStackLevels);
                Debug::SymbolStorage::DecodeFrames(&info.StackFrames[iFrame], numToDecode, lines);
                for (unsigned char i = 0; i < numToDecode; ++i) {
                    if (info.StackFrames[iFrame+i].IsValid()) {
                        V_Printf("Memory", " %s\n", lines[i]);
                    }
                }
                numStackLevels -= numToDecode;
                iFrame += numToDecode;
            }
        }
    }
    return true; // continue enumerating
}