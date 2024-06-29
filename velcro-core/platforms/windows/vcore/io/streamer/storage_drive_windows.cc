#include <climits>

#include <vcore/casting/numeric_cast.h>
#include <vcore/debug/profiler.h>
#include <vcore/io/streamer/file_request.h>
#include <vcore/io/streamer/streamer_context.h>
#include <vcore/io/streamer/storage_drive_windows.h>
#include <vcore/std/typetraits/decay.h>
#include <vcore/std/typetraits/is_unsigned.h>
#include <vcore/string_func/string_func.h>
#include <vcore/std/string/conversions.h>


namespace V::IO
{
#if V_STREAMER_ADD_EXTRA_PROFILING_INFO
    static constexpr char FileSwitchesName[] = "File switches";
    static constexpr char SeeksName[] = "Seeks";
    static constexpr char DirectReadsName[] = "Direct reads (no internal alloc)";
#endif // V_STREAMER_ADD_EXTRA_PROFILING_INFO

    const VStd::chrono::microseconds StorageDriveWin::_averageSeekTime =
        VStd::chrono::milliseconds(9) + // Common average seek time for desktop hdd drives.
        VStd::chrono::milliseconds(3); // Rotational latency for a 7200RPM disk

    //
    // ConstructionOptions
    //

    StorageDriveWin::ConstructionOptions::ConstructionOptions()
        : HasSeekPenalty(true)
        , EnableUnbufferedReads(true)
        , EnableSharing(false)
        , MinimalReporting(false)
    {}

    //
    // FileReadInformation
    //

    void StorageDriveWin::FileReadInformation::AllocateAlignedBuffer(size_t size, size_t sectorSize)
    {
        V_Assert(SectorAlignedOutput == nullptr, "Assign a sector aligned buffer when one is already assigned.");
        SectorAlignedOutput =  vmalloc(size, sectorSize, V::SystemAllocator);
    }

    void StorageDriveWin::FileReadInformation::Clear()
    {
        if (SectorAlignedOutput)
        {
            vfree(SectorAlignedOutput, V::SystemAllocator);
        }
        *this = FileReadInformation{};
    }

    //
    // StorageDriveWin
    //
    StorageDriveWin::StorageDriveWin(const VStd::vector<VStd::string_view>& drivePaths, u32 maxFileHandles, u32 maxMetaDataCacheEntries,
        size_t physicalSectorSize, size_t logicalSectorSize, u32 ioChannelCount, s32 overCommit, ConstructionOptions options)
        : m_maxFileHandles(maxFileHandles)
        , m_physicalSectorSize(physicalSectorSize)
        , m_logicalSectorSize(logicalSectorSize)
        , m_ioChannelCount(ioChannelCount)
        , m_overCommit(overCommit)
        , m_constructionOptions(options)
    {
        V_Assert(!drivePaths.empty(), "StorageDrive_win requires at least one drive path to work.");
        
        // Get drive paths
        m_drivePaths.reserve(drivePaths.size());
        for (VStd::string_view drivePath : drivePaths)
        {
            VStd::string path(drivePath);
            // Erase the slash as it's one less character to compare and avoids issues with forward and
            // backward slashes.
            const char lastChar = path.back();
            if (lastChar == V_CORRECT_FILESYSTEM_SEPARATOR || lastChar == V_WRONG_FILESYSTEM_SEPARATOR)
            {
                path.pop_back();
            }
            m_drivePaths.push_back(VStd::move(path));
        }

        // Create name for statistics. The name will include all drive mappings on this physical device
        // for instance "Storage drive (F,G,H)". Colons and slashes are removed.
        m_name = "Storage drive (";
        m_name += m_drivePaths[0].substr(0, m_drivePaths[0].length()-1);
        for (size_t i = 1; i < m_drivePaths.size(); ++i)
        {
            m_name += ',';
            // Add the path, but don't include the slash as that might cause formating issues with some profilers.
            m_name += m_drivePaths[i].substr(0, m_drivePaths[i].length()-1);
        }
        m_name += ')';
        if (!m_constructionOptions.MinimalReporting)
        {
            V_Printf("Streamer", "%s created.\n", m_name.c_str());
        }

        if (m_physicalSectorSize == 0)
        {
            m_physicalSectorSize = 16_kib;
            V_Error("StorageDriveWin", false,
                "Received physical sector size of 0 for %s. Picking a sector size of %zu instead.\n", m_name.c_str(), m_physicalSectorSize);
        }
        if (m_logicalSectorSize == 0)
        {
            m_logicalSectorSize = 4_kib;
            V_Error("StorageDriveWin", false,
                "Received logical sector size of 0 for %s. Picking a sector size of %zu instead.\n", m_name.c_str(), m_logicalSectorSize);
        }
        V_Error("StorageDriveWin", IStreamerTypes::IsPowerOf2(m_physicalSectorSize) && IStreamerTypes::IsPowerOf2(m_logicalSectorSize),
            "StorageDriveWin requires power-of-2 sector sizes. Received physical: %zu and logical: %zu",
            m_physicalSectorSize, m_logicalSectorSize);

        // Cap the IO channels to the maximum
        if (m_ioChannelCount == 0)
        {
            m_ioChannelCount = v_numeric_cast<u32>(V::Platform::StreamerContextThreadSync::MaxIoEvents);
            V_Warning("StorageDriveWin", false,
                "Received io channel count of 0 for %s. Picking a count of %zu instead.\n",
                m_name.c_str(), V::Platform::StreamerContextThreadSync::MaxIoEvents);
        }
        else
        {
            m_ioChannelCount = V::GetMin(m_ioChannelCount, v_numeric_cast<u32>(V::Platform::StreamerContextThreadSync::MaxIoEvents));
        }
        // Make sure that the overCommit isn't so small that no slots are ever reported.
        if (v_numeric_cast<s32>(m_ioChannelCount) + m_overCommit <= 0)
        {
            V_Error("StorageDriveWin", false,
                "Received overcommit (%i) for %s that subtracts more than the number of IO channels (%u). Setting combined count to 1.\n",
                m_overCommit, m_name.c_str(), m_ioChannelCount);
            m_overCommit = 1 - v_numeric_cast<s32>(m_ioChannelCount);
        }

        // Add initial dummy values to the stats to avoid division by zero later on and avoid needing branches.
        m_readSizeAverage.PushEntry(1);
        m_readTimeAverage.PushEntry(VStd::chrono::microseconds(1));

        V_Assert(IStreamerTypes::IsPowerOf2(maxMetaDataCacheEntries),
            "StorageDriveWin requires a power-of-2 for maxMetaDataCacheEntries. Received %zu", maxMetaDataCacheEntries);
        m_metaDataCache_paths.resize(maxMetaDataCacheEntries);
        m_metaDataCache_fileSize.resize(maxMetaDataCacheEntries);
    }

