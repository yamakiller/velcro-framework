#include <vcore/casting/numeric_cast.h>
#include <vcore/debug/profiler.h>
#include <vcore/io/streamer/file_request.h>
#include <vcore/io/Streamer/read_splitter.h>
#include <vcore/io/Streamer/streamer_context.h>
#include <vcore/std/smart_ptr/make_shared.h>

namespace V
{
    namespace IO
    {
        VStd::shared_ptr<StreamStackEntry> ReadSplitterConfig::AddStreamStackEntry(
            const HardwareInformation& hardware, VStd::shared_ptr<StreamStackEntry> parent)
        {
            size_t splitSize;
            switch (m_splitSize)
            {
            case SplitSize::MaxTransfer:
                splitSize = hardware.MaxTransfer;
                break;
            case SplitSize::MemoryAlignment:
                splitSize = hardware.MaxPhysicalSectorSize;
                break;
            default:
                splitSize = m_splitSize;
                break;
            }

            size_t bufferSize = m_bufferSizeMib * 1_mib;
            if (bufferSize < splitSize)
            {
                V_Warning("Streamer", false, "The buffer size for the Read Splitter is smaller than the individual split size. "
                    "It will be increased to fit at least one split.");
                bufferSize = splitSize;
            }

            auto stackEntry = VStd::make_shared<ReadSplitter>(
                splitSize,
                v_numeric_caster(hardware.MaxPhysicalSectorSize),
                v_numeric_caster(hardware.MaxLogicalSectorSize),
                bufferSize, m_adjustOffset, m_splitAlignedRequests);
            stackEntry->SetNext(VStd::move(parent));
            return stackEntry;
        }
        static constexpr char AvgNumSubReadsName[] = "Avg. num sub reads";
        static constexpr char AlignedReadsName[] = "Aligned reads";
        static constexpr char NumAvailableBufferSlotsName[] = "Num available buffer slots";
        static constexpr char NumPendingReadsName[] = "Num pending reads";

        ReadSplitter::ReadSplitter(u64 maxReadSize, u32 memoryAlignment, u32 sizeAlignment, size_t bufferSize,
            bool adjustOffset, bool splitAlignedRequests)
            : StreamStackEntry("Read splitter")
            , m_buffer(nullptr)
            , m_bufferSize(bufferSize)
            , m_maxReadSize(maxReadSize)
            , m_memoryAlignment(memoryAlignment)
            , m_sizeAlignment(sizeAlignment)
            , m_adjustOffset(adjustOffset)
            , m_splitAlignedRequests(splitAlignedRequests)
        {
            V_Assert(IStreamerTypes::IsPowerOf2(memoryAlignment), "Memory alignment needs to be a power of 2");
            V_Assert(IStreamerTypes::IsPowerOf2(sizeAlignment), "Size alignment needs to be a power of 2");
            V_Assert(IStreamerTypes::IsAlignedTo(maxReadSize, sizeAlignment),
                "Maximum read size isn't aligned to a multiple of the size alignment.");

            size_t numBufferSlots = bufferSize / maxReadSize;
            // Don't divide the reads up in more sub-reads than there are dependencies available.
            numBufferSlots = VStd::min(numBufferSlots, FileRequest::GetMaxNumDependencies());
            m_bufferCopyInformation = VStd::unique_ptr<BufferCopyInformation[]>(new BufferCopyInformation[numBufferSlots]);
            m_availableBufferSlots.reserve(numBufferSlots);
            for (u32 i = v_numeric_caster(numBufferSlots); i > 0; --i)
            {
                m_availableBufferSlots.push_back(i - 1);
            }
        }

        ReadSplitter::~ReadSplitter()
        {
            if (m_buffer)
            {
                V::AllocatorInstance<V::SystemAllocator>::Get().DeAllocate(m_buffer, m_bufferSize, m_memoryAlignment);
            }
        }

        void ReadSplitter::QueueRequest(FileRequest* request)
        {
            V_Assert(request, "QueueRequest was provided a null request.");
            if (!m_next)
            {
                request->SetStatus(IStreamerTypes::RequestStatus::Failed);
                m_context->MarkRequestAsCompleted(request);
                return;
            }

            auto data = VStd::get_if<FileRequest::ReadData>(&request->GetCommand());
            if (data == nullptr)
            {
                StreamStackEntry::QueueRequest(request);
                return;
            }

            m_averageNumSubReadsStat.PushSample(v_numeric_cast<double>((data->Size / m_maxReadSize) + 1));
            Statistic::PlotImmediate(m_name, AvgNumSubReadsName, m_averageNumSubReadsStat.GetMostRecentSample());

            bool isAligned = IStreamerTypes::IsAlignedTo(data->Output, m_memoryAlignment);
            if (m_adjustOffset)
            {
                isAligned = isAligned && IStreamerTypes::IsAlignedTo(data->Offset, m_sizeAlignment);
            }

            if (isAligned || m_bufferSize == 0)
            {
                m_alignedReadsStat.PushSample(isAligned ? 1.0 : 0.0);
                if (!m_splitAlignedRequests)
                {
                    StreamStackEntry::QueueRequest(request);
                }
                else
                {
                    QueueAlignedRead(request);
                }
            }
            else
            {
                m_alignedReadsStat.PushSample(0.0);
                InitializeBuffer();
                QueueBufferedRead(request);
            }
        }

