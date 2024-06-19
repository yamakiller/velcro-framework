#ifndef V_FRAMEWORK_CORE_PLATFORMS_WINDOWS_IPC_SHARED_MEMORY_WINDOWS_H
#define V_FRAMEWORK_CORE_PLATFORMS_WINDOWS_IPC_SHARED_MEMORY_WINDOWS_H

#include <vcore/ipc/shared_memory_common.h>
#include <vcore/platform_incl.h>

namespace V {
    class SharedMemory_Windows : public SharedMemory_Common
    {
    protected:
        SharedMemory_Windows();

        bool IsReady() const
        {
            return m_mapHandle != NULL;
        }

        bool IsMapHandleValid() const
        {
            return m_mapHandle != nullptr;
        }

        CreateResult Create(const char* name, unsigned int size, bool openIfCreated);
        bool Open(const char* name);
        void Close();
        bool Map(AccessMode mode, unsigned int size);
        bool UnMap();
        void lock();
        bool try_lock();
        void unlock();
        bool IsLockAbandoned();
        bool IsWaitFailed() const;

        HANDLE m_mapHandle;
        HANDLE m_globalMutex;
        int m_lastLockResult;
    };

    using SharedMemory_Platform = SharedMemory_Windows;
}

#endif //V_FRAMEWORK_CORE_PLATFORMS_WINDOWS_IPC_SHARED_MEMORY_WINDOWS_H