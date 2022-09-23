/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/IO/IStreamerTypes.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/string/string_view.h>
#include <AzCore/std/any.h>

// These Streamer includes need to be moved to Streamer internals/implementation,
// and pull out only what we need for visibility at IStreamer.h interface declaration.
#include <AzCore/IO/Streamer/Statistics.h>

namespace AZ::IO
{
    class ExternalFileRequest;
    class FileRequestHandle;

    using FileRequestPtr = AZStd::intrusive_ptr<ExternalFileRequest>;
    /**
     * Data Streamer Interface
     */
    class IStreamer
    {
    public:
        AZ_RTTI(IStreamer, "{0015594D-6EA5-4309-A2AD-1D704F264A66}");

        using OnCompleteCallback = AZStd::function<void(FileRequestHandle)>;

        IStreamer() = default;
        virtual ~IStreamer() = default;

        //
        // Streamer commands.
        // Commands for streamer are stored in a request. Requests won't be processed until they are queued with QueueRequest.
        // These functions can't be called after a request has been queued.
        //

        //! Creates a request to read a file.
        //! @param relativePath Relative path to the file to load. This can include aliases such as @products@.
        //! @param outputBuffer The buffer that will hold the loaded data. This must be able to at least hold "size" number of bytes.
        //! @param outputBufferSize The size of the buffer that will hold the loaded data. This must be equal or larger than "size" number of bytes.
        //! @param readSize The number of bytes to read from the file at the relative path.
        //! @param deadline The amount of time from calling Read that the request should complete. Is FileRequest::s_noDeadline
        //!         if the request doesn't need to be completed before a specific time.
        //! @param priority The priority used to order requests if multiple requests are at risk of missing their deadline.
        //! @param offset The offset into the file where reading begins. Warning: reading at an offset other than 0 has a
        //!         significant impact on performance.
        //! @return A smart pointer to the newly created request with the read command.
        virtual FileRequestPtr Read(
            AZStd::string_view relativePath,
            void* outputBuffer,
            size_t outputBufferSize,
            size_t readSize,
            IStreamerTypes::Deadline deadline = IStreamerTypes::s_noDeadline,
            IStreamerTypes::Priority priority = IStreamerTypes::s_priorityMedium,
            size_t offset = 0) = 0;

        //! Sets a request to the read command.
        //! @param request The request that will store the read command.
        //! @param relativePath Relative path to the file to load. This can include aliases such as @products@.
        //! @param outputBuffer The buffer that will hold the loaded data. This must be able to at least hold "size" number of bytes.
        //! @param outputBufferSize The size of the buffer that will hold the loaded data. This must be equal or larger than "size" number of bytes.
        //! @param readSize The number of bytes to read from the file at the relative path.
        //! @param deadline The amount of time from calling Read that the request should complete. Is FileRequest::s_noDeadline
        //!         if the request doesn't need to be completed before a specific time.
        //! @param priority The priority used to order requests if multiple requests are at risk of missing their deadline.
        //! @param offset The offset into the file where reading begins. Warning: reading at an offset other than 0 has a
        //!         significant impact on performance.
        //! @return A reference to the provided request.
        virtual FileRequestPtr& Read(
            FileRequestPtr& request,
            AZStd::string_view relativePath,
            void* outputBuffer,
            size_t outputBufferSize,
            size_t readSize,
            IStreamerTypes::Deadline deadline = IStreamerTypes::s_noDeadline,
            IStreamerTypes::Priority priority = IStreamerTypes::s_priorityMedium,
            size_t offset = 0) = 0;

