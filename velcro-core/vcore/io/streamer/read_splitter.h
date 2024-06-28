#ifndef V_FRAMEWORK_CORE_IO_STREAMER_READ_SPLITTER_H
#define V_FRAMEWORK_CORE_IO_STREAMER_READ_SPLITTER_H

#include <vcore/IO/streamer/statistics.h>
#include <vcore/IO/streamer/stream_stack_entry.h>
#include <vcore/memory/system_allocator.h>
#include <vcore/statistics/running_statistic.h>
#include <vcore/io/streamer/streamer_configuration.h>
#include <vcore/std/containers/deque.h>
#include <vcore/std/containers/vector.h>
#include <vcore/std/limits.h>

namespace V
{
    namespace IO
    {
        struct ReadSplitterConfig final :  public IStreamerStackConfig
        {
            VOBJECT_RTTI(V::IO::ReadSplitterConfig, "{c1b32eed-471e-4c53-b7db-bfacd7440b24}", IStreamerStackConfig);
            V_CLASS_ALLOCATOR(ReadSplitterConfig, V::SystemAllocator, 0);

            ~ReadSplitterConfig() override = default;
            VStd::shared_ptr<StreamStackEntry> AddStreamStackEntry(
                const HardwareInformation& hardware, VStd::shared_ptr<StreamStackEntry> parent) override;


            //! Dynamic options for the split size.
            //! It's possible to set static sizes or use the names from this enum to have AZ::IO::Streamer automatically fill in the sizes.
            //! Fixed sizes are set through the Settings Registry with "SplitSize": 524288, while dynamic values are set like
            //! "SplitSize": "MemoryAlignment". In the latter case AZ::IO::Streamer will use the available hardware information and fill
            //! in the actual value.
            enum SplitSize : u32
            {
                MaxTransfer = VStd::numeric_limits<u32>::max(),
                MemoryAlignment = MaxTransfer - 1 //!< The size of the minimal memory requirement of the storage device.
            };

            //! The size of the internal buffer that's used if reads need to be aligned.
            u32 m_bufferSizeMib{ 5 };
            //! The size at which reads are split. This can either be a fixed value that's explicitly supplied or a
            //! dynamic value that's retrieved from the provided hardware.
            SplitSize m_splitSize{ 20 };
            //! If set to true the read splitter will adjust offsets to align to the required size alignment. This should
            //! be disabled if the read splitter is in front of a cache like the block cache as it would negate the cache's
            //! ability to cache data.
            bool m_adjustOffset{ true };
            //! Whether or not to split reads even if they meet the alignment requirements. This is recommended for devices that
            //! can't cancel their requests.
            bool m_splitAlignedRequests{ true };
        };

        class ReadSplitter
            : public StreamStackEntry
        {
        public:
             ReadSplitter(u64 maxReadSize, u32 memoryAlignment, u32 sizeAlignment, size_t bufferSize,
                 bool adjustOffset, bool splitAlignedRequests);
            ~ReadSplitter() override;

            void QueueRequest(FileRequest* request) override;
            void UpdateStatus(Status& status) const override;

            void CollectStatistics(VStd::vector<Statistic>& statistics) const override;

        private:
            struct PendingRead
            {
                FileRequest* Request{ nullptr };
                FileRequest* Wait{ nullptr };
                u8* Output{ nullptr };
                u64 OutputSize{ 0 };
                u64 ReadSize{ 0 };
                u64 Offset : 63;
                u64 IsBuffered : 1;
            };

            struct BufferCopyInformation
            {
                u8* Target{ nullptr };
                size_t Size{ 0 };
                size_t BufferOffset{ 0 };
            };

            void QueueAlignedRead(FileRequest* request);
            bool QueueAlignedRead(PendingRead& pending);
            void QueueBufferedRead(FileRequest* request);
            bool QueueBufferedRead(PendingRead& pending);
            void QueuePendingRequest();
            
            void InitializeBuffer();
            u8* GetBufferSlot(size_t index);

            V::Statistics::RunningStatistic m_averageNumSubReadsStat;
            V::Statistics::RunningStatistic m_alignedReadsStat;
            VStd::unique_ptr<BufferCopyInformation[]> m_bufferCopyInformation;
            VStd::deque<PendingRead> m_pendingReads;
            VStd::vector<u32> m_availableBufferSlots;
            u8* m_buffer;
            size_t m_bufferSize;
            u64 m_maxReadSize;
            u32 m_memoryAlignment;
            u32 m_sizeAlignment;
            bool m_adjustOffset;
            bool m_splitAlignedRequests;
        };
    } // namespace IO
    VOBJECT_SPECIALIZE(V::IO::ReadSplitterConfig::SplitSize, "{bd6c775e-31b2-4e75-896c-79021e9ddd46}");
} // namesapce V


#endif // V_FRAMEWORK_CORE_IO_STREAMER_READ_SPLITTER_H