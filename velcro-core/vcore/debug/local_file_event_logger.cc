#include <vcore/platform.h>
#include <vcore/casting/lossy_cast.h>
#include <vcore/casting/numeric_cast.h>
#include <vcore/debug/local_file_event_logger.h>
#include <vcore/io/path/path.h>
#include <vcore/std/parallel/scoped_lock.h>
#include <vcore/std/parallel/thread.h>
//#include <vcore/Settings/SettingsRegistry.h>

#include <time.h>

namespace V::Debug {
    //
    // EventLogReader
    //

    bool EventLogReader::ReadLog(const char* filePath)
    {
        using namespace V::IO;

        if (SystemFile::Exists(filePath))
        {
            SystemFile::SizeType size = SystemFile::Length(filePath);
            if (size == 0)
            {
                return false;
            }
            m_buffer.resize_no_construct(size);
            uint8_t* buffer = m_buffer.data();

            SystemFile::SizeType readSize = SystemFile::Read(filePath, buffer, size);
            if (readSize == size)
            {
                memcpy(&m_logHeader, buffer, sizeof(m_logHeader));
                m_current = reinterpret_cast<IEventLogger::EventHeader*>(buffer + sizeof(m_logHeader));
                UpdateThreadId();
                return true;
            }
        }
        return false;
    }

        EventNameHash EventLogReader::GetEventName() const
    {
        return m_current->EventId;
    }

    uint16_t EventLogReader::GetEventSize() const
    {
        return m_current->Size;
    }

    uint16_t EventLogReader::GetEventFlags() const
    {
        return m_current->Flags;
    }

    uint64_t EventLogReader::GetThreadId() const
    {
        return m_currentThreadId;
    }

    VStd::string_view EventLogReader::GetString() const
    {
        return VStd::string_view(reinterpret_cast<char*>(m_current + 1), m_current->Size);
    }

    bool EventLogReader::Next()
    {
        size_t increment = V_SIZE_ALIGN_UP(sizeof(IEventLogger::EventHeader) + m_current->Size, EventBoundary);
        uint8_t* bufferPosition = reinterpret_cast<uint8_t*>(m_current) + increment;
        if (bufferPosition < m_buffer.end())
        {
            m_current = reinterpret_cast<IEventLogger::EventHeader*>(bufferPosition);
            UpdateThreadId();
            return true;
        }
        return false;
    }

    void EventLogReader::UpdateThreadId()
    {
        if (GetEventName() == PrologEventHash)
        {
            auto prolog = reinterpret_cast<IEventLogger::Prolog*>(m_current);
            m_currentThreadId = prolog->ThreadId;
        }
    }


    //
    // LocalFileEventLogger
    //

    LocalFileEventLogger::~LocalFileEventLogger()
    {
        if (m_file.IsOpen())
        {
            Stop();
        }

        // thread blocks should have already been flushed above, so
        // this is purely to clear the logger ownership safely
        while (!m_threadDataBlocks.empty())
        {
            m_threadDataBlocks.back()->Reset(nullptr);
        }
    }

    bool LocalFileEventLogger::Start(const V::IO::Path& filePath)
    {
        using namespace V::IO;

        VStd::scoped_lock lock(m_fileGuard);
        if (m_file.Open(filePath.c_str(), SystemFile::SF_OPEN_WRITE_ONLY | SystemFile::SF_OPEN_CREATE | SystemFile::SF_OPEN_CREATE_PATH))
        {
            LogHeader defaultHeader;
            m_file.Write(&defaultHeader, sizeof(LogHeader));
            return true;
        }
        return false;
    }

    bool LocalFileEventLogger::Start(VStd::string_view outputPath, VStd::string_view fileNameHint)
    {
        using namespace V::IO;

        FixedMaxPath filePath{ outputPath };
        FixedMaxPathString fileName{ fileNameHint };

        SystemFile::CreateDir(filePath.c_str());

        bool includeTimestamp = false;
        //V::SettingsRegistryInterface* settingsRegistry = AZ::SettingsRegistry::Get();
        //settingsRegistry->Get(includeTimestamp, RegistryKey_TimestampLogFiles);

        if (includeTimestamp)
        {
            time_t rawtime;
            time(&rawtime);

            tm timeinfo;
            v_localtime(&rawtime, &timeinfo);

            constexpr int timestampSize = 64;
            char timestamp[timestampSize]{ 0 };

            // based the ISO-8601 standard (YYYY-MM-DDTHH-mm-ssTZD) e.g., 20210224_1122
            strftime(timestamp, timestampSize, "%Y%m%d_%H%M", &timeinfo);

            fileName = V::IO::FixedMaxPathString::format("%.*s_%s",
                v_numeric_cast<int>(fileNameHint.size()), fileNameHint.data(),
                timestamp);
        }

        filePath /= fileName;
        filePath.ReplaceExtension("azel");

        return Start(filePath.c_str());
    }

    void LocalFileEventLogger::Stop()
    {
        Flush();
        m_file.Close();
    }