        //! Creates a request to the read command.
        //! @param relativePath Relative path to the file to load. This can include aliases such as @products@.
        //! @param allocator The allocator used to reserve and release memory for the read request. Memory allocated this way will
        //!         be automatically freed when there are no more references to the FileRequestPtr. To avoid this, use GetReadRequestResult
        //!         to claim the pointer and use the provided allocator to release the memory at a later point.
        //!         The allocator needs to live at least as long as the FileRequestPtr is in use.
        //!         Do not store the FileRequestPtr in the allocator as this will result in a circular dependency and a memory leak.
        //! @param size The number of bytes to read from the file at the relative path.
        //! @param deadline The amount of time from calling Read that the request should complete. Is FileRequest::s_noDeadline
        //!         if the request doesn't need to be completed before a specific time.
        //! @param priority The priority used to order requests if multiple requests are at risk of missing their deadline.
        //! @param offset The offset into the file where reading begins. Warning: reading at an offset other than 0 has a
        //!         significant impact on performance.
        //! @return A smart pointer to the newly created request with the read command.
        virtual FileRequestPtr Read(
            AZStd::string_view relativePath,
            IStreamerTypes::RequestMemoryAllocator& allocator,
            size_t size,
            IStreamerTypes::Deadline deadline = IStreamerTypes::s_noDeadline,
            IStreamerTypes::Priority priority = IStreamerTypes::s_priorityMedium,
            size_t offset = 0) = 0;

        //! Sets a request to the read command.
        //! @param request The request that will store the read command.
        //! @param relativePath Relative path to the file to load. This can include aliases such as @products@.
        //! @param allocator The allocator used to reserve and release memory for the read request. Memory allocated this way will
        //!         be automatically freed when there are no more references to the FileRequestPtr. To avoid this, use GetReadRequestResult
        //!         to claim the pointer and use the provided allocator to release the memory at a later point.
        //!         The allocator needs to live at least as long as the FileRequestPtr is in use.
        //!         Do not store the FileRequestPtr in the allocator as this will result in a circular dependency and a memory leak.
        //! @param size The number of bytes to read from the file at the relative path.
        //! @param deadline The amount of time from calling Read that the request should complete. Is FileRequest::s_noDeadline
        //!         if the request doesn't need to be completed before a specific time.
        //! @param priority The priority used to order requests if multiple requests are at risk of missing their deadline.
        //! @param offset The offset into the file where reading begins. Warning: reading at an offset other than 0 has a
        //!         significant impact on performance.
        //! @return A reference to the provided request.
        virtual FileRequestPtr& Read(
            FileRequestPtr& request,
            AZStd::string_view relativePath,
            IStreamerTypes::RequestMemoryAllocator& allocator,
            size_t size,
            IStreamerTypes::Deadline deadline = IStreamerTypes::s_noDeadline,
            IStreamerTypes::Priority priority = IStreamerTypes::s_priorityMedium,
            size_t offset = 0) = 0;

        //! Creates a request to cancel a previously queued request.
        //! When this request completes it's not guaranteed to have canceled the target request. Not all requests can be canceled and requests
        //! that already processing may complete. It's recommended to let the target request handle the completion of the request as normal
        //! and handle cancellation by checking the status on the target request is set to IStreamerTypes::RequestStatus::Canceled.
        //! @target The request to cancel.
        //! @result A smart pointer to the newly created request with the cancel command.
        virtual FileRequestPtr Cancel(FileRequestPtr target) = 0;

        //! Sets a request to the cancel command.
        //! When this request completes it's not guaranteed to have canceled the target request. Not all requests can be canceled and requests
        //! that already processing may complete. It's recommended to let the target request handle the completion of the request as normal
        //! and handle cancellation by checking the status on the target request is set to IStreamerTypes::RequestStatus::Canceled.
        //! @param request The request that will store the cancel command.
        //! @param target The request to cancel.
        //! @return A reference to the provided request.
        virtual FileRequestPtr& Cancel(FileRequestPtr& request, FileRequestPtr target) = 0;

        //! Adjusts the deadline and priority of a request.
        //! This has no effect if the requests is already being processed, if the
        //!     request doesn't contain a command that can be rescheduled or if the request hasn't been queued.
        //! @param target The request that will be rescheduled.
        //! @param newDeadline The new deadline for the request.
        //! @param newPriority The new priority for the request.
        //! @result A smart pointer to the newly created request with the reschedule command.
        virtual FileRequestPtr RescheduleRequest(
            FileRequestPtr target, IStreamerTypes::Deadline newDeadline, IStreamerTypes::Priority newPriority) = 0;

