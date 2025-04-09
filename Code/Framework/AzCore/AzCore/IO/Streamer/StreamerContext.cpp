/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/algorithm.h>
#include <AzCore/Debug/Profiler.h>
#include <AzCore/IO/Streamer/FileRequest.h>
#include <AzCore/IO/Streamer/StreamerContext.h>
#include <AzCore/IO/Streamer/StreamStackEntry.h>

namespace AZ
{
    namespace IO
    {
        static constexpr const char* ContextName = "Context";
#if AZ_STREAMER_ADD_EXTRA_PROFILING_INFO
        static constexpr const char* PredictionAccuracyName = "Prediction accuracy";
        static constexpr const char* LatePredictionName = "Early completions";
        static constexpr const char* MissedDeadlinesName = "Missed deadlines";
#endif // AZ_STREAMER_ADD_EXTRA_PROFILING_INFO

        StreamerContext::StreamerContext() = default;

        StreamerContext::~StreamerContext()
        {
            for (FileRequest* entry : m_internalRecycleBin)
            {
                delete entry;
            }

            AZStd::scoped_lock<AZStd::mutex> guard(m_externalRecycleBinGuard);
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
                    return aznew FileRequest();
                }
                else
                {
                    // Create a few requests up front so the engine initialization doesn't
                    // constantly create new instances and resizes the recycle bin.
                    m_internalRecycleBin.reserve(s_initialRecycleBinSize);
                    for (size_t i = 0; i < s_initialRecycleBinSize; ++i)
                    {
                        auto request = aznew FileRequest();
                        request->m_inRecycleBin = true;
                        m_internalRecycleBin.push_back(request);
                    }
                }
            }

