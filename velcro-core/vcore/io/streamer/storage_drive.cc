#include <limits>
#include <vcore/casting/numeric_cast.h>
#include <vcore/debug/profiler.h>
#include <vcore/io/streamer/file_request.h>
#include <vcore/io/streamer/streamer_context.h>
#include <vcore/io/streamer/storage_drive.h>
#include <vcore/std/smart_ptr/make_shared.h>
#include <vcore/std/typetraits/decay.h>

namespace V
{
    namespace IO
    {
        VStd::shared_ptr<StreamStackEntry> StorageDriveConfig::AddStreamStackEntry(
            [[maybe_unused]] const HardwareInformation& hardware, [[maybe_unused]] VStd::shared_ptr<StreamStackEntry> parent)
        {
            return VStd::make_shared<StorageDrive>(MaxFileHandles);
        }

      

        const VStd::chrono::microseconds StorageDrive::_averageSeekTime =
            VStd::chrono::milliseconds(9) + // Common average seek time for desktop hdd drives.
            VStd::chrono::milliseconds(3); // Rotational latency for a 7200RPM disk

        StorageDrive::StorageDrive(u32 maxFileHandles)
            : StreamStackEntry("Storage drive (generic)")
        {
            m_fileLastUsed.resize(maxFileHandles, VStd::chrono::system_clock::time_point::min());
            m_filePaths.resize(maxFileHandles);
            m_fileHandles.resize(maxFileHandles);

            // Add initial dummy values to the stats to avoid division by zero later on and avoid needing branches.
            m_readSizeAverage.PushEntry(1);
            m_readTimeAverage.PushEntry(VStd::chrono::microseconds(1));
        }

        void StorageDrive::SetNext(VStd::shared_ptr<StreamStackEntry> /*next*/)
        {
            V_Assert(false, "StorageDrive isn't allowed to have a node to forward requests to.");
        }

        void StorageDrive::PrepareRequest(FileRequest* request)
        {
            V_PROFILE_FUNCTION(Core);
            V_Assert(request, "PrepareRequest was provided a null request.");

            if (VStd::holds_alternative<FileRequest::ReadRequestData>(request->GetCommand()))
            {
                auto& readRequest = VStd::get<FileRequest::ReadRequestData>(request->GetCommand());

                FileRequest* read = m_context->GetNewInternalRequest();
                read->CreateRead(request, readRequest.Output, readRequest.OutputSize, readRequest.Path,
                    readRequest.Offset, readRequest.Size);
                m_context->PushPreparedRequest(read);
                return;
            }
            StreamStackEntry::PrepareRequest(request);
        }

        void StorageDrive::QueueRequest(FileRequest* request)
        {
            V_Assert(request, "QueueRequest was provided a null request.");
            VStd::visit([this, request](auto&& args)
            {
                using Command = VStd::decay_t<decltype(args)>;
                if constexpr (VStd::is_same_v<Command, FileRequest::ReadData> ||
                    VStd::is_same_v<Command, FileRequest::FileExistsCheckData> ||
                    VStd::is_same_v<Command, FileRequest::FileMetaDataRetrievalData>)
                {
                    m_pendingRequests.push_back(request);
                    return;
                }
                else if constexpr (VStd::is_same_v<Command, FileRequest::CancelData>)
                {
                    CancelRequest(request, args.Target);
                    return;
                }
                else
                {
                    if constexpr (VStd::is_same_v<Command, FileRequest::FlushData>)
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
                }
            }, request->GetCommand());
        }

        bool StorageDrive::ExecuteRequests()
        {
            if (!m_pendingRequests.empty())
            {
                FileRequest* request = m_pendingRequests.front();
                VStd::visit([this, request](auto&& args)
                {
                    using Command = VStd::decay_t<decltype(args)>;
                    if constexpr (VStd::is_same_v<Command, FileRequest::ReadData>)
                    {
                        ReadFile(request);
                    }
                    else if constexpr (VStd::is_same_v<Command, FileRequest::FileExistsCheckData>)
                    {
                        FileExistsRequest(request);
                    }
                    else if constexpr (VStd::is_same_v<Command, FileRequest::FileMetaDataRetrievalData>)
                    {
                        FileMetaDataRetrievalRequest(request);
                    }
                }, request->GetCommand());
                m_pendingRequests.pop_front();
                return true;
            }
            else
            {
                return false;
            }
        }