        //! Adjusts the deadline and priority of a request.
        //! This has no effect if the requests is already being processed, if the
        //!     request doesn't contain a command that can be rescheduled or if the request hasn't been queued.
        //! @param request The request that will store the reschedule command.
        //! @param target The request that will be rescheduled.
        //! @param newDeadline The new deadline for the request.
        //! @param newPriority The new priority for the request.
        //! @return A reference to the provided request.
        virtual FileRequestPtr& RescheduleRequest(
            FileRequestPtr& request, FileRequestPtr target, IStreamerTypes::Deadline newDeadline, IStreamerTypes::Priority newPriority) = 0;

        //! Creates a dedicated cache for the target file. The target file won't be removed from the cache until
        //! DestroyDedicatedCache is called. Typical use of a dedicated cache is for files that have their own compression
        //! and are periodically visited to read a section, e.g. streaming video play or streaming audio banks. This
        //! request will fail if there are no nodes in Streamer's stack that deal with dedicated caches.
        //! @param relativePath Relative path to the file to receive a dedicated cache. This can include aliases such as @products@.
        //! @return A smart pointer to the newly created request with the command to create a dedicated cache.
        virtual FileRequestPtr CreateDedicatedCache(AZStd::string_view relativePath) = 0;

        //! Creates a dedicated cache for the target file. The target file won't be removed from the cache until
        //! DestroyDedicatedCache is called. Typical use of a dedicated cache is for files that have their own compression
        //! and are periodically visited to read a section, e.g. streaming video play or streaming audio banks. This
        //! request will fail if there are no nodes in Streamer's stack that deal with dedicated caches.
        //! @param request The request that will store the command to create a dedicated cache.
        //! @param relativePath Relative path to the file to receive a dedicated cache. This can include aliases such as @products@.
        //! @return A reference to the provided request.
        virtual FileRequestPtr& CreateDedicatedCache(FileRequestPtr& request, AZStd::string_view relativePath) = 0;

        //! Destroy a dedicated cache created by CreateDedicatedCache. See CreateDedicatedCache for more details.
        //! @param relativePath Relative path to the file that got a dedicated cache. This can include aliases such as @products@.
        //! @return A smart pointer to the newly created request with the command to destroy a dedicated cache.
        virtual FileRequestPtr DestroyDedicatedCache(AZStd::string_view relativePath) = 0;

        //! Destroy a dedicated cache created by CreateDedicatedCache. See CreateDedicatedCache for more details.
        //! @param request The request that will store the command to destroy a dedicated cache.
        //! @param relativePath Relative path to the file that got a dedicated cache. This can include aliases such as @products@.
        //! @return A reference to the provided request.
        virtual FileRequestPtr& DestroyDedicatedCache(FileRequestPtr& request, AZStd::string_view relativePath) = 0;

        //! Clears a file from all caches in use by Streamer.
        //! Flushing the cache will cause the streaming stack to pause processing until it's idle before issuing the flush and resuming
        //! processing. This can result in a noticeable interruption.
        //! @param relativePath Relative path to the file that will be cleared from all caches. This can include aliases such as @products@.
        //! @return A smart pointer to the newly created request with the command to flush a file from all caches.
        virtual FileRequestPtr FlushCache(AZStd::string_view relativePath) = 0;

        //! Clears a file from all caches in use by Streamer.
        //! Flushing the cache will cause the streaming stack to pause processing until it's idle before issuing the flush and resuming
        //! processing. This can result in a noticeable interruption.
        //! @param request The request that will store the command to flush a file from all caches.
        //! @param relativePath Relative path to the file that will be cleared from all caches. This can include aliases such as @products@.
        //! @return A reference to the provided request.
        virtual FileRequestPtr& FlushCache(FileRequestPtr& request, AZStd::string_view relativePath) = 0;

        //! Forcefully clears out all caches internally held by the streaming stack. This removes all cached information and
        //! can result in needing to reload a substantial amount of data. Flushing will cause the streaming stack to pause processing
        //! until it's idle before issuing the flush and resuming processing. Both behaviors combined can result in a noticeable interruption.
        //! If possible, prefer to use FlushCache instead to reduce the amount of data that would potentially need to be reloaded.
        //! @return A smart pointer to the newly created request with the command to flush all caches.
        virtual FileRequestPtr FlushCaches() = 0;

