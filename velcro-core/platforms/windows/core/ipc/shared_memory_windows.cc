#include <vcore/ipc/shared_memory_windows.h>

#include <vcore/ipc/shared_memory.h>
#include <vcore/std/parallel/spin_mutex.h>
#include <vcore/std/string/conversions.h>
#include <vcore/std/string/fixed_string.h>

namespace V {
    SharedMemory_Windows::SharedMemory_Windows() :
        m_mapHandle(nullptr),
        m_globalMutex(nullptr),
        m_lastLockResult(WAIT_FAILED)
    {
    }

    void ComposeName(VStd::fixed_wstring<256>& dest, const char* name, const wchar_t* suffix)
    {
        VStd::to_wstring(dest, name);
        dest += L"_";
        dest += suffix;
    }

    SharedMemory_Common::CreateResult SharedMemory_Windows::Create(const char* name, unsigned int size, bool openIfCreated)
    {
        vstrncpy(m_name, V_ARRAY_SIZE(m_name), name, strlen(name));
        VStd::fixed_wstring<256> fullName;
        ComposeName(fullName, name, L"Mutex");

        // Security attributes
        SECURITY_ATTRIBUTES secAttr;
        char secDesc[SECURITY_DESCRIPTOR_MIN_LENGTH];
        secAttr.nLength = sizeof(secAttr);
        secAttr.bInheritHandle = TRUE;
        secAttr.lpSecurityDescriptor = &secDesc;
        InitializeSecurityDescriptor(secAttr.lpSecurityDescriptor, SECURITY_DESCRIPTOR_REVISION);
        SetSecurityDescriptorDacl(secAttr.lpSecurityDescriptor, TRUE, 0, FALSE);

        // Obtain global mutex
        m_globalMutex = CreateMutexW(&secAttr, FALSE, fullName.c_str());
        DWORD error = GetLastError();
        if (m_globalMutex == NULL || (error == ERROR_ALREADY_EXISTS && openIfCreated == false))
        {
            V_TracePrintf("VSystem", "CreateMutex failed with error %d\n", error);
            return CreateFailed;
        }

        // Create the file mapping.
        ComposeName(fullName, name, L"Data");
        m_mapHandle = CreateFileMappingW(INVALID_HANDLE_VALUE, &secAttr, PAGE_READWRITE, 0, size, fullName.c_str());
        error = GetLastError();
        if (m_mapHandle == NULL || (error == ERROR_ALREADY_EXISTS && openIfCreated == false))
        {
            V_TracePrintf("VSystem", "CreateFileMapping failed with error %d\n", error);
            return CreateFailed;
        }

        return (error == ERROR_ALREADY_EXISTS) ? CreatedExisting : CreatedNew;
    }

    bool SharedMemory_Windows::Open(const char* name)
    {
        vstrncpy(m_name, V_ARRAY_SIZE(m_name), name, strlen(name));
        VStd::fixed_wstring<256> fullName;
        ComposeName(fullName, name, L"Mutex");

        m_globalMutex = OpenMutex(SYNCHRONIZE, TRUE, fullName.c_str());
        V_Warning("VSystem", m_globalMutex != NULL, "Failed to open OS mutex [%s]\n", m_name);
        if (m_globalMutex == NULL)
        {
            V_TracePrintf("VSystem", "OpenMutex %s failed with error %d\n", m_name, GetLastError());
            return false;
        }

        ComposeName(fullName, name, L"Data");
        m_mapHandle = OpenFileMapping(FILE_MAP_WRITE, false, fullName.c_str());
        if (m_mapHandle == NULL)
        {
            V_TracePrintf("VSystem", "OpenFileMapping %s failed with error %d\n", m_name, GetLastError());
            return false;
        }

        return true;
    }

