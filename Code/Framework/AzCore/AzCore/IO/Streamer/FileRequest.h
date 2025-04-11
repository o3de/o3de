/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/base.h>
#include <AzCore/IO/CompressionBus.h>
#include <AzCore/IO/IStreamerTypes.h>
#include <AzCore/IO/Streamer/FileRange.h>
#include <AzCore/IO/Streamer/Statistics.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/std/any.h>
#include <AzCore/std/functional.h>
#include <AzCore/std/containers/variant.h>
#include <AzCore/std/chrono/chrono.h>
#include <AzCore/std/parallel/atomic.h>
#include <AzCore/std/smart_ptr/intrusive_ptr.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/string/string_view.h>

namespace AZ::IO
{
    class StreamStackEntry;
    class ExternalFileRequest;

    using FileRequestPtr = AZStd::intrusive_ptr<ExternalFileRequest>;
} // namespace AZ::IO

namespace AZ::IO::Requests
{
    //! Request to read data. This is a translated request and holds an absolute path and has been
    //! resolved to the archive file if needed.
    struct ReadData
    {
        inline constexpr static IStreamerTypes::Priority s_orderPriority = IStreamerTypes::s_priorityMedium;
        inline constexpr static bool s_failWhenUnhandled = true;

        ReadData(void* output, u64 outputSize, const RequestPath& path, u64 offset, u64 size, bool sharedRead);

        const RequestPath& m_path; //!< The path to the file that contains the requested data.
        void* m_output; //!< Target output to write the read data to.
        u64 m_outputSize; //!< Size of memory m_output points to. This needs to be at least as big as m_size, but can be bigger.
        u64 m_offset; //!< The offset in bytes into the file.
        u64 m_size; //!< The number of bytes to read from the file.
        bool m_sharedRead; //!< True if other code will be reading from the file or the stack entry can exclusively lock.
    };

    //! Request to read data. This is an untranslated request and holds a relative path. The Scheduler
    //! will translate this to the appropriate ReadData or CompressedReadData.
    struct ReadRequestData
    {
        inline constexpr static IStreamerTypes::Priority s_orderPriority = IStreamerTypes::s_priorityMedium;
        inline constexpr static bool s_failWhenUnhandled = true;

        ReadRequestData(
            RequestPath path,
            void* output,
            u64 outputSize,
            u64 offset,
            u64 size,
            AZStd::chrono::steady_clock::time_point deadline,
            IStreamerTypes::Priority priority);
        ReadRequestData(
            RequestPath path,
            IStreamerTypes::RequestMemoryAllocator* allocator,
            u64 offset,
            u64 size,
            AZStd::chrono::steady_clock::time_point deadline,
            IStreamerTypes::Priority priority);
        ~ReadRequestData();

        RequestPath m_path; //!< Relative path to the target file.
        IStreamerTypes::RequestMemoryAllocator* m_allocator; //!< Allocator used to manage the memory for this request.
        AZStd::chrono::steady_clock::time_point m_deadline; //!< Time by which this request should have been completed.
        void* m_output; //!< The memory address assigned (during processing) to store the read data to.
        u64 m_outputSize; //!< The memory size of the addressed used to store the read data.
        u64 m_offset; //!< The offset in bytes into the file.
        u64 m_size; //!< The number of bytes to read from the file.
        IStreamerTypes::Priority m_priority; //!< Priority used for ordering requests. This is used when requests have the same deadline.
        IStreamerTypes::MemoryType m_memoryType; //!< The type of memory provided by the allocator if used.
    };

    //! Creates a cache dedicated to a single file. This is best used for files where blocks are read from
    //! periodically such as audio banks of video files.
    struct CreateDedicatedCacheData
    {
        inline constexpr static IStreamerTypes::Priority s_orderPriority = IStreamerTypes::s_priorityHigh;
        inline constexpr static bool s_failWhenUnhandled = false;

        CreateDedicatedCacheData(RequestPath path, const FileRange& range);

        RequestPath m_path;
        FileRange m_range;
    };

    //! Destroys a cache dedicated to a single file that was previously created by CreateDedicatedCache
    struct DestroyDedicatedCacheData
    {
        inline constexpr static IStreamerTypes::Priority s_orderPriority = IStreamerTypes::s_priorityHigh;
        inline constexpr static bool s_failWhenUnhandled = false;

