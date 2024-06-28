#ifndef V_FRAMEWORK_CORE_DEBUG_LOCAL_FILE_EVENT_LOGGER_H
#define V_FRAMEWORK_CORE_DEBUG_LOCAL_FILE_EVENT_LOGGER_H

#include <limits>
#include <vcore/debug/ievent_logger.h>
#include <vcore/interface/interface.h>
#include <vcore/io/path/path.h>
#include <vcore/io/system_file.h>
#include <vcore/std/containers/fixed_vector.h>
#include <vcore/std/containers/vector.h>
#include <vcore/std/parallel/atomic.h>
#include <vcore/std/parallel/mutex.h>
#include <vcore/std/string/string_view.h>

namespace V::Debug
{
    class EventLogReader
    {
    public:
        bool ReadLog(const char* filePath);

        EventNameHash GetEventName() const;
        uint16_t GetEventSize() const;
        uint16_t GetEventFlags() const;
        uint64_t GetThreadId() const;

        VStd::string_view GetString() const;
        template<typename T>
        const T* GetValue() const
        {
            V_Assert(sizeof(T) <= m_current->Size, "Attempting to retrieve a value that's larger than the amount of stored data.");
            return reinterpret_cast<T*>(m_current + 1);
        }

        bool Next();

    private:
        void UpdateThreadId();

        VStd::vector<uint8_t> m_buffer;
        IEventLogger::LogHeader m_logHeader;

        uint64_t m_currentThreadId{ 0 };
        IEventLogger::EventHeader* m_current{ nullptr };
    };


    class LocalFileEventLogger
        : public Interface<IEventLogger>::Registrar
    {
    public:
        inline static constexpr size_t MaxThreadCount = 512;

        ~LocalFileEventLogger() override;

        bool Start(const V::IO::Path& filePath);
        bool Start(VStd::string_view outputPath, VStd::string_view fileNameHint);
        void Stop();

        void Flush() override;

        void* RecordEventBegin(EventNameHash id, uint16_t size, uint16_t flags = 0) override;
        void  RecordEventEnd() override;

        void  RecordStringEvent(EventNameHash id, VStd::string_view text, uint16_t flags = 0) override;

    protected:
        struct ThreadData
        {
            // ensure there is enough room for one large event with header + prolog
            static constexpr size_t BufferSize = VStd::numeric_limits<decltype(EventHeader::Size)>::max() + sizeof(EventHeader) + sizeof(Prolog);
            char Buffer[BufferSize]{ 0 };
            uint64_t ThreadId{ 0 };
            uint32_t UsedBytes{ sizeof(Prolog) }; // always front load the buffer with a prolog
        };

        struct ThreadStorage
        {
            ThreadStorage() = default;
            ~ThreadStorage();

            void Reset(LocalFileEventLogger* owner);

            VStd::atomic<ThreadData*> Data{ nullptr };
            ThreadData* PendingData{ nullptr };
            LocalFileEventLogger* Owner{ nullptr };
        };

        void WriteCacheToDisk(ThreadData& threadData);

        ThreadStorage& GetThreadStorage();

        VStd::fixed_vector<ThreadStorage*, MaxThreadCount> m_threadDataBlocks;

        V::IO::SystemFile m_file;
        VStd::recursive_mutex m_fileGuard;
        VStd::recursive_mutex m_fileWriteGuard;
    };
} // namespace IO::Debug



#endif // V_FRAMEWORK_CORE_DEBUG_LOCAL_FILE_EVENT_LOGGER_H