#include <vcore/std/algorithm.h>
#include <vcore/Debug/Profiler.h>
#include <vcore/io/streamer/file_request.h>
#include <vcore/io/streamer/streamer_context.h>
#include <vcore/io/streamer/stream_stack_entry.h>

namespace V
{
    namespace IO
    {
        static constexpr char ContextName[] = "Context";
#if V_STREAMER_ADD_EXTRA_PROFILING_INFO
        static constexpr char PredictionAccuracyName[] = "Prediction accuracy (ms)";
        static constexpr char LatePredictionName[] = "Early completions";
        static constexpr char MissedDeadlinesName[] = "Missed deadlines";
#endif // V_STREAMER_ADD_EXTRA_PROFILING_INFO

        StreamerContext::~StreamerContext()
        {
            for (FileRequest* entry : m_internalRecycleBin)
            {
                delete entry;
            }

            VStd::scoped_lock<VStd::mutex> guard(m_externalRecycleBinGuard);
            for (ExternalFileRequest* entry : m_externalRecycleBin)
            {
                delete entry;
            }
        }

         FileRequest* StreamerContext::GetNewInternalRequest()
        {
            if (m_internalRecycleBin.empty())
            {
                if (m_internalRecycleBin.capacity() > 0)
                {
                    // There are no requests left in the recycle bin so create a new one. This will
                    // eventually end up in the recycle bin again.
                    return vnew FileRequest();
                }
                else
                {
                    // Create a few requests up front so the engine initialization doesn't
                    // constantly create new instances and resizes the recycle bin.
                    m_internalRecycleBin.reserve(_initialRecycleBinSize);
                    for (size_t i = 0; i < _initialRecycleBinSize; ++i)
                    {
                        auto request = vnew FileRequest();
                        request->m_inRecycleBin = true;
                        m_internalRecycleBin.push_back(request);
                    }
                }
            }

            FileRequest* result = m_internalRecycleBin.back();
            V_Assert(result->m_usage == FileRequest::Usage::Internal, "FileRequest ended up in the wrong recycle bin.");
            V_Assert(VStd::holds_alternative<VStd::monostate>(result->m_command), "FileRequest wasn't properly reset.");
            result->m_inRecycleBin = false;
            m_internalRecycleBin.pop_back();
            return result;
        }

        FileRequestPtr StreamerContext::GetNewExternalRequest()
        {
            VStd::scoped_lock<VStd::mutex> guard(m_externalRecycleBinGuard);
            return GetNewExternalRequestUnguarded();
        }

        void StreamerContext::GetNewExternalRequestBatch(VStd::vector<FileRequestPtr>& requests, size_t count)
        {
            VStd::scoped_lock<VStd::mutex> guard(m_externalRecycleBinGuard);
            for (size_t i = 0; i < count; ++i)
            {
                requests.push_back(GetNewExternalRequestUnguarded());
            }
        }

               size_t StreamerContext::GetNumPreparedRequests() const
        {
            return m_preparedRequests.size();
        }

        FileRequest* StreamerContext::PopPreparedRequest()
        {
            FileRequest* result = m_preparedRequests.front();
            m_preparedRequests.pop_front();
            return result;
        }

        void StreamerContext::PushPreparedRequest(FileRequest* request)
        {
            request->m_pendingId = ++m_pendingIdCounter;
            m_preparedRequests.push_back(request);
        }

        auto StreamerContext::GetPreparedRequests()->PreparedQueue&
        {
            return m_preparedRequests;
        }

        auto StreamerContext::GetPreparedRequests() const->const PreparedQueue&
        {
            return m_preparedRequests;
        }

        void StreamerContext::MarkRequestAsCompleted(FileRequest* request)
        {
            VStd::scoped_lock<VStd::recursive_mutex> guard(m_completedGuard);
            V_Assert(request->m_dependencies == 0, "A request with dependencies is marked as completed.");
            V_Assert(!request->m_inRecycleBin, "Request that's already been marked for deletion has been added again.");
            m_completed.push(request);
        }

