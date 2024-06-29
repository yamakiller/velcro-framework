#ifndef V_FRAMEWORK_CORE_IO_STREAMER_STREAMER_H
#define V_FRAMEWORK_CORE_IO_STREAMER_STREAMER_H


#include <vcore/base.h>
#include <vcore/io/istreamer.h>
#include <vcore/io/streamer/scheduler.h>
#include <vcore/io/streamer/streamer_context.h>
#include <vcore/memory/pool_allocator.h>
#include <vcore/std/smart_ptr/unique_ptr.h>

namespace VStd
{
    struct thread_desc;
}

namespace V::IO
{
    class StreamStackEntry;

    /**
     * Data streamer.
     */
    class Streamer final : public V::IO::IStreamer
    {
    public:
        VOBJECT_RTTI(Streamer, "{499a6b3c-1d65-479a-9cac-bea2aa1ab3ad}", IStreamer);
        V_CLASS_ALLOCATOR(Streamer, SystemAllocator, 0);

        //
        // Streamer commands.
        //

        //! Creates a request to read a file with a user provided output buffer.
        FileRequestPtr Read(VStd::string_view relativePath, void* outputBuffer, size_t outputBufferSize, size_t readSize,
            VStd::chrono::microseconds deadline = IStreamerTypes::_noDeadline,
            IStreamerTypes::Priority priority = IStreamerTypes::_priorityMedium, size_t offset = 0) override;

        //! Sets a request to the read command with a user provided output buffer.
        FileRequestPtr& Read(FileRequestPtr& request, VStd::string_view relativePath, void* outputBuffer, size_t outputBufferSize,
            size_t readSize, VStd::chrono::microseconds deadline = IStreamerTypes::_noDeadline,
            IStreamerTypes::Priority priority = IStreamerTypes::_priorityMedium, size_t offset = 0) override;

        //! Creates a request to read a file with an allocator to create the output buffer.
        FileRequestPtr Read(VStd::string_view relativePath, IStreamerTypes::RequestMemoryAllocator& allocator,
            size_t size, VStd::chrono::microseconds deadline = IStreamerTypes::_noDeadline,
            IStreamerTypes::Priority priority = IStreamerTypes::_priorityMedium, size_t offset = 0) override;

        //! Set a request to the read command with an allocator to create the output buffer.
        FileRequestPtr& Read(FileRequestPtr& request, VStd::string_view relativePath, IStreamerTypes::RequestMemoryAllocator& allocator,
            size_t size, VStd::chrono::microseconds deadline = IStreamerTypes::_noDeadline,
            IStreamerTypes::Priority priority = IStreamerTypes::_priorityMedium, size_t offset = 0) override;


        //! Creates a request to cancel a previously queued request.
        FileRequestPtr Cancel(FileRequestPtr target) override;

        //! Sets a request to the cancel command.
        FileRequestPtr& Cancel(FileRequestPtr& request, FileRequestPtr target) override;

        //! Creates a request to adjust a previous queued request.
        FileRequestPtr RescheduleRequest(FileRequestPtr target, VStd::chrono::microseconds newDeadline,
            IStreamerTypes::Priority newPriority) override;

        //! Sets a request to the reschedule command.
        FileRequestPtr& RescheduleRequest(FileRequestPtr& request, FileRequestPtr target, VStd::chrono::microseconds newDeadline,
            IStreamerTypes::Priority newPriority) override;


        //
        // Dedicated Cache
        //

        //! Creates a dedicated cache for the target file.
        FileRequestPtr CreateDedicatedCache(VStd::string_view relativePath) override;

        //! Creates a dedicated cache for the target file.
        FileRequestPtr& CreateDedicatedCache(FileRequestPtr& request, VStd::string_view relativePath) override;

        //! Destroy a dedicated cache created by CreateDedicatedCache.
        FileRequestPtr DestroyDedicatedCache(VStd::string_view relativePath) override;

        //! Destroy a dedicated cache created by CreateDedicatedCache.
        FileRequestPtr& DestroyDedicatedCache(FileRequestPtr& request, VStd::string_view relativePath) override;

