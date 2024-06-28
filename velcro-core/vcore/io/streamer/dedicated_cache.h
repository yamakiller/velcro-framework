#ifndef V_FRAMEWORK_CORE_IO_STREAMER_DEDICATEDCACHE_H
#define V_FRAMEWORK_CORE_IO_STREAMER_DEDICATEDCACHE_H

#include <vcore/io/streamer/block_cache.h>
#include <vcore/io/streamer/file_range.h>
#include <vcore/io/streamer/statistics.h>
#include <vcore/io/streamer/streamer_configuration.h>
#include <vcore/io/streamer/stream_stack_entry.h>
#include <vcore/memory/system_allocator.h>
#include <vcore/std/limits.h>
#include <vcore/std/containers/vector.h>
#include <vcore/std/smart_ptr/unique_ptr.h>

namespace V
{
    namespace IO
    {
        struct DedicatedCacheConfig final :
            public IStreamerStackConfig
        {
            VOBJECT_RTTI(V::IO::DedicatedCacheConfig, "{DF0F6029-02B0-464C-9846-524654335BCC}", IStreamerStackConfig);
            V_CLASS_ALLOCATOR(DedicatedCacheConfig, V::SystemAllocator, 0);

            ~DedicatedCacheConfig() override = default;
            VStd::shared_ptr<StreamStackEntry> AddStreamStackEntry(
                const HardwareInformation& hardware, VStd::shared_ptr<StreamStackEntry> parent) override;
            

            //! The size of the individual blocks inside the cache.
            BlockCacheConfig::BlockSize BlockSizeValue{ BlockCacheConfig::BlockSize::MemoryAlignment };
            //! The overall size of the cache in megabytes.
            u32 CacheSizeMib{ 8 };
            //! If true, only the epilog is written otherwise the prolog and epilog are written. In either case both prolog and epilog are read.
            //! For uses of the cache that read mostly sequentially this flag should be set to true. If reads are more random than it's better
            //! to set this flag to false.
            bool WriteOnlyEpilog{ true };
        };

        class DedicatedCache
            : public StreamStackEntry
        {
        public:
            DedicatedCache(u64 cacheSize, u32 blockSize, u32 alignment, bool onlyEpilogWrites);
            
            void SetNext(VStd::shared_ptr<StreamStackEntry> next) override;
            void SetContext(StreamerContext& context) override;

            void PrepareRequest(FileRequest* request) override;
            void QueueRequest(FileRequest* request) override;
            bool ExecuteRequests() override;

            void UpdateStatus(Status& status) const override;

            void UpdateCompletionEstimates(VStd::chrono::system_clock::time_point now, VStd::vector<FileRequest*>& internalPending,
                StreamerContext::PreparedQueue::iterator pendingBegin, StreamerContext::PreparedQueue::iterator pendingEnd) override;

            void CollectStatistics(VStd::vector<Statistic>& statistics) const override;

        private:
            void CreateDedicatedCache(FileRequest* request, FileRequest::CreateDedicatedCacheData& data);
            void DestroyDedicatedCache(FileRequest* request, FileRequest::DestroyDedicatedCacheData& data);

            void ReadFile(FileRequest* request, FileRequest::ReadData& data);
            size_t FindCache(const RequestPath& filename, FileRange range);
            size_t FindCache(const RequestPath& filename, u64 offset);

            void FlushCache(const RequestPath& filePath);
            void FlushEntireCache();

            VStd::vector<RequestPath> m_cachedFileNames;
            VStd::vector<FileRange> m_cachedFileRanges;
            VStd::vector<VStd::unique_ptr<BlockCache>> m_cachedFileCaches;
            VStd::vector<size_t> m_cachedFileRefCounts;

            V::Statistics::RunningStatistic m_usagePercentageStat;
#if V_STREAMER_ADD_EXTRA_PROFILING_INFO
            V::Statistics::RunningStatistic m_overallHitRateStat;
            V::Statistics::RunningStatistic m_overallCacheableRateStat;
#endif

            u64 m_cacheSize;
            u32 m_alignment;
            u32 m_blockSize;
            bool m_onlyEpilogWrites;
        };
    } // namespace IO
} // namespace V



#endif // V_FRAMEWORK_CORE_IO_STREAMER_DEDICATEDCACHE_H