        void StorageDrive::UpdateStatus(Status& status) const
        {
            // Only participate if there are actually any reads done.
            if (m_fileOpenCloseTimeAverage.GetNumRecorded() > 0)
            {
                s32 availableSlots = _maxRequests - v_numeric_cast<s32>(m_pendingRequests.size());
                StreamStackEntry::UpdateStatus(status);
                status.NumAvailableSlots = VStd::min(status.NumAvailableSlots, availableSlots);
                status.IsIdle = status.IsIdle && m_pendingRequests.empty();
            }
            else
            {
                status.NumAvailableSlots = VStd::min(status.NumAvailableSlots, _maxRequests);
            }
        }
        
        void StorageDrive::UpdateCompletionEstimates(VStd::chrono::system_clock::time_point now,
            VStd::vector<FileRequest*>& internalPending, StreamerContext::PreparedQueue::iterator pendingBegin,
            StreamerContext::PreparedQueue::iterator pendingEnd)
        {
            StreamStackEntry::UpdateCompletionEstimates(now, internalPending, pendingBegin, pendingEnd);

            const RequestPath* activeFile = nullptr;
            if (m_activeCacheSlot != _fileNotFound)
            {
                activeFile = &m_filePaths[m_activeCacheSlot];
            }
            u64 activeOffset = m_activeOffset;

            // Estimate requests in this stack entry.
            for (FileRequest* request : m_pendingRequests)
            {
                EstimateCompletionTimeForRequest(request, now, activeFile, activeOffset);
            }

            // Estimate internally pending requests. Because this call will go from the top of the stack to the bottom,
            // but estimation is calculated from the bottom to the top, this list should be processed in reverse order.
            for (auto requestIt = internalPending.rbegin(); requestIt != internalPending.rend(); ++requestIt)
            {
                EstimateCompletionTimeForRequest(*requestIt, now, activeFile, activeOffset);
            }

            // Estimate pending requests that have not been queued yet.
            for (auto requestIt = pendingBegin; requestIt != pendingEnd; ++requestIt)
            {
                EstimateCompletionTimeForRequest(*requestIt, now, activeFile, activeOffset);
            }
        }