    StorageDriveWin::~StorageDriveWin()
    {
        for (HANDLE file : m_fileCache_handles)
        {
            if (file != INVALID_HANDLE_VALUE)
            {
                ::CloseHandle(file);
            }
        }
        if (!m_constructionOptions.MinimalReporting)
        {
            V_Printf("Streamer", "%s destroyed.\n", m_name.c_str());
        }
    }

    void StorageDriveWin::PrepareRequest(FileRequest* request)
    {
        V_PROFILE_FUNCTION(Core);
        V_Assert(request, "PrepareRequest was provided a null request.");

        if (VStd::holds_alternative<FileRequest::ReadRequestData>(request->GetCommand()))
        {
            auto& readRequest = VStd::get<FileRequest::ReadRequestData>(request->GetCommand());
            if (IsServicedByThisDrive(readRequest.Path.GetAbsolutePath()))
            {
                FileRequest* read = m_context->GetNewInternalRequest();
                read->CreateRead(request, readRequest.Output, readRequest.OutputSize, readRequest.Path,
                    readRequest.Offset, readRequest.Size);
                m_context->PushPreparedRequest(read);
                return;
            }
        }
        StreamStackEntry::PrepareRequest(request);
    }

    void StorageDriveWin::QueueRequest(FileRequest* request)
    {
        V_PROFILE_FUNCTION(Core);
        V_Assert(request, "QueueRequest was provided a null request.");

        VStd::visit([this, request](auto&& args)
        {
            using Command = VStd::decay_t<decltype(args)>;
            if constexpr (VStd::is_same_v<Command, FileRequest::ReadData>)
            {
                if (IsServicedByThisDrive(args.Path.GetAbsolutePath()))
                {
                    m_pendingReadRequests.push_back(request);
                    return;
                }
            }
            else if constexpr (VStd::is_same_v<Command, FileRequest::FileExistsCheckData> ||
                VStd::is_same_v<Command, FileRequest::FileMetaDataRetrievalData>)
            {
                if (IsServicedByThisDrive(args.Path.GetAbsolutePath()))
                {
                    m_pendingRequests.push_back(request);
                    return;
                }
            }
            else if constexpr (VStd::is_same_v<Command, FileRequest::CancelData>)
            {
                if (CancelRequest(request, args.Target))
                {
                    // Only forward if this isn't part of the request chain, otherwise the storage device should
                    // be the last step as it doesn't forward any (sub)requests.
                    return;
                }
            }
            else if constexpr (VStd::is_same_v<Command, FileRequest::FlushData>)
            {
                FlushCache(args.Path);
            }
            else if constexpr (VStd::is_same_v<Command, FileRequest::FlushAllData>)
            {
                FlushEntireCache();
            }
            else if constexpr (VStd::is_same_v<Command, FileRequest::ReportData>)
            {
                Report(args);
            }
            StreamStackEntry::QueueRequest(request);
        }, request->GetCommand());
    }

    bool StorageDriveWin::ExecuteRequests()
    {
        bool hasFinalizedReads = FinalizeReads();
        bool hasWorked = false;

        if (!m_pendingReadRequests.empty())
        {
            FileRequest* request = m_pendingReadRequests.front();
            if (ReadRequest(request))
            {
                m_pendingReadRequests.pop_front();
                hasWorked = true;
            }
        }
        else if (!m_pendingRequests.empty())
        {
            FileRequest* request = m_pendingRequests.front();
            hasWorked = VStd::visit([this, request](auto&& args)
            {
                using Command = VStd::decay_t<decltype(args)>;
                if constexpr (VStd::is_same_v<Command, FileRequest::FileExistsCheckData>)
                {
                    FileExistsRequest(request);
                    m_pendingRequests.pop_front();
                    return true;
                }
                else if constexpr (VStd::is_same_v<Command, FileRequest::FileMetaDataRetrievalData>)
                {
                    FileMetaDataRetrievalRequest(request);
                    m_pendingRequests.pop_front();
                    return true;
                }
                else
                {
                    V_Assert(false, "A request was added to StorageDriveWin's pending queue that isn't supported.");
                    return false;
                }
            }, request->GetCommand());
        }

        return StreamStackEntry::ExecuteRequests() || hasFinalizedReads || hasWorked;
    }

    void StorageDriveWin::UpdateStatus(Status& status) const
    {
        StreamStackEntry::UpdateStatus(status);
        status.NumAvailableSlots = VStd::min(status.NumAvailableSlots, CalculateNumAvailableSlots());
        status.IsIdle = status.IsIdle && m_pendingReadRequests.empty() && m_pendingRequests.empty() && (m_activeReads_Count == 0);
    }