        void ReadSplitter::QueueAlignedRead(FileRequest* request)
        {
            auto data = VStd::get_if<FileRequest::ReadData>(&request->GetCommand());
            V_Assert(data != nullptr, "Provided request to queue by the Read Splitter did not contain a read command.");

            if (data->Size <= m_maxReadSize)
            {
                StreamStackEntry::QueueRequest(request);
                return;
            }

            PendingRead pendingRead;
            pendingRead.Request = request;
            pendingRead.Output = reinterpret_cast<u8*>(data->Output);
            pendingRead.OutputSize = data->OutputSize;
            pendingRead.ReadSize = data->Size;
            pendingRead.Offset = data->Offset;
            pendingRead.IsBuffered = false;

            if (!m_pendingReads.empty())
            {
                m_pendingReads.push_back(pendingRead);
                return;
            }

            if (!QueueAlignedRead(pendingRead))
            {
                m_pendingReads.push_back(pendingRead);
            }
        }

        bool ReadSplitter::QueueAlignedRead(PendingRead& pending)
        {
            auto data = VStd::get_if<FileRequest::ReadData>(&pending.Request->GetCommand());
            V_Assert(data != nullptr, "Provided request to queue by the Read Splitter did not contain a read command.");

            while (pending.ReadSize > 0)
            {
                if (pending.Request->GetNumDependencies() >= FileRequest::GetMaxNumDependencies())
                {
                    // Add a wait to make sure the read request isn't completed if all sub-reads completed before
                    // the ReadSplitter has had a chance to add new sub-reads to complete the read.
                    if (pending.Wait == nullptr)
                    {
                        pending.Wait = m_context->GetNewInternalRequest();
                        pending.Wait->CreateWait(pending.Request);
                    }
                    return false;
                }

                u64 readSize = m_maxReadSize;
                size_t bufferSize = m_maxReadSize;
                if (pending.ReadSize < m_maxReadSize)
                {
                    readSize = pending.ReadSize;
                    // This will be the last read so give the remainder of the output buffer to the final request.
                    bufferSize = pending.OutputSize;
                }
                
                FileRequest* subRequest = m_context->GetNewInternalRequest();
                subRequest->CreateRead(pending.Request, pending.Output, bufferSize, data->Path, pending.Offset, readSize, data->SharedRead);
                subRequest->SetCompletionCallback([this](FileRequest&) {
                        V_PROFILE_FUNCTION(Core);
                        QueuePendingRequest();
                });
                m_next->QueueRequest(subRequest);

                pending.Offset += readSize;
                pending.ReadSize -= readSize;
                pending.OutputSize -= bufferSize;
                pending.Output += readSize;
            }
            if (pending.Wait != nullptr)
            {
                m_context->MarkRequestAsCompleted(pending.Wait);
                pending.Wait = nullptr;
            }
            return true;
        }

        void ReadSplitter::QueueBufferedRead(FileRequest* request)
        {
            auto data = VStd::get_if<FileRequest::ReadData>(&request->GetCommand());
            V_Assert(data != nullptr, "Provided request to queue by the Read Splitter did not contain a read command.");

            PendingRead pendingRead;
            pendingRead.Request = request;
            pendingRead.Output = reinterpret_cast<u8*>(data->Output);
            pendingRead.OutputSize = data->OutputSize;
            pendingRead.ReadSize = data->Size;
            pendingRead.Offset = data->Offset;
            pendingRead.IsBuffered = true;

            if (!m_pendingReads.empty())
            {
                m_pendingReads.push_back(pendingRead);
                return;
            }

            if (!QueueBufferedRead(pendingRead))
            {
                m_pendingReads.push_back(pendingRead);
            }
        }

