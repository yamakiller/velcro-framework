#include <vcore/platform_incl.h>
#include <vcore/ipc/shared_memory.h>

#include <vcore/std/algorithm.h>

#include <vcore/std/parallel/spin_mutex.h>


namespace V {
    namespace Internal {
        struct RingData {
            V::u32 ReadOffset;
            V::u32 WriteOffset;
            V::u32 StartOffset;
            V::u32 EndOffset;
            V::u32 DataToRead;
            V::u8  Pad[32 - sizeof(VStd::spin_mutex)];
        };
    } // namespace Internal
} // namespace V

using namespace V;

#include <stdio.h>

//=========================================================================
// SharedMemory
//=========================================================================
SharedMemory::SharedMemory()
    : m_data(nullptr) {
}

//=========================================================================
// ~SharedMemory
//=========================================================================
SharedMemory::~SharedMemory() {
    UnMap();
    Close();
}

//=========================================================================
// Create
//=========================================================================
SharedMemory_Common::CreateResult
SharedMemory::Create(const char* name, unsigned int size, bool openIfCreated) {
    V_Assert(name && strlen(name) > 1, "Invalid name!");
    V_Assert(size > 0, "Invalid buffer size!");
    if (IsReady()) {
        return CreateFailed;
    }

    CreateResult result = Platform::Create(name, size, openIfCreated);

    if (result != CreatedExisting) {
        MemoryGuard l(*this);
        if (Map()){
            Clear();
            UnMap();
        } else {
            return CreateFailed;
        }
        return CreatedNew;
    }

    return CreatedExisting;
}

//=========================================================================
// Open
//=========================================================================
bool
SharedMemory::Open(const char* name) {
    V_Assert(name && strlen(name) > 1, "Invalid name!");

    if (Platform::IsMapHandleValid() || m_globalMutex) {
        return false;
    }

    return Platform::Open(name);
}

//=========================================================================
// Close
//=========================================================================
void
SharedMemory::Close() {
    UnMap();
    Platform::Close();
}

//=========================================================================
// Map
//=========================================================================
bool
SharedMemory::Map(AccessMode mode, unsigned int size) {
    V_Assert(m_mappedBase == nullptr, "We already have data mapped");
    V_Assert(Platform::IsMapHandleValid(), "You must call Map() first!");

    bool result = Platform::Map(mode, size);

    if (result) {
        m_data = reinterpret_cast<char*>(m_mappedBase);
    }

    return result;
}

//=========================================================================
// UnMap
//=========================================================================
bool
SharedMemory::UnMap()
{
    if (m_mappedBase == nullptr) return false;

    if (!Platform::UnMap()) {
        V_TracePrintf("VSystem", "UnmapViewOfFile failed with error %d\n", GetLastError());
        return false;
    }
    m_mappedBase = nullptr;
    m_data = nullptr;
    m_dataSize = 0;
    return true;
}

//=========================================================================
// UnMap
//=========================================================================
void SharedMemory::lock() {
    V_Assert(m_globalMutex, "You need to create/open the global mutex first! Call Create or Open!");
    Platform::lock();
}

//=========================================================================
// UnMap
//=========================================================================
bool SharedMemory::try_lock() {
    V_Assert(m_globalMutex, "You need to create/open the global mutex first! Call Create or Open!");
    return Platform::try_lock();
}

//=========================================================================
// UnMap
//=========================================================================
void SharedMemory::unlock() {
    V_Assert(m_globalMutex, "You need to create/open the global mutex first! Call Create or Open!");
    Platform::unlock();
}

bool SharedMemory::IsLockAbandoned() { 
    return Platform::IsLockAbandoned();
}

//=========================================================================
// Clear
//=========================================================================
void  SharedMemory::Clear() {
    if (m_mappedBase != nullptr) {
        V_Warning("VSystem", !Platform::IsWaitFailed(), "You are clearing the shared memory %s while the Global lock is NOT locked! This can lead to data corruption!", m_name);
        memset(m_data, 0, m_dataSize);
    }
}