        //! Forcefully clears out all caches internally held by the streaming stack. This removes all cached information and
        //! can result in needing to reload a substantial amount of data. Flushing will cause the streaming stack to pause processing
        //! until it's idle before issuing the flush and resuming processing. Both behaviors combined can result in a noticeable interruption.
        //! If possible, prefer to use FlushCache instead to reduce the amount of data that would potentially need to be reloaded.
        //! @param request The request that will store the command to flush all caches.
        //! @return A reference to the provided request.
        virtual FileRequestPtr& FlushCaches(FileRequestPtr& request) = 0;

        //! Retrieves statistics for the requested type. This is meant for statistics that can't be retrieved in a lockless manner. For
        //! metrics that can be retrieved lockless use CollectStatistics.
        //! @param output The storage for the information that's being reported. The container needs to remain alive for the duration of
        //!     the call. Register a callback with SetRequestCompleteCallback to determine when the container is no longer needed.
        //! @param reportType The type of information to report.
        //! @return A smart pointer to the newly created request with the command to report statistics.
        virtual FileRequestPtr Report(AZStd::vector<Statistic>& output, IStreamerTypes::ReportType reportType) = 0;

        //! Retrieves statistics for the requested type. This is meant for statistics that can't be retrieved in a lockless manner. For
        //! metrics that can be retrieved lockless use CollectStatistics.
        //! @param request The request that will store the command to report.
        //! @param output The storage for the information that's being reported. The container needs to remain alive for the duration of
        //!     the call. Register a callback with SetRequestCompleteCallback to determine when the container is no longer needed.
        //! @param reportType The type of information to report.
        //! @return A reference to the provided request.
        virtual FileRequestPtr& Report(
            FileRequestPtr& request, AZStd::vector<Statistic>& output, IStreamerTypes::ReportType reportType) = 0;

        //! Creates a custom request. This can be used by extensions to Streamer's stack to add their own commands.
        //! @param data Storage for the arguments to the command.
        //! @return A smart pointer to the newly created request with the custom command.
        virtual FileRequestPtr Custom(AZStd::any data) = 0;

        //! Creates a custom request. This can be used by extensions to Streamer's stack to add their own commands.
        //! @param request The request that will store the custom command.
        //! @param data Storage for the arguments to the command.
        //! @return A reference to the provided request.
        virtual FileRequestPtr& Custom(FileRequestPtr& request, AZStd::any data) = 0;

        //
        // Streamer request configuration.
        // These functions configure settings that universal for all requests. Calling these on a request that has
        // already been queued is not allowed.
        //

        //! Sets a callback function that will trigger when the provided request completes. The callback will be triggered
        //! from within the Streamer's stack on a different thread. While in the callback function Streamer can't continue
        //! processing requests so it's recommended to keep the callback short, for instance by setting an atomic value
        //! to be picked up by another thread like the main thread or to queue a job (function) to complete the work.
        //! Note avoid storing a copy of the owning FileRequestPtr in a lambda provided as the callback. This will result in
        //! the same request storing a copy of itself indirectly, which means the reference counter can never reach zero due
        //! to the circular dependency and results in a resource leak.
        //! @param request The request that will get the callback assigned.
        //! @param callback A function with the signature "void(FileRequestHandle request);"
        //! @return A reference to the provided request.
        virtual FileRequestPtr& SetRequestCompleteCallback(FileRequestPtr& request, OnCompleteCallback callback) = 0;

        //
        // Streamer request management.
        //

        //! Create a new blank request.
        //! Before queuing this request a command must be assigned first.
        //! @return A smart pointer to the newly created request.
        virtual FileRequestPtr CreateRequest() = 0;

