#ifndef V_FRAMEWORK_CORE_IPC_SHARED_MEMORY_COMMON_H
#define V_FRAMEWORK_CORE_IPC_SHARED_MEMORY_COMMON_H

namespace V {
    class SharedMemory_Common {
    public:
        enum AccessMode
        {
            ReadOnly,
            ReadWrite
        };

        enum CreateResult
        {
            CreateFailed,
            CreatedNew,
            CreatedExisting,
        };

    protected:
        SharedMemory_Common() :
            m_mappedBase(nullptr),
            m_dataSize(0)
        {
            m_name[0] = '\0';
        }

        ~SharedMemory_Common()
        {
        }
    protected:
        char         m_name[128];
        void*        m_mappedBase;
        unsigned int m_dataSize;
    };
} // namespace V

#endif // V_FRAMEWORK_CORE_IPC_SHARED_MEMORY_COMMON_H