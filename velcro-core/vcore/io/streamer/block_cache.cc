#include <vcore/casting/numeric_cast.h>
#include <vcore/debug/profiler.h>
#include <vcore/io/streamer/block_cache.h>
#include <vcore/io/streamer/file_request.h>
#include <vcore/io/streamer/streamer_context.h>
#include <vcore/std/algorithm.h>
#include <vcore/std/smart_ptr/make_shared.h>

namespace V
{
    namespace IO
    {
        VStd::shared_ptr<StreamStackEntry> BlockCacheConfig::AddStreamStackEntry(
            const HardwareInformation& hardware, VStd::shared_ptr<StreamStackEntry> parent)
        {
            size_t blockSize;
            switch (BlockSizeValue)
            {
            case BlockSize::MaxTransfer:
                blockSize = hardware.MaxTransfer;
                break;
            case BlockSize::MemoryAlignment:
                blockSize = hardware.MaxPhysicalSectorSize;
                break;
            case BlockSize::SizeAlignment:
                blockSize = hardware.MaxLogicalSectorSize;
                break;
            default:
                blockSize = BlockSizeValue;
                break;
            }

            u32 cacheSize = static_cast<V::u32>(CacheSizeMib * 1_mib);
            if (blockSize * 2 > cacheSize)
            {
                V_Warning("Streamer", false, "Size (%u) for BlockCache isn't big enough to hold at least two cache blocks of size (%zu). "
                    "The cache size will be increased to fit 2 cache blocks.", cacheSize, blockSize);
                cacheSize = v_numeric_caster(blockSize * 2);
            }

            auto stackEntry = VStd::make_shared<BlockCache>(
                cacheSize, v_numeric_cast<V::u32>(blockSize), v_numeric_cast<V::u32>(hardware.MaxPhysicalSectorSize), false);
            stackEntry->SetNext(VStd::move(parent));
            return stackEntry;
        }


        static constexpr char CacheHitRateName[] = "Cache hit rate";
        static constexpr char CacheableName[] = "Cacheable";

        void BlockCache::Section::Prefix(const Section& section)
        {
            V_Assert(section.Used, "Trying to prefix an unused section");
            V_Assert(!Wait && !section.Wait, "Can't merge two section that are already waiting for data to be loaded.");

            if (Used)
            {
                V_Assert(BlockOffset == 0, "Unable to add a block cache to this one as this block requires an offset upon completion.");
                
                V_Assert(section.ReadOffset < ReadOffset, "The block that's being merged needs to come before this block.");
                ReadOffset = section.ReadOffset + section.BlockOffset; // Remove any alignment that might have been added.
                ReadSize += section.ReadSize - section.BlockOffset;
                
                V_Assert(section.Output < Output, "The block that's being merged needs to come before this block.");
                Output = section.Output;
                CopySize += section.CopySize;
            }
            else
            {
                Used = true;
                ReadOffset = section.ReadOffset + section.BlockOffset;
                ReadSize = section.ReadSize - section.BlockOffset;
                Output = section.Output;
                CopySize = section.CopySize;
            }
            BlockOffset = 0; // Two merged sections do not support caching.
        }
        
        BlockCache::BlockCache(u64 cacheSize, u32 blockSize, u32 alignment, bool onlyEpilogWrites)
            : StreamStackEntry("Block cache")
            , m_alignment(alignment)
            , m_onlyEpilogWrites(onlyEpilogWrites)
        {
            V_Assert(IStreamerTypes::IsPowerOf2(alignment), "Alignment needs to be a power of 2.");
            V_Assert(IStreamerTypes::IsAlignedTo(blockSize, alignment), "Block size needs to be a multiple of the alignment.");
            
            m_numBlocks = v_numeric_caster(cacheSize / blockSize);
            m_cacheSize = cacheSize - (cacheSize % blockSize); // Only use the amount needed for the cache.
            m_blockSize = blockSize;
            if (m_numBlocks == 1)
            {
                m_onlyEpilogWrites = true;
            }
            
            m_cache = reinterpret_cast<u8*>(V::AllocatorInstance<V::SystemAllocator>::Get().Allocate(
                m_cacheSize, alignment, 0, "V::IO::Streamer BlockCache", __FILE__, __LINE__));
            m_cachedPaths = VStd::unique_ptr<RequestPath[]>(new RequestPath[m_numBlocks]);
            m_cachedOffsets = VStd::unique_ptr<u64[]>(new u64[m_numBlocks]);
            m_blockLastTouched = VStd::unique_ptr<TimePoint[]>(new TimePoint[m_numBlocks]);
            m_inFlightRequests = VStd::unique_ptr<FileRequest*[]>(new FileRequest*[m_numBlocks]);
            
            ResetCache();
        }