        FileRequest* StreamerContext::RejectRequest(FileRequest* request)
        {
            V_Assert(request->m_dependencies == 0, "Requests with children can not be safely rejected.");
            FileRequest* parent = request->m_parent;
            if (parent)
            {
                parent->m_dependencies--;
            }
            RecycleRequest(request);
            return parent;
        }

        void StreamerContext::RecycleRequest(FileRequest* request)
        {
            V_Assert(request->m_dependencies == 0, "Requests with children can not be safely recycled.");
            V_Assert(request->m_usage == FileRequest::Usage::Internal, "An external file request is being released as an internal one.");
            V_Assert(!request->m_inRecycleBin, "Internal request that's already been recycled is being recycled again.");
            request->Reset();
            request->m_inRecycleBin = true;
            m_internalRecycleBin.push_back(request);
        }

        void StreamerContext::RecycleRequest(ExternalFileRequest* request)
        {
            V_Assert(request->m_request.m_dependencies == 0, "External requests with children can not be safely recycled.");
            V_Assert(!request->m_request.m_inRecycleBin, "External request that's already been recycled is being recycled again.");
            request->m_request.Reset();
            request->m_request.m_inRecycleBin = true;

            VStd::scoped_lock<VStd::mutex> guard(m_externalRecycleBinGuard);
            m_externalRecycleBin.push_back(request);
        }

        bool StreamerContext::FinalizeCompletedRequests()
        {
            V_PROFILE_FUNCTION(Core);

#if V_STREAMER_ADD_EXTRA_PROFILING_INFO
            auto now = VStd::chrono::system_clock::now();
#endif
            bool hasCompletedRequests = false;
            while (true)
            {
                VStd::queue<FileRequest*> completed;
                {
                    VStd::scoped_lock<VStd::recursive_mutex> guard(m_completedGuard);
                    completed.swap(m_completed);
                }
                if (completed.empty())
                {
                    return hasCompletedRequests;
                }

                hasCompletedRequests = true;
                while (!completed.empty())
                {
                    FileRequest* top = completed.front();
#if V_STREAMER_ADD_EXTRA_PROFILING_INFO
                    // It's possible for a request to be queued internally and processed between scheduling passes. In those
                    // cases, don't check the request for accurate prediction.
                    if (top->m_estimatedCompletion > VStd::chrono::system_clock::time_point())
                    {
                        if (top->m_estimatedCompletion < now)
                        {
                            auto estimationDeltaUs = VStd::chrono::microseconds(now - top->m_estimatedCompletion).count();
                            m_predictionAccuracyUsStat.PushSample(v_numeric_cast<double>(estimationDeltaUs));
                            Statistic::PlotImmediate(ContextName, PredictionAccuracyName,
                                m_predictionAccuracyUsStat.GetMostRecentSample());

                            m_latePredictionsPercentageStat.PushSample(0.0);
                            Statistic::PlotImmediate(ContextName, LatePredictionName,
                                m_latePredictionsPercentageStat.GetMostRecentSample());
                        }
                        else
                        {
                            auto estimationDeltaUs = VStd::chrono::microseconds(top->m_estimatedCompletion - now).count();
                            m_predictionAccuracyUsStat.PushSample(v_numeric_cast<double>(estimationDeltaUs));
                            Statistic::PlotImmediate(ContextName, PredictionAccuracyName,
                                m_predictionAccuracyUsStat.GetMostRecentSample());

                            m_latePredictionsPercentageStat.PushSample(1.0);
                            Statistic::PlotImmediate(ContextName, LatePredictionName,
                                m_latePredictionsPercentageStat.GetMostRecentSample());
                        }
                    }
                    auto readRequest = VStd::get_if<FileRequest::ReadRequestData>(&top->GetCommand());
                    if (readRequest != nullptr)
                    {
                        m_missedDeadlinePercentageStat.PushSample(now < readRequest->m_deadline ? 0.0 : 1.0);
                        Statistic::PlotImmediate(ContextName, MissedDeadlinesName, m_missedDeadlinePercentageStat.GetMostRecentSample());
                    }
#endif // V_STREAMER_ADD_EXTRA_PROFILING_INFO

                    // Get all information before calling the completion routine as it's technically possible that an external
                    // request is recycled during the callback.
                    IStreamerTypes::RequestStatus status = top->GetStatus();
                    FileRequest* parent = top->m_parent;
                    bool isInternal = top->m_usage == FileRequest::Usage::Internal;

                    {
                        V_PROFILE_SCOPE(Core,
                            isInternal ? "Completion callback internal" : "Completion callback external");
                        top->m_onCompletion(*top);
                        V_PROFILE_INTERVAL_END(Core, top);
                    }
                    
                    if (parent)
                    {
                        V_Assert(parent->m_dependencies > 0,
                            "A file request with a parent has completed, but the request wasn't registered as a dependency with the parent.");
                        --parent->m_dependencies;

                        parent->SetStatus(status);
                        if (parent->m_dependencies == 0)
                        {
                            completed.push(parent);
                        }
                    }

                    if (isInternal)
                    {
                        RecycleRequest(top);
                    }

                    completed.pop();
                }
            }
            return hasCompletedRequests;
        }