    void SharedMemory_Windows::Close()
    {
        if (m_mapHandle != NULL && !CloseHandle(m_mapHandle))
        {
            V_TracePrintf("VSystem", "CloseHandle failed with error %d\n", GetLastError());
        }

        if (m_globalMutex != NULL && !CloseHandle(m_globalMutex))
        {
            V_TracePrintf("VSystem", "CloseHandle failed with error %d\n", GetLastError());
        }

        m_mapHandle = NULL;
        m_globalMutex = NULL;
    }

    bool SharedMemory_Windows::Map(AccessMode mode, unsigned int size)
    {
        DWORD dwDesiredAccess = (mode == ReadOnly ? FILE_MAP_READ : FILE_MAP_WRITE);
        m_mappedBase = MapViewOfFile(m_mapHandle, dwDesiredAccess, 0, 0, size);

        if (!static_cast<SharedMemory*>(this)->CheckMappedBaseValid())
        {
            return false;
        }

        // Grab the size of the memory we have been given (a multiple of 4K on windows)
        MEMORY_BASIC_INFORMATION info;
        if (!VirtualQuery(m_mappedBase, &info, sizeof(info)))
        {
            V_TracePrintf("VSystem", "VirtualQuery failed\n");
            return false;
        }
        m_dataSize = static_cast<unsigned int>(info.RegionSize);

        return true;
    }

    bool SharedMemory_Windows::UnMap()
    {
        return UnmapViewOfFile(m_mappedBase) != FALSE;
    }

    void SharedMemory_Windows::lock()
    {
        DWORD lockResult = 0;
        do
        {
            lockResult = WaitForSingleObject(m_globalMutex, 5);
            if (lockResult == WAIT_OBJECT_0 || lockResult == WAIT_ABANDONED)
            {
                break;
            }

            // If the wait failed, we need to re-acquire the mutex, because most likely
            // something bad has happened to it (we have experienced what looks to be a
            // reference counting issue where the mutex is killed for a process, and an
            // INFINITE wait will indeed wait...infinitely on that mutex, while other
            // processes are able to acquire it just fine)
            if (lockResult == WAIT_FAILED)
            {
                DWORD lastError = ::GetLastError();
                (void)lastError;
                V_Warning("VSystem", lockResult != WAIT_FAILED, "WaitForSingleObject failed with code %u", lastError);
                SharedMemory* self = static_cast<SharedMemory*>(this);
                self->Close();
                self->Open(m_name);
            }
            else if (lockResult != WAIT_TIMEOUT)
            {
                // According to the docs: https://msdn.microsoft.com/en-us/library/windows/desktop/ms687032(v=vs.85).aspx
                // WaitForSingleObject can only return WAIT_OBJECT_0, WAIT_ABANDONED, WAIT_FAILED, and WAIT_TIMEOUT
                V_Error("VSystem", false, "WaitForSingleObject returned an undocumented error code: %u, GetLastError: %u", lockResult, ::GetLastError());
            }
        } while (lockResult != WAIT_OBJECT_0 && lockResult != WAIT_ABANDONED);

        m_lastLockResult = lockResult;

        V_Warning("VSystem", m_lastLockResult != WAIT_ABANDONED, "We locked an abandoned Mutex, the shared memory data may be in instable state (corrupted)!");
    }

    bool SharedMemory_Windows::try_lock()
    {
        m_lastLockResult = WaitForSingleObject(m_globalMutex, 0);
        V_Warning("VSystem", m_lastLockResult != WAIT_ABANDONED, "We locked an abandoned Mutex, the shared memory data may be in instable state (corrupted)!");
        return (m_lastLockResult == WAIT_OBJECT_0) || (m_lastLockResult == WAIT_ABANDONED);
    }

    void SharedMemory_Windows::unlock()
    {
        ReleaseMutex(m_globalMutex);
        m_lastLockResult = WAIT_FAILED;
    }

    bool SharedMemory_Windows::IsLockAbandoned()
    {
        return (m_lastLockResult == WAIT_ABANDONED);
    }

    bool SharedMemory_Windows::IsWaitFailed() const
    {
        return m_lastLockResult == WAIT_FAILED;
    }

}