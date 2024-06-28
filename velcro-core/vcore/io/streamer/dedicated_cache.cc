#include <vcore/io/streamer/file_request.h>
#include <vcore/io/streamer/dedicated_cache.h>
#include <vcore/std/string/string.h>
#include <vcore/std/smart_ptr/make_shared.h>

namespace V
{
    namespace IO
    {
        VStd::shared_ptr<StreamStackEntry> DedicatedCacheConfig::AddStreamStackEntry(
            const HardwareInformation& hardware, VStd::shared_ptr<StreamStackEntry> parent)
        {
            size_t blockSize;
            switch (BlockSizeValue)
            {
            case BlockCacheConfig::BlockSize::MaxTransfer:
                blockSize = hardware.MaxTransfer;
                break;
            case BlockCacheConfig::BlockSize::MemoryAlignment:
                blockSize = hardware.MaxPhysicalSectorSize;
                break;
            case BlockCacheConfig::BlockSize::SizeAlignment:
                blockSize = hardware.MaxLogicalSectorSize;
                break;
            default:
                blockSize = BlockSizeValue;
                break;
            }

            u32 cacheSize = static_cast<V::u32>(CacheSizeMib * 1_mib);
            if (blockSize > cacheSize)
            {
                V_Warning("Streamer", false, "Size (%u) for DedicatedCache isn't big enough to hold at least one cache blocks of size (%zu). "
                    "The cache size will be increased to fit one cache block.", cacheSize, blockSize);
                cacheSize = v_numeric_caster(blockSize);
            }

            auto stackEntry = VStd::make_shared<DedicatedCache>(
                cacheSize, v_numeric_cast<V::u32>(blockSize), v_numeric_cast<V::u32>(hardware.MaxPhysicalSectorSize), WriteOnlyEpilog);
            stackEntry->SetNext(VStd::move(parent));
            return stackEntry;
        }



        //
        // DedicatedCache
        //

        DedicatedCache::DedicatedCache(u64 cacheSize, u32 blockSize, u32 alignment, bool onlyEpilogWrites)
            : StreamStackEntry("Dedicated cache")
            , m_cacheSize(cacheSize)
            , m_alignment(alignment)
            , m_blockSize(blockSize)
            , m_onlyEpilogWrites(onlyEpilogWrites)
        {
        }

        void DedicatedCache::SetNext(VStd::shared_ptr<StreamStackEntry> next)
        {
            m_next = VStd::move(next);
            for (VStd::unique_ptr<BlockCache>& cache : m_cachedFileCaches)
            {
                cache->SetNext(m_next);
            }
        }

        void DedicatedCache::SetContext(StreamerContext& context)
        {
            StreamStackEntry::SetContext(context);
            for (VStd::unique_ptr<BlockCache>& cache : m_cachedFileCaches)
            {
                cache->SetContext(context);
            }
        }

        void DedicatedCache::PrepareRequest(FileRequest* request)
        {
            V_Assert(request, "PrepareRequest was provided a null request.");

            // Claim the requests so other entries can't claim it and make updates.
            VStd::visit([this, request](auto&& args)
            {
                using Command = VStd::decay_t<decltype(args)>;
                if constexpr (VStd::is_same_v<Command, FileRequest::CreateDedicatedCacheData>)
                {
                    args.Range = FileRange::CreateRangeForEntireFile();
                    m_context->PushPreparedRequest(request);
                }
                else if constexpr (VStd::is_same_v<Command, FileRequest::DestroyDedicatedCacheData>)
                {
                    args.Range = FileRange::CreateRangeForEntireFile();
                    m_context->PushPreparedRequest(request);
                }
                else
                {
                    StreamStackEntry::PrepareRequest(request);
                }
            }, request->GetCommand());
        }

