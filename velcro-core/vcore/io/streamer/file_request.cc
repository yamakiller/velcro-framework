#include <limits>
#include <vcore/casting/numeric_cast.h>
#include <vcore/io/streamer/request_path.h>
#include <vcore/io/streamer/file_request.h>
#include <vcore/io/streamer/streamer_context.h>

namespace V
{
    namespace IO
    {
        //
        // Command structures.
        //

        FileRequest::ExternalRequestData::ExternalRequestData(FileRequestPtr&& request)
            : Request(VStd::move(request))
        {}

        FileRequest::RequestPathStoreData::RequestPathStoreData(RequestPath path)
            : Path(VStd::move(path))
        {}

        FileRequest::ReadRequestData::ReadRequestData(RequestPath path, void* output, u64 outputSize, u64 offset, u64 size,
            VStd::chrono::system_clock::time_point deadline, IStreamerTypes::Priority priority)
            : Path(VStd::move(path))
            , Allocator(nullptr)
            , Deadline(deadline)
            , Output(output)
            , OutputSize(outputSize)
            , Offset(offset)
            , Size(size)
            , Priority(priority)
            , MemoryType(IStreamerTypes::MemoryType::ReadWrite) // Only generic memory can be assigned externally.
        {}

         FileRequest::ReadRequestData::ReadRequestData(RequestPath path, IStreamerTypes::RequestMemoryAllocator* allocator,
            u64 offset, u64 size, VStd::chrono::system_clock::time_point deadline, IStreamerTypes::Priority priority)
            : Path(VStd::move(path))
            , Allocator(allocator)
            , Deadline(deadline)
            , Output(nullptr)
            , OutputSize(0)
            , Offset(offset)
            , Size(size)
            , Priority(priority)
            , MemoryType(IStreamerTypes::MemoryType::ReadWrite) // Only generic memory can be assigned externally.
        {}

        FileRequest::ReadRequestData::~ReadRequestData()
        {
            if (Allocator != nullptr)
            {
                if (Output != nullptr)
                {
                    Allocator->Release(Output);
                }
                Allocator->UnlockAllocator();
            }
        }

         FileRequest::ReadData::ReadData(void* output, u64 outputSize, const RequestPath& path, u64 offset, u64 size, bool sharedRead)
            : Output(output)
            , OutputSize(outputSize)
            , Path(path)
            , Offset(offset)
            , Size(size)
            , SharedRead(sharedRead)
        {}

        FileRequest::CompressedReadData::CompressedReadData(CompressionInfo&& compressionInfo, void* output, u64 readOffset, u64 readSize)
            : CompressionInfoData(VStd::move(compressionInfo))
            , Output(output)
            , ReadOffset(readOffset)
            , ReadSize(readSize)
        {}

        FileRequest::FileExistsCheckData::FileExistsCheckData(const RequestPath& path)
            : Path(path)
        {}

        FileRequest::FileMetaDataRetrievalData::FileMetaDataRetrievalData(const RequestPath& path)
            : Path(path)
        {}

        FileRequest::CancelData::CancelData(FileRequestPtr target)
            : Target(VStd::move(target))
        {}

        FileRequest::FlushData::FlushData(RequestPath path)
            : Path(VStd::move(path))
        {}

        FileRequest::RescheduleData::RescheduleData(FileRequestPtr target, VStd::chrono::system_clock::time_point newDeadline,
            IStreamerTypes::Priority newPriority)
            : Target(VStd::move(target))
            , NewDeadline(newDeadline)
            , NewPriority(newPriority)
        {}

        FileRequest::CreateDedicatedCacheData::CreateDedicatedCacheData(RequestPath path, const FileRange& range)
            : Path(VStd::move(path))
            , Range(range)
        {}

        FileRequest::DestroyDedicatedCacheData::DestroyDedicatedCacheData(RequestPath path, const FileRange& range)
            : Path(VStd::move(path))
            , Range(range)
        {}

        FileRequest::ReportData::ReportData(ReportType reportType)
            : ReportTypeValue(reportType)
        {}

        FileRequest::CustomData::CustomData(VStd::any data, bool failWhenUnhandled)
            : Data(VStd::move(data))
            , FailWhenUnhandled(failWhenUnhandled)
        {}