    void StorageDriveWin::UpdateCompletionEstimates(VStd::chrono::system_clock::time_point now, VStd::vector<FileRequest*>& internalPending,
        StreamerContext::PreparedQueue::iterator pendingBegin, StreamerContext::PreparedQueue::iterator pendingEnd)
    {
        StreamStackEntry::UpdateCompletionEstimates(now, internalPending, pendingBegin, pendingEnd);

        const RequestPath* activeFile = nullptr;
        if (m_activeCacheSlot != InvalidFileCacheIndex)
        {
            activeFile = &m_fileCache_paths[m_activeCacheSlot];
        }
        u64 activeOffset = m_activeOffset;

        // Determine the time of the first available slot
        VStd::chrono::system_clock::time_point earliestSlot = VStd::chrono::system_clock::time_point::max();
        for (size_t i = 0; i < m_readSlots_readInfo.size(); ++i)
        {
            if (m_readSlots_active[i])
            {
                FileReadInformation& read = m_readSlots_readInfo[i];
                u64 totalBytesRead = m_readSizeAverage.GetTotal();
                double totalReadTimeUSec = v_numeric_caster(m_readTimeAverage.GetTotal().count());
                auto readCommand = VStd::get_if<FileRequest::ReadData>(&read.Request->GetCommand());
                V_Assert(readCommand, "Request currently reading doesn't contain a read command.");
                auto endTime = read.StartTime + VStd::chrono::microseconds(v_numeric_cast<u64>((readCommand->Size * totalReadTimeUSec) / totalBytesRead));
                earliestSlot = VStd::min(earliestSlot, endTime);
                read.Request->SetEstimatedCompletion(endTime);
            }
        }
        if (earliestSlot != VStd::chrono::system_clock::time_point::max())
        {
            now = earliestSlot;
        }

        // Estimate requests in this stack entry.
        for (FileRequest* request : m_pendingReadRequests)
        {
            EstimateCompletionTimeForRequest(request, now, activeFile, activeOffset);
        }
        for (FileRequest* request : m_pendingRequests)
        {
            EstimateCompletionTimeForRequest(request, now, activeFile, activeOffset);
        }

        // Estimate internally pending requests. Because this call will go from the top of the stack to the bottom,
        // but estimation is calculated from the bottom to the top, this list should be processed in reverse order.
        for (auto requestIt = internalPending.rbegin(); requestIt != internalPending.rend(); ++requestIt)
        {
            EstimateCompletionTimeForRequestChecked(*requestIt, now, activeFile, activeOffset);
        }

        // Estimate pending requests that have not been queued yet.
        for (auto requestIt = pendingBegin; requestIt != pendingEnd; ++requestIt)
        {
            EstimateCompletionTimeForRequestChecked(*requestIt, now, activeFile, activeOffset);
        }
    }

    void StorageDriveWin::EstimateCompletionTimeForRequest(FileRequest* request, VStd::chrono::system_clock::time_point& startTime,
        const RequestPath*& activeFile, u64& activeOffset) const
    {
        u64 readSize = 0;
        u64 offset = 0;
        const RequestPath* targetFile = nullptr;

        VStd::visit([&](auto&& args)
        {
            using Command = VStd::decay_t<decltype(args)>;
            if constexpr (VStd::is_same_v<Command, FileRequest::ReadData>)
            {
                targetFile = &args.Path;
                readSize = args.Size;
                offset = args.Offset;
            }
            else if constexpr (VStd::is_same_v<Command, FileRequest::CompressedReadData>)
            {
                targetFile = &args.CompressionInfoData.ArchiveFilename;
                readSize = args.CompressionInfoData.CompressedSize;
                offset = args.CompressionInfoData.Offset;
            }
            else if constexpr (VStd::is_same_v<Command, FileRequest::FileExistsCheckData>)
            {
                readSize = 0;
                VStd::chrono::microseconds getFileExistsTimeAverage = m_getFileExistsTimeAverage.CalculateAverage();
                startTime += getFileExistsTimeAverage;
            }
            else if constexpr (VStd::is_same_v<Command, FileRequest::FileMetaDataRetrievalData>)
            {
                readSize = 0;
                VStd::chrono::microseconds getFileExistsTimeAverage = m_getFileMetaDataRetrievalTimeAverage.CalculateAverage();
                startTime += getFileExistsTimeAverage;
            }
        }, request->GetCommand());

        if (readSize > 0)
        {
            if (activeFile && activeFile != targetFile)
            {
                if (FindInFileHandleCache(*targetFile) == InvalidFileCacheIndex)
                {
                    VStd::chrono::microseconds fileOpenCloseTimeAverage = m_fileOpenCloseTimeAverage.CalculateAverage();
                    startTime += fileOpenCloseTimeAverage;
                }
                activeOffset = std::numeric_limits<u64>::max();
            }

            if (activeOffset != offset && m_constructionOptions.HasSeekPenalty)
            {
                startTime += _averageSeekTime;
            }

            u64 totalBytesRead = m_readSizeAverage.GetTotal();
            double totalReadTimeUSec = v_numeric_caster(m_readTimeAverage.GetTotal().count());
            startTime += VStd::chrono::microseconds(v_numeric_cast<u64>((readSize * totalReadTimeUSec) / totalBytesRead));
            activeOffset = offset + readSize;
        }
        request->SetEstimatedCompletion(startTime);
    }

    void StorageDriveWin::EstimateCompletionTimeForRequestChecked(FileRequest* request,
        VStd::chrono::system_clock::time_point startTime, const RequestPath*& activeFile, u64& activeOffset) const
    {
        VStd::visit([&, this](auto&& args)
        {
            using Command = VStd::decay_t<decltype(args)>;
            if constexpr (VStd::is_same_v<Command, FileRequest::ReadData> ||
                          VStd::is_same_v<Command, FileRequest::FileExistsCheckData>)
            {
                if (IsServicedByThisDrive(args.Path.GetAbsolutePath()))
                {
                    EstimateCompletionTimeForRequest(request, startTime, activeFile, activeOffset);
                }
            }
            else if constexpr (VStd::is_same_v<Command, FileRequest::CompressedReadData>)
            {
                if (IsServicedByThisDrive(args.CompressionInfoData.ArchiveFilename.GetAbsolutePath()))
                {
                    EstimateCompletionTimeForRequest(request, startTime, activeFile, activeOffset);
                }
            }
        }, request->GetCommand());
    }

    s32 StorageDriveWin::CalculateNumAvailableSlots() const
    {
        return (m_overCommit + v_numeric_cast<s32>(m_ioChannelCount)) - v_numeric_cast<s32>(m_pendingReadRequests.size()) -
            v_numeric_cast<s32>(m_pendingRequests.size()) - m_activeReads_Count;
    }