            FileRequest* result = m_internalRecycleBin.back();
            AZ_Assert(result->m_usage == FileRequest::Usage::Internal, "FileRequest ended up in the wrong recycle bin.");
            AZ_Assert(AZStd::holds_alternative<AZStd::monostate>(result->m_command), "FileRequest wasn't properly reset.");
            result->m_inRecycleBin = false;
            m_internalRecycleBin.pop_back();
            return result;
        }

        FileRequestPtr StreamerContext::GetNewExternalRequest()
        {
            AZStd::scoped_lock<AZStd::mutex> guard(m_externalRecycleBinGuard);
            return GetNewExternalRequestUnguarded();
        }

        void StreamerContext::GetNewExternalRequestBatch(AZStd::vector<FileRequestPtr>& requests, size_t count)
        {
            AZStd::scoped_lock<AZStd::mutex> guard(m_externalRecycleBinGuard);
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
            AZStd::scoped_lock<AZStd::recursive_mutex> guard(m_completedGuard);
            AZ_Assert(request->m_dependencies == 0, "A request with dependencies is marked as completed.");
            AZ_Assert(!request->m_inRecycleBin, "Request that's already been marked for deletion has been added again.");
            m_completed.push(request);
        }

        FileRequest* StreamerContext::RejectRequest(FileRequest* request)
        {
            AZ_Assert(request->m_dependencies == 0, "Requests with children can not be safely rejected.");
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
            AZ_Assert(request->m_dependencies == 0, "Requests with children can not be safely recycled.");
            AZ_Assert(request->m_usage == FileRequest::Usage::Internal, "An external file request is being released as an internal one.");
            AZ_Assert(!request->m_inRecycleBin, "Internal request that's already been recycled is being recycled again.");
            request->Reset();
            request->m_inRecycleBin = true;
            m_internalRecycleBin.push_back(request);
        }

        void StreamerContext::RecycleRequest(ExternalFileRequest* request)
        {
            AZ_Assert(request->m_request.m_dependencies == 0, "External requests with children can not be safely recycled.");
            AZ_Assert(!request->m_request.m_inRecycleBin, "External request that's already been recycled is being recycled again.");
            request->m_request.Reset();
            request->m_request.m_inRecycleBin = true;

            AZStd::scoped_lock<AZStd::mutex> guard(m_externalRecycleBinGuard);
            m_externalRecycleBin.push_back(request);
        }

        bool StreamerContext::FinalizeCompletedRequests()
        {
            AZ_PROFILE_FUNCTION(AzCore);

#if AZ_STREAMER_ADD_EXTRA_PROFILING_INFO
            auto now = AZStd::chrono::steady_clock::now();
#endif
            bool hasCompletedRequests = false;
            while (true)
            {
                AZStd::queue<FileRequest*> completed;
                {
                    AZStd::scoped_lock<AZStd::recursive_mutex> guard(m_completedGuard);
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
#if AZ_STREAMER_ADD_EXTRA_PROFILING_INFO
                    // It's possible for a request to be queued internally and processed between scheduling passes. In those
                    // cases, don't check the request for accurate prediction.
                    if (top->m_estimatedCompletion > AZStd::chrono::steady_clock::time_point())
                    {
                        if (top->m_estimatedCompletion < now)
                        {
                            auto estimationDelta = AZStd::chrono::duration_cast<Statistic::TimeValue>(now - top->m_estimatedCompletion);
                            m_predictionAccuracyStat.PushEntry(estimationDelta);
                            Statistic::PlotImmediate(ContextName, PredictionAccuracyName, aznumeric_cast<double>(estimationDelta.count()));

                            m_latePredictionsPercentageStat.PushSample(0.0);
                            Statistic::PlotImmediate(ContextName, LatePredictionName,
                                m_latePredictionsPercentageStat.GetMostRecentSample());
                        }
                        else
                        {
                            auto estimationDelta = AZStd::chrono::duration_cast<Statistic::TimeValue>(top->m_estimatedCompletion - now);
                            m_predictionAccuracyStat.PushEntry(estimationDelta);
                            Statistic::PlotImmediate(ContextName, PredictionAccuracyName, aznumeric_cast<double>(estimationDelta.count()));

                            m_latePredictionsPercentageStat.PushSample(1.0);
                            Statistic::PlotImmediate(ContextName, LatePredictionName,
                                m_latePredictionsPercentageStat.GetMostRecentSample());
                        }
                    }
                    auto readRequest = AZStd::get_if<Requests::ReadRequestData>(&top->GetCommand());
                    if (readRequest != nullptr)
                    {
                        m_missedDeadlinePercentageStat.PushSample(now < readRequest->m_deadline ? 0.0 : 1.0);
                        Statistic::PlotImmediate(ContextName, MissedDeadlinesName, m_missedDeadlinePercentageStat.GetMostRecentSample());
                    }
#endif // AZ_STREAMER_ADD_EXTRA_PROFILING_INFO

                    // Get all information before calling the completion routine as it's technically possible that an external
                    // request is recycled during the callback.
                    IStreamerTypes::RequestStatus status = top->GetStatus();
                    FileRequest* parent = top->m_parent;
                    bool isInternal = top->m_usage == FileRequest::Usage::Internal;

                    {
#if AZ_STREAMER_ADD_EXTRA_PROFILING_INFO
                        if (isInternal)
                        {
                            TIMED_AVERAGE_WINDOW_SCOPE(m_internalCompletionTimeAverage);
                            AZ_PROFILE_SCOPE(AzCore, "Completion callback internal");
                            top->m_onCompletion(*top);
                            AZ_PROFILE_INTERVAL_END(AzCore, top);
                        }
                        else
                        {
                            TIMED_AVERAGE_WINDOW_SCOPE(m_externalCompletionTimeAverage);
                            AZ_PROFILE_SCOPE(AzCore, "Completion callback external");
                            top->m_onCompletion(*top);
                            AZ_PROFILE_INTERVAL_END(AzCore, top);
                        }
#else
                        AZ_PROFILE_SCOPE(AzCore,
                            isInternal ? "Completion callback internal" : "Completion callback external");
                        top->m_onCompletion(*top);
                        AZ_PROFILE_INTERVAL_END(AzCore, top);
#endif
                    }

                    if (parent)
                    {
                        AZ_Assert(parent->m_dependencies > 0,
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

        AZ::Platform::StreamerContextThreadSync& StreamerContext::GetStreamerThreadSynchronizer()
        {
            return m_threadSync;
        }

        void StreamerContext::CollectStatistics(AZStd::vector<Statistic>& statistics)
        {
            statistics.push_back(Statistic::CreateInteger(
                ContextName, "Pending requests", aznumeric_caster(m_preparedRequests.size()),
                "The number of requests that are waiting to be queued for processing. These are the requests that can still be scheduled. "
                "Larger numbers allow the scheduler to do more optimizations."));
#if AZ_STREAMER_ADD_EXTRA_PROFILING_INFO
            statistics.push_back(Statistic::CreateTimeRange(
                ContextName, PredictionAccuracyName, m_predictionAccuracyStat.CalculateAverage(), m_predictionAccuracyStat.GetMinimum(),
                m_predictionAccuracyStat.GetMaximum(),
                "The delta between time the scheduler predicated the request would be done and the actual time the request completed in "
                "milliseconds. The smaller this value, the more accurate the scheduler is."));
            statistics.push_back(Statistic::CreatePercentageRange(
                ContextName, LatePredictionName, m_latePredictionsPercentageStat.GetAverage(), m_latePredictionsPercentageStat.GetMinimum(),
                m_latePredictionsPercentageStat.GetMaximum(),
                "The amount of requests that were predicted to be done later than the actual completion time. If this value is small it "
                "means that the scheduler may do immediate reads too often, although this is preferable over the alternative as that would "
                "lead to requests coming in too late. How impactful this value is will depend largely on the prediction accuracy."));
            statistics.push_back(Statistic::CreatePercentageRange(
                ContextName, MissedDeadlinesName, m_missedDeadlinePercentageStat.GetAverage(), m_missedDeadlinePercentageStat.GetMinimum(),
                m_missedDeadlinePercentageStat.GetMaximum(),
                "The percentage of requests that were completed after their deadline. This value should be kept as low as possible. Other "
                "statistics may provide information on what the reason is too many requests miss their deadline."));
            statistics.push_back(Statistic::CreateTimeRange(
                ContextName, "Internal callback duration", m_internalCompletionTimeAverage.CalculateAverage(),
                m_internalCompletionTimeAverage.GetMinimum(), m_internalCompletionTimeAverage.GetMaximum(),
                "The average amount of time in microseconds spend on processing internal callbacks."));
            statistics.push_back(Statistic::CreateTimeRange(
                ContextName, "External callback duration", m_externalCompletionTimeAverage.CalculateAverage(),
                m_externalCompletionTimeAverage.GetMinimum(), m_externalCompletionTimeAverage.GetMaximum(),
                "The average amount of time in microseconds spend on processing external callbacks. If this takes up a long time there may "
                "be a callback on a request that should be handed off to another thread. While the callback is processing Streamer can not "
                "schedule or process any other requests."));
#endif // AZ_STREAMER_ADD_EXTRA_PROFILNG_INFO
            statistics.push_back(Statistic::CreateInteger(
                ContextName, "Total requests", aznumeric_caster(m_pendingIdCounter), "The total number of requests Streamer has processed.",
                Statistic::GraphType::None));
            statistics.push_back(Statistic::CreateInteger(
                ContextName, "Internal bucket size", aznumeric_caster(m_internalRecycleBin.size()),
                "The total number of requests available in the internal recycle bin. Having at least a few available helps avoids memory "
                "allocations from Streamer for internal management."));
            statistics.push_back(Statistic::CreateInteger(
                ContextName, "External bucket size", aznumeric_caster(m_externalRecycleBin.size()),
                "The total number of requests available in the external recycle bin. Having at least a few available helps avoid memory "
                "allocations from Streamer and speeds up creating new requests to issue to Streamer."));
        }

        FileRequestPtr StreamerContext::GetNewExternalRequestUnguarded()
        {
            if (m_externalRecycleBin.empty())
            {
                if (m_externalRecycleBin.capacity() > 0)
                {
                    // There are no requests left in the recycle bin so create a new one. This will
                    // eventually end up in the recycle bin again.
                    return FileRequestPtr(aznew ExternalFileRequest(this));
                }
                else
                {
                    // Create a few requests up front so the engine initialization doesn't
                    // constantly create new instances and resizes the recycle bin.
                    m_externalRecycleBin.reserve(s_initialRecycleBinSize);
                    for (size_t i = 0; i < s_initialRecycleBinSize; ++i)
                    {
                        auto request = aznew ExternalFileRequest(this);
                        request->m_request.m_inRecycleBin = true;
                        m_externalRecycleBin.push_back(request);
                    }
                }
            }

            FileRequestPtr result = m_externalRecycleBin.back();
            AZ_Assert(AZStd::holds_alternative<AZStd::monostate>(result->m_request.m_command), "ExternalFileRequest wasn't properly reset.");
            m_externalRecycleBin.pop_back();
            result->m_request.m_inRecycleBin = false;
            return result;
        }
    } // namespace IO
} // namespace AZ
