#include <vcore/casting/numeric_cast.h>
#include <vcore/debug/profiler.h>
#include <vcore/io/compression_bus.h>
#include <vcore/io/streamer/streamer.h>
#include <vcore/io/streamer/streamer_configuration.h>
#include <vcore/io/streamer/stream_stack_entry.h>
#include <vcore/std/smart_ptr/make_shared.h>


namespace V::IO
{
    FileRequestPtr Streamer::Read(VStd::string_view relativePath, void* outputBuffer, size_t outputBufferSize, size_t readSize,
        VStd::chrono::microseconds deadline, IStreamerTypes::Priority priority, size_t offset)
    {
        FileRequestPtr result = CreateRequest();
        Read(result, relativePath, outputBuffer, outputBufferSize, readSize, deadline, priority, offset);
        return result;
    }

    FileRequestPtr& Streamer::Read(FileRequestPtr& request, VStd::string_view relativePath, void* outputBuffer,
        size_t outputBufferSize, size_t readSize, VStd::chrono::microseconds deadline, IStreamerTypes::Priority priority, size_t offset)
    {
        RequestPath path;
        path.InitFromRelativePath(relativePath);
        VStd::chrono::system_clock::time_point deadlineTimePoint = (deadline == IStreamerTypes::_noDeadline)
            ? FileRequest::s_noDeadlineTime
            : VStd::chrono::system_clock::now() + deadline;
        request->m_request.CreateReadRequest(VStd::move(path), outputBuffer, outputBufferSize, offset, readSize, deadlineTimePoint, priority);
        return request;
    }

    FileRequestPtr Streamer::Read(VStd::string_view relativePath, IStreamerTypes::RequestMemoryAllocator& allocator,
        size_t size, VStd::chrono::microseconds deadline, IStreamerTypes::Priority priority, size_t offset)
    {
        FileRequestPtr result = CreateRequest();
        Read(result, relativePath, allocator, size, deadline, priority, offset);
        return result;
    }

    FileRequestPtr& Streamer::Read(FileRequestPtr& request, VStd::string_view relativePath, IStreamerTypes::RequestMemoryAllocator& allocator,
        size_t size, VStd::chrono::microseconds deadline, IStreamerTypes::Priority priority, size_t offset)
    {
        RequestPath path;
        path.InitFromRelativePath(relativePath);
        VStd::chrono::system_clock::time_point deadlineTimePoint = (deadline == IStreamerTypes::_noDeadline)
            ? FileRequest::s_noDeadlineTime
            : VStd::chrono::system_clock::now() + deadline;
        request->m_request.CreateReadRequest(VStd::move(path), &allocator, offset, size, deadlineTimePoint, priority);
        return request;
    }

    FileRequestPtr Streamer::Cancel(FileRequestPtr target)
    {
        FileRequestPtr result = CreateRequest();
        Cancel(result, VStd::move(target));
        return result;
    }

    FileRequestPtr& Streamer::Cancel(FileRequestPtr& request, FileRequestPtr target)
    {
        request->m_request.CreateCancel(VStd::move(target));
        return request;
    }

    FileRequestPtr Streamer::RescheduleRequest(FileRequestPtr target, VStd::chrono::microseconds newDeadline,
        IStreamerTypes::Priority newPriority)
    {
        FileRequestPtr result = CreateRequest();
        RescheduleRequest(result, VStd::move(target), newDeadline, newPriority);
        return result;
    }

    FileRequestPtr& Streamer::RescheduleRequest(FileRequestPtr& request, FileRequestPtr target, VStd::chrono::microseconds newDeadline,
        IStreamerTypes::Priority newPriority)
    {
        VStd::chrono::system_clock::time_point deadlineTimePoint = (newDeadline == IStreamerTypes::_noDeadline)
            ? FileRequest::s_noDeadlineTime
            : VStd::chrono::system_clock::now() + newDeadline;
        request->m_request.CreateReschedule(VStd::move(target), deadlineTimePoint, newPriority);
        return request;
    }