    auto StorageDriveWin::OpenFile(HANDLE& fileHandle, size_t& cacheSlot, FileRequest* request, const FileRequest::ReadData& data) -> OpenFileResult
    {
        HANDLE file = INVALID_HANDLE_VALUE;

        // If the file is already opened for use, use that file handle and update it's last touched time.
        size_t cacheIndex = FindInFileHandleCache(data.Path);
        if (cacheIndex != InvalidFileCacheIndex)
        {
            file = m_fileCache_handles[cacheIndex];
            V_Assert(file != INVALID_HANDLE_VALUE, "Found the file '%s' in cache, but file handle is invalid.\n",
                data.Path.GetRelativePath());
        }
        else
        {
            // If the file is not already found in the cache, attempt to claim an available cache entry.
            cacheIndex = FindAvailableFileHandleCacheIndex();
            if (cacheIndex == InvalidFileCacheIndex)
            {
                // No files ready to be evicted.
                return OpenFileResult::CacheFull;
            }

            // Adding explicit scope here for profiling file Open & Close
            {
                V_PROFILE_SCOPE(Core, "StorageDriveWin::ReadRequest OpenFile %s", m_name.c_str());
                TIMED_AVERAGE_WINDOW_SCOPE(m_fileOpenCloseTimeAverage);

                // All reads are overlapped (asynchronous).
                // Depending on configuration, reads can also be unbuffered.
                // Unbuffered means Windows does not use its own file cache for these files.
                DWORD createFlags = m_constructionOptions.EnableUnbufferedReads ? (FILE_FLAG_OVERLAPPED | FILE_FLAG_NO_BUFFERING) : FILE_FLAG_OVERLAPPED;
                DWORD shareMode = (m_constructionOptions.EnableSharing || data.SharedRead) ? FILE_SHARE_READ: 0;

                VStd::wstring filenameW;
                VStd::to_wstring(filenameW, data.Path.GetAbsolutePath());
                file = ::CreateFileW(
                    filenameW.c_str(),              // file name
                    FILE_GENERIC_READ,              // desired access
                    shareMode,                      // share mode
                    nullptr,                        // security attributes
                    OPEN_EXISTING,                  // creation disposition
                    createFlags,                    // flags and attributes
                    nullptr);                       // template file

                if (file == INVALID_HANDLE_VALUE)
                {
                    // Failed to open the file, so let the next entry in the stack try.
                    StreamStackEntry::QueueRequest(request);
                    return OpenFileResult::RequestForwarded;
                }

                // Remove any alertable IO completion notifications that could be queued by the IO Manager.
                if (!::SetFileCompletionNotificationModes(file, FILE_SKIP_SET_EVENT_ON_HANDLE | FILE_SKIP_COMPLETION_PORT_ON_SUCCESS))
                {
                    V_Warning("StorageDriveWin", false, "Failed to remove alertable IO completion notifications. (Error: %u)\n", ::GetLastError());
                }

                if (m_fileCache_handles[cacheIndex] != INVALID_HANDLE_VALUE)
                {
                    ::CloseHandle(m_fileCache_handles[cacheIndex]);
                }
            }

            // Fill the cache entry with data about the new file.
            m_fileCache_handles[cacheIndex] = file;
            m_fileCache_activeReads[cacheIndex] = 0;
            m_fileCache_paths[cacheIndex] = data.Path;
        }

        V_Assert(file != INVALID_HANDLE_VALUE, "While searching for file '%s' in StorageDeviceWin::OpenFile failed to detect a problem.",
            data.Path.GetRelativePath());

        // Set the current request and update timestamp, regardless of cache hit or miss.
        m_fileCache_lastTimeUsed[cacheIndex] = VStd::chrono::system_clock::now();
        fileHandle = file;
        cacheSlot = cacheIndex;
        return OpenFileResult::FileOpened;
    }

    bool StorageDriveWin::ReadRequest(FileRequest* request)
    {
        V_PROFILE_SCOPE(Core, "StorageDriveWin::ReadRequest %s", m_name.c_str());

        if (!m_cachesInitialized)
        {
            m_fileCache_lastTimeUsed.resize(m_maxFileHandles, VStd::chrono::system_clock::time_point::min());
            m_fileCache_paths.resize(m_maxFileHandles);
            m_fileCache_handles.resize(m_maxFileHandles, INVALID_HANDLE_VALUE);
            m_fileCache_activeReads.resize(m_maxFileHandles, 0);

            m_readSlots_readInfo.resize(m_ioChannelCount);
            m_readSlots_statusInfo.resize(m_ioChannelCount);
            m_readSlots_active.resize(m_ioChannelCount);

            m_cachesInitialized = true;
        }

        if (m_activeReads_Count >= m_ioChannelCount)
        {
            return false;
        }

        size_t readSlot = FindAvailableReadSlot();
        V_Assert(readSlot != InvalidReadSlotIndex, "Active read slot count indicates there's a read slot available, but no read slot was found.");

        return ReadRequest(request, readSlot);
    }