        void StreamerContext::WakeUpSchedulingThread()
        {
            m_threadSync.Resume();
        }

        void StreamerContext::SuspendSchedulingThread()
        {
            m_threadSync.Suspend();
        }

        V::Platform::StreamerContextThreadSync& StreamerContext::GetStreamerThreadSynchronizer()
        {
            return m_threadSync;
        }

        void StreamerContext::CollectStatistics(VStd::vector<Statistic>& statistics)
        {
            statistics.push_back(
                Statistic::CreateInteger(ContextName, "Pending requests", v_numeric_caster(m_preparedRequests.size())));
#if V_STREAMER_ADD_EXTRA_PROFILING_INFO
            statistics.push_back(Statistic::CreateFloat(ContextName, PredictionAccuracyName, m_predictionAccuracyUsStat.GetAverage() / 1000.0));
            statistics.push_back(Statistic::CreatePercentage(ContextName, LatePredictionName, m_latePredictionsPercentageStat.GetAverage()));
            statistics.push_back(Statistic::CreatePercentage(ContextName, MissedDeadlinesName, m_missedDeadlinePercentageStat.GetAverage()));
#endif // V_STREAMER_ADD_EXTRA_PROFILING_INFO
            statistics.push_back(Statistic::CreateInteger(ContextName, "Total requests", v_numeric_caster(m_pendingIdCounter)));
            statistics.push_back(Statistic::CreateInteger(ContextName, "Internal bucket size", v_numeric_caster(m_internalRecycleBin.size())));
            statistics.push_back(Statistic::CreateInteger(ContextName, "External bucket size", v_numeric_caster(m_externalRecycleBin.size())));
        }

        FileRequestPtr StreamerContext::GetNewExternalRequestUnguarded()
        {
            if (m_externalRecycleBin.empty())
            {
                if (m_externalRecycleBin.capacity() > 0)
                {
                    // There are no requests left in the recycle bin so create a new one. This will
                    // eventually end up in the recycle bin again.
                    return FileRequestPtr(vnew ExternalFileRequest(this));
                }
                else
                {
                    // Create a few requests up front so the engine initialization doesn't
                    // constantly create new instances and resizes the recycle bin.
                    m_externalRecycleBin.reserve(_initialRecycleBinSize);
                    for (size_t i = 0; i < _initialRecycleBinSize; ++i)
                    {
                        auto request = vnew ExternalFileRequest(this);
                        request->m_request.m_inRecycleBin = true;
                        m_externalRecycleBin.push_back(request);
                    }
                }
            }

            FileRequestPtr result = m_externalRecycleBin.back();
            V_Assert(VStd::holds_alternative<VStd::monostate>(result->m_request.m_command), "ExternalFileRequest wasn't properly reset.");
            m_externalRecycleBin.pop_back();
            result->m_request.m_inRecycleBin = false;
            return result;
        }
    } // namespace IO
} // namespace V