          //
        // FileRequest
        //

        FileRequest::FileRequest(Usage usage)
            : m_usage(usage)
        {
            Reset();
        }
        
        FileRequest::~FileRequest()
        {
            Reset();
        }

        void FileRequest::CreateRequestLink(FileRequestPtr&& request)
        {
            V_Assert(VStd::holds_alternative<VStd::monostate>(m_command),
                "Attempting to set FileRequest to 'RequestLink', but another task was already assigned.");
            m_parent = request->m_request.m_parent;
            request->m_request.m_parent = this;
            m_dependencies++;
            m_command.emplace<ExternalRequestData>(VStd::move(request));
        }

        void FileRequest::CreateRequestPathStore(FileRequest* parent, RequestPath path)
        {
            V_Assert(VStd::holds_alternative<VStd::monostate>(m_command),
                "Attempting to set FileRequest to 'CreateRequestPathStore', but another task was already assigned.");
            m_command.emplace<RequestPathStoreData>(VStd::move(path));
            SetOptionalParent(parent);
        }

        void FileRequest::CreateReadRequest(RequestPath path, void* output, u64 outputSize, u64 offset, u64 size,
            VStd::chrono::system_clock::time_point deadline, IStreamerTypes::Priority priority)
        {
            V_Assert(VStd::holds_alternative<VStd::monostate>(m_command),
                "Attempting to set FileRequest to 'ReadRequest', but another task was already assigned.");
            m_command.emplace<ReadRequestData>(VStd::move(path), output, outputSize, offset, size, deadline, priority);
        }

         void FileRequest::CreateReadRequest(RequestPath path, IStreamerTypes::RequestMemoryAllocator* allocator, u64 offset, u64 size,
            VStd::chrono::system_clock::time_point deadline, IStreamerTypes::Priority priority)
        {
            V_Assert(VStd::holds_alternative<VStd::monostate>(m_command),
                "Attempting to set FileRequest to 'ReadRequest', but another task was already assigned.");
            m_command.emplace<ReadRequestData>(VStd::move(path), allocator, offset, size, deadline, priority);
        }

        void FileRequest::CreateRead(FileRequest* parent, void* output, u64 outputSize, const RequestPath& path,
            u64 offset, u64 size, bool sharedRead)
        {
            V_Assert(VStd::holds_alternative<VStd::monostate>(m_command),
                "Attempting to set FileRequest to 'Read', but another task was already assigned.");
            m_command.emplace<ReadData>(output, outputSize, VStd::move(path), offset, size, sharedRead);
            SetOptionalParent(parent);
        }

        void FileRequest::CreateCompressedRead(FileRequest* parent, const CompressionInfo& compressionInfo,
            void* output, u64 readOffset, u64 readSize)
        {
            CreateCompressedRead(parent, CompressionInfo(compressionInfo), output, readOffset, readSize);
        }

        void FileRequest::CreateCompressedRead(FileRequest* parent, CompressionInfo&& compressionInfo, void* output, u64 readOffset, u64 readSize)
        {
            V_Assert(VStd::holds_alternative<VStd::monostate>(m_command),
                "Attempting to set FileRequest to 'CompressedRead', but another task was already assigned.");
            m_command.emplace<CompressedReadData>(VStd::move(compressionInfo), output, readOffset, readSize);
            SetOptionalParent(parent);
        }

        void FileRequest::CreateWait(FileRequest* parent)
        {
            V_Assert(VStd::holds_alternative<VStd::monostate>(m_command),
                "Attempting to set FileRequest to 'Wait', but another task was already assigned.");
            m_command.emplace<WaitData>();
            SetOptionalParent(parent);
        }

        void FileRequest::CreateFileExistsCheck(const RequestPath& path)
        {
            V_Assert(VStd::holds_alternative<VStd::monostate>(m_command),
                "Attempting to set FileRequest to 'FileExistsCheck', but another task was already assigned.");
            m_command.emplace<FileExistsCheckData>(path);
        }

        void FileRequest::CreateFileMetaDataRetrieval(const RequestPath& path)
        {
            V_Assert(VStd::holds_alternative<VStd::monostate>(m_command),
                "Attempting to set FileRequest to 'FileMetaDataRetrieval', but another task was already assigned.");
            m_command.emplace<FileMetaDataRetrievalData>(path);
        }

