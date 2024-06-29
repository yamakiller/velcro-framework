#ifndef V_FRAMEWORK_CORE_PLATFORMS_CORE_IO_STREAMER_STORAGE_DRIVE_WINDOWS_H
#define V_FRAMEWORK_CORE_PLATFORMS_CORE_IO_STREAMER_STORAGE_DRIVE_WINDOWS_H

#include <vcore/platform_incl.h>
#include <vcore/io/streamer/statistics.h>
#include <vcore/io/streamer/streamer_configuration.h>
#include <vcore/io/streamer/stream_stack_entry.h>
#include <vcore/std/containers/deque.h>
#include <vcore/std/containers/vector.h>
#include <vcore/std/chrono/clocks.h>
#include <vcore/std/string/string.h>
#include <vcore/std/string/string_view.h>
#include <vcore/statistics/running_statistic.h>

namespace V::IO 
{
    class StorageDriveWin : public StreamStackEntry
    {
    public:
        struct ConstructionOptions
        {
            ConstructionOptions();

            //! Whether or not the device has a cost for seeking, such as happens on platter disks. This
            //! will be accounted for when predicting file reads.
            u8 HasSeekPenalty : 1;
            //! Use unbuffered reads for the fastest possible read speeds by bypassing the Windows
            //! file cache. This results in a faster read the first time a file is read, but subsequent reads will possibly be
            //! slower as those could have been serviced from the faster OS cache. During development or for games that reread
            //! files frequently it's recommended to set this option to false, but generally it's best to be turned on.
            //! Unbuffered reads have alignment restrictions. Many of the other stream stack entry are (optionally) aware and
            //! make adjustments. For the most optimal performance align read buffers to the physicalSectorSize.
            u8 EnableUnbufferedReads : 1;
            //! Globally enable file sharing. This allows files to used outside V::IO::Streamer, including other applications
            //! while in use by V::IO::Streamer. 
            u8 EnableSharing : 1;
            //! If true, only information that's explicitly requested or issues are reported. If false, status information
            //! such as when drives are created and destroyed is reported as well.
            u8 MinimalReporting : 1;
        };

        //! Creates an instance of a storage device that's optimized for use on Windows.
        //! @param drivePaths The paths to the drives that are supported by this device. A single device can have multiple
        //!     logical disks.
        //! @param maxFileHandles The maximum number of file handles that are cached. Only a small number are needed when
        //!     running from archives, but it's recommended that a larger number are kept open when reading from loose files.
        //! @param maxMetaDataCacheEntires The maximum number of files to keep meta data, such as the file size, to cache. Only
        //!     a small number are needed when running from archives, but it's recommended that a larger number are kept open
        //!     when reading from loose files.
        //! @param physicalSectorSize The minimal sector size as instructed by the device. When unbuffered reads are used the output
        //!     buffer needs to be aligned to this value.
        //! @param logicalSectorSize The minimal sector size as instructed by the device. When unbuffered reads are used the
        //!     file size and read offset need to be aligned to this value.
        //! @param ioChannelCount The maximum number of requests that the IO controller driving the device supports. This value
        //!     will be capped by the maximum number of parallel requests that can be issued per thread.
        //! @param overCommit The number of additional slots that will be reported as available. This makes sure that there are
        //!     always a few requests pending to avoid starvation. An over-commit that is too large can negatively impact the
        //!     scheduler's ability to re-order requests for optimal read order. A negative value will under-commit and will
        //!     avoid saturating the IO controller which can be needed if the drive is used by other applications.
        //! @param options Additional configuration options. See ConstructionOptions for more details.
        StorageDriveWin(const VStd::vector<VStd::string_view>& drivePaths, u32 maxFileHandles, u32 maxMetaDataCacheEntries,
            size_t physicalSectorSize, size_t logicalSectorSize, u32 ioChannelCount, s32 overCommit, ConstructionOptions options);
        ~StorageDriveWin() override;

        void PrepareRequest(FileRequest* request) override;
        void QueueRequest(FileRequest* request) override;
        bool ExecuteRequests() override;

        void UpdateStatus(Status& status) const override;
        void UpdateCompletionEstimates(VStd::chrono::system_clock::time_point now, VStd::vector<FileRequest*>& internalPending,
            StreamerContext::PreparedQueue::iterator pendingBegin, StreamerContext::PreparedQueue::iterator pendingEnd) override;

        void CollectStatistics(VStd::vector<Statistic>& statistics) const override;

    protected:
        static const VStd::chrono::microseconds _averageSeekTime;

        inline static constexpr size_t InvalidFileCacheIndex = std::numeric_limits<size_t>::max();
        inline static constexpr size_t InvalidReadSlotIndex = std::numeric_limits<size_t>::max();
        inline static constexpr size_t InvalidMetaDataCacheIndex = std::numeric_limits<size_t>::max();

