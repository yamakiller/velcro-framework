#ifndef V_FRAMEWORK_CORE_IO_STREAMER_FILE_REQUEST_H
#define V_FRAMEWORK_CORE_IO_STREAMER_FILE_REQUEST_H

#include <vcore/base.h>
#include <vcore/io/compression_bus.h>
#include <vcore/io/istreamer_types.h>
#include <vcore/io/streamer/file_range.h>
#include <vcore/io/streamer/request_path.h>
#include <vcore/memory/memory.h>
#include <vcore/std/any.h>
#include <vcore/std/functional.h>
#include <vcore/std/containers/variant.h>
#include <vcore/std/chrono/clocks.h>
#include <vcore/std/parallel/atomic.h>
#include <vcore/std/smart_ptr/intrusive_ptr.h>
#include <vcore/std/smart_ptr/shared_ptr.h>
#include <vcore/std/string/string_view.h>

namespace V
{
    namespace IO
    {
        class StreamStackEntry;
        class ExternalFileRequest;

        using FileRequestPtr = VStd::intrusive_ptr<ExternalFileRequest>;
        
        class FileRequest final
        {
        public:
            inline constexpr static VStd::chrono::system_clock::time_point s_noDeadlineTime = VStd::chrono::system_clock::time_point::max();

            friend class StreamerContext;
            friend class ExternalFileRequest;

            //! Stores a reference to the external request so it stays alive while the request is being processed.
            //! This is needed because Streamer supports fire-and-forget requests since completion can be handled by
            //! registering a callback.
            struct ExternalRequestData
            {
                inline constexpr static IStreamerTypes::Priority _orderPriority = IStreamerTypes::_priorityMedium;
                inline constexpr static bool _failWhenUnhandled = true;

                explicit ExternalRequestData(FileRequestPtr&& request);
                
                FileRequestPtr Request; //!< The request that was send to Streamer.
            };

            //! Stores an instance of a RequestPath. To reduce copying instances of a RequestPath functions that
            //! need a path take them by reference to the original request. In some cases a path originates from
            //! within in the stack and temporary storage is needed. This struct allows for that temporary storage
            //! so it can be safely referenced later.
            struct RequestPathStoreData
            {
                inline constexpr static IStreamerTypes::Priority _orderPriority = IStreamerTypes::_priorityMedium;
                inline constexpr static bool _failWhenUnhandled = true;

                explicit RequestPathStoreData(RequestPath path);

                RequestPath Path;
            };

            //! Request to read data. This is an untranslated request and holds a relative path. The Scheduler
            //! will translate this to the appropriate ReadData or CompressedReadData.
            struct ReadRequestData
            {
                inline constexpr static IStreamerTypes::Priority _orderPriority = IStreamerTypes::_priorityMedium;
                inline constexpr static bool _failWhenUnhandled = true;

                ReadRequestData(RequestPath path, void* output, u64 outputSize, u64 offset, u64 size,
                    VStd::chrono::system_clock::time_point deadline, IStreamerTypes::Priority priority);
                ReadRequestData(RequestPath path, IStreamerTypes::RequestMemoryAllocator* allocator, u64 offset, u64 size,
                    VStd::chrono::system_clock::time_point deadline, IStreamerTypes::Priority priority);
                ~ReadRequestData();
                
                RequestPath Path; //!< Relative path to the target file.
                IStreamerTypes::RequestMemoryAllocator* Allocator; //!< Allocator used to manage the memory for this request.
                VStd::chrono::system_clock::time_point  Deadline; //!< Time by which this request should have been completed.
                void* Output; //!< The memory address assigned (during processing) to store the read data to.
                u64 OutputSize; //!< The memory size of the addressed used to store the read data.
                u64 Offset; //!< The offset in bytes into the file.
                u64 Size; //!< The number of bytes to read from the file.
                IStreamerTypes::Priority Priority; //!< Priority used for ordering requests. This is used when requests have the same deadline.
                IStreamerTypes::MemoryType MemoryType; //!< The type of memory provided by the allocator if used.
            };

            //! Request to read data. This is a translated request and holds an absolute path and has been
            //! resolved to the archive file if needed.
            struct ReadData
            {
                inline constexpr static IStreamerTypes::Priority _orderPriority = IStreamerTypes::_priorityMedium;
                inline constexpr static bool _failWhenUnhandled = true;

                ReadData(void* output, u64 outputSize, const RequestPath& path, u64 offset, u64 size, bool sharedRead);

