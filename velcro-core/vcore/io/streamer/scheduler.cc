#include <vcore/casting/numeric_cast.h>
#include <vcore/debug/profiler.h>
#include <vcore/io/Streamer/Scheduler.h>
#include <vcore/std/containers/deque.h>
#include <vcore/std/sort.h>

namespace V::IO
{
#if V_STREAMER_ADD_EXTRA_PROFILING_INFO
    static constexpr char SchedulerName[] = "Scheduler";
    static constexpr char ImmediateReadsName[] = "Immediate reads";
#endif // V_STREAMER_ADD_EXTRA_PROFILING_INFO

    Scheduler::Scheduler(VStd::shared_ptr<StreamStackEntry> streamStack, u64 memoryAlignment, u64 sizeAlignment, u64 granularity)
    {
        V_Assert(IStreamerTypes::IsPowerOf2(memoryAlignment), "Memory alignment provided to V::IO::Scheduler isn't a power of two.");
        V_Assert(IStreamerTypes::IsPowerOf2(sizeAlignment), "Size alignment provided to V::IO::Scheduler isn't a power of two.");

        StreamStackEntry::Status status;
        streamStack->UpdateStatus(status);
        V_Assert(status.IsIdle, "Stream stack provided to the scheduler is already active.");
        m_recommendations.m_memoryAlignment = memoryAlignment;
        m_recommendations.m_sizeAlignment = sizeAlignment;
        m_recommendations.m_maxConcurrentRequests = v_numeric_caster(status.NumAvailableSlots);
        m_recommendations.m_granularity = granularity;

        m_threadData.StreamStack = VStd::move(streamStack);
    }

    void Scheduler::Start(const VStd::thread_desc& threadDesc)
    {
        if (!m_isRunning)
        {
            m_isRunning = true;

            m_mainLoopDesc = threadDesc;
            m_mainLoopDesc.m_name = "IO Scheduler";
            m_mainLoop = VStd::thread(
                m_mainLoopDesc,
                [this]()
                {
                    Thread_MainLoop();
                });
        }
    }

    void Scheduler::Stop()
    {
        m_isRunning = false;
        m_context.WakeUpSchedulingThread();
        m_mainLoop.join();
    }

    FileRequestPtr Scheduler::CreateRequest()
    {
        return m_context.GetNewExternalRequest();
    }

    void Scheduler::CreateRequestBatch(VStd::vector<FileRequestPtr>& requests, size_t count)
    {
        requests.reserve(requests.size() + count);
        m_context.GetNewExternalRequestBatch(requests, count);
    }


    void Scheduler::QueueRequest(FileRequestPtr request)
    {
        V_Assert(m_isRunning, "Trying to queue a request when Streamer's scheduler isn't running.");

        {
            VStd::scoped_lock lock(m_pendingRequestsLock);
            m_pendingRequests.push_back(VStd::move(request));
        }
        m_context.WakeUpSchedulingThread();
    }

    void Scheduler::QueueRequestBatch(const VStd::vector<FileRequestPtr>& requests)
    {
        V_Assert(m_isRunning, "Trying to queue a batch of requests when Streamer's scheduler isn't running.");

        {
            VStd::scoped_lock lock(m_pendingRequestsLock);
            m_pendingRequests.insert(m_pendingRequests.end(), requests.begin(), requests.end());
        }
        m_context.WakeUpSchedulingThread();
    }

    void Scheduler::QueueRequestBatch(VStd::vector<FileRequestPtr>&& requests)
    {
        V_Assert(m_isRunning, "Trying to queue a batch of requests when Streamer's scheduler isn't running.");

        {
            VStd::scoped_lock lock(m_pendingRequestsLock);
            VStd::move(requests.begin(), requests.end(), VStd::back_inserter(m_pendingRequests));
        }
        m_context.WakeUpSchedulingThread();
    }

    void Scheduler::SuspendProcessing()
    {
        m_isSuspended = true;
    }