        DestroyDedicatedCacheData(RequestPath path, const FileRange& range);

        RequestPath m_path;
        FileRange m_range;
    };

    struct ReportData
    {
        inline constexpr static IStreamerTypes::Priority s_orderPriority = IStreamerTypes::s_priorityLow;
        inline constexpr static bool s_failWhenUnhandled = false;

        ReportData(AZStd::vector<AZ::IO::Statistic>& output, IStreamerTypes::ReportType reportType);

        AZStd::vector<AZ::IO::Statistic>& m_output;
        IStreamerTypes::ReportType m_reportType;
    };

    //! Stores a reference to the external request so it stays alive while the request is being processed.
    //! This is needed because Streamer supports fire-and-forget requests since completion can be handled by
    //! registering a callback.
    struct ExternalRequestData
    {
        inline constexpr static IStreamerTypes::Priority s_orderPriority = IStreamerTypes::s_priorityMedium;
        inline constexpr static bool s_failWhenUnhandled = true;

        explicit ExternalRequestData(FileRequestPtr&& request);

        FileRequestPtr m_request; //!< The request that was send to Streamer.
    };

    //! Stores an instance of a RequestPath. To reduce copying instances of a RequestPath functions that
    //! need a path take them by reference to the original request. In some cases a path originates from
    //! within in the stack and temporary storage is needed. This struct allows for that temporary storage
    //! so it can be safely referenced later.
    struct RequestPathStoreData
    {
        inline constexpr static IStreamerTypes::Priority s_orderPriority = IStreamerTypes::s_priorityMedium;
        inline constexpr static bool s_failWhenUnhandled = true;

        explicit RequestPathStoreData(RequestPath path);

        RequestPath m_path;
    };

    //! Request to read and decompress data.
    struct CompressedReadData
    {
        inline constexpr static IStreamerTypes::Priority s_orderPriority = IStreamerTypes::s_priorityMedium;
        inline constexpr static bool s_failWhenUnhandled = true;

        CompressedReadData(CompressionInfo&& compressionInfo, void* output, u64 readOffset, u64 readSize);

        CompressionInfo m_compressionInfo;
        void* m_output; //!< Target output to write the read data to.
        u64 m_readOffset; //!< The offset into the decompressed to start copying from.
        u64 m_readSize; //!< Number of bytes to read from the decompressed file.
    };

    //! Holds the progress of an operation chain until this request is explicitly completed.
    struct WaitData
    {
        inline constexpr static IStreamerTypes::Priority s_orderPriority = IStreamerTypes::s_priorityMedium;
        inline constexpr static bool s_failWhenUnhandled = true;
    };

    //! Checks to see if any node in the stack can find a file at the provided path.
    struct FileExistsCheckData
    {
        inline constexpr static IStreamerTypes::Priority s_orderPriority = IStreamerTypes::s_priorityHigh;
        inline constexpr static bool s_failWhenUnhandled = false;

        explicit FileExistsCheckData(const RequestPath& path);

        const RequestPath& m_path;
        bool m_found{ false };
    };

    //! Searches for a file in the stack and retrieves the meta data. This may be slower than a file exists
    //! check.
    struct FileMetaDataRetrievalData
    {
        inline constexpr static IStreamerTypes::Priority s_orderPriority = IStreamerTypes::s_priorityHigh;
        inline constexpr static bool s_failWhenUnhandled = false;

        explicit FileMetaDataRetrievalData(const RequestPath& path);

        const RequestPath& m_path;
        u64 m_fileSize{ 0 };
        bool m_found{ false };
    };

    //! Cancels a request in the stream stack, if possible.
    struct CancelData
    {
        inline constexpr static IStreamerTypes::Priority s_orderPriority = IStreamerTypes::s_priorityHighest;
        inline constexpr static bool s_failWhenUnhandled = false;

        explicit CancelData(FileRequestPtr target);

        FileRequestPtr m_target; //!< The request that will be canceled.
    };

    //! Updates the priority and deadline of a request that has not been queued yet.
    struct RescheduleData
    {
        inline constexpr static IStreamerTypes::Priority s_orderPriority = IStreamerTypes::s_priorityHigh;
        inline constexpr static bool s_failWhenUnhandled = false;