        BlockCache::~BlockCache()
        {
            V::AllocatorInstance<V::SystemAllocator>::Get().DeAllocate(m_cache, m_cacheSize, m_alignment);
        }

        void BlockCache::QueueRequest(FileRequest* request)
        {
            V_Assert(request, "QueueRequest was provided a null request.");

            VStd::visit([this, request](auto&& args)
            {
                using Command = VStd::decay_t<decltype(args)>;
                if constexpr (VStd::is_same_v<Command, FileRequest::ReadData>)
                {
                    ReadFile(request, args);
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
                    StreamStackEntry::QueueRequest(request);
                }
            }, request->GetCommand());
        }

        bool BlockCache::ExecuteRequests()
        {
            size_t delayedCount = m_delayedSections.size();
            
            bool delayedRequestProcessed = false;
            for (size_t i = 0; i < delayedCount; ++i)
            {
                Section& delayed = m_delayedSections.front();
                V_Assert(delayed.Parent, "Delayed section doesn't have a reference to the original request.");
                auto data = VStd::get_if<FileRequest::ReadData>(&delayed.Parent->GetCommand());
                V_Assert(data, "A request in the delayed queue of the BlockCache didn't have a parent with read data.");
                // This call can add the same section to the back of the queue if there's not
                // enough space. Because of this the entry needs to be removed from the delayed
                // list no matter what the result is of ServiceFromCache.
                if (ServiceFromCache(delayed.Parent, delayed, data->Path, data->SharedRead) != CacheResult::Delayed)
                {
                    delayedRequestProcessed = true;
                }
                m_delayedSections.pop_front();
            }
            bool nextResult = StreamStackEntry::ExecuteRequests();
            return nextResult || delayedRequestProcessed;
        }

        void BlockCache::UpdateStatus(Status& status) const
        {
            StreamStackEntry::UpdateStatus(status);
            s32 numAvailableSlots = CalculateAvailableRequestSlots();
            status.NumAvailableSlots = VStd::min(status.NumAvailableSlots, numAvailableSlots);
            status.IsIdle = status.IsIdle &&
                static_cast<u32>(numAvailableSlots) == m_numBlocks &&
                m_delayedSections.empty();
        }

        void BlockCache::UpdateCompletionEstimates(VStd::chrono::system_clock::time_point now, VStd::vector<FileRequest*>& internalPending,
            StreamerContext::PreparedQueue::iterator pendingBegin, StreamerContext::PreparedQueue::iterator pendingEnd)
        {
            // Have the stack downstream estimate the completion time for the requests that are waiting for a slot to execute in.
            AddDelayedRequests(internalPending);

            StreamStackEntry::UpdateCompletionEstimates(now, internalPending, pendingBegin, pendingEnd);

            // The in-flight requests don't have to be updated because the subdivided request will bubble up in order so the final
            // write will be the latest completion time. Requests that have a wait on another request though will need to be update
            // as the estimation of the in-flight request needs to be copied to the wait request to get an accurate prediction.
            UpdatePendingRequestEstimations();

            // Technically here the wait commands for the delayed sections should be updated as well, but it's the parent that's interesting,
            // not the wait so don't waste cycles updating the wait.
        }

        void BlockCache::AddDelayedRequests(VStd::vector<FileRequest*>& internalPending)
        {
            for (auto& section : m_delayedSections)
            {
                internalPending.push_back(section.Parent);
            }
        }