    bool StorageDriveWin::ReadRequest(FileRequest* request, size_t readSlot)
    {
        V_PROFILE_SCOPE(Core, "StorageDriveWin::ReadRequest %s", m_name.c_str());

        if (!m_context->GetStreamerThreadSynchronizer().AreEventHandlesAvailable())
        {
            // There are no more events handles available so delay executing this request until events become available.
            return false;
        }

        auto data = VStd::get_if<FileRequest::ReadData>(&request->GetCommand());
        V_Assert(data, "Read request in StorageDriveWin doesn't contain read data.");

        HANDLE file = INVALID_HANDLE_VALUE;
        size_t fileCacheSlot = InvalidFileCacheIndex;
        switch (OpenFile(file, fileCacheSlot, request, *data))
        {
        case OpenFileResult::FileOpened:
            break;
        case OpenFileResult::RequestForwarded:
            return true;
        case OpenFileResult::CacheFull:
            return false;
        default:
            V_Assert(false, "Unsupported OpenFileRequest returned.");
        }

        DWORD readSize = v_numeric_cast<DWORD>(data->Size);
        u64 readOffs = data->Offset;
        void* output = data->Output;

        FileReadInformation& readInfo = m_readSlots_readInfo[readSlot];
        readInfo.Request = request;

        if (m_constructionOptions.EnableUnbufferedReads)
        {
            // Check alignment of the file read information: size, offset, and address.
            // If any are unaligned to the sector sizes, make adjustments and allocate an aligned buffer.
            const bool alignedAddr = IStreamerTypes::IsAlignedTo(data->Output, v_numeric_caster(m_physicalSectorSize));
            const bool alignedOffs = IStreamerTypes::IsAlignedTo(data->Offset, v_numeric_caster(m_logicalSectorSize));
            
            // Adjust the offset if it's misaligned.
            // Align the offset down to next lowest sector.
            // Change the size to compensate.
            //
            // Before:
            // +---------------+---------------+
            // |   XXXXXXXXXXXX|XXXXXXX        |
            // +---------------+---------------+
            //     ^--- offset
            //     <--- size --------->
            //
            // After:
            // +---------------+---------------+
            // |###XXXXXXXXXXXX|XXXXXXX        |
            // +---------------+---------------+
            // ^--- offset
            // <--- size ------------->
            // <--> copyBackOffset
            //
            // Store the size of the adjustment in copyBackOffset, which will be used
            // later to copy only the X's and not the #'s (from diagram above).
            if (!alignedOffs)
            {
                readOffs = V_SIZE_ALIGN_DOWN(readOffs, m_logicalSectorSize);
                u64 offsetCorrection = data->Offset - readOffs;
                readInfo.CopyBackOffset = offsetCorrection;
                readSize = v_numeric_cast<DWORD>(data->Size + offsetCorrection);
            }

            bool alignedSize = IStreamerTypes::IsAlignedTo(readSize, v_numeric_caster(m_logicalSectorSize));
            if (!alignedSize)
            {
                DWORD alignedReadSize = v_numeric_caster(V_SIZE_ALIGN_UP(readSize, m_logicalSectorSize));
                if (alignedReadSize <= data->OutputSize)
                {
                    alignedSize = true;
                    readSize = alignedReadSize;
                }
            }

            // Adjust the size again if the end is misaligned.
            // Aligns the new read size up to be a multiple of the the sector size.
            //
            // Before:
            // +---------------+---------------+
            // |XXXXXXXXXXXXXXX|XXXXXXX        |
            // +---------------+---------------+
            // ^--- offset
            // <--- size ------------->
            //
            // After:
            // +---------------+---------------+
            // |XXXXXXXXXXXXXXX|XXXXXXX########|
            // +---------------+---------------+
            // ^--- offset
            // <--- size ---------------------->
            //
            // Once everything is aligned, allocate the temporary buffer.
            // Again, when read completes from OS, only copy back the X's and not
            // the #'s (from diagram above).
            const bool isAligned = (alignedAddr && alignedSize && alignedOffs);
            if (!isAligned)
            {
                readSize = v_numeric_cast<DWORD>(V_SIZE_ALIGN_UP(readSize, m_logicalSectorSize));
                readInfo.AllocateAlignedBuffer(readSize, m_physicalSectorSize);
                output = readInfo.SectorAlignedOutput;
            }
#if V_STREAMER_ADD_EXTRA_PROFILING_INFO
            m_directReadsPercentageStat.PushSample(isAligned ? 1.0 : 0.0);
            Statistic::PlotImmediate(m_name, DirectReadsName, m_directReadsPercentageStat.GetMostRecentSample());
#endif // V_STREAMER_ADD_EXTRA_PROFILING_INFO
        }
        
        FileReadStatus& readStatus = m_readSlots_statusInfo[readSlot];
        LPOVERLAPPED overlapped = &readStatus.Overlapped;
        overlapped->Offset = v_numeric_caster(readOffs);
        overlapped->OffsetHigh = v_numeric_caster(readOffs >> (sizeof(overlapped->Offset) << 3));
        overlapped->hEvent = m_context->GetStreamerThreadSynchronizer().CreateEventHandle();
        readStatus.FileHandleIndex = fileCacheSlot;

        bool result = false;
        {
            V_PROFILE_SCOPE(Core, "StorageDriveWin::ReadRequest ::ReadFile");
            result = ::ReadFile(file, output, readSize, nullptr, overlapped);
        }

        if (!result)
        {
            DWORD error = ::GetLastError();
            if (error != ERROR_IO_PENDING)
            {
                V_Warning("StorageDriveWin", false, "::ReadFile failed with error: %u\n", error);

                m_context->GetStreamerThreadSynchronizer().DestroyEventHandle(overlapped->hEvent);

                // Finish the request since this drive opened the file handle but the read failed.
                request->SetStatus(IStreamerTypes::RequestStatus::Failed);
                m_context->MarkRequestAsCompleted(request);
                readInfo.Clear();
                return true;
            }
        }
        else
        {
            // If this scope is reached, it means that ::ReadFile processed the read synchronously.  This can happen if
            // the OS already had the file in the cache.  The OVERLAPPED struct will still be filled out so we proceed as
            // if the read was fully asynchronous.
        }

        auto now = VStd::chrono::system_clock::now();
        if (m_activeReads_Count++ == 0)
        {
            m_activeReads_startTime = now;
        }
        readInfo.StartTime = now;
        m_readSlots_active[readSlot] = true;

#if V_STREAMER_ADD_EXTRA_PROFILING_INFO
        if (m_activeCacheSlot == fileCacheSlot)
        {
            m_fileSwitchPercentageStat.PushSample(0.0);
            m_seekPercentageStat.PushSample(m_activeOffset == data->Offset ? 0.0 : 1.0);
        }
        else
        {
            m_fileSwitchPercentageStat.PushSample(1.0);
            m_seekPercentageStat.PushSample(0.0);
        }

        Statistic::PlotImmediate(m_name, FileSwitchesName, m_fileSwitchPercentageStat.GetMostRecentSample());
        Statistic::PlotImmediate(m_name, SeeksName, m_seekPercentageStat.GetMostRecentSample());
#endif // AZ_STREAMER_ADD_EXTRA_PROFILING_INFO
        
        m_fileCache_activeReads[fileCacheSlot]++;
        m_activeCacheSlot = fileCacheSlot;
        m_activeOffset = readOffs + readSize;

        return true;
    }

