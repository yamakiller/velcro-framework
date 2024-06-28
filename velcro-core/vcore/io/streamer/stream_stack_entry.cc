#include <limits>
#include <vcore/io/streamer/file_request.h>
#include <vcore/io/streamer/stream_stack_entry.h>

namespace V
{
    namespace IO
    {
        StreamStackEntry::StreamStackEntry(VStd::string&& name)
            : m_name(VStd::move(name))
        {
        }

        const VStd::string& StreamStackEntry::GetName() const
        {
            return m_name;
        }

        void StreamStackEntry::SetNext(VStd::shared_ptr<StreamStackEntry> next)
        {
            m_next = VStd::move(next);
        }

        VStd::shared_ptr<StreamStackEntry> StreamStackEntry::GetNext() const
        {
            return m_next;
        }

        void StreamStackEntry::SetContext(StreamerContext& context)
        {
            m_context = &context;
            if (m_next)
            {
                m_next->SetContext(context);
            }
        }

        void StreamStackEntry::PrepareRequest(FileRequest* request)
        {
            if (m_next)
            {
                m_next->PrepareRequest(request);
            }
            else
            {
                m_context->PushPreparedRequest(request);
            }
        }

        void StreamStackEntry::QueueRequest(FileRequest* request)
        {
            if (m_next)
            {
                m_next->QueueRequest(request);
            }
            else
            {
                request->SetStatus(request->FailsWhenUnhandled() ? IStreamerTypes::RequestStatus::Failed : IStreamerTypes::RequestStatus::Completed);
                m_context->MarkRequestAsCompleted(request);
            }
        }

        bool StreamStackEntry::ExecuteRequests()
        {
            if (m_next)
            {
                return m_next->ExecuteRequests();
            }
            else
            {
                return false;
            }
        }

        void StreamStackEntry::UpdateCompletionEstimates(VStd::chrono::system_clock::time_point now,
            VStd::vector<FileRequest*>& internalPending, StreamerContext::PreparedQueue::iterator pendingBegin,
            StreamerContext::PreparedQueue::iterator pendingEnd)
        {
            if (m_next)
            {
                m_next->UpdateCompletionEstimates(now, internalPending, pendingBegin, pendingEnd);
            }
        }

        void StreamStackEntry::UpdateStatus(Status& status) const
        {
            if (m_next)
            {
                m_next->UpdateStatus(status);
            }
        }

        void StreamStackEntry::CollectStatistics(VStd::vector<Statistic>& statistics) const
        {
            if (m_next)
            {
                m_next->CollectStatistics(statistics);
            }
        }
    } // namespace IO
} // namespace V