        void FileRequest::CreateCancel(FileRequestPtr target)
        {
            V_Assert(VStd::holds_alternative<VStd::monostate>(m_command),
                "Attempting to set FileRequest to 'Cancel', but another task was already assigned.");
            m_command.emplace<CancelData>(VStd::move(target));
        }

        void FileRequest::CreateReschedule(FileRequestPtr target, VStd::chrono::system_clock::time_point newDeadline,
            IStreamerTypes::Priority newPriority)
        {
            V_Assert(VStd::holds_alternative<VStd::monostate>(m_command),
                "Attempting to set FileRequest to 'Reschedule', but another task was already assigned.");
            m_command.emplace<RescheduleData>(VStd::move(target), newDeadline, newPriority);
        }

        void FileRequest::CreateFlush(RequestPath path)
        {
            V_Assert(VStd::holds_alternative<VStd::monostate>(m_command),
                "Attempting to set FileRequest to 'Flush', but another task was already assigned.");
            m_command.emplace<FlushData>(VStd::move(path));
        }

        void FileRequest::CreateFlushAll()
        {
            V_Assert(VStd::holds_alternative<VStd::monostate>(m_command),
                "Attempting to set FileRequest to 'FlushAll', but another task was already assigned.");
            m_command.emplace<FlushAllData>();
        }

        void FileRequest::CreateDedicatedCacheCreation(RequestPath path, const FileRange& range, FileRequest* parent)
        {
            V_Assert(VStd::holds_alternative<VStd::monostate>(m_command),
                "Attempting to set FileRequest to 'CreateDedicateCache', but another task was already assigned.");
            m_command.emplace<CreateDedicatedCacheData>(VStd::move(path), range);
            SetOptionalParent(parent);
        }

        void FileRequest::CreateDedicatedCacheDestruction(RequestPath path, const FileRange& range, FileRequest* parent)
        {
            V_Assert(VStd::holds_alternative<VStd::monostate>(m_command),
                "Attempting to set FileRequest to 'DestroyDedicateCache', but another task was already assigned.");
            m_command.emplace<DestroyDedicatedCacheData>(VStd::move(path), range);
            SetOptionalParent(parent);
        }

        void FileRequest::CreateReport(ReportData::ReportType reportType)
        {
            V_Assert(VStd::holds_alternative<VStd::monostate>(m_command),
                "Attempting to set FileRequest to 'Report', but another task was already assigned.");
            m_command.emplace<ReportData>(reportType);
        }

        void FileRequest::CreateCustom(VStd::any data, bool failWhenUnhandled, FileRequest* parent)
        {
            V_Assert(VStd::holds_alternative<VStd::monostate>(m_command),
                "Attempting to set FileRequest to 'Custom', but another task was already assigned.");
            m_command.emplace<CustomData>(VStd::move(data), failWhenUnhandled);
            SetOptionalParent(parent);
        }

        void FileRequest::SetCompletionCallback(OnCompletionCallback callback)
        {
            m_onCompletion = VStd::move(callback);
        }

        FileRequest::CommandVariant& FileRequest::GetCommand()
        {
            return m_command;
        }

        const FileRequest::CommandVariant& FileRequest::GetCommand() const
        {
            return m_command;
        }

                IStreamerTypes::RequestStatus FileRequest::GetStatus() const
        {
            return m_status;
        }

        void FileRequest::SetStatus(IStreamerTypes::RequestStatus newStatus)
        {
            IStreamerTypes::RequestStatus currentStatus = m_status;
            switch (newStatus)
            {
            case IStreamerTypes::RequestStatus::Pending:
                [[fallthrough]];
            case IStreamerTypes::RequestStatus::Queued:
                [[fallthrough]];
            case IStreamerTypes::RequestStatus::Processing:
                if (currentStatus == IStreamerTypes::RequestStatus::Failed ||
                    currentStatus == IStreamerTypes::RequestStatus::Canceled ||
                    currentStatus == IStreamerTypes::RequestStatus::Completed)
                {
                    return;
                }
                break;
            case IStreamerTypes::RequestStatus::Completed:
                if (currentStatus == IStreamerTypes::RequestStatus::Failed || currentStatus == IStreamerTypes::RequestStatus::Canceled)
                {
                    return;
                }
                break;
            case IStreamerTypes::RequestStatus::Canceled:
                if (currentStatus == IStreamerTypes::RequestStatus::Failed || currentStatus == IStreamerTypes::RequestStatus::Completed)
                {
                    return;
                }
                break;
            case IStreamerTypes::RequestStatus::Failed:
                [[fallthrough]];
            default:
                break;
            }
            m_status = newStatus;
        }