    bool StorageDriveWin::CancelRequest(FileRequest* cancelRequest, FileRequestPtr& target)
    {
        bool ownsRequestChain = false;
        for (auto it = m_pendingReadRequests.begin(); it != m_pendingReadRequests.end();)
        {
            if ((*it)->WorksOn(target))
            {
                (*it)->SetStatus(IStreamerTypes::RequestStatus::Canceled);
                m_context->MarkRequestAsCompleted(*it);
                it = m_pendingReadRequests.erase(it);
                ownsRequestChain = true;
            }
            else
            {
                ++it;
            }
        }

        // Pending requests have been accounted for, now address any active reads and issue a Cancel call to the OS.

        size_t fileCacheIndex = InvalidFileCacheIndex;

        for (size_t readSlot = 0; readSlot < m_readSlots_active.size(); ++readSlot)
        {
            if (m_readSlots_active[readSlot] && m_readSlots_readInfo[readSlot].Request->WorksOn(target))
            {
                if (fileCacheIndex == InvalidFileCacheIndex)
                {
                    fileCacheIndex = m_readSlots_statusInfo[readSlot].FileHandleIndex;
                }
                V_Assert(fileCacheIndex == m_readSlots_statusInfo[readSlot].FileHandleIndex,
                    "Active file reads for a target read request have mismatched file cache indexes.");

                ownsRequestChain = true;
                if (!::CancelIoEx(m_fileCache_handles[fileCacheIndex], &m_readSlots_statusInfo[readSlot].Overlapped))
                {
                    DWORD error = ::GetLastError();
                    if (error != ERROR_NOT_FOUND)
                    {
                        V_Error("StorageDriveWin", false, "::CancelIoEx failed with error: %u\n", error);
                    }
                }
            }
        }

        if (ownsRequestChain)
        {
            cancelRequest->SetStatus(IStreamerTypes::RequestStatus::Completed);
            m_context->MarkRequestAsCompleted(cancelRequest);
        }

        return ownsRequestChain;
    }