        void BlockCache::UpdatePendingRequestEstimations()
        {
            for (auto it : m_pendingRequests)
            {
                Section& section = it.second;
                V_Assert(section.CacheBlockIndex != _fileNotCached, "An in-flight cache section doesn't have a cache block associated with it.");
                V_Assert(m_inFlightRequests[section.CacheBlockIndex],
                    "Cache block %i is reported as being in-flight but has no request.", section.CacheBlockIndex);
                if (section.Wait)
                {
                    V_Assert(section.Parent, "A cache section with a wait request pending is missing a parent to wait on.");
                    auto largestTime = VStd::max(section.Parent->GetEstimatedCompletion(), it.first->GetEstimatedCompletion());
                    section.Wait->SetEstimatedCompletion(largestTime);
                }
            }
        }

        void BlockCache::ReadFile(FileRequest* request, FileRequest::ReadData& data)
        {
            if (!m_next)
            {
                request->SetStatus(IStreamerTypes::RequestStatus::Failed);
                m_context->MarkRequestAsCompleted(request);
                return;
            }

            auto continueReadFile = [this, request](FileRequest& fileSizeRequest)
            {
                V_PROFILE_FUNCTION(Core);
                V_Assert(m_numMetaDataRetrievalInProgress > 0,
                    "More requests have completed meta data retrieval in the Block Cache than were requested.");
                m_numMetaDataRetrievalInProgress--;
                if (fileSizeRequest.GetStatus() == IStreamerTypes::RequestStatus::Completed)
                {
                    auto& requestInfo = VStd::get<FileRequest::FileMetaDataRetrievalData>(fileSizeRequest.GetCommand());
                    if (requestInfo.Found)
                    {
                        ContinueReadFile(request, requestInfo.FileSize);
                        return;
                    }
                }
                // Couldn't find the file size so don't try to split and pass the request to the next entry in the stack.
                StreamStackEntry::QueueRequest(request);
            };
            m_numMetaDataRetrievalInProgress++;
            FileRequest* fileSizeRequest = m_context->GetNewInternalRequest();
            fileSizeRequest->CreateFileMetaDataRetrieval(data.Path);
            fileSizeRequest->SetCompletionCallback(VStd::move(continueReadFile));
            StreamStackEntry::QueueRequest(fileSizeRequest);
        }
        void BlockCache::ContinueReadFile(FileRequest* request, u64 fileLength)
        {
            Section prolog;
            Section main;
            Section epilog;

            auto& data = VStd::get<FileRequest::ReadData>(request->GetCommand());

            if (!SplitRequest(prolog, main, epilog, data.Path, fileLength, data.Offset, data.Size,
                reinterpret_cast<u8*>(data.Output)))
            {
                m_context->MarkRequestAsCompleted(request);
                return;
            }

            if (prolog.Used || epilog.Used)
            {
                m_cacheableStat.PushSample(1.0);
                Statistic::PlotImmediate(m_name, CacheableName, m_cacheableStat.GetMostRecentSample());
            }
            else
            {
                // Nothing to cache so simply forward the call to the next entry in the stack for direct reading.
                m_cacheableStat.PushSample(0.0);
                Statistic::PlotImmediate(m_name, CacheableName, m_cacheableStat.GetMostRecentSample());
                m_next->QueueRequest(request);
                return;
            }

            bool fullyCached = true;
            if (prolog.Used)
            {
                if (m_onlyEpilogWrites && (main.Used || epilog.Used))
                {
                    // Only the epilog is allowed to write to the cache, but a previous read could
                    // still have cached the prolog, so check the cache and use the data if it's there
                    // otherwise merge the section with the main section to have the data read.
                    if (ReadFromCache(request, prolog, data.Path) == CacheResult::CacheMiss)
                    {
                        // The data isn't cached so put the prolog in front of the main section
                        // so it's read in one read request. If main wasn't used, prefixing the prolog 
                        // will cause it to be filled in and used.
                        main.Prefix(prolog);
                        m_hitRateStat.PushSample(0.0);
                        Statistic::PlotImmediate(m_name, CacheHitRateName, m_hitRateStat.GetMostRecentSample());
                    }
                    else
                    {
                        m_hitRateStat.PushSample(1.0);
                        Statistic::PlotImmediate(m_name, CacheHitRateName, m_hitRateStat.GetMostRecentSample());
                    }
                }
                else
                {
                    // If m_onlyEpilogWrites is set but main and epilog are not filled in, it means that 
                    // the request was so small it fits in one cache block, in which case the prolog and
                    // epilog are practically the same. Or this code is reached because both prolog and
                    // epilog are allowed to write.
                    bool readFromCache = (ServiceFromCache(request, prolog, data.Path, data.SharedRead) == CacheResult::ReadFromCache);
                    fullyCached = readFromCache && fullyCached;

                    m_hitRateStat.PushSample(readFromCache ? 1.0 : 0.0);
                    Statistic::PlotImmediate(m_name, CacheHitRateName, m_hitRateStat.GetMostRecentSample());
                }
            }

            if (main.Used)
            {
                FileRequest* mainRequest = m_context->GetNewInternalRequest();
                // No need for a callback as there's nothing to do after the read has been completed.
                mainRequest->CreateRead(request, main.Output, main.ReadSize, data.Path,
                    main.ReadOffset, main.ReadSize, data.SharedRead);
                m_next->QueueRequest(mainRequest);
                fullyCached = false;
            }

            if (epilog.Used)
            {
                bool readFromCache = (ServiceFromCache(request, epilog, data.Path, data.SharedRead) == CacheResult::ReadFromCache);
                fullyCached = readFromCache && fullyCached;

                m_hitRateStat.PushSample(readFromCache ? 1.0 : 0.0);
                Statistic::PlotImmediate(m_name, CacheHitRateName, m_hitRateStat.GetMostRecentSample());
            }

            if (fullyCached)
            {
                request->SetStatus(IStreamerTypes::RequestStatus::Completed);
                m_context->MarkRequestAsCompleted(request);
            }
        }