        FileRequest* FileRequest::GetParent()
        {
            return m_parent;
        }

        const FileRequest* FileRequest::GetParent() const
        {
            return m_parent;
        }

        size_t FileRequest::GetNumDependencies() const
        {
            return m_dependencies;
        }

        bool FileRequest::FailsWhenUnhandled() const
        {
            return VStd::visit([](auto&& args)
            {
                using Command = VStd::decay_t<decltype(args)>;
                if constexpr (VStd::is_same_v<Command, VStd::monostate>)
                {
                    V_Assert(false,
                        "Request does not contain a valid command. It may have been reset already or was never assigned a command.");
                    return true;
                }
                else if constexpr (VStd::is_same_v<Command, FileRequest::CustomData>)
                {
                    return args.FailWhenUnhandled;
                }
                else
                {
                    return Command::_failWhenUnhandled;
                }
            }, m_command);
        }

        void FileRequest::Reset()
        {
            m_command = VStd::monostate{};
            m_onCompletion = &OnCompletionPlaceholder;
            m_estimatedCompletion = VStd::chrono::system_clock::time_point();
            m_parent = nullptr;
            m_status = IStreamerTypes::RequestStatus::Pending;
            m_dependencies = 0;
        }

        void FileRequest::SetOptionalParent(FileRequest* parent)
        {
            if (parent)
            {
                m_parent = parent;
                V_Assert(parent->m_dependencies < std::numeric_limits<decltype(parent->m_dependencies)>::max(),
                    "A file request dependency was added, but the parent can't have any more dependencies.");
                ++parent->m_dependencies;
            }
        }

        bool FileRequest::WorksOn(FileRequestPtr& request) const
        {
            const FileRequest* current = this;
            while (current)
            {
                auto* link = VStd::get_if<ExternalRequestData>(&current->m_command);
                if (!link)
                {
                    current = current->m_parent;
                }
                else
                {
                    return link->Request == request;
                }
            }
            return false;
        }

                size_t FileRequest::GetPendingId() const
        {
            return m_pendingId;
        }

        void FileRequest::SetEstimatedCompletion(VStd::chrono::system_clock::time_point time)
        {
            FileRequest* current = this;
            do
            {
                current->m_estimatedCompletion = time;
                current = current->m_parent;
            } while (current);
        }

        VStd::chrono::system_clock::time_point FileRequest::GetEstimatedCompletion() const
        {
            return m_estimatedCompletion;
        }

             //
        // ExternalFileRequest
        //

        ExternalFileRequest::ExternalFileRequest(StreamerContext* owner)
            : m_request(FileRequest::Usage::External)
            , m_owner(owner)
        {
        }

        void ExternalFileRequest::add_ref()
        {
            m_refCount++;
        }

        void ExternalFileRequest::release()
        {
            if (--m_refCount == 0)
            {
                V_Assert(m_owner, "No owning context set for the file request.");
                m_owner->RecycleRequest(this);
            }
        }

        bool operator==(const FileRequestHandle& lhs, const FileRequestPtr& rhs)
        {
            return lhs.m_request == &rhs->m_request;
        }

        bool operator==(const FileRequestPtr& lhs, const FileRequestHandle& rhs)
        {
            return rhs == lhs;
        }

        bool operator!=(const FileRequestHandle& lhs, const FileRequestPtr& rhs)
        {
            return !(lhs == rhs);
        }

        bool operator!=(const FileRequestPtr& lhs, const FileRequestHandle& rhs)
        {
            return !(rhs == lhs);
        }
    } // namespace IO
} // namespace V