                const RequestPath& Path; //!< The path to the file that contains the requested data.
                void* Output; //!< Target output to write the read data to.
                u64 OutputSize; //!< Size of memory m_output points to. This needs to be at least as big as m_size, but can be bigger.
                u64 Offset; //!< The offset in bytes into the file.
                u64 Size; //!< The number of bytes to read from the file.
                bool SharedRead; //!< True if other code will be reading from the file or the stack entry can exclusively lock.
            };

            //! Request to read and decompress data.
            struct CompressedReadData
            {
                inline constexpr static IStreamerTypes::Priority _orderPriority = IStreamerTypes::_priorityMedium;
                inline constexpr static bool _failWhenUnhandled = true;

                CompressedReadData(CompressionInfo&& compressionInfo, void* output, u64 readOffset, u64 readSize);

                CompressionInfo CompressionInfoData;
                void* Output; //!< Target output to write the read data to.
                u64 ReadOffset; //!< The offset into the decompressed to start copying from.
                u64 ReadSize; //!< Number of bytes to read from the decompressed file.
            };

              //! Holds the progress of an operation chain until this request is explicitly completed.
            struct WaitData
            {
                inline constexpr static IStreamerTypes::Priority _orderPriority = IStreamerTypes::_priorityMedium;
                inline constexpr static bool _failWhenUnhandled = true;
            };

            //! Checks to see if any node in the stack can find a file at the provided path.
            struct FileExistsCheckData
            {
                inline constexpr static IStreamerTypes::Priority _orderPriority = IStreamerTypes::_priorityHigh;
                inline constexpr static bool _failWhenUnhandled = false;

                explicit FileExistsCheckData(const RequestPath& path);

                const RequestPath& Path;
                bool Found{ false };
            };

            //! Searches for a file in the stack and retrieves the meta data. This may be slower than a file exists
            //! check.
            struct FileMetaDataRetrievalData
            {
                inline constexpr static IStreamerTypes::Priority _orderPriority = IStreamerTypes::_priorityHigh;
                inline constexpr static bool _failWhenUnhandled = false;

                explicit FileMetaDataRetrievalData(const RequestPath& path);

                const RequestPath& Path;
                u64 FileSize{ 0 };
                bool Found{ false };
            };

            //! Cancels a request in the stream stack, if possible. 
            struct CancelData
            {
                inline constexpr static IStreamerTypes::Priority _orderPriority = IStreamerTypes::_priorityHighest;
                inline constexpr static bool _failWhenUnhandled = false;

                explicit CancelData(FileRequestPtr target);

                FileRequestPtr Target; //!< The request that will be canceled.
            };

            //! Updates the priority and deadline of a request that has not been queued yet.
            struct RescheduleData
            {
                inline constexpr static IStreamerTypes::Priority _orderPriority = IStreamerTypes::_priorityHigh;
                inline constexpr static bool _failWhenUnhandled = false;

                RescheduleData(FileRequestPtr target, VStd::chrono::system_clock::time_point newDeadline, IStreamerTypes::Priority newPriority);

                FileRequestPtr Target; //!< The request that will be rescheduled.
                VStd::chrono::system_clock::time_point NewDeadline; //!< The new deadline for the request.
                IStreamerTypes::Priority NewPriority; //!< The new priority for the request.
            };

            //! Flushes all references to the provided file in the streaming stack.
            struct FlushData
            {
                inline constexpr static IStreamerTypes::Priority _orderPriority = IStreamerTypes::_priorityHigh;
                inline constexpr static bool _failWhenUnhandled = false;

                explicit FlushData(RequestPath path);

                RequestPath Path;
            };

            //! Flushes all caches in the streaming stack.
            struct FlushAllData
            {
                inline constexpr static IStreamerTypes::Priority _orderPriority = IStreamerTypes::_priorityHigh;
                inline constexpr static bool _failWhenUnhandled = false;
            };

            //! Creates a cache dedicated to a single file. This is best used for files where blocks are read from
            //! periodically such as audio banks of video files.
            struct CreateDedicatedCacheData
            {
                inline constexpr static IStreamerTypes::Priority _orderPriority = IStreamerTypes::_priorityHigh;
                inline constexpr static bool _failWhenUnhandled = false;

                CreateDedicatedCacheData(RequestPath path, const FileRange& range);

                RequestPath Path;
                FileRange   Range;
            };

            //! Destroys a cache dedicated to a single file that was previously created by CreateDedicatedCache
            struct DestroyDedicatedCacheData
            {
                inline constexpr static IStreamerTypes::Priority _orderPriority = IStreamerTypes::_priorityHigh;
                inline constexpr static bool _failWhenUnhandled = false;