        void BlockCache::FlushCache(const RequestPath& filePath)
        {
            for (u32 i = 0; i < m_numBlocks; ++i)
            {
                if (m_cachedPaths[i] == filePath)
                {
                    ResetCacheEntry(i);
                }
            }
        }

        void BlockCache::FlushEntireCache()
        {
            ResetCache();
        }

        void BlockCache::CollectStatistics(VStd::vector<Statistic>& statistics) const
        {
            statistics.push_back(Statistic::CreatePercentage(m_name, CacheHitRateName, CalculateHitRatePercentage()));
            statistics.push_back(Statistic::CreatePercentage(m_name, CacheableName, CalculateCacheableRatePercentage()));
            statistics.push_back(Statistic::CreateInteger(m_name, "Available slots", CalculateAvailableRequestSlots()));

            StreamStackEntry::CollectStatistics(statistics);
        }

        double BlockCache::CalculateHitRatePercentage() const
        {
            return m_hitRateStat.GetAverage();
        }

        double BlockCache::CalculateCacheableRatePercentage() const
        {
            return m_cacheableStat.GetAverage();
        }

        s32 BlockCache::CalculateAvailableRequestSlots() const
        {
            return  v_numeric_cast<s32>(m_numBlocks) - m_numInFlightRequests - m_numMetaDataRetrievalInProgress -
                v_numeric_cast<s32>(m_delayedSections.size());
        }

        BlockCache::CacheResult BlockCache::ReadFromCache(FileRequest* request, Section& section, const RequestPath& filePath)
        {
            u32 cacheLocation = FindInCache(filePath, section.ReadOffset);
            if (cacheLocation != _fileNotCached)
            {
                return ReadFromCache(request, section, cacheLocation);
            }
            else
            {
                return CacheResult::CacheMiss;
            }
        }