        struct FileReadStatus
        {
            OVERLAPPED Overlapped{};
            size_t FileHandleIndex{ InvalidFileCacheIndex };
        };

        struct FileReadInformation
        {
            VStd::chrono::system_clock::time_point StartTime;
            FileRequest* Request{ nullptr };
            void*   SectorAlignedOutput{ nullptr };    // Internally allocated buffer that is sector aligned.
            size_t  CopyBackOffset{ 0 };
            
            void AllocateAlignedBuffer(size_t size, size_t sectorSize);
            void Clear();
        };

        enum class OpenFileResult
        {
            FileOpened,
            RequestForwarded,
            CacheFull
        };

        OpenFileResult OpenFile(HANDLE& fileHandle, size_t& cacheSlot, FileRequest* request, const FileRequest::ReadData& data);
        bool ReadRequest(FileRequest* request);
        bool ReadRequest(FileRequest* request, size_t readSlot);
        bool CancelRequest(FileRequest* cancelRequest, FileRequestPtr& target);
        void FileExistsRequest(FileRequest* request);
        void FileMetaDataRetrievalRequest(FileRequest* request);
        size_t FindInFileHandleCache(const RequestPath& filePath) const;
        size_t FindAvailableFileHandleCacheIndex() const;
        size_t FindAvailableReadSlot();
        size_t FindInMetaDataCache(const RequestPath& filePath) const;
        size_t GetNextMetaDataCacheSlot();
        bool IsServicedByThisDrive(const char* filePath) const;

        void EstimateCompletionTimeForRequest(FileRequest* request, VStd::chrono::system_clock::time_point& startTime,
            const RequestPath*& activeFile, u64& activeOffset) const;
        void EstimateCompletionTimeForRequestChecked(FileRequest* request,
            VStd::chrono::system_clock::time_point startTime, const RequestPath*& activeFile, u64& activeOffset) const;
        s32 CalculateNumAvailableSlots() const;

        void FlushCache(const RequestPath& filePath);
        void FlushEntireCache();

        bool FinalizeReads();
        void FinalizeSingleRequest(FileReadStatus& status, size_t readSlot, DWORD numBytesTransferred,
            bool isCanceled, bool encounteredError);

        void Report(const FileRequest::ReportData& data) const;

        TimedAverageWindow<_statisticsWindowSize> m_fileOpenCloseTimeAverage;
        TimedAverageWindow<_statisticsWindowSize> m_getFileExistsTimeAverage;
        TimedAverageWindow<_statisticsWindowSize> m_getFileMetaDataRetrievalTimeAverage;
        TimedAverageWindow<_statisticsWindowSize> m_readTimeAverage;
        AverageWindow<u64, float, _statisticsWindowSize> m_readSizeAverage;
#if V_STREAMER_ADD_EXTRA_PROFILING_INFO
        V::Statistics::RunningStatistic m_fileSwitchPercentageStat;
        V::Statistics::RunningStatistic m_seekPercentageStat;
        V::Statistics::RunningStatistic m_directReadsPercentageStat;
#endif
        VStd::chrono::system_clock::time_point m_activeReads_startTime;

        VStd::deque<FileRequest*> m_pendingReadRequests;
        VStd::deque<FileRequest*> m_pendingRequests;

        VStd::vector<FileReadInformation> m_readSlots_readInfo;
        VStd::vector<FileReadStatus> m_readSlots_statusInfo;
        VStd::vector<bool> m_readSlots_active;

        VStd::vector<VStd::chrono::system_clock::time_point> m_fileCache_lastTimeUsed;
        VStd::vector<RequestPath> m_fileCache_paths;
        VStd::vector<HANDLE> m_fileCache_handles;
        VStd::vector<u16> m_fileCache_activeReads;

        VStd::vector<RequestPath> m_metaDataCache_paths;
        VStd::vector<u64> m_metaDataCache_fileSize;

        VStd::vector<VStd::string> m_drivePaths;

        size_t m_activeReads_ByteCount{ 0 };

        size_t m_physicalSectorSize{ 0 };
        size_t m_logicalSectorSize{ 0 };
        size_t m_activeCacheSlot{ InvalidFileCacheIndex };
        size_t m_metaDataCache_front{ 0 };
        u64 m_activeOffset{ 0 };
        u32 m_maxFileHandles{ 1 };
        u32 m_ioChannelCount{ 1 };
        s32 m_overCommit{ 0 };

        u16 m_activeReads_Count{ 0 };

        ConstructionOptions m_constructionOptions;
        bool m_cachesInitialized{ false };
    };
} // namespace V::IO 


#endif // V_FRAMEWORK_CORE_PLATFORMS_CORE_IO_STREAMER_STORAGE_DRIVE_WINDOWS_H