                DestroyDedicatedCacheData(RequestPath path, const FileRange& range);

                RequestPath Path;
                FileRange   Range;
            };

            struct ReportData
            {
                inline constexpr static IStreamerTypes::Priority _orderPriority = IStreamerTypes::_priorityLow;
                inline constexpr static bool _failWhenUnhandled = false;

                enum class ReportType
                {
                    FileLocks
                };

                explicit ReportData(ReportType reportType);

                ReportType ReportTypeValue;
            };

            //! Data for a custom command. This can be used by nodes added extensions that need data that can't be stored
            //! in the already provided data.
            struct CustomData
            {
                inline constexpr static IStreamerTypes::Priority _orderPriority = IStreamerTypes::_priorityMedium;
                
                CustomData(VStd::any data, bool failWhenUnhandled);

                VStd::any Data; //!< The data for the custom request.
                bool FailWhenUnhandled; //!< Whether or not the request is marked as failed or success when no node process it.
            };

            using CommandVariant = VStd::variant<VStd::monostate, ExternalRequestData, RequestPathStoreData, ReadRequestData, ReadData,
                CompressedReadData, WaitData, FileExistsCheckData, FileMetaDataRetrievalData, CancelData, RescheduleData, FlushData,
                FlushAllData, CreateDedicatedCacheData, DestroyDedicatedCacheData, ReportData, CustomData>;
            using OnCompletionCallback = VStd::function<void(FileRequest& request)>;

            V_CLASS_ALLOCATOR(FileRequest, SystemAllocator, 0);

            enum class Usage : u8
            {
                Internal,
                External
            };

            void CreateRequestLink(FileRequestPtr&& request);
            void CreateRequestPathStore(FileRequest* parent, RequestPath path);
            void CreateReadRequest(RequestPath path, void* output, u64 outputSize, u64 offset, u64 size,
                VStd::chrono::system_clock::time_point deadline, IStreamerTypes::Priority priority);
            void CreateReadRequest(RequestPath path, IStreamerTypes::RequestMemoryAllocator* allocator, u64 offset, u64 size,
                VStd::chrono::system_clock::time_point deadline, IStreamerTypes::Priority priority);
            void CreateRead(FileRequest* parent, void* output, u64 outputSize, const RequestPath& path, u64 offset, u64 size, bool sharedRead = false);
            void CreateCompressedRead(FileRequest* parent, const CompressionInfo& compressionInfo, void* output,
                u64 readOffset, u64 readSize);
            void CreateCompressedRead(FileRequest* parent, CompressionInfo&& compressionInfo, void* output,
                u64 readOffset, u64 readSize);
            void CreateWait(FileRequest* parent);
            void CreateFileExistsCheck(const RequestPath& path);
            void CreateFileMetaDataRetrieval(const RequestPath& path);
            void CreateCancel(FileRequestPtr target);
            void CreateReschedule(FileRequestPtr target, VStd::chrono::system_clock::time_point newDeadline, IStreamerTypes::Priority newPriority);
            void CreateFlush(RequestPath path);
            void CreateFlushAll();
            void CreateDedicatedCacheCreation(RequestPath path, const FileRange& range = {}, FileRequest* parent = nullptr);
            void CreateDedicatedCacheDestruction(RequestPath path, const FileRange& range = {}, FileRequest* parent = nullptr);
            void CreateReport(ReportData::ReportType reportType);
            void CreateCustom(VStd::any data, bool failWhenUnhandled = true, FileRequest* parent = nullptr);

            void SetCompletionCallback(OnCompletionCallback callback);

            CommandVariant& GetCommand();
            const CommandVariant& GetCommand() const;

                IStreamerTypes::RequestStatus GetStatus() const;
            void SetStatus(IStreamerTypes::RequestStatus newStatus);
            FileRequest* GetParent();
            const FileRequest* GetParent() const;
            size_t GetNumDependencies() const;
            static constexpr size_t GetMaxNumDependencies();
            //! Whether or not this request should fail if no node in the chain has picked up the request.
            bool FailsWhenUnhandled() const;
            
            //! Checks the chain of request for the provided command. Returns the command if found, otherwise null.
            template<typename T> T* GetCommandFromChain();
            //! Checks the chain of request for the provided command. Returns the command if found, otherwise null.
            template<typename T> const T* GetCommandFromChain() const;

            //! Determines if this request is contributing to the external request.
            bool WorksOn(FileRequestPtr& request) const;