    FileRequestPtr Streamer::CreateDedicatedCache(VStd::string_view relativePath)
    {
        FileRequestPtr result = CreateRequest();
        CreateDedicatedCache(result, relativePath);
        return result;
    }

    FileRequestPtr& Streamer::CreateDedicatedCache(FileRequestPtr& request, VStd::string_view relativePath)
    {
        RequestPath path;
        path.InitFromRelativePath(relativePath);
        request->m_request.CreateDedicatedCacheCreation(VStd::move(path));
        return request;
    }

    FileRequestPtr Streamer::DestroyDedicatedCache(VStd::string_view relativePath)
    {
        FileRequestPtr result = CreateRequest();
        DestroyDedicatedCache(result, relativePath);
        return result;
    }

    FileRequestPtr& Streamer::DestroyDedicatedCache(FileRequestPtr& request, VStd::string_view relativePath)
    {
        RequestPath path;
        path.InitFromRelativePath(relativePath);
        request->m_request.CreateDedicatedCacheDestruction(VStd::move(path));
        return request;
    }

    FileRequestPtr Streamer::FlushCache(VStd::string_view relativePath)
    {
        FileRequestPtr result = CreateRequest();
        FlushCache(result, relativePath);
        return result;
    }

    FileRequestPtr& Streamer::FlushCache(FileRequestPtr& request, VStd::string_view relativePath)
    {
        RequestPath path;
        path.InitFromRelativePath(relativePath);
        request->m_request.CreateFlush(VStd::move(path));
        return request;
    }

    FileRequestPtr Streamer::FlushCaches()
    {
        FileRequestPtr result = CreateRequest();
        FlushCaches(result);
        return result;
    }

    FileRequestPtr& Streamer::FlushCaches(FileRequestPtr& request)
    {
        request->m_request.CreateFlushAll();
        return request;
    }

    FileRequestPtr Streamer::Custom(VStd::any data)
    {
        FileRequestPtr result = CreateRequest();
        Custom(result, VStd::move(data));
        return result;
    }

    FileRequestPtr& Streamer::Custom(FileRequestPtr& request, VStd::any data)
    {
        request->m_request.CreateCustom(VStd::move(data));
        return request;
    }

    FileRequestPtr& Streamer::SetRequestCompleteCallback(FileRequestPtr& request, OnCompleteCallback callback)
    {
        request->m_request.SetCompletionCallback(VStd::move(callback));
        return request;
    }

    FileRequestPtr Streamer::CreateRequest()
    {
        return m_streamStack->CreateRequest();
    }

    void Streamer::CreateRequestBatch(VStd::vector<FileRequestPtr>& requests, size_t count)
    {
        m_streamStack->CreateRequestBatch(requests, count);
    }

    void Streamer::QueueRequest(const FileRequestPtr& request)
    {
        m_streamStack->QueueRequest(request);
    }

    void Streamer::QueueRequestBatch(const VStd::vector<FileRequestPtr>& requests)
    {
        m_streamStack->QueueRequestBatch(requests);
    }

    void Streamer::QueueRequestBatch(VStd::vector<FileRequestPtr>&& requests)
    {
        m_streamStack->QueueRequestBatch(VStd::move(requests));
    }

    bool Streamer::HasRequestCompleted(FileRequestHandle request) const
    {
        return GetRequestStatus(request) >= IStreamerTypes::RequestStatus::Completed;
    }

    IStreamerTypes::RequestStatus Streamer::GetRequestStatus(FileRequestHandle request) const
    {
        V_Assert(request.m_request, "The request handle provided to Streamer::GetRequestStatus is invalid.");
        return request.m_request->GetStatus();
    }

    VStd::chrono::system_clock::time_point Streamer::GetEstimatedRequestCompletionTime(FileRequestHandle request) const
    {
        V_Assert(request.m_request, "The request handle provided to Streamer::GetEstimatedRequestCompletionTime is invalid.");
        return request.m_request->GetEstimatedCompletion();
    }