        //
        // Cache Flush
        //

        //! Clears a file from all caches in use by Streamer.
        FileRequestPtr FlushCache(VStd::string_view relativePath) override;

        //! Clears a file from all caches in use by Streamer.
        FileRequestPtr& FlushCache(FileRequestPtr& request, VStd::string_view relativePath) override;

        //! Forcefully clears out all caches internally held by the available devices.
        FileRequestPtr FlushCaches() override;

        //! Forcefully clears out all caches internally held by the available devices.
        FileRequestPtr& FlushCaches(FileRequestPtr& request) override;

        //
        // Custom requests.
        //

        //! Creates a custom request.
        FileRequestPtr Custom(VStd::any data) override;

        //! Creates a custom request.
        FileRequestPtr& Custom(FileRequestPtr& request, VStd::any data) override;

        //
        // Streamer request configuration.
        //

        //! Sets a callback function that will trigger when the provided request completes.
        FileRequestPtr& SetRequestCompleteCallback(FileRequestPtr& request, OnCompleteCallback callback) override;

        //
        // Streamer request management.
        //

        //! Create a new blank request.
        FileRequestPtr CreateRequest() override;

        //! Creates a number of new blank requests.
        void CreateRequestBatch(VStd::vector<FileRequestPtr>& requests, size_t count) override;

        //! Queues a request for processing by Streamer's stack.
        void QueueRequest(const FileRequestPtr& request) override;

        //! Queue a batch of requests for processing by Streamer's stack.
        void QueueRequestBatch(const VStd::vector<FileRequestPtr>& requests) override;

        //! Queue a batch of requests for processing by Streamer's stack.
        void QueueRequestBatch(VStd::vector<FileRequestPtr>&& requests) override;

        //
        // Streamer request queries
        //

        //! Check if the provided request has completed.
        bool HasRequestCompleted(FileRequestHandle request) const override;

        //! Check the status of a request.
        IStreamerTypes::RequestStatus GetRequestStatus(FileRequestHandle request) const override;

        //! Returns the time that the provided request will complete.
        VStd::chrono::system_clock::time_point GetEstimatedRequestCompletionTime(FileRequestHandle request) const override;

        //! Gets the result for operations that read data.
        bool GetReadRequestResult(FileRequestHandle request, void*& buffer, u64& numBytesRead,
            IStreamerTypes::ClaimMemory claimMemory = IStreamerTypes::ClaimMemory::No) const override;

        //
        // General Streamer functions
        //

        //! Call to collect statistics from all the components that make up Streamer.
        void CollectStatistics(VStd::vector<Statistic>& statistics) override;

        //! Returns configuration recommendations as reported by the scheduler.
        const IStreamerTypes::Recommendations& GetRecommendations() const override;

        //! Suspends processing of requests in Streamer's stack.
        void SuspendProcessing() override;

        //! Resumes processing after a previous call to SuspendProcessing.
        void ResumeProcessing() override;

        //
        // Streamer specific functions.
        // These functions are specific to the AZ::IO::Streamer and in practice are only used by the StreamerComponent.
        //

        //! Records the statistics to a profiler.
        void RecordStatistics();

        //! Tells AZ::IO::Streamer the report the information for the report to the output.
        FileRequestPtr Report(FileRequest::ReportData::ReportType reportType);
        //! Tells AZ::IO::Streamer the report the information for the report to the output.
        FileRequestPtr& Report(FileRequestPtr& request, FileRequest::ReportData::ReportType reportType);


        Streamer(const VStd::thread_desc& threadDesc, VStd::unique_ptr<Scheduler> streamStack);
        ~Streamer() override;

    private:
        StreamerContext m_streamerContext;
        VStd::unique_ptr<Scheduler> m_streamStack;
        IStreamerTypes::Recommendations m_recommendations;
    };
} // namespace V::IO

#endif // V_FRAMEWORK_CORE_IO_STREAMER_STREAMER_H