            //! Returns the id that's assigned to the request when it was added to the pending queue.
            //! The id will always increment so a smaller id means it was originally queued earlier.
            size_t GetPendingId() const;

            //! Set the estimated completion time for this request and it's immediate parent. The general approach
            //! to getting the final estimation is to bubble up the estimation, with ever entry in the stack adding
            //! it's own additional delay.
            void SetEstimatedCompletion(VStd::chrono::system_clock::time_point time);
            VStd::chrono::system_clock::time_point GetEstimatedCompletion() const;
        private:
            explicit FileRequest(Usage usage = Usage::Internal);
            ~FileRequest();

            void Reset();
            void SetOptionalParent(FileRequest* parent);

            inline static void OnCompletionPlaceholder(const FileRequest& /*request*/) {}
        private:
            //! Command and parameters for the request.
            CommandVariant m_command;

            //! Status of the request.
            VStd::atomic<IStreamerTypes::RequestStatus> m_status{ IStreamerTypes::RequestStatus::Pending };

             //! Called once the request has completed. This will always be called from the Streamer thread
            //! and thread safety is the responsibility of called function. When assigning a lambda avoid
            //! capturing a FileRequestPtr by value as this will cause a circular reference which causes
            //! the FileRequestPtr to never be released and causes a memory leak. This call will
            //! block the main Streamer thread until it returns so callbacks should be kept short. If
            //! a longer running task is needed consider using a job to do the work.
            OnCompletionCallback m_onCompletion;

            //! Estimated time this request will complete. This is an estimation and depends on many
            //! factors which can cause it to change drastically from moment to moment.
            VStd::chrono::system_clock::time_point m_estimatedCompletion;

            //! The file request that has a dependency on this one. This can be null if there are no
            //! other request depending on this one to complete.
            FileRequest* m_parent{ nullptr };

            //! Id assigned when the request is added to the pending queue.
            size_t m_pendingId{ 0 };

            //! The number of dependent file request that need to complete before this one is done.
            u16 m_dependencies{ 0 };

            //! Internal request. If this is true the request is created inside the streaming stack and never
            //! leaves it. If true it will automatically be maintained by the scheduler, if false than it's
            //! up to the owner to recycle this request.
            Usage m_usage{ Usage::Internal };

            //! Whether or not this request is currently in a recycle bin. This allows detecting double deletes.
            bool m_inRecycleBin{ false };
        };

        class StreamerContext;
        class FileRequestHandle;

        //! ExternalFileRequest is a wrapper around the FileRequest so it's safe to use outside the
        //! Streaming Stack. The main differences are that ExternalFileRequest is used in a thread-safe
        //! context and it doesn't get automatically destroyed upon completion. Instead intrusive_ptr is
        //! used to handle clean up.
        class ExternalFileRequest final
        {
            friend struct VStd::IntrusivePtrCountPolicy<ExternalFileRequest>;
            friend class FileRequestHandle;
            friend class FileRequest;
            friend class Streamer;
            friend class StreamerContext;
            friend class Scheduler;
            friend class Device;
            friend bool operator==(const FileRequestHandle& lhs, const FileRequestPtr& rhs);

        public:
            V_CLASS_ALLOCATOR(ExternalFileRequest, SystemAllocator, 0);

            explicit ExternalFileRequest(StreamerContext* owner);

        private:
            void add_ref();
            void release();

            FileRequest m_request;
            VStd::atomic_uint64_t m_refCount{ 0 };
            StreamerContext* m_owner;
        };

        class FileRequestHandle
        {
        public:
            friend class Streamer;
            friend bool operator==(const FileRequestHandle& lhs, const FileRequestPtr& rhs);

            // Intentional cast operator.
            FileRequestHandle(FileRequest& request)
                : m_request(&request)
            {}

            // Intentional cast operator.
            FileRequestHandle(const FileRequestPtr& request)
                : m_request(request ? &request->m_request : nullptr)
            {}

        private:
            FileRequest* m_request;
        };

        bool operator==(const FileRequestHandle& lhs, const FileRequestPtr& rhs);
        bool operator==(const FileRequestPtr& lhs, const FileRequestHandle& rhs);
        bool operator!=(const FileRequestHandle& lhs, const FileRequestPtr& rhs);
        bool operator!=(const FileRequestPtr& lhs, const FileRequestHandle& rhs);
    } // namespace IO
} // namespace V

#include <vcore/io/streamer/file_request.inl>

#endif // V_FRAMEWORK_CORE_IO_STREAMER_FILE_REQUEST_H