    void StorageDriveWin::FileExistsRequest(FileRequest* request)
    {
        auto& fileExists = VStd::get<FileRequest::FileExistsCheckData>(request->GetCommand());

        V_PROFILE_SCOPE(Core, "StorageDriveWin::FileExistsRequest %s : %s",
            m_name.c_str(), fileExists.Path.GetRelativePath());
        TIMED_AVERAGE_WINDOW_SCOPE(m_getFileExistsTimeAverage);

        V_Assert(IsServicedByThisDrive(fileExists.Path.GetAbsolutePath()),
            "FileExistsRequest was queued on a StorageDriveWin that doesn't service files on the given path '%s'.",
            fileExists.Path.GetRelativePath());

        size_t cacheIndex = FindInFileHandleCache(fileExists.Path);
        if (cacheIndex != InvalidFileCacheIndex)
        {
            fileExists.Found = true;
            request->SetStatus(IStreamerTypes::RequestStatus::Completed);
            m_context->MarkRequestAsCompleted(request);
            return;
        }

        cacheIndex = FindInMetaDataCache(fileExists.Path);
        if (cacheIndex != InvalidMetaDataCacheIndex)
        {
            fileExists.Found = true;
            request->SetStatus(IStreamerTypes::RequestStatus::Completed);
            m_context->MarkRequestAsCompleted(request);
            return;
        }

        WIN32_FILE_ATTRIBUTE_DATA attributes;
        VStd::wstring filenameW;
        VStd::to_wstring(filenameW, fileExists.Path.GetAbsolutePath());
        if (::GetFileAttributesExW(filenameW.c_str(), GetFileExInfoStandard, &attributes))
        {
            if ((attributes.dwFileAttributes != INVALID_FILE_ATTRIBUTES) &&
                ((attributes.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0))
            {
                LARGE_INTEGER fileSize;
                fileSize.LowPart = attributes.nFileSizeLow;
                fileSize.HighPart = attributes.nFileSizeHigh;

                cacheIndex = GetNextMetaDataCacheSlot();
                m_metaDataCache_paths[cacheIndex] = fileExists.Path;
                m_metaDataCache_fileSize[cacheIndex] = v_numeric_caster(fileSize.QuadPart);
                fileExists.Found = true;

                request->SetStatus(IStreamerTypes::RequestStatus::Completed);
                m_context->MarkRequestAsCompleted(request);
            }
            return;
        }

        StreamStackEntry::QueueRequest(request);
    }

    void StorageDriveWin::FileMetaDataRetrievalRequest(FileRequest* request)
    {
        auto& command = VStd::get<FileRequest::FileMetaDataRetrievalData>(request->GetCommand());

        V_PROFILE_SCOPE(Core, "StorageDriveWin::FileMetaDataRetrievalRequest %s : %s",
            m_name.c_str(), command.Path.GetRelativePath());
        TIMED_AVERAGE_WINDOW_SCOPE(m_getFileMetaDataRetrievalTimeAverage);

        size_t cacheIndex = FindInMetaDataCache(command.Path);
        if (cacheIndex != InvalidMetaDataCacheIndex)
        {
            command.FileSize = m_metaDataCache_fileSize[cacheIndex];
            command.Found = true;
            request->SetStatus(IStreamerTypes::RequestStatus::Completed);
            m_context->MarkRequestAsCompleted(request);
            return;
        }

        LARGE_INTEGER fileSize{};

        cacheIndex = FindInFileHandleCache(command.Path);
        if (cacheIndex != InvalidFileCacheIndex)
        {
            V_Assert(m_fileCache_handles[cacheIndex] != INVALID_HANDLE_VALUE,
                "File path '%s' doesn't have an associated file handle.", m_fileCache_paths[cacheIndex].GetRelativePath());
            if (::GetFileSizeEx(m_fileCache_handles[cacheIndex], &fileSize) == FALSE)
            {
                StreamStackEntry::QueueRequest(request);
                return;
            }
        }
        else
        {
            WIN32_FILE_ATTRIBUTE_DATA attributes;
            VStd::wstring filenameW;
            VStd::to_wstring(filenameW, command.Path.GetAbsolutePath());
            if (::GetFileAttributesExW(filenameW.c_str(), GetFileExInfoStandard, &attributes) &&
                (attributes.dwFileAttributes != INVALID_FILE_ATTRIBUTES) && ((attributes.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0))
            {
                fileSize.LowPart = attributes.nFileSizeLow;
                fileSize.HighPart = attributes.nFileSizeHigh;
            }
            else
            {
                StreamStackEntry::QueueRequest(request);
                return;
            }
        }

        command.FileSize = v_numeric_caster(fileSize.QuadPart);
        command.Found = true;

        cacheIndex = GetNextMetaDataCacheSlot();

        m_metaDataCache_paths[cacheIndex] = command.Path;
        m_metaDataCache_fileSize[cacheIndex] = v_numeric_caster(fileSize.QuadPart);

        request->SetStatus(IStreamerTypes::RequestStatus::Completed);
        m_context->MarkRequestAsCompleted(request);
    }

    void StorageDriveWin::FlushCache(const RequestPath& filePath)
    {
        if (m_cachesInitialized)
        {
            size_t cacheIndex = FindInFileHandleCache(filePath);
            if (cacheIndex != InvalidFileCacheIndex)
            {
                if (m_fileCache_handles[cacheIndex] != INVALID_HANDLE_VALUE)
                {
                    V_Assert(m_fileCache_activeReads[cacheIndex] == 0, "Flushing '%s' but it has %u active reads\n",
                        filePath.GetRelativePath(), m_fileCache_activeReads[cacheIndex]);
                    ::CloseHandle(m_fileCache_handles[cacheIndex]);
                    m_fileCache_handles[cacheIndex] = INVALID_HANDLE_VALUE;
                }
                m_fileCache_activeReads[cacheIndex] = 0;
                m_fileCache_lastTimeUsed[cacheIndex] = VStd::chrono::system_clock::time_point();
                m_fileCache_paths[cacheIndex].Clear();
            }

            cacheIndex = FindInMetaDataCache(filePath);
            if (cacheIndex != InvalidMetaDataCacheIndex)
            {
                m_metaDataCache_paths[cacheIndex].Clear();
                m_metaDataCache_fileSize[cacheIndex] = 0;
            }
        }
    }

    void StorageDriveWin::FlushEntireCache()
    {
        if (m_cachesInitialized)
        {
            // Clear file handle cache
            for (size_t cacheIndex = 0; cacheIndex < m_maxFileHandles; ++cacheIndex)
            {
                if (m_fileCache_handles[cacheIndex] != INVALID_HANDLE_VALUE)
                {
                    V_Assert(m_fileCache_activeReads[cacheIndex] == 0, "Flushing '%s' but it has %u active reads\n",
                        m_fileCache_paths[cacheIndex].GetRelativePath(), m_fileCache_activeReads[cacheIndex]);
                    ::CloseHandle(m_fileCache_handles[cacheIndex]);
                    m_fileCache_handles[cacheIndex] = INVALID_HANDLE_VALUE;
                }
                m_fileCache_activeReads[cacheIndex] = 0;
                m_fileCache_lastTimeUsed[cacheIndex] = VStd::chrono::system_clock::time_point();
                m_fileCache_paths[cacheIndex].Clear();
            }

            // Clear meta data cache
            auto metaDataCacheSize = m_metaDataCache_paths.size();
            m_metaDataCache_paths.clear();
            m_metaDataCache_fileSize.clear();
            m_metaDataCache_front = 0;
            m_metaDataCache_paths.resize(metaDataCacheSize);
            m_metaDataCache_fileSize.resize(metaDataCacheSize);
        }
    }

    bool StorageDriveWin::FinalizeReads()
    {
        V_PROFILE_FUNCTION(Core);

        bool hasWorked = false;
        for (size_t readSlot = 0; readSlot < m_readSlots_active.size(); ++readSlot)
        {
            if (m_readSlots_active[readSlot])
            {
                FileReadStatus& status = m_readSlots_statusInfo[readSlot];

                if (HasOverlappedIoCompleted(&status.Overlapped))
                {
                    DWORD bytesTransferred = 0;
                    BOOL result = ::GetOverlappedResult(m_fileCache_handles[status.FileHandleIndex],
                        &status.Overlapped, &bytesTransferred, FALSE);
                    DWORD error = ::GetLastError();

                    if (result || error == ERROR_OPERATION_ABORTED)
                    {
                        hasWorked = true;
                        constexpr bool encounteredError = false;
                        FinalizeSingleRequest(status, readSlot, bytesTransferred, error == ERROR_OPERATION_ABORTED, encounteredError);
                    }
                    else if (error != ERROR_IO_PENDING && error != ERROR_IO_INCOMPLETE)
                    {
                        V_Error("StorageDriveWin", false, "Async file read operation completed with extended error code %u\n", error);
                        hasWorked = true;
                        constexpr bool encounteredError = true;
                        FinalizeSingleRequest(status, readSlot, bytesTransferred, false, encounteredError);
                    }
                }
            }
        }
        return hasWorked;
    }

    void StorageDriveWin::FinalizeSingleRequest(FileReadStatus& status, size_t readSlot, DWORD numBytesTransferred,
        bool isCanceled, bool encounteredError)
    {
        m_activeReads_ByteCount += numBytesTransferred;
        if (--m_activeReads_Count == 0)
        {
            // Update read stats now that the operation is done.
            m_readSizeAverage.PushEntry(m_activeReads_ByteCount);
            m_readTimeAverage.PushEntry(VStd::chrono::duration_cast<VStd::chrono::microseconds>(
                VStd::chrono::system_clock::now() - m_activeReads_startTime));

            m_activeReads_ByteCount = 0;
        }

        FileReadInformation& fileReadInfo = m_readSlots_readInfo[readSlot];

        auto readCommand = VStd::get_if<FileRequest::ReadData>(&fileReadInfo.Request->GetCommand());
        V_Assert(readCommand != nullptr, "Request stored with the overlapped I/O call did not contain a read request.");
        
        if (fileReadInfo.SectorAlignedOutput && !encounteredError)
        {
            auto offsetAddress = reinterpret_cast<u8*>(fileReadInfo.SectorAlignedOutput) + fileReadInfo.CopyBackOffset;
            ::memcpy(readCommand->Output, offsetAddress, readCommand->Size);
        }

        // The request could be reading more due to alignment requirements. It should however never read less that the amount of
        // requested data.
        bool isSuccess = !encounteredError && (readCommand->Size <= numBytesTransferred);

        fileReadInfo.Request->SetStatus(
            isCanceled
                ? IStreamerTypes::RequestStatus::Canceled
                : isSuccess
                    ? IStreamerTypes::RequestStatus::Completed
                    : IStreamerTypes::RequestStatus::Failed
        );
        m_context->MarkRequestAsCompleted(fileReadInfo.Request);

        m_fileCache_activeReads[status.FileHandleIndex]--;
        m_readSlots_active[readSlot] = false;
        m_context->GetStreamerThreadSynchronizer().DestroyEventHandle(status.Overlapped.hEvent);
        fileReadInfo.Clear();

        // There's now a slot available to queue the next request, if there is one.
        if (!m_pendingReadRequests.empty())
        {
            FileRequest* request = m_pendingReadRequests.front();
            if (ReadRequest(request, readSlot))
            {
                m_pendingReadRequests.pop_front();
            }
        }
    }

    size_t StorageDriveWin::FindInFileHandleCache(const RequestPath& filePath) const
    {
        size_t numFiles = m_fileCache_paths.size();
        for (size_t i = 0; i < numFiles; ++i)
        {
            if (m_fileCache_paths[i] == filePath)
            {
                return i;
            }
        }
        return InvalidFileCacheIndex;
    }

    size_t StorageDriveWin::FindAvailableFileHandleCacheIndex() const
    {
        V_Assert(m_cachesInitialized, "Using file cache before it has been (lazily) initialized\n");

        // This needs to look for files with no active reads, and the oldest file among those.
        size_t cacheIndex = InvalidFileCacheIndex;
        VStd::chrono::system_clock::time_point oldest = VStd::chrono::system_clock::time_point::max();
        for (size_t index = 0; index < m_maxFileHandles; ++index)
        {
            if (m_fileCache_activeReads[index] == 0 && m_fileCache_lastTimeUsed[index] < oldest)
            {
                oldest = m_fileCache_lastTimeUsed[index];
                cacheIndex = index;
            }
        }

        return cacheIndex;
    }

    size_t StorageDriveWin::FindAvailableReadSlot()
    {
        for (size_t i = 0; i < m_readSlots_active.size(); ++i)
        {
            if (!m_readSlots_active[i])
            {
                return i;
            }
        }
        return InvalidReadSlotIndex;
    }

    size_t StorageDriveWin::FindInMetaDataCache(const RequestPath& filePath) const
    {
        size_t numFiles = m_metaDataCache_paths.size();
        for (size_t i = 0; i < numFiles; ++i)
        {
            if (m_metaDataCache_paths[i] == filePath)
            {
                return i;
            }
        }
        return InvalidMetaDataCacheIndex;
    }

    size_t StorageDriveWin::GetNextMetaDataCacheSlot()
    {
        m_metaDataCache_front = (m_metaDataCache_front + 1) & (m_metaDataCache_paths.size() - 1);
        return m_metaDataCache_front;
    }

    bool StorageDriveWin::IsServicedByThisDrive(const char* filePath) const
    {
        // This approach doesn't allow paths to be resolved to the correct drive when junctions are used or when a drive
        // is mapped as folder of another drive. To do this correctly "GetVolumePathName" should be used, but this takes
        // about 1 to 1.5 ms per request, so this introduces unacceptably large overhead particularly when the user has
        // multiple disks.
        for (const VStd::string& drivePath : m_drivePaths)
        {
            if (v_strnicmp(filePath, drivePath.c_str(), drivePath.length()) == 0)
            {
                return true;
            }
        }
        return false;
    }

    void StorageDriveWin::CollectStatistics(VStd::vector<Statistic>& statistics) const
    {
        if (m_cachesInitialized)
        {
            constexpr double bytesToMB = v_numeric_cast<double>(1_mib);
            using DoubleSeconds = VStd::chrono::duration<double>;

            double totalBytesReadMB = m_readSizeAverage.GetTotal() / bytesToMB;
            double totalReadTimeSec = VStd::chrono::duration_cast<DoubleSeconds>(m_readTimeAverage.GetTotal()).count();
            statistics.push_back(Statistic::CreateFloat(m_name, "Read Speed (avg. mbps)", totalBytesReadMB / totalReadTimeSec));
            statistics.push_back(Statistic::CreateInteger(m_name, "File Open & Close (avg. us)", m_fileOpenCloseTimeAverage.CalculateAverage().count()));
            statistics.push_back(Statistic::CreateInteger(m_name, "Get file exists (avg. us)", m_getFileExistsTimeAverage.CalculateAverage().count()));
            statistics.push_back(Statistic::CreateInteger(m_name, "Get file meta data (avg. us)", m_getFileMetaDataRetrievalTimeAverage.CalculateAverage().count()));

            statistics.push_back(Statistic::CreateInteger(m_name, "Available slots", CalculateNumAvailableSlots()));

#if V_STREAMER_ADD_EXTRA_PROFILING_INFO
            statistics.push_back(Statistic::CreatePercentage(m_name, FileSwitchesName, m_fileSwitchPercentageStat.GetAverage()));
            statistics.push_back(Statistic::CreatePercentage(m_name, SeeksName, m_seekPercentageStat.GetAverage()));
            statistics.push_back(Statistic::CreatePercentage(m_name, DirectReadsName, m_directReadsPercentageStat.GetAverage()));
#endif
        }
        StreamStackEntry::CollectStatistics(statistics);
    }

    void StorageDriveWin::Report(const FileRequest::ReportData& data) const
    {
        switch (data.ReportTypeValue)
        {
        case FileRequest::ReportData::ReportType::FileLocks:
            if (m_cachesInitialized)
            {
                for (u32 i = 0; i < m_maxFileHandles; ++i)
                {
                    if (m_fileCache_handles[i] != INVALID_HANDLE_VALUE)
                    {
                        V_Printf("Streamer", "File lock in %s : '%s'.\n", m_name.c_str(), m_fileCache_paths[i].GetRelativePath());
                    }
                }
            }
            else
            {
                V_Printf("Streamer", "File lock in %s : No files have been streamed.\n", m_name.c_str());
            }
            break;
        default:
            break;
        }
    }
} // namespace V::IO