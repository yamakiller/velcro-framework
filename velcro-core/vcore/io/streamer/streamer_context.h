#ifndef V_FRAMEWORK_CORE_IO_STREAMER_CONTEXT_H
#define V_FRAMEWORK_CORE_IO_STREAMER_CONTEXT_H

#include <vcore/base.h>
#include <vcore/io/streamer/file_request.h>
#include <vcore/io/streamer/statistics.h>
#include <vcore/io/streamer/streamer_context_platform.h>

#include <vcore/std/containers/deque.h>
#include <vcore/std/containers/vector.h>
#include <vcore/std/containers/queue.h>

#include <vcore/statistics/running_statistic.h>

namespace V
{
    namespace IO
    {
        class StreamerContext
        {
        public:
            using PreparedQueue = VStd::deque<FileRequest*>;

            ~StreamerContext();

            //! Gets a new file request, either by creating a new instance or
            //! picking one from the recycle bin. This version should only be used
            //! by nodes on the streaming stack as it's not thread safe, but faster.
            //! The scheduler will automatically recycle these requests.
            FileRequest* GetNewInternalRequest();

            //! Gets a new file request, either by creating a new instance or
            //! picking one from the recycle bin. This version is for use by 
            //! any system outside the stream stack and is thread safe. Once the
            //! reference count in the request hits zero it will automatically be recycled.
            FileRequestPtr GetNewExternalRequest();

            //! Gets a batch of new file requests, either by creating new instances or
            //! picking from the recycle bin. This version is for use by 
            //! any system outside the stream stack and is thread safe. The owner
            //! needs to manually recycle these requests once they're done. Requests
            //! with a reference count of zero will automatically be recycled.
            //! If multiple requests need to be create this is preferable as it only locks the
            //! recycle bin once.
            void GetNewExternalRequestBatch(VStd::vector<FileRequestPtr>& requests, size_t count);

            //! Gets the number of prepared requests. Prepared requests are requests
            //! that are ready to be queued up for further processing.
            size_t GetNumPreparedRequests() const;
            //! Gets the next prepared request that should be queued. Prepared requests
            //! are requests that are ready to be queued up for further processing.
            FileRequest* PopPreparedRequest();

               //! Adds a prepared request for later queuing and processing.
            void PushPreparedRequest(FileRequest* request);
            //! Gets the prepared requests that are queued to be processed.
            PreparedQueue& GetPreparedRequests();
            //! Gets the prepared requests that are queued to be processed.
            const PreparedQueue& GetPreparedRequests() const;

            //! Marks a request as completed so the main thread in Streamer can close it out.
            //! This can be safely called from multiple threads.
            void MarkRequestAsCompleted(FileRequest* request);
            //! Rejects a request by removing it from the chain and recycling it.
            //! Only requests without children can be rejected. If the rejected request has a parent it might need to be processed
            //! further.
            //! @param request The request to remove and recycle.
            //! @return The parent request of the rejected request or null if there was no parent.
            FileRequest* RejectRequest(FileRequest* request);

            void RecycleRequest(FileRequest* request);
            //! Adds an old external request to the recycle bin so it can be reused later.
            void RecycleRequest(ExternalFileRequest* request);

              //! Does the FinalizeRequest callback where appropriate and does some bookkeeping to finalize requests.
            //! @return True if any requests were finalized, otherwise false.
            bool FinalizeCompletedRequests();

            //! Causes the main thread for streamer to wake up and process any pending requests. If the thread
            //! is already awake, nothing happens.
            void WakeUpSchedulingThread();
            //! If there's no pending messages this will cause the main thread for streamer to go to sleep.
            void SuspendSchedulingThread();
            //! Returns the native primitive(s) used to suspend and wake up the scheduling thread and possibly other threads.
            V::Platform::StreamerContextThreadSync& GetStreamerThreadSynchronizer();

            //! Collects statistics recorded during processing. This will only return statistics for the
            //! context. Use the CollectStatistics on V::IO::Streamer to get all statistics.
            void CollectStatistics(VStd::vector<Statistic>& statistics);
        private:
            //! Gets a new FileRequestPtr. This version is for internal use only and is not thread-safe.
            //! This will be called by GetNewExternalRequest or GetNewExternalRequestBatch which are responsible
            //! for managing the lock to the recycle bin.
            FileRequestPtr GetNewExternalRequestUnguarded();

            inline static constexpr size_t _initialRecycleBinSize = 64;

            VStd::mutex m_externalRecycleBinGuard;
            VStd::vector<ExternalFileRequest*> m_externalRecycleBin;
            VStd::vector<FileRequest*> m_internalRecycleBin;
            
            // The completion is guarded so other threads can perform async IO and safely mark requests as completed.
            VStd::recursive_mutex m_completedGuard;
            VStd::queue<FileRequest*> m_completed;

            // The prepared request queue is not guarded and should only be called from the main Streamer thread.
            PreparedQueue m_preparedRequests;
#if V_STREAMER_ADD_EXTRA_PROFILING_INFO
            //! By how much time the prediction was off. This mostly covers the latter part of scheduling, which
            //! gets more precise the closer the request gets to completion.
            V::Statistics::RunningStatistic m_predictionAccuracyUsStat;

            //! Tracks the percentage of requests with late predictions where the request completed earlier than expected,
            //! versus the requests that completed later than predicted.
            V::Statistics::RunningStatistic m_latePredictionsPercentageStat;

            //! Percentage of requests that missed their deadline. If percentage is too high it can indicate that
            //! there are too many file requests or the deadlines for requests are too tight.
            V::Statistics::RunningStatistic m_missedDeadlinePercentageStat;
#endif // V_STREAMER_ADD_EXTRA_PROFILING_INFO

            //! Platform-specific synchronization object used to suspend the Streamer thread and wake it up to resume procesing.
            V::Platform::StreamerContextThreadSync m_threadSync;

            size_t m_pendingIdCounter{ 0 };
        };
    }
}
#endif // V_FRAMEWORK_CORE_IO_STREAMER_CONTEXT_H