    void Scheduler::ResumeProcessing()
    {
        if (m_isSuspended)
        {
            m_isSuspended = false;
            m_context.WakeUpSchedulingThread();
        }
    }

    void Scheduler::CollectStatistics(VStd::vector<Statistic>& statistics)
    {
#if V_STREAMER_ADD_EXTRA_PROFILING_INFO
        statistics.push_back(Statistic::CreateInteger(SchedulerName, "Is idle", m_stackStatus.IsIdle ? 1 : 0));
        statistics.push_back(Statistic::CreateInteger(SchedulerName, "Available slots", m_stackStatus.NumAvailableSlots));
        statistics.push_back(Statistic::CreateFloat(SchedulerName, "Processing speed (avg. mbps)", m_processingSpeedStat.CalculateAverage()));
        statistics.push_back(Statistic::CreatePercentage(SchedulerName, ImmediateReadsName, m_immediateReadsPercentageStat.GetAverage()));
#endif
        m_context.CollectStatistics(statistics);
        m_threadData.StreamStack->CollectStatistics(statistics);
    }

    void Scheduler::GetRecommendations(IStreamerTypes::Recommendations& recommendations) const
    {
        recommendations = m_recommendations;
    }

    void Scheduler::Thread_MainLoop()
    {
        m_threadData.StreamStack->SetContext(m_context);
        VStd::vector<FileRequestPtr> outstandingRequests;

        while (m_isRunning)
        {
            {
                V_PROFILE_SCOPE(Core, "Scheduler suspended.");
                m_context.SuspendSchedulingThread();
            }

            // Only do processing if the thread hasn't been suspended.
            while (!m_isSuspended)
            {
                V_PROFILE_SCOPE(Core, "Scheduler main loop.");

                // Always schedule requests first as the main Streamer thread could have been asleep for a long time due to slow reading
                // but also don't schedule after every change in the queue as scheduling is not cheap.
                Thread_ScheduleRequests();
                do
                {
                    do
                    {
                        V_PROFILE_SCOPE(Core, "Scheduler queue requests.");
                        // If there are pending requests and available slots, queue the next requests.
                        while(m_context.GetNumPreparedRequests() > 0)
                        {
                            m_stackStatus = StreamStackEntry::Status{};
                            m_threadData.StreamStack->UpdateStatus(m_stackStatus);
                            if (m_stackStatus.NumAvailableSlots > 0)
                            {
                                Thread_QueueNextRequest();
                            }
                            else
                            {
                                break;
                            }
                        }
                        // Check if there are any requests that have completed their work and if so complete them. This will open slots so
                        // try to queue more pending requests.
                    } while (m_context.FinalizeCompletedRequests());
                    // Tick the stream stack to do work. If work was done there might be requests to complete and new slots might have opened up.
#if V_STREAMER_ADD_EXTRA_PROFILING_INFO
                    m_stackStatus = StreamStackEntry::Status{};
                    m_threadData.StreamStack->UpdateStatus(m_stackStatus);
#endif
                } while (Thread_ExecuteRequests());

                // Check if there are requests queued from other threads, if so move them to the main Streamer thread by executing them. This then
                // requires another cycle or scheduling and executing commands.
                if (!Thread_PrepareRequests(outstandingRequests))
                {
                    break;
                }
            }

#if V_STREAMER_ADD_EXTRA_PROFILING_INFO
            m_stackStatus = StreamStackEntry::Status{};
            m_threadData.StreamStack->UpdateStatus(m_stackStatus);
            if (m_stackStatus.IsIdle)
            {
                auto duration = VStd::chrono::system_clock::now() - m_processingStartTime;
                double processingSizeMib = v_numeric_cast<double>(m_processingSize) / 1_mib;
                auto durationSec = VStd::chrono::duration_cast<VStd::chrono::duration<double>>(duration);
                m_processingSpeedStat.PushEntry(processingSizeMib / durationSec.count());
                m_processingSize = 0;
            }
#endif
        }

        // Make sure all requests in the stack are cleared out. This dangling async processes or async processes crashing as assigned
        // such as memory buffers are no longer available.
        Thread_ProcessTillIdle();
    }