        //! Creates a number of new blank requests.
        //! The requests need to be assigned a command before they can be queued.
        //!     This function is preferable over CreateRequest if multiple requests are needed as it avoids repeatedly
        //!     syncing with Streamer's stack.
        //! @param requests Container where the requests will be stored in.
        //! @param count The number of requests to create.
        virtual void CreateRequestBatch(AZStd::vector<FileRequestPtr>& requests, size_t count) = 0;

        //! Queues a request for processing by Streamer's stack. After a request has been queued the request pointer
        //! can optionally be stored as Streamer supports fire-and-forget for requests.
        //! @param request The request that will be processed.
        virtual void QueueRequest(const FileRequestPtr& request) = 0;

        //! Queue a batch of requests for processing by Streamer's stack.
        //!     This function is preferable over QueueRequest if multiple requests need to be queued at the same time as it
        //!     avoids repeatedly syncing with the Streamer's stack.
        //! @param requests The requests that will be processed.
        virtual void QueueRequestBatch(const AZStd::vector<FileRequestPtr>& requests) = 0;

        //! Queue a batch of requests for processing by Streamer's stack.
        //! This version will move requests from the container, but not delete them.
        //!     This function is preferable over QueueRequest if multiple requests need to be queued at the same time as it
        //!     avoids repeatedly syncing with the Streamer's stack.
        //! @param requests The requests that will be processed.
        virtual void QueueRequestBatch(AZStd::vector<FileRequestPtr>&& requests) = 0;

        //
        // Streamer request queries
        // The following functions accept FileRequest, FileRequestPtr or FileRequestHandle.
        //

        //! Check if the provided request has completed.
        //! This can mean it was successful, failed or was canceled.
        //! @param request The request to query.
        //! @return True if the request is no longer pending or processing and has completed, otherwise false.
        virtual bool HasRequestCompleted(FileRequestHandle request) const = 0;

        //! Check the status of a request.
        //! This can be called even if the request has not been queued yet.
        //! @param request The request to query.
        //! @return The current status of the request.
        virtual IStreamerTypes::RequestStatus GetRequestStatus(FileRequestHandle request) const = 0;

        //! Returns the estimated time that the provided request will complete.
        //! Estimation doesn't happen until Streamer's stack
        //! has picked up the request and zero will be returned until then. As requests complete and new ones are added
        //! the estimated completion time will change over time until the request completes. Not all requests well get
        //! an estimation.
        //! @param request The request to get the estimation for.
        //! @return The system time the request will complete or zero of no estimation has been done.
        virtual AZStd::chrono::steady_clock::time_point GetEstimatedRequestCompletionTime(FileRequestHandle request) const = 0;

        //! Get the result for operations that read data.
        //! @param request The request to query.
        //! @param buffer The buffer the data was written to.
        //! @param numBytesRead The total number of bytes that were read from the file.
        //! @return True if data could be retrieved, otherwise false.
        virtual bool GetReadRequestResult(FileRequestHandle request, void*& buffer, u64& numBytesRead,
            IStreamerTypes::ClaimMemory claimMemory = IStreamerTypes::ClaimMemory::No) const = 0;

        //
        // General Streamer functions
        //

        //! Collect statistics from all the components that make up Streamer.
        //! This function is immediately executed and returns immediately with the results. Due to this behavior there's a limit
        //! to the number of statistics that can be returned as only lockless retrieval data can be retrieved. The data is also a
        //! snapshot so may be slightly out of date. To get additional data use Report.
        //! @param statistics The container where statistics will be added to.
        virtual void CollectStatistics(AZStd::vector<Statistic>& statistics) = 0;

        //! Returns the recommended configuration options for read requests. Following these recommendations
        //! will help improve performance, but are optional.
        virtual const IStreamerTypes::Recommendations& GetRecommendations() const = 0;

        //! Suspends processing of requests in Streamer's stack. New requests can still be queued, but they won't
        //! be processed until ResumeProcessing is called.
        virtual void SuspendProcessing() = 0;

        //! Resumes processing requests in Streamer's stack if previously one or more calls to SuspendProcessing
        //! were made.
        virtual void ResumeProcessing() = 0;

        //! Whether or not processing of requests has been suspended.
        virtual bool IsSuspended() const = 0;
    };

} // namespace AZ::IO