    bool Streamer::GetReadRequestResult(FileRequestHandle request, void*& buffer, u64& numBytesRead,
        IStreamerTypes::ClaimMemory claimMemory) const
    {
        V_Assert(request.m_request, "The request handle provided to Streamer::GetReadRequestResult is invalid.");
        auto readRequest = VStd::get_if<FileRequest::ReadRequestData>(&request.m_request->GetCommand());
        if (readRequest != nullptr)
        {
            buffer = readRequest->Output;
            numBytesRead = readRequest->Size;
            if (claimMemory == IStreamerTypes::ClaimMemory::Yes)
            {
                V_Assert(HasRequestCompleted(request), "Claiming memory from a read request that's still in progress. "
                    "This can lead to crashing if data is still being streamed to the request's buffer.");
                // The caller has claimed the buffer and is now responsible for clearing it. 
                readRequest->Allocator->UnlockAllocator();
                readRequest->Allocator = nullptr;
            }
            return true;
        }
        else
        {
            V_Assert(false, "Provided file request did not contain read information");
            buffer = nullptr;
            numBytesRead = 0;
            return false;
        }
    }

    void Streamer::CollectStatistics(VStd::vector<Statistic>& statistics)
    {
        m_streamStack->CollectStatistics(statistics);
    }

    const IStreamerTypes::Recommendations& Streamer::GetRecommendations() const
    {
        return m_recommendations;
    }

    void Streamer::SuspendProcessing()
    {
        m_streamStack->SuspendProcessing();
    }

    void Streamer::ResumeProcessing()
    {
        m_streamStack->ResumeProcessing();
    }

    void Streamer::RecordStatistics()
    {
        VStd::vector<Statistic> statistics;
        m_streamStack->CollectStatistics(statistics);
        for (Statistic& stat : statistics)
        {
            switch (stat.GetType())
            {
            case Statistic::Type::FloatingPoint:
                V_PROFILE_DATAPOINT(Core, stat.GetFloatValue(), "Streamer/%.*s/%.*s",
                    v_numeric_cast<int>(stat.GetOwner().length()), stat.GetOwner().data(), v_numeric_cast<int>(stat.GetName().length()), stat.GetName().data());
                break;
            case Statistic::Type::Integer:
                V_PROFILE_DATAPOINT(Core, stat.GetIntegerValue(), "Streamer/%.*s/%.*s",
                    v_numeric_cast<int>(stat.GetOwner().length()), stat.GetOwner().data(), v_numeric_cast<int>(stat.GetName().length()), stat.GetName().data());
                break;
            case Statistic::Type::Percentage:
                V_PROFILE_DATAPOINT_PERCENT(Core, stat.GetPercentage(), "Streamer/%.*s/%.*s (percent)",
                    v_numeric_cast<int>(stat.GetOwner().length()), stat.GetOwner().data(), v_numeric_cast<int>(stat.GetName().length()), stat.GetName().data());
                break;
            default:
                V_Assert(false, "Unsupported statistic type: %i", stat.GetType());
                break;
            }
        }
    }

    FileRequestPtr Streamer::Report(FileRequest::ReportData::ReportType reportType)
    {
        FileRequestPtr result = CreateRequest();
        Report(result, reportType);
        return result;
    }

    FileRequestPtr& Streamer::Report(FileRequestPtr& request, FileRequest::ReportData::ReportType reportType)
    {
        request->m_request.CreateReport(reportType);
        return request;
    }
    
    Streamer::Streamer(const VStd::thread_desc& threadDesc, VStd::unique_ptr<Scheduler> streamStack)
        : m_streamStack(VStd::move(streamStack))
    {
        m_streamStack->GetRecommendations(m_recommendations);
        m_streamStack->Start(threadDesc);
    }

    Streamer::~Streamer()
    {
        m_streamStack->Stop();
    }
} // namespace V::IO
