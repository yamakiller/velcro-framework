#ifndef V_FRAMEWORK_CORE_IO_STREAMER_DRIVE_H
#define V_FRAMEWORK_CORE_IO_STREAMER_DRIVE_H

#include <vcore/io/streamer/statistics.h>
#include <vcore/io/streamer/streamer_configuration.h>
#include <vcore/io/streamer/stream_stack_entry.h>
#include <vcore/io/system_file.h>
#include <vcore/std/containers/deque.h>
#include <vcore/std/containers/vector.h>
#include <vcore/std/chrono/clocks.h>

namespace V
{
    namespace IO
    {
        struct StorageDriveConfig final : public IStreamerStackConfig
        {
            VOBJECT_RTTI(V::IO::StorageDriveConfig, "{65b60937-1e63-4373-b46a-f5616de5b381}", IStreamerStackConfig);
            V_CLASS_ALLOCATOR(StorageDriveConfig, V::SystemAllocator, 0);

            ~StorageDriveConfig() override = default;
            VStd::shared_ptr<StreamStackEntry> AddStreamStackEntry(
                const HardwareInformation& hardware, VStd::shared_ptr<StreamStackEntry> parent) override;

            u32 MaxFileHandles{1024};
        };

        //! Platform agnostic version of a storage drive, such as hdd, ssd, dvd, etc.
        //! This stream stack entry is responsible for accessing a storage drive to 
        //! retrieve file information and data.
        //! This entry is designed as a catch-all for any reads that weren't handled
        //! by platform specific implementations or the virtual file system. It should
        //! by the last entry in the stack as it will not forward calls to the next entry.
        class StorageDrive : public StreamStackEntry
        {
        public:
            explicit StorageDrive(u32 maxFileHandles);
            ~StorageDrive() override = default;

            void SetNext(VStd::shared_ptr<StreamStackEntry> next) override;

            void PrepareRequest(FileRequest* request) override;
            void QueueRequest(FileRequest* request) override;
            bool ExecuteRequests() override;

            void UpdateStatus(Status& status) const override;
            void UpdateCompletionEstimates(VStd::chrono::system_clock::time_point now, VStd::vector<FileRequest*>& internalPending,
                StreamerContext::PreparedQueue::iterator pendingBegin, StreamerContext::PreparedQueue::iterator pendingEnd) override;

            void CollectStatistics(VStd::vector<Statistic>& statistics) const override;

        protected:
            static const VStd::chrono::microseconds _averageSeekTime;
            static constexpr s32 _maxRequests = 1;

            size_t FindFileInCache(const RequestPath& filePath) const;
            void ReadFile(FileRequest* request);
            void CancelRequest(FileRequest* cancelRequest, FileRequestPtr& target);
            void FileExistsRequest(FileRequest* request);
            void FileMetaDataRetrievalRequest(FileRequest* request);
            void FlushCache(const RequestPath& filePath);
            void FlushEntireCache();

            void EstimateCompletionTimeForRequest(FileRequest* request, VStd::chrono::system_clock::time_point& startTime,
                const RequestPath*& activeFile, u64& activeOffset) const;

            void Report(const FileRequest::ReportData& data) const;

            TimedAverageWindow<_statisticsWindowSize> m_fileOpenCloseTimeAverage;
            TimedAverageWindow<_statisticsWindowSize> m_getFileExistsTimeAverage;
            TimedAverageWindow<_statisticsWindowSize> m_getFileMetaDataTimeAverage;
            TimedAverageWindow<_statisticsWindowSize> m_readTimeAverage;
            AverageWindow<u64, float, _statisticsWindowSize> m_readSizeAverage;
            //! File requests that are queued for processing.
            VStd::deque<FileRequest*> m_pendingRequests;

            //! The last time a file handle was used to access a file. The handle is stored in m_fileHandles.
            VStd::vector<VStd::chrono::system_clock::time_point> m_fileLastUsed;
            //! The file path to the file handle. The handle is stored in m_fileHandles.
            VStd::vector<RequestPath> m_filePaths;
            //! A list of file handles that's being cached in case they're needed again in the future.
            VStd::vector<VStd::unique_ptr<SystemFile>> m_fileHandles;

            //! The offset into the file that's cached by the active cache slot.
            u64 m_activeOffset = 0;
            //! The index into m_fileHandles for the file that's currently being read.
            size_t m_activeCacheSlot = _fileNotFound;
        };
    } // namespace io
} // namespace AZ

#endif // V_FRAMEWORK_CORE_IO_STREAMER_DRIVE_H