        BlockCache::CacheResult BlockCache::ReadFromCache(FileRequest* request, Section& section, u32 cacheBlock)
        {
            if (!IsCacheBlockInFlight(cacheBlock))
            {
                TouchBlock(cacheBlock);
                memcpy(section.Output, GetCacheBlockData(cacheBlock) + section.BlockOffset, section.CopySize);
                return CacheResult::ReadFromCache;
            }
            else
            {
                V_Assert(section.Wait == nullptr, "A wait request has to be set on a block cache section, but one has already been assigned.");
                FileRequest* wait = m_context->GetNewInternalRequest();
                wait->CreateWait(request);
                section.CacheBlockIndex = cacheBlock;
                section.Parent = request;
                section.Wait = wait;
                m_pendingRequests.emplace(m_inFlightRequests[cacheBlock], section);
                return CacheResult::Queued;
            }
        }

        BlockCache::CacheResult BlockCache::ServiceFromCache(
            FileRequest* request, Section& section, const RequestPath& filePath, bool sharedRead)
        {
            V_Assert(m_next, "ServiceFromCache in BlockCache was called when the cache doesn't have a way to read files.");

            u32 cacheLocation = FindInCache(filePath, section.ReadOffset);
            if (cacheLocation == _fileNotCached)
            {
                m_hitRateStat.PushSample(0.0);
                Statistic::PlotImmediate(m_name, CacheHitRateName, m_hitRateStat.GetMostRecentSample());

                section.Parent = request;
                cacheLocation = RecycleOldestBlock(filePath, section.ReadOffset);
                if (cacheLocation != _fileNotCached)
                {
                    FileRequest* readRequest = m_context->GetNewInternalRequest();
                    readRequest->CreateRead(request, GetCacheBlockData(cacheLocation), m_blockSize, filePath, section.ReadOffset,
                        section.ReadSize, sharedRead);
                    readRequest->SetCompletionCallback([this](FileRequest& request)
                        {
                            V_PROFILE_FUNCTION(Core);
                            CompleteRead(request);
                        });
                    section.CacheBlockIndex = cacheLocation;
                    m_inFlightRequests[cacheLocation] = readRequest;
                    m_numInFlightRequests++;
                    
                    // If set, this is the wait added by the delay.
                    if (section.Wait)
                    {
                        m_context->MarkRequestAsCompleted(section.Wait);
                        section.Wait = nullptr;
                    }

                    m_pendingRequests.emplace(readRequest, section);
                    m_next->QueueRequest(readRequest);
                    return CacheResult::Queued;
                }
                else
                {
                    // There's no more space in the cache to store this request to. This is because there are more in-flight requests than 
                    // there are slots in the cache. Delay the request until there's a slot available but add a wait for the section to
                    // make sure the request can't complete if some parts are read.
                    if (!section.Wait)
                    {
                        section.Wait = m_context->GetNewInternalRequest();
                        section.Wait->CreateWait(request);
                    }
                    m_delayedSections.push_back(section);
                    return CacheResult::Delayed;
                }
            }
            else
            {
                // If set, this is the wait added by the delay when the cache was full.
                if (section.Wait)
                {
                    m_context->MarkRequestAsCompleted(section.Wait);
                    section.Wait = nullptr;
                }

                m_hitRateStat.PushSample(1.0);
                Statistic::PlotImmediate(m_name, CacheHitRateName, m_hitRateStat.GetMostRecentSample());

                return ReadFromCache(request, section, cacheLocation);
            }
        }

        void BlockCache::CompleteRead(FileRequest& request)
        {
            auto requestInfo = m_pendingRequests.equal_range(&request);
            V_Assert(requestInfo.first != requestInfo.second, "Block cache was asked to complete a file request it never queued.");

            IStreamerTypes::RequestStatus requestStatus = request.GetStatus();
            bool requestWasSuccessful = requestStatus == IStreamerTypes::RequestStatus::Completed;
            u32 cacheBlockIndex = requestInfo.first->second.CacheBlockIndex;

            for (auto it = requestInfo.first; it != requestInfo.second; ++it)
            {
                Section& section = it->second;
                V_Assert(section.CacheBlockIndex == cacheBlockIndex,
                    "Section associated with the file request is referencing the incorrect cache block (%u vs %u).", cacheBlockIndex, section.CacheBlockIndex);
                if (section.Wait)
                {
                    section.Wait->SetStatus(requestStatus);
                    m_context->MarkRequestAsCompleted(section.Wait);
                    section.Wait = nullptr;
                }

                if (requestWasSuccessful)
                {
                    memcpy(section.Output, GetCacheBlockData(cacheBlockIndex) + section.BlockOffset, section.CopySize);
                }
            }

            if (requestWasSuccessful)
            {
                TouchBlock(cacheBlockIndex);
                m_inFlightRequests[cacheBlockIndex] = nullptr;
            }
            else
            {
                ResetCacheEntry(cacheBlockIndex);
            }
            V_Assert(m_numInFlightRequests > 0, "Clearing out an in-flight request, but there shouldn't be any in flight according to records.");
            m_numInFlightRequests--;
            m_pendingRequests.erase(&request);
        }

