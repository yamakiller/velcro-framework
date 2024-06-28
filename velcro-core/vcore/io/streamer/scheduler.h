#ifndef V_FRAMEWORK_CORE_IO_STREAMER_SCHEDULER_H
#define V_FRAMEWORK_CORE_IO_STREAMER_SCHEDULER_H

#include <vcore/io/istreamer_types.h>
#include <vcore/io/streamer/statistics.h>
#include <vcore/io/streamer/streamer_configuration.h>
#include <vcore/io/streamer/streamer_context.h>
#include <vcore/io/streamer/stream_stack_entry.h>
#include <vcore/std/containers/vector.h>
#include <vcore/std/parallel/atomic.h>
#include <vcore/std/parallel/mutex.h>
#include <vcore/std/parallel/thread.h>
#include <vcore/std/smart_ptr/shared_ptr.h>
#include <vcore/statistics/running_statistic.h>

namespace V::IO
{
    class FileRequest;
    
    class Scheduler final
    {
    public:
        explicit Scheduler(VStd::shared_ptr<StreamStackEntry> streamStack, u64 memoryAlignment = VCORE_GLOBAL_NEW_ALIGNMENT,
            u64 sizeAlignment = 1, u64 granularity = 1_mib);
        void Start(const VStd::thread_desc& threadDesc);
        void Stop();

        FileRequestPtr CreateRequest();
        void CreateRequestBatch(VStd::vector<FileRequestPtr>& requests, size_t count);
        void QueueRequest(FileRequestPtr request);
        void QueueRequestBatch(const VStd::vector<FileRequestPtr>& requests);
        void QueueRequestBatch(VStd::vector<FileRequestPtr>&& requests);

        //! Stops the Scheduler's main thread from processing any requests. Requests can still be queued,
        //! but won't be picked up.
        void SuspendProcessing();
        //! Resumes processing of requests on the Scheduler's main thread.
        void ResumeProcessing();

        //! Collects various metrics that are recorded by streamer and its stream stack.
        //! This function is deliberately not thread safe. All stats use a sliding window and never
        //! allocate memory. This might mean in some cases that values of individual stats may not be
        //! referencing the same window of requests, but in practice these differences are too minute
        //! to detect.
        void CollectStatistics(VStd::vector<Statistic>& statistics);

        void GetRecommendations(IStreamerTypes::Recommendations& recommendations) const;

    private:
        inline static constexpr u32 ProfilerColor = 0x0080ffff; //!< A lite shade of blue. (See https://www.color-hex.com/color/0080ff).

        void Thread_MainLoop();
        void Thread_QueueNextRequest();
        bool Thread_ExecuteRequests();
        bool Thread_PrepareRequests(VStd::vector<FileRequestPtr>& outstandingRequests);
        void Thread_ProcessTillIdle();
        void Thread_ProcessCancelRequest(FileRequest* request, FileRequest::CancelData& data);
        void Thread_ProcessRescheduleRequest(FileRequest* request, FileRequest::RescheduleData& data);
        
        enum class Order
        {
            FirstRequest, //< The first request is the most important to process next.
            SecondRequest, //< The second request is the most important to process next.
            Equal //< Both requests are equally important.
        };
        //! Determine which of the two provided requests is more important to process next.
        Order Thread_PrioritizeRequests(const FileRequest* first, const FileRequest* second) const;
        void Thread_ScheduleRequests();

        // Stores data that's unguarded and should only be changed by the scheduling thread.
        struct ThreadData final
        {
            //! Requests pending in the Streaming stack entries. Cached here so it doesn't need to allocate
            //! and free memory whenever scheduling happens.
            VStd::vector<FileRequest*> InternalPendingRequests;
            RequestPath LastFilePath; //!< Path of the last file queued for reading.
            VStd::shared_ptr<StreamStackEntry> StreamStack;
            u64 LastFileOffset{ 0 }; //!< Offset of into the last file queued after reading has completed.
        };
        ThreadData m_threadData;
        StreamerContext m_context;

        IStreamerTypes::Recommendations m_recommendations;

        StreamStackEntry::Status m_stackStatus;
#if V_STREAMER_ADD_EXTRA_PROFILING_INFO
        VStd::chrono::system_clock::time_point m_processingStartTime;
        size_t m_processingSize{ 0 };
        //! Indication of how efficient the scheduler works. For loose files a large gap with the read speed of
        //! the storage drive means that the scheduler is having to spend too much time between requests. For archived
        //! files this value should be larger than the read speed of the storage drive otherwise compressing files has
        //! no benefit (other than reduced disk space).
        AverageWindow<double, double, _statisticsWindowSize> m_processingSpeedStat;

        //! Percentage of reads that come in that are already on their deadline. Requests like this are disruptive
        //! as they cause the scheduler to prioritize these over the most optimal read layout.
        V::Statistics::RunningStatistic m_immediateReadsPercentageStat;
#endif

        VStd::mutex m_pendingRequestsLock;
        VStd::vector<FileRequestPtr> m_pendingRequests;

        VStd::thread m_mainLoop;
        VStd::atomic_bool m_isRunning{ false };
        VStd::atomic_bool m_isSuspended{ false };
        VStd::thread_desc m_mainLoopDesc;
    };
} // namespace V::IO


#endif // V_FRAMEWORK_CORE_IO_STREAMER_SCHEDULER_H