        void DedicatedCache::QueueRequest(FileRequest* request)
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
                else if constexpr (VStd::is_same_v<Command, FileRequest::CreateDedicatedCacheData>)
                {
                    CreateDedicatedCache(request, args);
                    return;
                }
                else if constexpr (VStd::is_same_v<Command, FileRequest::DestroyDedicatedCacheData>)
                {
                    DestroyDedicatedCache(request, args);
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

        bool DedicatedCache::ExecuteRequests()
        {
            bool hasProcessedRequest = false;
            for (VStd::unique_ptr<BlockCache>& cache : m_cachedFileCaches)
            {
                hasProcessedRequest = cache->ExecuteRequests() || hasProcessedRequest;
            }
            return StreamStackEntry::ExecuteRequests() || hasProcessedRequest;
        }

        void DedicatedCache::UpdateStatus(Status& status) const
        {
            // Available slots are not updated because the dedicated caches are often
            // small and specific to a tiny subset of files that are loaded. It would therefore
            // return a small number of slots that would needlessly hamper streaming as it doesn't
            // apply to the majority of files.

            bool isIdle = true;
            for (auto& cache : m_cachedFileCaches)
            {
                Status blockStatus;
                cache->UpdateStatus(blockStatus);
                isIdle = isIdle && blockStatus.IsIdle;
            }
            status.IsIdle = status.IsIdle && isIdle;
            StreamStackEntry::UpdateStatus(status);
        }

        void DedicatedCache::UpdateCompletionEstimates(VStd::chrono::system_clock::time_point now,
            VStd::vector<FileRequest*>& internalPending, StreamerContext::PreparedQueue::iterator pendingBegin,
            StreamerContext::PreparedQueue::iterator pendingEnd)
        {
            for (auto& cache : m_cachedFileCaches)
            {
                cache->AddDelayedRequests(internalPending);
            }

            StreamStackEntry::UpdateCompletionEstimates(now, internalPending, pendingBegin, pendingEnd);

            for (auto& cache : m_cachedFileCaches)
            {
                cache->UpdatePendingRequestEstimations();
            }
        }

        void DedicatedCache::ReadFile(FileRequest* request, FileRequest::ReadData& data)
        {
            size_t index = FindCache(data.Path, data.Offset);
            if (index == _fileNotFound)
            {
                m_usagePercentageStat.PushSample(0.0);
                if (m_next)
                {
                    m_next->QueueRequest(request);
                }
            }
            else
            {
                m_usagePercentageStat.PushSample(1.0);
                BlockCache& cache = *m_cachedFileCaches[index];
                cache.QueueRequest(request);
#if AZ_STREAMER_ADD_EXTRA_PROFILING_INFO
                m_overallHitRateStat.PushSample(cache.CalculateHitRatePercentage());
                m_overallCacheableRateStat.PushSample(cache.CalculateCacheableRatePercentage());
#endif
            }
        }

        void DedicatedCache::FlushCache(const RequestPath& filePath)
        {
            size_t count = m_cachedFileNames.size();
            for (size_t i = 0; i < count; ++i)
            {
                if (m_cachedFileNames[i] == filePath)
                {
                    // Flush the entire block cache as it's entirely dedicated to the found file.
                    m_cachedFileCaches[i]->FlushEntireCache();
                }
            }
        }

        void DedicatedCache::FlushEntireCache()
        {
            for (VStd::unique_ptr<BlockCache>& cache : m_cachedFileCaches)
            {
                cache->FlushEntireCache();
            }
        }

        void DedicatedCache::CollectStatistics(VStd::vector<Statistic>& statistics) const
        {
            statistics.push_back(Statistic::CreatePercentage(m_name, "Reads from dedicated cache", m_usagePercentageStat.GetAverage()));
#if AZ_STREAMER_ADD_EXTRA_PROFILING_INFO
            statistics.push_back(Statistic::CreatePercentage(m_name, "Overall cacheable rate", m_overallCacheableRateStat.GetAverage()));
            statistics.push_back(Statistic::CreatePercentage(m_name, "Overall hit rate", m_overallHitRateStat.GetAverage()));
#endif
            statistics.push_back(Statistic::CreateInteger(m_name, "Num dedicated caches", v_numeric_caster(m_cachedFileNames.size())));
            StreamStackEntry::CollectStatistics(statistics);
        }

        void DedicatedCache::CreateDedicatedCache(FileRequest* request, FileRequest::CreateDedicatedCacheData& data)
        {
            size_t index = FindCache(data.Path, data.Range);
            if (index == _fileNotFound)
            {
                index = m_cachedFileCaches.size();
                m_cachedFileNames.push_back(data.Path);
                m_cachedFileRanges.push_back(data.Range);
                m_cachedFileCaches.push_back(VStd::make_unique<BlockCache>(m_cacheSize, m_blockSize, m_alignment, m_onlyEpilogWrites));
                m_cachedFileCaches[index]->SetNext(m_next);
                m_cachedFileCaches[index]->SetContext(*m_context);
                m_cachedFileRefCounts.push_back(1);
            }
            else
            {
                ++m_cachedFileRefCounts[index];
            }
            request->SetStatus(IStreamerTypes::RequestStatus::Completed);
            m_context->MarkRequestAsCompleted(request);
        }

        void DedicatedCache::DestroyDedicatedCache(FileRequest* request, FileRequest::DestroyDedicatedCacheData& data)
        {
            size_t index = FindCache(data.Path, data.Range);
            if (index != _fileNotFound)
            {
                if (m_cachedFileRefCounts[index] > 0)
                {
                    --m_cachedFileRefCounts[index];
                    if (m_cachedFileRefCounts[index] == 0)
                    {
                        m_cachedFileNames.erase(m_cachedFileNames.begin() + index);
                        m_cachedFileRanges.erase(m_cachedFileRanges.begin() + index);
                        m_cachedFileCaches.erase(m_cachedFileCaches.begin() + index);
                        m_cachedFileRefCounts.erase(m_cachedFileRefCounts.begin() + index);
                    }
                    request->SetStatus(IStreamerTypes::RequestStatus::Completed);
                    m_context->MarkRequestAsCompleted(request);
                    return;
                }
            }
            request->SetStatus(IStreamerTypes::RequestStatus::Failed);
            m_context->MarkRequestAsCompleted(request);
        }

        size_t DedicatedCache::FindCache(const RequestPath& filename, FileRange range)
        {
            size_t count = m_cachedFileNames.size();
            for (size_t i = 0; i < count; ++i)
            {
                if (m_cachedFileNames[i] == filename && m_cachedFileRanges[i] == range)
                {
                    return i;
                }
            }
            return _fileNotFound;
        }

        size_t DedicatedCache::FindCache(const RequestPath& filename, u64 offset)
        {
            size_t count = m_cachedFileNames.size();
            for (size_t i = 0; i < count; ++i)
            {
                if (m_cachedFileNames[i] == filename && m_cachedFileRanges[i].IsInRange(offset))
                {
                    return i;
                }
            }
            return _fileNotFound;
        }
    } // namespace IO
} // namespace V
