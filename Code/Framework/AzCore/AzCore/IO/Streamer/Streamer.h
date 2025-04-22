/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/base.h>
#include <AzCore/IO/IStreamer.h>
#include <AzCore/IO/Streamer/Scheduler.h>
#include <AzCore/IO/Streamer/StreamerContext.h>
#include <AzCore/Memory/PoolAllocator.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

namespace AZStd
{
    struct thread_desc;
}

namespace AZ::IO::Requests
{
    enum class ReportType : int8_t;
}

namespace AZ::IO
{
    class StreamStackEntry;
    class Streamer_SchedulerTest_RequestSorting_Test;

    /**
     * Data streamer.
     */
    class Streamer final
        : public AZ::IO::IStreamer
    {
        friend Streamer_SchedulerTest_RequestSorting_Test;
    public:
        AZ_RTTI(Streamer, "{3D880982-6E3F-4913-9947-55E01030D4AA}", IStreamer);
        AZ_CLASS_ALLOCATOR(Streamer, SystemAllocator);

        //
        // Streamer commands.
        //

        //! Creates a request to read a file with a user provided output buffer.
        FileRequestPtr Read(AZStd::string_view relativePath, void* outputBuffer, size_t outputBufferSize, size_t readSize,
            IStreamerTypes::Deadline deadline = IStreamerTypes::s_noDeadline,
            IStreamerTypes::Priority priority = IStreamerTypes::s_priorityMedium, size_t offset = 0) override;

        //! Sets a request to the read command with a user provided output buffer.
        FileRequestPtr& Read(FileRequestPtr& request, AZStd::string_view relativePath, void* outputBuffer, size_t outputBufferSize,
            size_t readSize, IStreamerTypes::Deadline deadline = IStreamerTypes::s_noDeadline,
            IStreamerTypes::Priority priority = IStreamerTypes::s_priorityMedium, size_t offset = 0) override;

        //! Creates a request to read a file with an allocator to create the output buffer.
        FileRequestPtr Read(AZStd::string_view relativePath, IStreamerTypes::RequestMemoryAllocator& allocator,
            size_t size, IStreamerTypes::Deadline deadline = IStreamerTypes::s_noDeadline,
            IStreamerTypes::Priority priority = IStreamerTypes::s_priorityMedium, size_t offset = 0) override;

        //! Set a request to the read command with an allocator to create the output buffer.
        FileRequestPtr& Read(FileRequestPtr& request, AZStd::string_view relativePath, IStreamerTypes::RequestMemoryAllocator& allocator,
            size_t size, IStreamerTypes::Deadline deadline = IStreamerTypes::s_noDeadline,
            IStreamerTypes::Priority priority = IStreamerTypes::s_priorityMedium, size_t offset = 0) override;


        //! Creates a request to cancel a previously queued request.
        FileRequestPtr Cancel(FileRequestPtr target) override;

        //! Sets a request to the cancel command.
        FileRequestPtr& Cancel(FileRequestPtr& request, FileRequestPtr target) override;

        //! Creates a request to adjust a previous queued request.
        FileRequestPtr RescheduleRequest(
            FileRequestPtr target, IStreamerTypes::Deadline newDeadline, IStreamerTypes::Priority newPriority) override;

        //! Sets a request to the reschedule command.
        FileRequestPtr& RescheduleRequest(
            FileRequestPtr& request,
            FileRequestPtr target,
            IStreamerTypes::Deadline newDeadline,
            IStreamerTypes::Priority newPriority) override;


        //
        // Dedicated Cache
        //

        //! Creates a dedicated cache for the target file.
        FileRequestPtr CreateDedicatedCache(AZStd::string_view relativePath) override;

        //! Creates a dedicated cache for the target file.
        FileRequestPtr& CreateDedicatedCache(FileRequestPtr& request, AZStd::string_view relativePath) override;

        //! Destroy a dedicated cache created by CreateDedicatedCache.
        FileRequestPtr DestroyDedicatedCache(AZStd::string_view relativePath) override;

        //! Destroy a dedicated cache created by CreateDedicatedCache.
        FileRequestPtr& DestroyDedicatedCache(FileRequestPtr& request, AZStd::string_view relativePath) override;

        //
        // Cache Flush
        //

        //! Clears a file from all caches in use by Streamer.
        FileRequestPtr FlushCache(AZStd::string_view relativePath) override;

        //! Clears a file from all caches in use by Streamer.
        FileRequestPtr& FlushCache(FileRequestPtr& request, AZStd::string_view relativePath) override;

        //! Forcefully clears out all caches internally held by the available devices.
        FileRequestPtr FlushCaches() override;

        //! Forcefully clears out all caches internally held by the available devices.
        FileRequestPtr& FlushCaches(FileRequestPtr& request) override;

        //
        // Custom requests.
        //

        //! Creates a custom request.
        FileRequestPtr Custom(AZStd::any data) override;

        //! Creates a custom request.
        FileRequestPtr& Custom(FileRequestPtr& request, AZStd::any data) override;

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
        void CreateRequestBatch(AZStd::vector<FileRequestPtr>& requests, size_t count) override;

        //! Queues a request for processing by Streamer's stack.
        void QueueRequest(const FileRequestPtr& request) override;

        //! Queue a batch of requests for processing by Streamer's stack.
        void QueueRequestBatch(const AZStd::vector<FileRequestPtr>& requests) override;

        //! Queue a batch of requests for processing by Streamer's stack.
        void QueueRequestBatch(AZStd::vector<FileRequestPtr>&& requests) override;

        //
        // Streamer request queries
        //

        //! Check if the provided request has completed.
        bool HasRequestCompleted(FileRequestHandle request) const override;

        //! Check the status of a request.
        IStreamerTypes::RequestStatus GetRequestStatus(FileRequestHandle request) const override;

        //! Returns the time that the provided request will complete.
        AZStd::chrono::steady_clock::time_point GetEstimatedRequestCompletionTime(FileRequestHandle request) const override;

        //! Gets the result for operations that read data.
        bool GetReadRequestResult(FileRequestHandle request, void*& buffer, u64& numBytesRead,
            IStreamerTypes::ClaimMemory claimMemory = IStreamerTypes::ClaimMemory::No) const override;

        //
        // General Streamer functions
        //

        //! Call to collect statistics from all the components that make up Streamer.
        void CollectStatistics(AZStd::vector<Statistic>& statistics) override;

        //! Tells AZ::IO::Streamer the collect information for the selected report type and store it in the provided output.
        FileRequestPtr Report(AZStd::vector<Statistic>& output, IStreamerTypes::ReportType reportType) override;
        //! Tells AZ::IO::Streamer the collect information for the selected report type and store it in the provided output.
        FileRequestPtr& Report(FileRequestPtr& request, AZStd::vector<Statistic>& output, IStreamerTypes::ReportType reportType) override;

        //! Returns configuration recommendations as reported by the scheduler.
        const IStreamerTypes::Recommendations& GetRecommendations() const override;

        //! Suspends processing of requests in Streamer's stack.
        void SuspendProcessing() override;

        //! Resumes processing after a previous call to SuspendProcessing.
        void ResumeProcessing() override;

        //! Whether or not processing of requests has been suspended.
        bool IsSuspended() const override;

        //
        // Streamer specific functions.
        // These functions are specific to the AZ::IO::Streamer and in practice are only used by the StreamerComponent.
        //

        //! Records the statistics to a profiler.
        void RecordStatistics();

        Streamer(const AZStd::thread_desc& threadDesc, AZStd::unique_ptr<Scheduler> streamStack);
        ~Streamer() override;

    private:
        StreamerContext m_streamerContext;
        AZStd::unique_ptr<Scheduler> m_streamStack;
        IStreamerTypes::Recommendations m_recommendations;
    };
} // namespace AZ::IO