    void Scheduler::Thread_QueueNextRequest()
    {
        V_PROFILE_FUNCTION(Core);

        FileRequest* next = m_context.PopPreparedRequest();
        next->SetStatus(IStreamerTypes::RequestStatus::Processing);

        VStd::visit([this, next](auto&& args)
        {
            using Command = VStd::decay_t<decltype(args)>;
            if constexpr (
                VStd::is_same_v<Command, FileRequest::ReadData> ||
                VStd::is_same_v<Command, FileRequest::CompressedReadData>)
            {
                auto parentReadRequest = next->GetCommandFromChain<FileRequest::ReadRequestData>();
                V_Assert(parentReadRequest != nullptr, "The issued read request can't be found for the (compressed) read command.");
                
                size_t size = parentReadRequest->Size;
                if (parentReadRequest->Output == nullptr)
                {
                    V_Assert(parentReadRequest->Allocator,
                        "The read request was issued without a memory allocator or valid output address.");
                    u64 recommendedSize = size;
                    if constexpr (VStd::is_same_v<Command, FileRequest::ReadData>)
                    {
                        recommendedSize = m_recommendations.CalculateRecommendedMemorySize(size, parentReadRequest->Offset);
                    }
                    IStreamerTypes::RequestMemoryAllocatorResult allocation =
                        parentReadRequest->Allocator->Allocate(size, recommendedSize, m_recommendations.m_memoryAlignment);
                    if (allocation.Address == nullptr || allocation.Size < parentReadRequest->Size)
                    {
                        next->SetStatus(IStreamerTypes::RequestStatus::Failed);
                        m_context.MarkRequestAsCompleted(next);
                        return;
                    }
                    parentReadRequest->Output = allocation.Address;
                    parentReadRequest->OutputSize = allocation.Size;
                    parentReadRequest->MemoryType = allocation.Type;
                    if constexpr (VStd::is_same_v<Command, FileRequest::ReadData>)
                    {
                        args.Output = parentReadRequest->Output;
                        args.OutputSize = allocation.Size;
                    }
                    else if constexpr (VStd::is_same_v<Command, FileRequest::CompressedReadData>)
                    {
                        args.Output = parentReadRequest->Output;
                    }
                }

#if V_STREAMER_ADD_EXTRA_PROFILING_INFO
                if (m_processingSize == 0)
                {
                    m_processingStartTime = VStd::chrono::system_clock::now();
                }
#endif
                
                if constexpr (VStd::is_same_v<Command, FileRequest::ReadData>)
                {
                    m_threadData.LastFilePath = args.Path;
                    m_threadData.LastFileOffset = args.Offset + args.Size;
#if V_STREAMER_ADD_EXTRA_PROFILING_INFO
                    m_processingSize += args.Size;
#endif
                }
                else if constexpr (VStd::is_same_v<Command, FileRequest::CompressedReadData>)
                {
                    const CompressionInfo& info = args.CompressionInfoData;
                    m_threadData.LastFilePath = info.ArchiveFilename;
                    m_threadData.LastFileOffset = info.Offset + info.CompressedSize;
#if V_STREAMER_ADD_EXTRA_PROFILING_INFO
                    m_processingSize += info.UncompressedSize;
#endif
                }
                V_PROFILE_INTERVAL_START_COLORED(Core, next, ProfilerColor,
                    "Streamer queued %zu: %s", next->GetCommand().index(), parentReadRequest->m_path.GetRelativePath());
                m_threadData.StreamStack->QueueRequest(next);
            }
            else if constexpr (VStd::is_same_v<Command, FileRequest::CancelData>)
            {
                return Thread_ProcessCancelRequest(next, args);
            }
            else if constexpr (VStd::is_same_v<Command, FileRequest::RescheduleData>)
            {
                return Thread_ProcessRescheduleRequest(next, args);
            }
            else if constexpr (VStd::is_same_v<Command, FileRequest::FlushData> || VStd::is_same_v<Command, FileRequest::FlushAllData>)
            {
                V_PROFILE_INTERVAL_START_COLORED(Core, next, ProfilerColor,
                    "Streamer queued %zu", next->GetCommand().index());
                // Flushing becomes a lot less complicated if there are no jobs and/or asynchronous I/O running. This does mean overall
                // longer processing time as bubbles are introduced into the pipeline, but flushing is an infrequent event that only
                // happens during development.
                Thread_ProcessTillIdle();
                m_threadData.StreamStack->QueueRequest(next);
            }
            else
            {
                V_PROFILE_INTERVAL_START_COLORED(Core, next, ProfilerColor,
                    "Streamer queued %zu", next->GetCommand().index());
                m_threadData.StreamStack->QueueRequest(next);
            }
        }, next->GetCommand());
    }