        bool BlockCache::SplitRequest(Section& prolog, Section& main, Section& epilog,
            [[maybe_unused]] const RequestPath& filePath, u64 fileLength,
            u64 offset, u64 size, u8* buffer) const
        {
            V_Assert(offset + size <= fileLength, "File at path '%s' is being read past the end of the file.", filePath.GetRelativePath());

            //
            // Prolog
            // This looks at the request and sees if there's anything in front of the file that should be cached. This also
            // deals with the situation where the entire file request fits inside the cache which could mean there's data
            // left after the file as well that could be cached.
            //
            u64 roundedOffsetStart = V_SIZE_ALIGN_DOWN(offset, v_numeric_cast<u64>(m_blockSize));
            
            u64 blockReadSizeStart = VStd::min(fileLength - roundedOffsetStart, v_numeric_cast<u64>(m_blockSize));
            // Check if the request is on the left edge of the cache block, which means there's nothing in front of it
            // that could be cached.
            if (roundedOffsetStart == offset)
            {
                if (offset + size >= fileLength)
                {
                    // The entire (remainder) of the file is read so there's nothing to cache
                    main.ReadOffset = offset;
                    main.ReadSize = size;
                    main.Output = buffer;
                    main.Used = true;
                    return true;
                }
                else if (size < blockReadSizeStart)
                {
                    // The entire request fits inside a single cache block, but there's more file to read.
                    prolog.ReadOffset = offset;
                    prolog.ReadSize = blockReadSizeStart;
                    prolog.BlockOffset = 0;
                    prolog.Output = buffer;
                    prolog.CopySize = size;
                    prolog.Used = true;
                    return true;
                }
                // In any other case it means that the entire block would be read so caching has no effect.
            }
            else
            {
                // There is a portion of the file before that's not requested so always cache this block.
                const u64 blockOffset = offset - roundedOffsetStart;
                prolog.ReadOffset = roundedOffsetStart;
                prolog.BlockOffset = blockOffset;
                prolog.Output = buffer;
                prolog.Used = true;

                const bool isEntirelyInCache = blockOffset + size <= blockReadSizeStart;
                if (isEntirelyInCache)
                {
                    // The read size is already clamped to the file size above when blockReadSizeStart is set.
                    V_Assert(roundedOffsetStart + blockReadSizeStart <= fileLength,
                        "Read size in block cache was set to %llu but this is beyond the file length of %llu.",
                        roundedOffsetStart + blockReadSizeStart, fileLength);
                    prolog.ReadSize = blockReadSizeStart;
                    prolog.CopySize = size;

                    // There won't be anything else coming after this so continue reading.
                    return true;
                }
                else
                {
                    prolog.ReadSize = blockReadSizeStart;
                    prolog.CopySize = blockReadSizeStart - blockOffset;
                }
            }


            //
            // Epilog
            // Since the prolog already takes care of the situation where the file fits entirely in the cache the epilog is
            // much simpler as it only has to look at the case where there is more file after the request to read for caching.
            //
            u64 roundedOffsetEnd = V_SIZE_ALIGN_DOWN(offset + size, v_numeric_cast<u64>(m_blockSize));
            u64 copySize = offset + size - roundedOffsetEnd;
            u64 blockReadSizeEnd = m_blockSize;
            if ((roundedOffsetEnd + blockReadSizeEnd) > fileLength)
            {
                blockReadSizeEnd = fileLength - roundedOffsetEnd;
            }

            // If the read doesn't align with the edge of the cache
            if (copySize != 0 && copySize < blockReadSizeEnd)
            {
                epilog.ReadOffset = roundedOffsetEnd;
                epilog.ReadSize = blockReadSizeEnd;
                epilog.BlockOffset = 0;
                epilog.Output = buffer + (roundedOffsetEnd - offset);
                epilog.CopySize = copySize;
                epilog.Used = true;
            }

            //
            // Main
            // If this point is reached there's potentially a block between the prolog and epilog that can be directly read.
            //
            u64 adjustedOffset = offset;
            if (prolog.Used)
            {
                adjustedOffset += prolog.CopySize;
                size -= prolog.CopySize;
            }
            if (epilog.Used)
            {
                size -= epilog.CopySize;
            }
            V_Assert(IStreamerTypes::IsAlignedTo(adjustedOffset, m_blockSize),
                "The adjustments made by the prolog should guarantee the offset is aligned to a cache block.");
            if (size != 0)
            {
                main.ReadOffset = adjustedOffset;
                main.ReadSize = size;
                main.Output = buffer + (adjustedOffset - offset);
                main.Used = true;
            }

            return true;
        }