    void LocalFileEventLogger::Flush()
    {
        // Create new storage for a thread to write to. This will replace the storage already on the thread
        // so it can continue to write and is not blocked during a flush. The data that was swapped in can
        // then again be used for the next thread.
        ThreadData* replacementData = new ThreadData();
        bool flushedThread[MaxThreadCount] = {};

        {
            VStd::scoped_lock fileGuardLock(m_fileGuard);
            bool allFlushed;
            do
            {
                allFlushed = true;
                for (size_t i = 0; i < m_threadDataBlocks.size(); ++i)
                {
                    // Don't flush threads that have already been flushed because during high activity
                    // this can cause this loop to always find more threads to flush resulting in
                    // taking a long time to exit the Flush function. As a side effect of this it will
                    // decrease the time between retrying a thread it previous failed to claim which
                    // increases the chance it gets to switch the data.
                    if (flushedThread[i])
                    {
                        continue;
                    }

                    ThreadStorage* thread = m_threadDataBlocks[i];
                    ThreadData* threadData = thread->Data;
                    if (!threadData)
                    {
                        allFlushed = false;
                        continue;
                    }

                    // ensure the thread ID propagates after the exchange
                    replacementData->ThreadId = threadData->ThreadId;
                    if (!thread->Data.compare_exchange_strong(threadData, replacementData))
                    {
                        // Since no other flush can reach this point due to the lock, failing this means
                        // that between looking up the address for the data and the swap the owning thread
                        // has started a write, so bail for now and come back to this one at a later time
                        // to try again.
                        allFlushed = false;
                        continue;
                    }

                    {
                        VStd::scoped_lock fileWriteGuardLock(m_fileWriteGuard);
                        WriteCacheToDisk(*threadData);
                    }
                    replacementData = threadData;
                    flushedThread[i] = true;
                }
            } while (!allFlushed);
            m_file.Flush();
        }

        delete replacementData;
    }

    void* LocalFileEventLogger::RecordEventBegin(EventNameHash id, uint16_t size, uint16_t flags)
    {
        ThreadStorage& threadStorage = GetThreadStorage();
        ThreadData* threadData = threadStorage.Data;

        // Set to nullptr so other threads doing a flush can't pick this up.
        while (!threadStorage.Data.compare_exchange_strong(threadData, nullptr))
        {
        }

        uint32_t writeSize = V_SIZE_ALIGN_UP(sizeof(EventHeader) + size, EventBoundary);
        if (threadData->UsedBytes + writeSize >= ThreadData::BufferSize)
        {
            VStd::scoped_lock lock(m_fileWriteGuard);
            WriteCacheToDisk(*threadData);
        }

        char* eventBuffer = (threadData->Buffer + threadData->UsedBytes);
        EventHeader* header = reinterpret_cast<EventHeader*>(eventBuffer);
        header->EventId = id;
        header->Size = size;
        header->Flags = flags;
        threadData->UsedBytes += writeSize;

        // cache the event data so it doesn't get picked up by calls to flush
        // before it has been committed
        threadStorage.PendingData = threadData;

        return (eventBuffer + sizeof(EventHeader));
    }

    void LocalFileEventLogger::RecordEventEnd()
    {
        // swap the pending data to commit the event
        ThreadStorage& threadStorage = GetThreadStorage();
        ThreadData* expectedData = nullptr;
        while (!threadStorage.Data.compare_exchange_strong(expectedData, threadStorage.PendingData))
        {
        }
        threadStorage.PendingData = nullptr;
    }

    void LocalFileEventLogger::RecordStringEvent(EventNameHash id, VStd::string_view text, uint16_t flags)
    {
        constexpr size_t maxSize = VStd::numeric_limits<decltype(EventHeader::Size)>::max();
        const size_t stringLen = text.length();

        if (stringLen > maxSize)
        {
            V_Assert(false, "Failed to write event!  String too large to store with the event logger.");
            return;
        }

        void* eventText = RecordEventBegin(id, v_numeric_cast<uint16_t>(stringLen), flags);
        memcpy(eventText, text.data(), stringLen);
        RecordEventEnd();
    }

    void LocalFileEventLogger::WriteCacheToDisk(ThreadData& threadData)
    {
        // ensure the front loaded prolog is accurate, ThreadData objects
        // are recycled during flush
        Prolog* prologHeader = reinterpret_cast<Prolog*>(threadData.Buffer);
        prologHeader->EventId = PrologEventHash;
        prologHeader->Size = sizeof(prologHeader->ThreadId);
        prologHeader->Flags = 0; // unused in the prolog
        prologHeader->ThreadId = threadData.ThreadId;

        m_file.Write(threadData.Buffer, threadData.UsedBytes);
        threadData.UsedBytes = sizeof(Prolog); // keep enough room for the next chunk's prolog
    }

    auto LocalFileEventLogger::GetThreadStorage()->ThreadStorage&
    {
        thread_local static ThreadStorage _storage;
        _storage.Reset(this);
        return _storage;
    }

    //
    // LocalFileEventLogger::ThreadStorage
    //

    LocalFileEventLogger::ThreadStorage::~ThreadStorage()
    {
        Reset(nullptr);
    }

    void LocalFileEventLogger::ThreadStorage::Reset(LocalFileEventLogger* owner)
    {
        if (Owner == owner)
        {
            return;
        }

        if (Owner)
        {
            VStd::scoped_lock guard(Owner->m_fileGuard);

            // Save to access thread data because of the lock.
            ThreadData* data = Data;
            if (data->UsedBytes > 0)
            {
                Owner->WriteCacheToDisk(*data);
            }

            auto it = VStd::find(Owner->m_threadDataBlocks.begin(), Owner->m_threadDataBlocks.end(), this);
            if (it != Owner->m_threadDataBlocks.end())
            {
                Owner->m_threadDataBlocks.erase(it);
            }

            delete data;
        }

        Owner = owner;

        if (Owner)
        {
            // Deliberately using system memory instead of regular allocators. If debug allocators
            // are available in the future those should be used instead.
            ThreadData* data = new ThreadData();
            data->ThreadId = vlossy_caster(VStd::hash<VStd::thread_id>{}(VStd::this_thread::get_id()));
            Data = data;

            VStd::scoped_lock guard(Owner->m_fileGuard);
            Owner->m_threadDataBlocks.push_back(this);
        }
    }
} // namespace V::Debug