    bool Scheduler::Thread_ExecuteRequests()
    {
        V_PROFILE_FUNCTION(Core);
        return m_threadData.StreamStack->ExecuteRequests();
    }

    bool Scheduler::Thread_PrepareRequests(VStd::vector<FileRequestPtr>& outstandingRequests)
    {
        V_PROFILE_FUNCTION(Core);

        {
            VStd::scoped_lock lock(m_pendingRequestsLock);
            if (!m_pendingRequests.empty())
            {
                outstandingRequests.swap(m_pendingRequests);
            }
            else
            {
                return false;
            }
        }

#if V_STREAMER_ADD_EXTRA_PROFILING_INFO
        VStd::chrono::system_clock::time_point now = VStd::chrono::system_clock::now();
        auto visitor = [this, now](auto&& args) -> void
#else
        auto visitor = [](auto&& args) -> void
#endif
        {
            using Command = VStd::decay_t<decltype(args)>;
            if constexpr (VStd::is_same_v<Command, FileRequest::ReadRequestData>)
            {
                if (args.Output == nullptr && args.Allocator != nullptr)
                {
                    args.Allocator->LockAllocator();
                }
#if V_STREAMER_ADD_EXTRA_PROFILING_INFO
                m_immediateReadsPercentageStat.PushSample(args.Deadline <= now ? 1.0 : 0.0);
                Statistic::PlotImmediate(SchedulerName, ImmediateReadsName, m_immediateReadsPercentageStat.GetMostRecentSample());
#endif
            }
        };

        for (auto& request : outstandingRequests)
        {
            VStd::visit(visitor, request->m_request.GetCommand());
        }
        for (auto& request : outstandingRequests)
        {
            // Add a link in front of the external request to keep a reference to the FileRequestPtr alive while it's being processed.
            FileRequest* requestPtr = &request->m_request;
            FileRequest* linkRequest = m_context.GetNewInternalRequest();
            linkRequest->CreateRequestLink(VStd::move(request));
            requestPtr->SetStatus(IStreamerTypes::RequestStatus::Queued);
            m_threadData.StreamStack->PrepareRequest(requestPtr);
        }
        outstandingRequests.clear();
        return true;
    }

    void Scheduler::Thread_ProcessTillIdle()
    {
        V_PROFILE_FUNCTION(Core);

        while (true)
        {
            m_stackStatus = StreamStackEntry::Status{};
            m_threadData.StreamStack->UpdateStatus(m_stackStatus);
            if (m_stackStatus.IsIdle)
            {
                return;
            }

            Thread_ExecuteRequests();
            m_context.FinalizeCompletedRequests();
        }
    }

