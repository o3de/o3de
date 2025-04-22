/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/IO/Streamer/RequestPath.h>
#include <AzCore/IO/IStreamerTypes.h>
#include <AzCore/IO/Streamer/Statistics.h>
#include <AzCore/IO/Streamer/StreamerConfiguration.h>
#include <AzCore/IO/Streamer/StreamerContext.h>
#include <AzCore/IO/Streamer/StreamStackEntry.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/parallel/atomic.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/parallel/thread.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/Statistics/RunningStatistic.h>

namespace AZ::IO
{
    class FileRequest;
    class Streamer_SchedulerTest_RequestSorting_Test;

    namespace Requests
    {
        struct CancelData;
        struct RescheduleData;
    } // namespace Requests

    class Scheduler final
    {
    public:
        explicit Scheduler(AZStd::shared_ptr<StreamStackEntry> streamStack, u64 memoryAlignment = AZCORE_GLOBAL_NEW_ALIGNMENT,
            u64 sizeAlignment = 1, u64 granularity = 1_mib);
        ~Scheduler();

        void Start(const AZStd::thread_desc& threadDesc);
        void Stop();

        FileRequestPtr CreateRequest();
        void CreateRequestBatch(AZStd::vector<FileRequestPtr>& requests, size_t count);
        void QueueRequest(FileRequestPtr request);
        void QueueRequestBatch(const AZStd::vector<FileRequestPtr>& requests);
        void QueueRequestBatch(AZStd::vector<FileRequestPtr>&& requests);

        //! Stops the Scheduler's main thread from processing any requests. Requests can still be queued,
        //! but won't be picked up.
        void SuspendProcessing();
        //! Resumes processing of requests on the Scheduler's main thread.
        void ResumeProcessing();
        //! Whether or not processing of requests has been suspended.
        bool IsSuspended() const;

        //! Collects various metrics that are recorded by streamer and its stream stack.
        //! This function is deliberately not thread safe. All stats use a sliding window and never
        //! allocate memory. This might mean in some cases that values of individual stats may not be
        //! referencing the same window of requests, but in practice these differences are too minute
        //! to detect.
        void CollectStatistics(AZStd::vector<Statistic>& statistics);

        void GetRecommendations(IStreamerTypes::Recommendations& recommendations) const;

    private:
        friend class Streamer_SchedulerTest_RequestSorting_Test;
        inline static constexpr u32 ProfilerColor = 0x0080ffff; //!< A lite shade of blue. (See https://www.color-hex.com/color/0080ff).

        void Thread_MainLoop();
        void Thread_QueueNextRequest();
        bool Thread_ExecuteRequests();
        bool Thread_PrepareRequests(AZStd::vector<FileRequestPtr>& outstandingRequests);
        void Thread_ProcessTillIdle();
        void Thread_ProcessCancelRequest(FileRequest* request, Requests::CancelData& data);
        void Thread_ProcessRescheduleRequest(FileRequest* request, Requests::RescheduleData& data);

        enum class Order
        {
            FirstRequest, //!< The first request is the most important to process next.
            SecondRequest, //!< The second request is the most important to process next.
            Equal //!< Both requests are equally important.
        };
        //! Determine which of the two provided requests is more important to process next.
        Order Thread_PrioritizeRequests(const FileRequest* first, const FileRequest* second) const;
        void Thread_ScheduleRequests();

        // Stores data that's unguarded and should only be changed by the scheduling thread.
        struct ThreadData final
        {
            //! Requests pending in the Streaming stack entries. Cached here so it doesn't need to allocate
            //! and free memory whenever scheduling happens.
            AZStd::vector<FileRequest*> m_internalPendingRequests;
            RequestPath m_lastFilePath; //!< Path of the last file queued for reading.
            AZStd::shared_ptr<StreamStackEntry> m_streamStack;
            u64 m_lastFileOffset{ 0 }; //!< Offset of into the last file queued after reading has completed.
        };
        ThreadData m_threadData;
        StreamerContext m_context;

        IStreamerTypes::Recommendations m_recommendations;

        StreamStackEntry::Status m_stackStatus;
#if AZ_STREAMER_ADD_EXTRA_PROFILING_INFO
        AZStd::chrono::steady_clock::time_point m_processingStartTime;
        size_t m_processingSize{ 0 };
        //! Indication of how efficient the scheduler works. For loose files a large gap with the read speed of
        //! the storage drive means that the scheduler is having to spend too much time between requests. For archived
        //! files this value should be larger than the read speed of the storage drive otherwise compressing files has
        //! no benefit (other than reduced disk space).
        AverageWindow<double, double, s_statisticsWindowSize> m_processingSpeedStat;
        TimedAverageWindow<s_statisticsWindowSize> m_schedulingTimeStat;
        TimedAverageWindow<s_statisticsWindowSize> m_queuingTimeStat;
        TimedAverageWindow<s_statisticsWindowSize> m_executingTimeStat;
        TimedAverageWindow<s_statisticsWindowSize> m_preparingTimeStat;

        //! Percentage of reads that come in that are already on their deadline. Requests like this are disruptive
        //! as they cause the scheduler to prioritize these over the most optimal read layout.
        AZ::Statistics::RunningStatistic m_immediateReadsPercentageStat;
#endif

        AZStd::mutex m_pendingRequestsLock;
        AZStd::vector<FileRequestPtr> m_pendingRequests;

        AZStd::thread m_mainLoop;
        AZStd::atomic_bool m_isRunning{ false };
        AZStd::atomic_bool m_isSuspended{ false };
        AZStd::thread_desc m_mainLoopDesc;
    };
} // namespace AZ::IO
