/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/base.h>
#include <AzCore/IO/Streamer/FileRequest.h>
#include <AzCore/IO/Streamer/Statistics.h>
#include <AzCore/IO/Streamer/StreamerConfiguration.h>
#include <AzCore/IO/Streamer/StreamerContext_Platform.h>
#include <AzCore/std/containers/deque.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/queue.h>
#include <AzCore/Statistics/RunningStatistic.h>

namespace AZ
{
    namespace IO
    {
        class StreamerContext
        {
        public:
            using PreparedQueue = AZStd::deque<FileRequest*>;

            ~StreamerContext();

            //! Gets a new file request, either by creating a new instance or
            //! picking one from the recycle bin. This version should only be used
            //! by nodes on the streaming stack as it's not thread safe, but faster.
            //! The scheduler will automatically recycle these requests.
            FileRequest* GetNewInternalRequest();
            //! Gets a new file request, either by creating a new instance or
            //! picking one from the recycle bin. This version is for use by 
            //! any system outside the stream stack and is thread safe. Once the
            //! reference count in the request hits zero it will automatically be recycled.
            FileRequestPtr GetNewExternalRequest();
            //! Gets a batch of new file requests, either by creating new instances or
            //! picking from the recycle bin. This version is for use by 
            //! any system outside the stream stack and is thread safe. The owner
            //! needs to manually recycle these requests once they're done. Requests
            //! with a reference count of zero will automatically be recycled.
            //! If multiple requests need to be create this is preferable as it only locks the
            //! recycle bin once.
            void GetNewExternalRequestBatch(AZStd::vector<FileRequestPtr>& requests, size_t count);

            //! Gets the number of prepared requests. Prepared requests are requests
            //! that are ready to be queued up for further processing.
            size_t GetNumPreparedRequests() const;
            //! Gets the next prepared request that should be queued. Prepared requests
            //! are requests that are ready to be queued up for further processing.
            FileRequest* PopPreparedRequest();
            //! Adds a prepared request for later queuing and processing.
            void PushPreparedRequest(FileRequest* request);
            //! Gets the prepared requests that are queued to be processed.
            PreparedQueue& GetPreparedRequests();
            //! Gets the prepared requests that are queued to be processed.
            const PreparedQueue& GetPreparedRequests() const;

            //! Marks a request as completed so the main thread in Streamer can close it out.
            //! This can be safely called from multiple threads.
            void MarkRequestAsCompleted(FileRequest* request);
            //! Rejects a request by removing it from the chain and recycling it.
            //! Only requests without children can be rejected. If the rejected request has a parent it might need to be processed
            //! further.
            //! @param request The request to remove and recycle.
            //! @return The parent request of the rejected request or null if there was no parent.
            FileRequest* RejectRequest(FileRequest* request);
            //! Adds an old request to the recycle bin so it can be reused later.
            void RecycleRequest(FileRequest* request);
            //! Adds an old external request to the recycle bin so it can be reused later.
            void RecycleRequest(ExternalFileRequest* request);

            //! Does the FinalizeRequest callback where appropriate and does some bookkeeping to finalize requests.
            //! @return True if any requests were finalized, otherwise false.
            bool FinalizeCompletedRequests();

            //! Causes the main thread for streamer to wake up and process any pending requests. If the thread
            //! is already awake, nothing happens.
            void WakeUpSchedulingThread();
            //! If there's no pending messages this will cause the main thread for streamer to go to sleep.
            void SuspendSchedulingThread();
            //! Returns the native primitive(s) used to suspend and wake up the scheduling thread and possibly other threads.
            AZ::Platform::StreamerContextThreadSync& GetStreamerThreadSynchronizer();

            //! Collects statistics recorded during processing. This will only return statistics for the
            //! context. Use the CollectStatistics on AZ::IO::Streamer to get all statistics.
            void CollectStatistics(AZStd::vector<Statistic>& statistics);

        private:
            //! Gets a new FileRequestPtr. This version is for internal use only and is not thread-safe.
            //! This will be called by GetNewExternalRequest or GetNewExternalRequestBatch which are responsible
            //! for managing the lock to the recycle bin.
            FileRequestPtr GetNewExternalRequestUnguarded();

            inline static constexpr size_t s_initialRecycleBinSize = 64;

            AZStd::mutex m_externalRecycleBinGuard;
            AZStd::vector<ExternalFileRequest*> m_externalRecycleBin;
            AZStd::vector<FileRequest*> m_internalRecycleBin;
            
            // The completion is guarded so other threads can perform async IO and safely mark requests as completed.
            AZStd::recursive_mutex m_completedGuard;
            AZStd::queue<FileRequest*> m_completed;

            // The prepared request queue is not guarded and should only be called from the main Streamer thread.
            PreparedQueue m_preparedRequests;

#if AZ_STREAMER_ADD_EXTRA_PROFILING_INFO
            //! By how much time the prediction was off. This mostly covers the latter part of scheduling, which
            //! gets more precise the closer the request gets to completion.
            AZ::Statistics::RunningStatistic m_predictionAccuracyUsStat;

            //! Tracks the percentage of requests with late predictions where the request completed earlier than expected,
            //! versus the requests that completed later than predicted.
            AZ::Statistics::RunningStatistic m_latePredictionsPercentageStat;

            //! Percentage of requests that missed their deadline. If percentage is too high it can indicate that
            //! there are too many file requests or the deadlines for requests are too tight.
            AZ::Statistics::RunningStatistic m_missedDeadlinePercentageStat;
#endif // AZ_STREAMER_ADD_EXTRA_PROFILING_INFO

            //! Platform-specific synchronization object used to suspend the Streamer thread and wake it up to resume procesing.
            AZ::Platform::StreamerContextThreadSync m_threadSync;

            size_t m_pendingIdCounter{ 0 };
        };
    } // namespace IO
} // namespace AZ