        RescheduleData(FileRequestPtr target, AZStd::chrono::steady_clock::time_point newDeadline, IStreamerTypes::Priority newPriority);

        FileRequestPtr m_target; //!< The request that will be rescheduled.
        AZStd::chrono::steady_clock::time_point m_newDeadline; //!< The new deadline for the request.
        IStreamerTypes::Priority m_newPriority; //!< The new priority for the request.
    };

    //! Flushes all references to the provided file in the streaming stack.
    struct FlushData
    {
        inline constexpr static IStreamerTypes::Priority s_orderPriority = IStreamerTypes::s_priorityHigh;
        inline constexpr static bool s_failWhenUnhandled = false;

        explicit FlushData(RequestPath path);

        RequestPath m_path;
    };

    //! Flushes all caches in the streaming stack.
    struct FlushAllData
    {
        inline constexpr static IStreamerTypes::Priority s_orderPriority = IStreamerTypes::s_priorityHigh;
        inline constexpr static bool s_failWhenUnhandled = false;
    };

    //! Data for a custom command. This can be used by nodes added extensions that need data that can't be stored
    //! in the already provided data.
    struct CustomData
    {
        inline constexpr static IStreamerTypes::Priority s_orderPriority = IStreamerTypes::s_priorityMedium;

        CustomData(AZStd::any data, bool failWhenUnhandled);

        AZStd::any m_data; //!< The data for the custom request.
        bool m_failWhenUnhandled; //!< Whether or not the request is marked as failed or success when no node process it.
    };
    using CommandVariant = AZStd::variant<
        AZStd::monostate,
        ExternalRequestData,
        RequestPathStoreData,
        ReadRequestData,
        ReadData,
        CompressedReadData,
        WaitData,
        FileExistsCheckData,
        FileMetaDataRetrievalData,
        CancelData,
        RescheduleData,
        FlushData,
        FlushAllData,
        CreateDedicatedCacheData,
        DestroyDedicatedCacheData,
        ReportData,
        CustomData>;

} // namespace AZ::IO::Requests

namespace AZ::IO
{
    class Streamer_SchedulerTest_RequestSorting_Test;

    // Note to API Callers
    // The FileRequest class is used to create requests that are processed by the Streamer, 
    // If you do use this class, note that the read requests are hierarchical, and in general, a "Parent" high level request is created to
    // for example read the entire contents of a file into a buffer, and the system then internally creates child requests with that parent
    // to split reads up and to queue them and sort them, with one parent potentially having several child requests pointed at the same file name
    // but potentially different sizes and offsets.
    // To avoid copying strings all over the place, the child requests usually take the file name by const FilePath& reference, and establish a reference
    // to the given memory instead of a copy.
    //
    // If you write code that exercises the low level child request APIs directly
    // (For example, by using CreateRead instead of CreateReadRequest), be aware that input parameters such as `const RequestPath& path` 
    // expect the given memory to be valid until the child request is complete as it is usually owned by the parent request.
    // Be especially careful if writing tests that the memory is not from a temporary object that is going to be destroyed before the streamer
    // system is exercised.

    class FileRequest final
    {
        friend Streamer_SchedulerTest_RequestSorting_Test;

    public:
        inline constexpr static AZStd::chrono::steady_clock::time_point s_noDeadlineTime = AZStd::chrono::steady_clock::time_point::max();

        friend class StreamerContext;
        friend class ExternalFileRequest;

        using CommandVariant = Requests::CommandVariant;
        using OnCompletionCallback = AZStd::function<void(FileRequest& request)>;

        AZ_CLASS_ALLOCATOR(FileRequest, SystemAllocator);

        enum class Usage : u8
        {
            Internal,
            External
        };

        void CreateRequestLink(FileRequestPtr&& request);
        void CreateRequestPathStore(FileRequest* parent, RequestPath path);

        // Public-facing API - creates a root request and stores the path and desired offset and size to be processed by the streamer.
        void CreateReadRequest(RequestPath path, void* output, u64 outputSize, u64 offset, u64 size,
            AZStd::chrono::steady_clock::time_point deadline, IStreamerTypes::Priority priority);
        void CreateReadRequest(RequestPath path, IStreamerTypes::RequestMemoryAllocator* allocator, u64 offset, u64 size,
            AZStd::chrono::steady_clock::time_point deadline, IStreamerTypes::Priority priority);

