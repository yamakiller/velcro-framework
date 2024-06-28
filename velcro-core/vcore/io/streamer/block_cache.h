#ifndef V_FRAMEWORK_CORE_IO_STREAMER_BLOCKCACHE_H
#define V_FRAMEWORK_CORE_IO_STREAMER_BLOCKCACHE_H
 
#include <vcore/io/streamer/statistics.h>
#include <vcore/io/streamer/streamer_configuration.h>
#include <vcore/io/streamer/stream_stack_entry.h>
#include <vcore/memory/system_allocator.h>
#include <vcore/statistics/running_statistic.h>
#include <vcore/std/limits.h>
#include <vcore/std/chrono/clocks.h>
#include <vcore/std/containers/unordered_map.h>
#include <vcore/std/containers/deque.h>
#include <vcore/std/smart_ptr/unique_ptr.h>

namespace V
{
    namespace IO
    {
        struct BlockCacheConfig final :
            public IStreamerStackConfig
        {
            VOBJECT_RTTI(V::IO::BlockCacheConfig, "{c56a0c9b-71b4-44de-9e06-1a2a2e6df399}", IStreamerStackConfig);
            V_CLASS_ALLOCATOR(BlockCacheConfig, V::SystemAllocator, 0);

            ~BlockCacheConfig() override = default;
            VStd::shared_ptr<StreamStackEntry> AddStreamStackEntry(
                const HardwareInformation& hardware, VStd::shared_ptr<StreamStackEntry> parent) override;
            

            //! Dynamic options for the blocks size.
            //! It's possible to set static sizes or use the names from this enum to have V::IO::Streamer automatically fill in the sizes.
            //! Fixed sizes are set through the Settings Registry with "BlockSize": 524288, while dynamic values are set like
            //! "BlockSize": "MemoryAlignment". In the latter case V::IO::Streamer will use the available hardware information and fill
            //! in the actual value.
            enum BlockSize : u32
            {
                MaxTransfer = VStd::numeric_limits<u32>::max(), //!< The largest possible block size.
                MemoryAlignment = MaxTransfer - 1, //!< The size of the minimal memory requirement of the storage device.
                SizeAlignment = MemoryAlignment - 1 //!< The minimal read size required by the storage device.
            };

            //! The overall size of the cache in megabytes.
            u32 CacheSizeMib{ 8 };
            //! The size of the individual blocks inside the cache.
            BlockSize BlockSizeValue{ BlockSize::MemoryAlignment };
        };