    void Scheduler::Thread_ProcessCancelRequest(FileRequest* request, FileRequest::CancelData& data)
    {
        V_PROFILE_INTERVAL_START_COLORED(Core, request, ProfilerColor, "Streamer queued cancel");
        auto& pending = m_context.GetPreparedRequests();
        auto pendingIt = pending.begin();
        while (pendingIt != pending.end())
        {
            if ((*pendingIt)->WorksOn(data.Target))
            {
                (*pendingIt)->SetStatus(IStreamerTypes::RequestStatus::Canceled);
                m_context.MarkRequestAsCompleted(*pendingIt);
                pendingIt = pending.erase(pendingIt);
            }
            else
            {
                ++pendingIt;
            }
        }
        
        m_threadData.StreamStack->QueueRequest(request);
    }

    void Scheduler::Thread_ProcessRescheduleRequest(FileRequest* request, FileRequest::RescheduleData& data)
    {
        V_PROFILE_INTERVAL_START_COLORED(Core, request, ProfilerColor, "Streamer queued reschedule");
        auto& pendingRequests = m_context.GetPreparedRequests();
        for (FileRequest* pending : pendingRequests)
        {
            if (pending->WorksOn(data.Target))
            {
                // Read requests are the only requests that use deadlines and dynamic priorities.
                auto readRequest = pending->GetCommandFromChain<FileRequest::ReadRequestData>();
                if (readRequest)
                {
                    readRequest->Deadline = data.NewDeadline;
                    readRequest->Priority = data.NewPriority;
                }
                // Nothing more needs to happen as the request will be rescheduled in the next full pass in the main loop or
                // it's going to be one of the next requests to be picked up, in which case the time between the reschedule
                // request and read being processed that this similar to the reschedule being too late.
            }
        }

        request->SetStatus(IStreamerTypes::RequestStatus::Completed);
        m_context.MarkRequestAsCompleted(request);
    }