        // Internal API.   The above internally creates the below individual child requests.  See note at the top of this class.
        void CreateRead(FileRequest* parent, void* output, u64 outputSize, const RequestPath& path, u64 offset, u64 size, bool sharedRead = false);

        // Ensure compressionInfo& outlives the request if you call this API. 
        void CreateCompressedRead(FileRequest* parent, const CompressionInfo& compressionInfo, void* output, u64 readOffset, u64 readSize);
        void CreateCompressedRead(FileRequest* parent, CompressionInfo&& compressionInfo, void* output, u64 readOffset, u64 readSize);

        void CreateWait(FileRequest* parent);

        // See the note at the top of this class about const & references to RequestPath.
        void CreateFileExistsCheck(const RequestPath& path);
        void CreateFileMetaDataRetrieval(const RequestPath& path);

        void CreateCancel(FileRequestPtr target);
        void CreateReschedule(FileRequestPtr target, AZStd::chrono::steady_clock::time_point newDeadline, IStreamerTypes::Priority newPriority);
        void CreateFlush(RequestPath path);
        void CreateFlushAll();

        // The following copy the FileRange, so it does not need to exist beyond the call to this function.
        void CreateDedicatedCacheCreation(RequestPath path, const FileRange& range = {}, FileRequest* parent = nullptr);
        void CreateDedicatedCacheDestruction(RequestPath path, const FileRange& range = {}, FileRequest* parent = nullptr);

        void CreateReport(AZStd::vector<AZ::IO::Statistic>& output, IStreamerTypes::ReportType reportType);
        void CreateCustom(AZStd::any data, bool failWhenUnhandled = true, FileRequest* parent = nullptr);

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
        void SetEstimatedCompletion(AZStd::chrono::steady_clock::time_point time);
        AZStd::chrono::steady_clock::time_point GetEstimatedCompletion() const;

    private:
        explicit FileRequest(Usage usage = Usage::Internal);
        ~FileRequest();

        void Reset();
        void SetOptionalParent(FileRequest* parent);

        inline static void OnCompletionPlaceholder(const FileRequest& /*request*/) {}

        //! Command and parameters for the request.
        CommandVariant m_command;

        //! Estimated time this request will complete. This is an estimation and depends on many
        //! factors which can cause it to change drastically from moment to moment.
        AZStd::chrono::steady_clock::time_point m_estimatedCompletion;

        //! The file request that has a dependency on this one. This can be null if there are no
        //! other request depending on this one to complete.
        FileRequest* m_parent{ nullptr };


        //! Id assigned when the request is added to the pending queue.
        size_t m_pendingId{ 0 };

        //! Called once the request has completed. This will always be called from the Streamer thread
        //! and thread safety is the responsibility of called function. When assigning a lambda avoid
        //! capturing a FileRequestPtr by value as this will cause a circular reference which causes
        //! the FileRequestPtr to never be released and causes a memory leak. This call will
        //! block the main Streamer thread until it returns so callbacks should be kept short. If
        //! a longer running task is needed consider using a job to do the work.
        OnCompletionCallback m_onCompletion;

        //! Status of the request.
        AZStd::atomic<IStreamerTypes::RequestStatus> m_status{ IStreamerTypes::RequestStatus::Pending };

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
        friend struct AZStd::IntrusivePtrCountPolicy<ExternalFileRequest>;
        friend class FileRequestHandle;
        friend class FileRequest;
        friend class Streamer;
        friend class StreamerContext;
        friend class Scheduler;
        friend class Device;
        friend class Streamer_SchedulerTest_RequestSorting_Test;
        friend bool operator==(const FileRequestHandle& lhs, const FileRequestPtr& rhs);

    public:
        AZ_CLASS_ALLOCATOR(ExternalFileRequest, SystemAllocator);

        explicit ExternalFileRequest(StreamerContext* owner);

    private:
        void add_ref();
        void release();

        FileRequest m_request;
        AZStd::atomic_uint64_t m_refCount{ 0 };
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

} // namespace AZ::IO

#include <AzCore/IO/Streamer/FileRequest.inl>