        void StorageDrive::EstimateCompletionTimeForRequest(FileRequest* request, VStd::chrono::system_clock::time_point& startTime,
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
                    VStd::chrono::microseconds averageTime = m_getFileExistsTimeAverage.CalculateAverage();
                    startTime += averageTime;
                }
                else if constexpr (VStd::is_same_v<Command, FileRequest::FileMetaDataRetrievalData>)
                {
                    readSize = 0;
                    VStd::chrono::microseconds averageTime = m_getFileMetaDataTimeAverage.CalculateAverage();
                    startTime += averageTime;
                }
            }, request->GetCommand());

            if (readSize > 0)
            {
                if (activeFile && activeFile != targetFile)
                {
                    if (FindFileInCache(*targetFile) == _fileNotFound)
                    {
                        VStd::chrono::microseconds fileOpenCloseTimeAverage = m_fileOpenCloseTimeAverage.CalculateAverage();
                        startTime += fileOpenCloseTimeAverage;
                    }
                    startTime += _averageSeekTime;
                    activeOffset = std::numeric_limits<u64>::max();
                }
                else if (activeOffset != offset)
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

        void StorageDrive::ReadFile(FileRequest* request)
        {
            V_PROFILE_FUNCTION(Core);

            auto data = VStd::get_if<FileRequest::ReadData>(&request->GetCommand());
            V_Assert(data, "FileRequest queued on StorageDrive to be read didn't contain read data.");
            
            SystemFile* file = nullptr;

            // If the file is already open, use that file handle and update it's last touched time.
            size_t cacheIndex = FindFileInCache(data->Path);
            if (cacheIndex != _fileNotFound)
            {
                file = m_fileHandles[cacheIndex].get();
                m_fileLastUsed[cacheIndex] = VStd::chrono::high_resolution_clock::now();
            }
            
            // If the file is not open, eject the entry from the cache that hasn't been used for the longest time 
            // and open the file for reading.
            if (!file)
            {
                VStd::chrono::system_clock::time_point oldest = m_fileLastUsed[0];
                cacheIndex = 0;
                size_t numFiles = m_filePaths.size();
                for (size_t i = 1; i < numFiles; ++i)
                {
                    if (m_fileLastUsed[i] < oldest)
                    {
                        oldest = m_fileLastUsed[i];
                        cacheIndex = i;
                    }
                }

                TIMED_AVERAGE_WINDOW_SCOPE(m_fileOpenCloseTimeAverage);
                VStd::unique_ptr<SystemFile> newFile = VStd::make_unique<SystemFile>();
                bool isOpen = newFile->Open(data->Path.GetAbsolutePath(), SystemFile::OpenMode::SF_OPEN_READ_ONLY);
                if (!isOpen)
                {
                    request->SetStatus(IStreamerTypes::RequestStatus::Failed);
                    m_context->MarkRequestAsCompleted(request);
                    return;
                }

                file = newFile.get();
                m_fileLastUsed[cacheIndex] = VStd::chrono::high_resolution_clock::now();
                m_fileHandles[cacheIndex] = VStd::move(newFile);
                m_filePaths[cacheIndex] = data->Path;
            }
            
            V_Assert(file, "While searching for file '%s' StorageDevice::ReadFile failed to detect a problem.", data->Path.GetRelativePath());
            u64 bytesRead = 0;
            {
                TIMED_AVERAGE_WINDOW_SCOPE(m_readTimeAverage);
                if (file->Tell() != data->Offset)
                {
                    file->Seek(data->Offset, SystemFile::SeekMode::SF_SEEK_BEGIN);
                }
                bytesRead = file->Read(data->Size, data->Output);
            }
            m_readSizeAverage.PushEntry(bytesRead);

            m_activeCacheSlot = cacheIndex;
            m_activeOffset = data->Offset + bytesRead;

            request->SetStatus(bytesRead == data->Size ? IStreamerTypes::RequestStatus::Completed : IStreamerTypes::RequestStatus::Failed);
            m_context->MarkRequestAsCompleted(request);
        }

        void StorageDrive::CancelRequest(FileRequest* cancelRequest, FileRequestPtr& target)
        {
            for (auto it = m_pendingRequests.begin(); it != m_pendingRequests.end();)
            {
                if ((*it)->WorksOn(target))
                {
                    (*it)->SetStatus(IStreamerTypes::RequestStatus::Canceled);
                    m_context->MarkRequestAsCompleted(*it);
                    it = m_pendingRequests.erase(it);
                }
                else
                {
                    ++it;
                }
            }
            cancelRequest->SetStatus(IStreamerTypes::RequestStatus::Completed);
            m_context->MarkRequestAsCompleted(cancelRequest);
        }

        void StorageDrive::FileExistsRequest(FileRequest* request)
        {
            V_PROFILE_FUNCTION(Core);
            TIMED_AVERAGE_WINDOW_SCOPE(m_getFileExistsTimeAverage);

            auto& fileExists = VStd::get<FileRequest::FileExistsCheckData>(request->GetCommand());
            size_t cacheIndex = FindFileInCache(fileExists.Path);
            if (cacheIndex != _fileNotFound)
            {
                fileExists.Found = true;
            }
            else
            {
                fileExists.Found = SystemFile::Exists(fileExists.Path.GetAbsolutePath());
            }
            m_context->MarkRequestAsCompleted(request);
        }

        void StorageDrive::FileMetaDataRetrievalRequest(FileRequest* request)
        {
            V_PROFILE_FUNCTION(Core);
            TIMED_AVERAGE_WINDOW_SCOPE(m_getFileMetaDataTimeAverage);

            auto& command = VStd::get<FileRequest::FileMetaDataRetrievalData>(request->GetCommand());
            // If the file is already open, use the file handle which usually is cheaper than asking for the file by name.
            size_t cacheIndex = FindFileInCache(command.Path);
            if (cacheIndex != _fileNotFound)
            {
                V_Assert(m_fileHandles[cacheIndex],
                    "File path '%s' doesn't have an associated file handle.", m_filePaths[cacheIndex].GetRelativePath());
                command.FileSize = m_fileHandles[cacheIndex]->Length();
                command.Found = true;
                request->SetStatus(IStreamerTypes::RequestStatus::Completed);
            }
            else
            {
                // The file is not open yet, so try to get the file size by name.
                u64 size = SystemFile::Length(command.Path.GetAbsolutePath());
                if (size != 0) // SystemFile::Length doesn't allow telling a zero-sized file apart from a invalid path.
                {
                    command.FileSize = size;
                    command.Found = true;
                    request->SetStatus(IStreamerTypes::RequestStatus::Completed);
                }
                else
                {
                    request->SetStatus(IStreamerTypes::RequestStatus::Failed);
                }
            }

            m_context->MarkRequestAsCompleted(request);
        }

        void StorageDrive::FlushCache(const RequestPath& filePath)
        {
            size_t cacheIndex = FindFileInCache(filePath);
            if (cacheIndex != _fileNotFound)
            {
                m_fileLastUsed[cacheIndex] = VStd::chrono::system_clock::time_point();
                m_fileHandles[cacheIndex].reset();
                m_filePaths[cacheIndex].Clear();
            }
        }

        void StorageDrive::FlushEntireCache()
        {
            size_t numFiles = m_filePaths.size();
            for (size_t i = 0; i < numFiles; ++i)
            {
                m_fileLastUsed[i] = VStd::chrono::system_clock::time_point();
                m_fileHandles[i].reset();
                m_filePaths[i].Clear();
            }
        }

        size_t StorageDrive::FindFileInCache(const RequestPath& filePath) const
        {
            size_t numFiles = m_filePaths.size();
            for (size_t i = 0; i < numFiles; ++i)
            {
                if (m_filePaths[i] == filePath)
                {
                    return i;
                }
            }
            return _fileNotFound;
        }

        void StorageDrive::CollectStatistics(VStd::vector<Statistic>& statistics) const
        {
            constexpr double bytesToMB = (1024.0 * 1024.0);
            using DoubleSeconds = VStd::chrono::duration<double>;
            
            double totalBytesReadMB = m_readSizeAverage.GetTotal() / bytesToMB;
            double totalReadTimeSec = VStd::chrono::duration_cast<DoubleSeconds>(m_readTimeAverage.GetTotal()).count();
            if (m_readSizeAverage.GetTotal() > 1) // A default value is always added.
            {
                statistics.push_back(Statistic::CreateFloat(m_name, "Read Speed (avg. mbps)", totalBytesReadMB / totalReadTimeSec));
            }

            if (m_fileOpenCloseTimeAverage.GetNumRecorded() > 0)
            {
                statistics.push_back(Statistic::CreateInteger(m_name, "File Open & Close (avg. us)", m_fileOpenCloseTimeAverage.CalculateAverage().count()));
                statistics.push_back(Statistic::CreateInteger(m_name, "Get file exists (avg. us)", m_getFileExistsTimeAverage.CalculateAverage().count()));
                statistics.push_back(Statistic::CreateInteger(m_name, "Get file meta data (avg. us)", m_getFileMetaDataTimeAverage.CalculateAverage().count()));
                statistics.push_back(Statistic::CreateInteger(m_name, "Available slots", s64{ _maxRequests } - m_pendingRequests.size()));
            }
        }

        void StorageDrive::Report(const FileRequest::ReportData& data) const
        {
            switch (data.ReportTypeValue)
            {
            case FileRequest::ReportData::ReportType::FileLocks:
                for (u32 i = 0; i < m_fileHandles.size(); ++i)
                {
                    if (m_fileHandles[i] != nullptr)
                    {
                        V_Printf("Streamer", "File lock in %s : '%s'.\n", m_name.c_str(), m_filePaths[i].GetRelativePath());
                    }
                }
                break;
            default:
                break;
            }
        }
    } // namespace IO
} // namespace V