        bool ReadSplitter::QueueBufferedRead(PendingRead& pending)
        {
            auto data = VStd::get_if<FileRequest::ReadData>(&pending.Request->GetCommand());
            V_Assert(data != nullptr, "Provided request to queue by the Read Splitter did not contain a read command.");

            while (pending.ReadSize > 0)
            {
                if (!m_availableBufferSlots.empty())
                {
                    u32 bufferSlot = m_availableBufferSlots.back();
                    m_availableBufferSlots.pop_back();

                    u64 readSize;
                    u64 copySize;
                    u64 offset;
                    BufferCopyInformation& copyInfo = m_bufferCopyInformation[bufferSlot];
                    copyInfo.Target = pending.Output;

                    if (m_adjustOffset)
                    {
                        offset = V_SIZE_ALIGN_DOWN(pending.Offset, v_numeric_cast<u64>(m_sizeAlignment));
                        size_t bufferOffset = pending.Offset - offset;
                        copyInfo.BufferOffset = bufferOffset;
                        readSize = VStd::min(pending.ReadSize + bufferOffset, m_maxReadSize);
                        copySize = readSize - bufferOffset;
                    }
                    else
                    {
                        offset = pending.Offset;
                        readSize = VStd::min(pending.ReadSize, m_maxReadSize);
                        copySize = readSize;
                    }
                    V_Assert(readSize <= m_maxReadSize, "Read size %llu in read splitter exceeds the maximum split size of %llu.",
                        readSize, m_maxReadSize);
                    copyInfo.Size = copySize;

                    FileRequest* subRequest = m_context->GetNewInternalRequest();
                    subRequest->CreateRead(pending.Request, GetBufferSlot(bufferSlot), m_maxReadSize, data->Path,
                        offset, readSize, data->SharedRead);
                    subRequest->SetCompletionCallback([this, bufferSlot]([[maybe_unused]] FileRequest& request)
                        {
                            V_PROFILE_FUNCTION(Core);

                            BufferCopyInformation& copyInfo = m_bufferCopyInformation[bufferSlot];
                            memcpy(copyInfo.Target, GetBufferSlot(bufferSlot) + copyInfo.BufferOffset, copyInfo.Size);
                            m_availableBufferSlots.push_back(bufferSlot);

                            QueuePendingRequest();
                        });
                    m_next->QueueRequest(subRequest);

                    pending.Offset += copySize;
                    pending.ReadSize -= copySize;
                    pending.OutputSize -= copySize;
                    pending.Output += copySize;
                }
                else
                {
                    // Add a wait to make sure the read request isn't completed if all sub-reads completed before
                    // the ReadSplitter has had a chance to add new sub-reads to complete the read.
                    if (pending.Wait == nullptr)
                    {
                        pending.Wait = m_context->GetNewInternalRequest();
                        pending.Wait->CreateWait(pending.Request);
                    }
                    return false;
                }
            }
            if (pending.Wait != nullptr)
            {
                m_context->MarkRequestAsCompleted(pending.Wait);
                pending.Wait = nullptr;
            }
            return true;
        }

        void ReadSplitter::QueuePendingRequest()
        {
            if (!m_pendingReads.empty())
            {
                PendingRead& pendingRead = m_pendingReads.front();
                if (pendingRead.IsBuffered ? QueueBufferedRead(pendingRead) : QueueAlignedRead(pendingRead))
                {
                    m_pendingReads.pop_front();
                }
            }
        }

        void ReadSplitter::UpdateStatus(Status& status) const
        {
            StreamStackEntry::UpdateStatus(status);
            if (m_bufferSize > 0)
            {
                s32 numAvailableSlots = v_numeric_cast<s32>(m_availableBufferSlots.size());
                status.NumAvailableSlots = VStd::min(status.NumAvailableSlots, numAvailableSlots);
                status.IsIdle = status.IsIdle && m_pendingReads.empty();
            }
        }

        void ReadSplitter::CollectStatistics(VStd::vector<Statistic>& statistics) const
        {
            statistics.push_back(Statistic::CreateFloat(m_name, AvgNumSubReadsName, m_averageNumSubReadsStat.GetAverage()));
            statistics.push_back(Statistic::CreatePercentage(m_name, AlignedReadsName, m_alignedReadsStat.GetAverage()));
            statistics.push_back(Statistic::CreateInteger(m_name, NumAvailableBufferSlotsName, v_numeric_caster(m_availableBufferSlots.size())));
            statistics.push_back(Statistic::CreateInteger(m_name, NumPendingReadsName, v_numeric_caster(m_pendingReads.size())));
            StreamStackEntry::CollectStatistics(statistics);
        }

        void ReadSplitter::InitializeBuffer()
        {
            // Lazy initialization to avoid allocating memory if it's not needed.
            if (m_bufferSize != 0 && m_buffer == nullptr)
            {
                m_buffer = reinterpret_cast<u8*>(V::AllocatorInstance<V::SystemAllocator>::Get().Allocate(
                    m_bufferSize, m_memoryAlignment, 0, "V::IO::Streamer ReadSplitter", __FILE__, __LINE__));
            }
        }

        u8* ReadSplitter::GetBufferSlot(size_t index)
        {
            V_Assert(m_buffer != nullptr, "A buffer slot was requested by the Read Splitter before the buffer was initialized.");
            return m_buffer + (index * m_maxReadSize);
        }
    } // namespace IO
} // namesapce AZ