bool SharedMemory::CheckMappedBaseValid()
{
    // Check that m_mappedBase is valid, and clean up if it isn't
    if (m_mappedBase == nullptr)
    {
        V_TracePrintf("VSystem", "MapViewOfFile failed with error %d\n", GetLastError());
        Close();
        return false;
    }

    return true;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
// Shared Memory ring buffer
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

//=========================================================================
// SharedMemoryRingBuffer
//=========================================================================
SharedMemoryRingBuffer::SharedMemoryRingBuffer()
    : m_info(nullptr)
{}

//=========================================================================
// Create
//=========================================================================
bool
SharedMemoryRingBuffer::Create(const char* name, unsigned int size, bool openIfCreated) {
    return SharedMemory::Create(name, size + sizeof(Internal::RingData), openIfCreated) != SharedMemory::CreateFailed;
}

//=========================================================================
// Map
//=========================================================================
bool
SharedMemoryRingBuffer::Map(AccessMode mode, unsigned int size) {
    if (SharedMemory::Map(mode, size)) {
        MemoryGuard l(*this);
        m_info = reinterpret_cast<Internal::RingData*>(m_data);
        m_data = m_info + 1;
        m_dataSize -= sizeof(Internal::RingData);
        if (m_info->EndOffset == 0)  // if endOffset == 0 we have never set the info structure, do this only once.
        {
            m_info->StartOffset = 0;
            m_info->ReadOffset = m_info->StartOffset;
            m_info->WriteOffset = m_info->StartOffset;
            m_info->EndOffset = m_info->StartOffset + m_dataSize;
            m_info->DataToRead = 0;
        }
        return true;
    }

    return false;
}

//=========================================================================
// UnMap
//=========================================================================
bool
SharedMemoryRingBuffer::UnMap()
{
    m_info = nullptr;
    return SharedMemory::UnMap();
}

//=========================================================================
// Write
//=========================================================================
bool
SharedMemoryRingBuffer::Write(const void* data, unsigned int dataSize) {
    V_Warning("VSystem", !Platform::IsWaitFailed(), "You are writing the ring buffer %s while the Global lock is NOT locked! This can lead to data corruption!", m_name);
    V_Assert(m_info != nullptr, "You need to Create and Map the buffer first!");
    if (m_info->WriteOffset >= m_info->ReadOffset)
    {
        unsigned int freeSpace = m_dataSize - (m_info->WriteOffset - m_info->ReadOffset);
        // if we are full or don't have enough space return false
        if (m_info->DataToRead == m_dataSize || freeSpace < dataSize) {
            return false;
        }
        unsigned int copy1MaxSize = m_info->EndOffset - m_info->WriteOffset;
        unsigned int dataToCopy1 = VStd::GetMin(copy1MaxSize, dataSize);
        if (dataToCopy1) {
            memcpy(reinterpret_cast<char*>(m_data) + m_info->WriteOffset, data, dataToCopy1);
        }
        unsigned int dataToCopy2 = dataSize - dataToCopy1;
        if (dataToCopy2) {
            memcpy(reinterpret_cast<char*>(m_data) + m_info->StartOffset, reinterpret_cast<const char*>(data) + dataToCopy1, dataToCopy2);
            m_info->WriteOffset = m_info->StartOffset + dataToCopy2;
        } else {
            m_info->WriteOffset += dataToCopy1;
        }
    } else {
        unsigned int freeSpace = m_info->ReadOffset - m_info->WriteOffset;
        if (freeSpace < dataSize)
        {
            return false;
        }
        memcpy(reinterpret_cast<char*>(m_data) + m_info->WriteOffset, data, dataSize);
        m_info->WriteOffset += dataSize;
    }
    m_info->DataToRead += dataSize;

    return true;
}

//=========================================================================
// Read
//=========================================================================
unsigned int
SharedMemoryRingBuffer::Read(void* data, unsigned int maxDataSize) {

    V_Warning("VSystem", !Platform::IsWaitFailed(), "You are reading the ring buffer %s while the Global lock is NOT locked! This can lead to data corruption!", m_name);

    if (m_info->DataToRead == 0) {
        return 0;
    }

    V_Assert(m_info != nullptr, "You need to Create and Map the buffer first!");
    unsigned int dataRead;
    if (m_info->WriteOffset > m_info->ReadOffset) {
        unsigned int dataToRead = VStd::GetMin(m_info->WriteOffset - m_info->ReadOffset, maxDataSize);
        if (dataToRead) {
            memcpy(data, reinterpret_cast<char*>(m_data) + m_info->ReadOffset, dataToRead);
        }
        m_info->ReadOffset += dataToRead;
        dataRead = dataToRead;
    } else {
        unsigned int dataToRead1 = VStd::GetMin(m_info->EndOffset - m_info->ReadOffset, maxDataSize);
        if (dataToRead1) {
            maxDataSize -= dataToRead1;
            memcpy(data, reinterpret_cast<char*>(m_data) + m_info->ReadOffset, dataToRead1);
        }
        unsigned int dataToRead2 = VStd::GetMin(m_info->WriteOffset - m_info->StartOffset, maxDataSize);
        if (dataToRead2) {
            memcpy(reinterpret_cast<char*>(data) + dataToRead1, reinterpret_cast<char*>(m_data) + m_info->StartOffset, dataToRead2);
            m_info->ReadOffset = m_info->StartOffset + dataToRead2;
        } else {
            m_info->ReadOffset += dataToRead1;
        }
        dataRead = dataToRead1 + dataToRead2;
    }
    m_info->DataToRead -= dataRead;

    return dataRead;
}

//=========================================================================
// Read
//=========================================================================
unsigned int
SharedMemoryRingBuffer::DataToRead() const {
    return m_info ? m_info->DataToRead : 0;
}

//=========================================================================
// Read
//=========================================================================
unsigned int
SharedMemoryRingBuffer::MaxToWrite() const {
    return m_info ? (m_dataSize - m_info->DataToRead) : 0;
}

//=========================================================================
// Clear
//=========================================================================
void
SharedMemoryRingBuffer::Clear() {
    SharedMemory::Clear();
    if (m_info) {
        m_info->ReadOffset = m_info->StartOffset;
        m_info->WriteOffset = m_info->StartOffset;
        m_info->DataToRead = 0;
    }
}