        class BlockCache
            : public StreamStackEntry
        {
        public:
            BlockCache(u64 cacheSize, u32 blockSize, u32 alignment, bool onlyEpilogWrites);
            BlockCache(BlockCache&& rhs) = delete;
            BlockCache(const BlockCache& rhs) = delete;
            ~BlockCache() override;

            BlockCache& operator=(BlockCache&& rhs) = delete;
            BlockCache& operator=(const BlockCache& rhs) = delete;

            void QueueRequest(FileRequest* request) override;
            bool ExecuteRequests() override;

            void UpdateStatus(Status& status) const override;
            void UpdateCompletionEstimates(VStd::chrono::system_clock::time_point now, VStd::vector<FileRequest*>& internalPending,
                StreamerContext::PreparedQueue::iterator pendingBegin, StreamerContext::PreparedQueue::iterator pendingEnd) override;
            void AddDelayedRequests(VStd::vector<FileRequest*>& internalPending);
            void UpdatePendingRequestEstimations();

            void FlushCache(const RequestPath& filePath);
            void FlushEntireCache();

            void CollectStatistics(VStd::vector<Statistic>& statistics) const override;

            double CalculateHitRatePercentage() const;
            double CalculateCacheableRatePercentage() const;
            s32 CalculateAvailableRequestSlots() const;

        protected:
            static constexpr u32 _fileNotCached = static_cast<u32>(-1);

            enum class CacheResult
            {
                ReadFromCache, //!< Data was found in the cache and reused.
                CacheMiss, //!< Data wasn't found in the cache and no sub request was queued.
                Queued, //!< A sub request was created or appended and queued for processing on the next entry in the streamer stack.
                Delayed //!< There's no more room to queue a new request, so delay the request until a slot becomes available.
            };

            struct Section
            {
                u8* Output{ nullptr }; //!< The buffer to write the data to.
                FileRequest* Parent{ nullptr }; //!< If set, the file request that is split up by this section.
                FileRequest* Wait{ nullptr }; //!< If set, this contains a "wait"-operation that blocks an operation chain from continuing until this section has been loaded.
                u64 ReadOffset{ 0 }; //!< Offset into the file to start reading from.
                u64 ReadSize{ 0 }; //!< Number of bytes to read from file.
                u64 BlockOffset{ 0 }; //!< Offset into the cache block to start copying from.
                u64 CopySize{ 0 }; //!< Number of bytes to copy from cache.
                u32 CacheBlockIndex{ _fileNotCached }; //!< If assigned, the index of the cache block assigned to this section.
                bool Used{ false }; //!< Whether or not this section is used in further processing.

                // Add the provided section in front of this one.
                void Prefix(const Section& section);
            };

            using TimePoint = VStd::chrono::system_clock::time_point;

            void ReadFile(FileRequest* request, FileRequest::ReadData& data);
            void ContinueReadFile(FileRequest* request, u64 fileLength);
            CacheResult ReadFromCache(FileRequest* request, Section& section, const RequestPath& filePath);
            CacheResult ReadFromCache(FileRequest* request, Section& section, u32 cacheBlock);
            CacheResult ServiceFromCache(FileRequest* request, Section& section, const RequestPath& filePath, bool sharedRead);
            void CompleteRead(FileRequest& request);
            bool SplitRequest(Section& prolog, Section& main, Section& epilog, const RequestPath& filePath, u64 fileLength,
                u64 offset, u64 size, u8* buffer) const;

            u8* GetCacheBlockData(u32 index);
            void TouchBlock(u32 index);
            V::u32 RecycleOldestBlock(const RequestPath& filePath, u64 offset);
            u32 FindInCache(const RequestPath& filePath, u64 offset) const;
            bool IsCacheBlockInFlight(u32 index) const;
            void ResetCacheEntry(u32 index);
            void ResetCache();

            //! Map of the file requests that are being processed and the sections of the parent requests they'll complete.
            VStd::unordered_multimap<FileRequest*, Section> m_pendingRequests;
            //! List of file sections that were delayed because the cache was full.
            VStd::deque<Section> m_delayedSections;

            V::Statistics::RunningStatistic m_hitRateStat;
            V::Statistics::RunningStatistic m_cacheableStat;

            u8* m_cache;
            u64 m_cacheSize;
            u32 m_blockSize;
            u32 m_alignment;
            u32 m_numBlocks;
            s32 m_numInFlightRequests{ 0 };
            //! The file path associated with a cache block.
            VStd::unique_ptr<RequestPath[]> m_cachedPaths; // Array of m_numBlocks size.
            //! The offset into the file the cache blocks starts at.
            VStd::unique_ptr<u64[]> m_cachedOffsets; // Array of m_numBlocks size.
            //! The last time the cache block was read from.
            VStd::unique_ptr<TimePoint[]> m_blockLastTouched; // Array of m_numBlocks size.
            //! The file request that's currently read data into the cache block. If null, the block has been read.
            VStd::unique_ptr<FileRequest*[]> m_inFlightRequests; // Array of m_numbBlocks size.
            
            //! The number of requests waiting for meta data to be retrieved.
            s32 m_numMetaDataRetrievalInProgress{ 0 };
            //! Whether or not only the epilog ever writes to the cache.
            bool m_onlyEpilogWrites;
        };
    } // namespace IO

    VOBJECT_SPECIALIZE(V::IO::BlockCacheConfig::BlockSize, "{5D4D597D-4605-462D-A27D-8046115C5381}");
} // namespace V

#endif // V_FRAMEWORK_CORE_IO_STREAMER_BLOCKCACHE_H