    auto Scheduler::Thread_PrioritizeRequests(const FileRequest* first, const FileRequest* second) const -> Order
    {
        // Sort by order priority of the command in the request. This allows to for instance have cancel request
        // always happen before any other requests.
        auto order = [](auto&& args)
        {
            using Command = VStd::decay_t<decltype(args)>;
            if constexpr (VStd::is_same_v<Command, VStd::monostate>)
            {
                return IStreamerTypes::_priorityLowest;
            }
            else
            {
                return Command::_orderPriority;
            }
        };
        IStreamerTypes::Priority firstOrderPriority = VStd::visit(order, first->GetCommand());
        IStreamerTypes::Priority secondOrderPriority = VStd::visit(order, second->GetCommand());
        if (firstOrderPriority > secondOrderPriority) { return Order::FirstRequest; }
        if (firstOrderPriority < secondOrderPriority) { return Order::SecondRequest; }

        // Order is the same for both requests, so prioritize the request that are at risk of missing
        // it's deadline.
        const FileRequest::ReadRequestData* firstRead = first->GetCommandFromChain<FileRequest::ReadRequestData>();
        const FileRequest::ReadRequestData* secondRead = second->GetCommandFromChain<FileRequest::ReadRequestData>();

        if (firstRead == nullptr || secondRead == nullptr)
        {
            // One of the two requests or both don't have information for scheduling so leave them
            // in the same order.
            return Order::Equal;
        }

        bool firstInPanic = first->GetEstimatedCompletion() > firstRead->Deadline;
        bool secondInPanic = second->GetEstimatedCompletion() > secondRead->Deadline;
        // Both request are at risk of not completing before their deadline.
        if (firstInPanic && secondInPanic)
        {
            // Let the one with the highest priority go first.
            if (firstRead->Priority != secondRead->Priority)
            {
                return firstRead->Priority < secondRead->Priority ? Order::FirstRequest : Order::SecondRequest;
            }

            // If neither has started and have the same priority, prefer to start the closest deadline.
            return firstRead->Deadline <= secondRead->Deadline ? Order::FirstRequest : Order::SecondRequest;
        }

        // Check if one of the requests is in panic and prefer to prioritize that request
        if (firstInPanic) { return Order::FirstRequest; }
        if (secondInPanic) { return Order::SecondRequest; }

        // Both are not in panic so base the order on the number of IO steps (opening files, seeking, etc.) are needed.
        auto sameFile = [this](auto&& args)
        {
            using Command = VStd::decay_t<decltype(args)>;
            if constexpr (VStd::is_same_v<Command, FileRequest::ReadData>)
            {
                return m_threadData.LastFilePath == args.Path;
            }
            else if constexpr (VStd::is_same_v<Command, FileRequest::CompressedReadData>)
            {
                return m_threadData.LastFilePath == args.CompressionInfoData.ArchiveFilename;
            }
            else
            {
                return false;
            }
        };
        bool firstInSameFile = VStd::visit(sameFile, first->GetCommand());
        bool secondInSameFile = VStd::visit(sameFile, second->GetCommand());
        // If both request are in the active file, prefer to pick the file that's closest to the last read.
        if (firstInSameFile && secondInSameFile)
        {
            auto offset = [](auto&& args) -> s64
            {
                using Command = VStd::decay_t<decltype(args)>;
                if constexpr (VStd::is_same_v<Command, FileRequest::ReadData>)
                {
                    return v_numeric_caster(args.Offset);
                }
                else if constexpr (VStd::is_same_v<Command, FileRequest::CompressedReadData>)
                {
                    return v_numeric_caster(args.CompressionInfoData.Offset);
                }
                else
                {
                    return std::numeric_limits<s64>::max();
                }
            };
            s64 firstReadOffset = VStd::visit(offset, first->GetCommand());
            s64 secondReadOffset = VStd::visit(offset, second->GetCommand());
            s64 firstSeekDistance = abs(v_numeric_cast<s64>(m_threadData.LastFileOffset) - firstReadOffset);
            s64 secondSeekDistance = abs(v_numeric_cast<s64>(m_threadData.LastFileOffset) - secondReadOffset);
            return firstSeekDistance <= secondSeekDistance ? Order::FirstRequest : Order::SecondRequest;
        }

        // Prefer to continue in the same file so prioritize the request that's in the same file
        if (firstInSameFile) { return Order::FirstRequest; }
        if (secondInSameFile) { return Order::SecondRequest; }

        // If both requests need to open a new file, keep them in the same order as there's no information available
        // to indicate which request would be able to load faster or more efficiently.
        return Order::Equal;
    }

    void Scheduler::Thread_ScheduleRequests()
    {
        V_PROFILE_FUNCTION(Core);

        VStd::chrono::system_clock::time_point now = VStd::chrono::system_clock::now();
        auto& pendingQueue = m_context.GetPreparedRequests();

        m_threadData.StreamStack->UpdateCompletionEstimates(now, m_threadData.InternalPendingRequests,
            pendingQueue.begin(), pendingQueue.end());
        m_threadData.InternalPendingRequests.clear();

        if (m_context.GetNumPreparedRequests() > 1)
        {
            V_PROFILE_SCOPE(Core,
                "Scheduler::Thread_ScheduleRequests - Sorting %i requests", m_context.GetNumPreparedRequests());
            auto sorter = [this](const FileRequest* lhs, const FileRequest* rhs) -> bool
            {
                Order order = Thread_PrioritizeRequests(lhs, rhs);
                switch (order)
                {
                case Order::FirstRequest:
                    return true;
                case Order::SecondRequest:
                    return false;
                case Order::Equal:
                    // If both requests can't be prioritized relative to each other than keep them in the order they
                    // were originally queued.
                    return lhs->GetPendingId() < rhs->GetPendingId();
                default:
                    V_Assert(false, "Unsupported request order: %i.", order);
                    return true;
                }
            };

            VStd::sort(pendingQueue.begin(), pendingQueue.end(), sorter);
        }
    }
} // namespace V::IO