        u8* BlockCache::GetCacheBlockData(u32 index)
        {
            V_Assert(index < m_numBlocks, "Index for touch a cache entry in the BlockCache is out of bounds.");
            return m_cache + (index * m_blockSize);
        }

        void BlockCache::TouchBlock(u32 index)
        {
            V_Assert(index < m_numBlocks, "Index for touch a cache entry in the BlockCache is out of bounds.");
            m_blockLastTouched[index] = VStd::chrono::high_resolution_clock::now();
        }

        u32 BlockCache::RecycleOldestBlock(const RequestPath& filePath, u64 offset)
        {
            V_Assert((offset & (m_blockSize - 1)) == 0, "The offset used to recycle a block cache needs to be a multiple of the block size.");

            // Find the oldest cache block.
            TimePoint oldest = m_blockLastTouched[0];
            u32 oldestIndex = 0;
            for (u32 i = 1; i < m_numBlocks; ++i)
            {
                if (m_blockLastTouched[i] < oldest && !m_inFlightRequests[i])
                {
                    oldest = m_blockLastTouched[i];
                    oldestIndex = i;
                }
            }

            if (!IsCacheBlockInFlight(oldestIndex))
            {
                // Recycle the block.
                m_cachedPaths[oldestIndex] = filePath;
                m_cachedOffsets[oldestIndex] = offset;
                TouchBlock(oldestIndex);
                return oldestIndex;
            }
            else
            {
                return _fileNotCached;
            }
        }

        u32 BlockCache::FindInCache(const RequestPath& filePath, u64 offset) const
        {
            V_Assert((offset & (m_blockSize - 1)) == 0, "The offset used to find a block in the block cache needs to be a multiple of the block size.");
            for (u32 i = 0; i < m_numBlocks; ++i)
            {
                if (m_cachedPaths[i] == filePath && m_cachedOffsets[i] == offset)
                {
                    return i;
                }
            }

            return _fileNotCached;
        }

        bool BlockCache::IsCacheBlockInFlight(u32 index) const
        {
            V_Assert(index < m_numBlocks, "Index for checking if a cache block is in flight is out of bounds.");
            return m_inFlightRequests[index] != nullptr;
        }

        void BlockCache::ResetCacheEntry(u32 index)
        {
            V_Assert(index < m_numBlocks, "Index for resetting a cache entry in the BlockCache is out of bounds.");

            m_cachedPaths[index].Clear();
            m_cachedOffsets[index] = 0;
            m_blockLastTouched[index] = TimePoint::min();
            m_inFlightRequests[index] = nullptr;
        }

        void BlockCache::ResetCache()
        {
            for (u32 i = 0; i < m_numBlocks; ++i)
            {
                ResetCacheEntry(i);
            }
            m_numInFlightRequests = 0;
        }
    } // namespace IO
} // namespace AZ
