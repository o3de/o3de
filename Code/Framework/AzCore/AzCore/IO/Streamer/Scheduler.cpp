/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/IO/Streamer/Scheduler.h>

#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Debug/Profiler.h>
#include <AzCore/IO/Streamer/FileRequest.h>
#include <AzCore/std/containers/deque.h>
#include <AzCore/std/sort.h>

namespace AZ::IO
{
    static constexpr const char* SchedulerName = "Scheduler";
#if AZ_STREAMER_ADD_EXTRA_PROFILING_INFO
    static constexpr const char* ImmediateReadsName = "Immediate reads";
#endif // AZ_STREAMER_ADD_EXTRA_PROFILING_INFO

    Scheduler::Scheduler(AZStd::shared_ptr<StreamStackEntry> streamStack, u64 memoryAlignment, u64 sizeAlignment, u64 granularity)
    {
        AZ_Assert(IStreamerTypes::IsPowerOf2(memoryAlignment), "Memory alignment provided to AZ::IO::Scheduler isn't a power of two.");
        AZ_Assert(IStreamerTypes::IsPowerOf2(sizeAlignment), "Size alignment provided to AZ::IO::Scheduler isn't a power of two.");

        StreamStackEntry::Status status;
        streamStack->UpdateStatus(status);
        AZ_Assert(status.m_isIdle, "Stream stack provided to the scheduler is already active.");
        m_recommendations.m_memoryAlignment = memoryAlignment;
        m_recommendations.m_sizeAlignment = sizeAlignment;
        m_recommendations.m_maxConcurrentRequests = aznumeric_caster(status.m_numAvailableSlots);
        m_recommendations.m_granularity = granularity;

        m_threadData.m_streamStack = AZStd::move(streamStack);
    }

    Scheduler::~Scheduler() = default;

    void Scheduler::Start(const AZStd::thread_desc& threadDesc)
    {
        if (!m_isRunning)
        {
            m_isRunning = true;

            m_mainLoopDesc = threadDesc;
            m_mainLoopDesc.m_name = "IO Scheduler";
            m_mainLoop = AZStd::thread(
                m_mainLoopDesc,
                [this]()
                {
                    Thread_MainLoop();
                });
        }
    }

    void Scheduler::Stop()
    {
        m_isRunning = false;
        m_context.WakeUpSchedulingThread();
        m_mainLoop.join();
    }

    FileRequestPtr Scheduler::CreateRequest()
    {
        return m_context.GetNewExternalRequest();
    }

    void Scheduler::CreateRequestBatch(AZStd::vector<FileRequestPtr>& requests, size_t count)
    {
        requests.reserve(requests.size() + count);
        m_context.GetNewExternalRequestBatch(requests, count);
    }


    void Scheduler::QueueRequest(FileRequestPtr request)
    {
        AZ_Assert(m_isRunning, "Trying to queue a request when Streamer's scheduler isn't running.");

        {
            AZStd::scoped_lock lock(m_pendingRequestsLock);
            m_pendingRequests.push_back(AZStd::move(request));
        }
        m_context.WakeUpSchedulingThread();
    }

    void Scheduler::QueueRequestBatch(const AZStd::vector<FileRequestPtr>& requests)
    {
        AZ_Assert(m_isRunning, "Trying to queue a batch of requests when Streamer's scheduler isn't running.");

        {
            AZStd::scoped_lock lock(m_pendingRequestsLock);
            m_pendingRequests.insert(m_pendingRequests.end(), requests.begin(), requests.end());
        }
        m_context.WakeUpSchedulingThread();
    }

    void Scheduler::QueueRequestBatch(AZStd::vector<FileRequestPtr>&& requests)
    {
        AZ_Assert(m_isRunning, "Trying to queue a batch of requests when Streamer's scheduler isn't running.");

        {
            AZStd::scoped_lock lock(m_pendingRequestsLock);
            AZStd::move(requests.begin(), requests.end(), AZStd::back_inserter(m_pendingRequests));
        }
        m_context.WakeUpSchedulingThread();
    }

    void Scheduler::SuspendProcessing()
    {
        m_isSuspended = true;
    }

    void Scheduler::ResumeProcessing()
    {
        if (m_isSuspended)
        {
            m_isSuspended = false;
            m_context.WakeUpSchedulingThread();
        }
    }

    bool Scheduler::IsSuspended() const
    {
        return m_isSuspended;
    }

    void Scheduler::CollectStatistics(AZStd::vector<Statistic>& statistics)
    {
        statistics.push_back(Statistic::CreateBoolean(
            SchedulerName, "Is suspended", m_isSuspended,
            "Whether or not the scheduler is suspended. When suspended the scheduler will not do any processing and effectively prevents "
            "Streamer from doing any work.", Statistic::GraphType::None));
#if AZ_STREAMER_ADD_EXTRA_PROFILING_INFO
        statistics.push_back(Statistic::CreateBoolean(
            SchedulerName, "Is idle", m_stackStatus.m_isIdle,
            "Whether or not Streamer is scheduling requests. If this is not true most of the time, it indicates scheduling is too expensive, "
            "that there are nodes that take too long and/or there are callbacks that take too long. Note that some nodes are not "
            "fully asynchronous, such as the default storage drive, which will cause this value to be false more often."));
        statistics.push_back(Statistic::CreateInteger(
            SchedulerName, "Available slots", m_stackStatus.m_numAvailableSlots,
            "The number of free slots to schedule requests in. If this is a positive value, it indicates that Streamer could be doing more "
            "work. This value should be negative as this indicates that various nodes have requests queued up for immediate processing."));
        statistics.push_back(Statistic::CreateBytesPerSecond(
            SchedulerName, "Processing speed", m_processingSpeedStat.CalculateAverage(),
            "The average processing speed. This value indicates the total amount of bytes per second Streamer has processed and includes "
            "requests that don't read any data from files. It will generally be smaller than read speeds reported from nodes. If the gap "
            "is too big it can indicate that there's too much time spend on scheduling, nodes take too long to prepare/process, there are "
            "too many non-read requests made, callbacks take too long and/or there are not enough read requests to efficiently amortize "
            "out the cost of scheduling. The timings for the individual scheduling steps can provide an indication of the cost."));
        statistics.push_back(Statistic::CreateInteger(
            SchedulerName, "Scheduling request count", m_context.GetNumPreparedRequests(),
            "The average number of requests that the scheduler is analyzing for reordering."));
        statistics.push_back(Statistic::CreateTimeRange(
            SchedulerName, "Preparing", m_preparingTimeStat.CalculateAverage(), m_preparingTimeStat.GetMinimum(),
            m_preparingTimeStat.GetMaximum(),
            "The average amount of time that the scheduler spends preparing requests for processing."));
        statistics.push_back(Statistic::CreateTimeRange(
            SchedulerName, "Scheduling", m_schedulingTimeStat.CalculateAverage(), m_schedulingTimeStat.GetMinimum(),
            m_schedulingTimeStat.GetMaximum(),
            "The average amount of time that the scheduler spends ordering requests for optimal execution."));
        statistics.push_back(Statistic::CreateTimeRange(
            SchedulerName, "Queuing", m_queuingTimeStat.CalculateAverage(), m_queuingTimeStat.GetMinimum(), m_queuingTimeStat.GetMaximum(),
            "The average amount of time the scheduler spends on queuing up requests for processing. This includes any time individual "
            "nodes spend on queuing requests."));
        statistics.push_back(Statistic::CreateTimeRange(
            SchedulerName, "Executing", m_executingTimeStat.CalculateAverage(), m_executingTimeStat.GetMinimum(),
            m_executingTimeStat.GetMaximum(),
            "The average amount of time the scheduler spends executing commands. This includes any time individual nodes spend on "
            "executing requests."));
        statistics.push_back(Statistic::CreatePercentageRange(
            SchedulerName, ImmediateReadsName, m_immediateReadsPercentageStat.GetAverage(), m_immediateReadsPercentageStat.GetMinimum(),
            m_immediateReadsPercentageStat.GetMaximum(),
            "The number of read requests that were queued and needed immediate processing. These requests are immediately set to be "
            "processed and don't get scheduled. If this value is high there may be too many requests that are set to 'now' or have "
            "deadlines that too tight. Reducing these cases will help allow Streamer to schedule better and improves performance."));
#endif
        m_context.CollectStatistics(statistics);
        m_threadData.m_streamStack->CollectStatistics(statistics);
    }

    void Scheduler::GetRecommendations(IStreamerTypes::Recommendations& recommendations) const
    {
        recommendations = m_recommendations;
    }

    void Scheduler::Thread_MainLoop()
    {
        m_threadData.m_streamStack->SetContext(m_context);
        AZStd::vector<FileRequestPtr> outstandingRequests;

        while (m_isRunning)
        {
            {
                AZ_PROFILE_SCOPE(AzCore, "Scheduler suspended.");
                m_context.SuspendSchedulingThread();
            }

            // Only do processing if the thread hasn't been suspended.
            while (!m_isSuspended)
            {
                AZ_PROFILE_SCOPE(AzCore, "Scheduler main loop.");

                // Always schedule requests first as the main Streamer thread could have been asleep for a long time due to slow reading
                // but also don't schedule after every change in the queue as scheduling is not cheap.
                Thread_ScheduleRequests();
                do
                {
                    do
                    {
                        AZ_PROFILE_SCOPE(AzCore, "Scheduler queue requests.");
                        // If there are pending requests and available slots, queue the next requests.
                        while(m_context.GetNumPreparedRequests() > 0)
                        {
                            m_stackStatus = StreamStackEntry::Status{};
                            m_threadData.m_streamStack->UpdateStatus(m_stackStatus);
                            if (m_stackStatus.m_numAvailableSlots > 0)
                            {
                                Thread_QueueNextRequest();
                            }
                            else
                            {
                                break;
                            }
                        }
                        // Check if there are any requests that have completed their work and if so complete them. This will open slots so
                        // try to queue more pending requests.
                    } while (m_context.FinalizeCompletedRequests());
                    // Tick the stream stack to do work. If work was done there might be requests to complete and new slots might have opened up.
#if AZ_STREAMER_ADD_EXTRA_PROFILING_INFO
                    m_stackStatus = StreamStackEntry::Status{};
                    m_threadData.m_streamStack->UpdateStatus(m_stackStatus);
#endif
                } while (Thread_ExecuteRequests());

                // Check if there are requests queued from other threads, if so move them to the main Streamer thread by executing them. This then
                // requires another cycle or scheduling and executing commands.
                if (!Thread_PrepareRequests(outstandingRequests))
                {
                    break;
                }
            }

#if AZ_STREAMER_ADD_EXTRA_PROFILING_INFO
            m_stackStatus = StreamStackEntry::Status{};
            m_threadData.m_streamStack->UpdateStatus(m_stackStatus);
            if (m_stackStatus.m_isIdle)
            {
                auto duration = AZStd::chrono::steady_clock::now() - m_processingStartTime;
                auto durationSec = AZStd::chrono::duration_cast<AZStd::chrono::duration<double>>(duration);
                m_processingSpeedStat.PushEntry(m_processingSize / durationSec.count());
                m_processingSize = 0;
            }
#endif
        }

        // Make sure all requests in the stack are cleared out. This dangling async processes or async processes crashing as assigned
        // such as memory buffers are no longer available.
        Thread_ProcessTillIdle();
    }

    void Scheduler::Thread_QueueNextRequest()
    {
#if AZ_STREAMER_ADD_EXTRA_PROFILING_INFO
        TIMED_AVERAGE_WINDOW_SCOPE(m_queuingTimeStat);
#endif
        AZ_PROFILE_FUNCTION(AzCore);

        FileRequest* next = m_context.PopPreparedRequest();
        next->SetStatus(IStreamerTypes::RequestStatus::Processing);

        AZStd::visit([this, next](auto&& args)
        {
            using Command = AZStd::decay_t<decltype(args)>;
            if constexpr (
                AZStd::is_same_v<Command, Requests::ReadData> ||
                AZStd::is_same_v<Command, Requests::CompressedReadData>)
            {
                auto parentReadRequest = next->GetCommandFromChain<Requests::ReadRequestData>();
                AZ_Assert(parentReadRequest != nullptr, "The issued read request can't be found for the (compressed) read command.");

                size_t size = parentReadRequest->m_size;
                if (parentReadRequest->m_output == nullptr)
                {
                    AZ_Assert(parentReadRequest->m_allocator,
                        "The read request was issued without a memory allocator or valid output address.");
                    u64 recommendedSize = size;
                    if constexpr (AZStd::is_same_v<Command, Requests::ReadData>)
                    {
                        recommendedSize = m_recommendations.CalculateRecommendedMemorySize(size, parentReadRequest->m_offset);
                    }
                    IStreamerTypes::RequestMemoryAllocatorResult allocation =
                        parentReadRequest->m_allocator->Allocate(size, recommendedSize, m_recommendations.m_memoryAlignment);
                    if (allocation.m_address == nullptr || allocation.m_size < parentReadRequest->m_size)
                    {
                        next->SetStatus(IStreamerTypes::RequestStatus::Failed);
                        m_context.MarkRequestAsCompleted(next);
                        return;
                    }
                    parentReadRequest->m_output = allocation.m_address;
                    parentReadRequest->m_outputSize = allocation.m_size;
                    parentReadRequest->m_memoryType = allocation.m_type;
                    if constexpr (AZStd::is_same_v<Command, Requests::ReadData>)
                    {
                        args.m_output = parentReadRequest->m_output;
                        args.m_outputSize = allocation.m_size;
                    }
                    else if constexpr (AZStd::is_same_v<Command, Requests::CompressedReadData>)
                    {
                        args.m_output = parentReadRequest->m_output;
                    }
                }

#if AZ_STREAMER_ADD_EXTRA_PROFILING_INFO
                if (m_processingSize == 0)
                {
                    m_processingStartTime = AZStd::chrono::steady_clock::now();
                }
#endif

                if constexpr (AZStd::is_same_v<Command, Requests::ReadData>)
                {
                    m_threadData.m_lastFilePath = args.m_path;
                    m_threadData.m_lastFileOffset = args.m_offset + args.m_size;
#if AZ_STREAMER_ADD_EXTRA_PROFILING_INFO
                    m_processingSize += args.m_size;
#endif
                }
                else if constexpr (AZStd::is_same_v<Command, Requests::CompressedReadData>)
                {
                    const CompressionInfo& info = args.m_compressionInfo;
                    m_threadData.m_lastFilePath = info.m_archiveFilename;
                    m_threadData.m_lastFileOffset = info.m_offset + info.m_compressedSize;
#if AZ_STREAMER_ADD_EXTRA_PROFILING_INFO
                    m_processingSize += info.m_uncompressedSize;
#endif
                }
                AZ_PROFILE_INTERVAL_START_COLORED(AzCore, next, ProfilerColor,
                    "Streamer queued %zu: %s", next->GetCommand().index(), parentReadRequest->m_path.GetRelativePath());
                m_threadData.m_streamStack->QueueRequest(next);
            }
            else if constexpr (AZStd::is_same_v<Command, Requests::CancelData>)
            {
                return Thread_ProcessCancelRequest(next, args);
            }
            else if constexpr (AZStd::is_same_v<Command, Requests::RescheduleData>)
            {
                return Thread_ProcessRescheduleRequest(next, args);
            }
            else if constexpr (AZStd::is_same_v<Command, Requests::FlushData> || AZStd::is_same_v<Command, Requests::FlushAllData>)
            {
                AZ_PROFILE_INTERVAL_START_COLORED(AzCore, next, ProfilerColor,
                    "Streamer queued %zu", next->GetCommand().index());
                // Flushing becomes a lot less complicated if there are no jobs and/or asynchronous I/O running. This does mean overall
                // longer processing time as bubbles are introduced into the pipeline, but flushing is an infrequent event that only
                // happens during development.
                Thread_ProcessTillIdle();
                m_threadData.m_streamStack->QueueRequest(next);
            }
            else
            {
                AZ_PROFILE_INTERVAL_START_COLORED(AzCore, next, ProfilerColor,
                    "Streamer queued %zu", next->GetCommand().index());
                m_threadData.m_streamStack->QueueRequest(next);
            }
        }, next->GetCommand());
    }

    bool Scheduler::Thread_ExecuteRequests()
    {
#if AZ_STREAMER_ADD_EXTRA_PROFILING_INFO
        TIMED_AVERAGE_WINDOW_SCOPE(m_executingTimeStat);
#endif
        AZ_PROFILE_FUNCTION(AzCore);
        return m_threadData.m_streamStack->ExecuteRequests();
    }

    bool Scheduler::Thread_PrepareRequests(AZStd::vector<FileRequestPtr>& outstandingRequests)
    {
#if AZ_STREAMER_ADD_EXTRA_PROFILING_INFO
        TIMED_AVERAGE_WINDOW_SCOPE(m_preparingTimeStat);
#endif
        AZ_PROFILE_FUNCTION(AzCore);

        {
            AZStd::scoped_lock lock(m_pendingRequestsLock);
            if (!m_pendingRequests.empty())
            {
                outstandingRequests.swap(m_pendingRequests);
            }
            else
            {
                return false;
            }
        }

#if AZ_STREAMER_ADD_EXTRA_PROFILING_INFO
        AZStd::chrono::steady_clock::time_point now = AZStd::chrono::steady_clock::now();
        auto visitor = [this, now](auto&& args) -> void
#else
        auto visitor = [](auto&& args) -> void
#endif
        {
            using Command = AZStd::decay_t<decltype(args)>;
            if constexpr (AZStd::is_same_v<Command, Requests::ReadRequestData>)
            {
                if (args.m_output == nullptr && args.m_allocator != nullptr)
                {
                    args.m_allocator->LockAllocator();
                }
#if AZ_STREAMER_ADD_EXTRA_PROFILING_INFO
                m_immediateReadsPercentageStat.PushSample(args.m_deadline < now ? 1.0 : 0.0);
                Statistic::PlotImmediate(SchedulerName, ImmediateReadsName, m_immediateReadsPercentageStat.GetMostRecentSample());
#endif
            }
        };

        for (auto& request : outstandingRequests)
        {
            AZStd::visit(visitor, request->m_request.GetCommand());
        }
        for (auto& request : outstandingRequests)
        {
            // Add a link in front of the external request to keep a reference to the FileRequestPtr alive while it's being processed.
            FileRequest* requestPtr = &request->m_request;
            FileRequest* linkRequest = m_context.GetNewInternalRequest();
            linkRequest->CreateRequestLink(AZStd::move(request));
            requestPtr->SetStatus(IStreamerTypes::RequestStatus::Queued);
            m_threadData.m_streamStack->PrepareRequest(requestPtr);
        }
        outstandingRequests.clear();
        return true;
    }

    void Scheduler::Thread_ProcessTillIdle()
    {
        AZ_PROFILE_FUNCTION(AzCore);

        while (true)
        {
            m_stackStatus = StreamStackEntry::Status{};
            m_threadData.m_streamStack->UpdateStatus(m_stackStatus);
            if (m_stackStatus.m_isIdle)
            {
                return;
            }

            Thread_ExecuteRequests();
            m_context.FinalizeCompletedRequests();
        }
    }

    void Scheduler::Thread_ProcessCancelRequest(FileRequest* request, Requests::CancelData& data)
    {
        AZ_PROFILE_INTERVAL_START_COLORED(AzCore, request, ProfilerColor, "Streamer queued cancel");
        auto& pending = m_context.GetPreparedRequests();
        auto pendingIt = pending.begin();
        while (pendingIt != pending.end())
        {
            if ((*pendingIt)->WorksOn(data.m_target))
            {
                (*pendingIt)->SetStatus(IStreamerTypes::RequestStatus::Canceled);
                m_context.MarkRequestAsCompleted(*pendingIt);
                pendingIt = pending.erase(pendingIt);
            }
            else
            {
                ++pendingIt;
            }
        }

        m_threadData.m_streamStack->QueueRequest(request);
    }

    void Scheduler::Thread_ProcessRescheduleRequest(FileRequest* request, Requests::RescheduleData& data)
    {
        AZ_PROFILE_INTERVAL_START_COLORED(AzCore, request, ProfilerColor, "Streamer queued reschedule");
        auto& pendingRequests = m_context.GetPreparedRequests();
        for (FileRequest* pending : pendingRequests)
        {
            if (pending->WorksOn(data.m_target))
            {
                // Read requests are the only requests that use deadlines and dynamic priorities.
                auto readRequest = pending->GetCommandFromChain<Requests::ReadRequestData>();
                if (readRequest)
                {
                    readRequest->m_deadline = data.m_newDeadline;
                    readRequest->m_priority = data.m_newPriority;
                }
                // Nothing more needs to happen as the request will be rescheduled in the next full pass in the main loop or
                // it's going to be one of the next requests to be picked up, in which case the time between the reschedule
                // request and read being processed that this similar to the reschedule being too late.
            }
        }

        request->SetStatus(IStreamerTypes::RequestStatus::Completed);
        m_context.MarkRequestAsCompleted(request);
    }

    auto Scheduler::Thread_PrioritizeRequests(const FileRequest* first, const FileRequest* second) const -> Order
    {
        if (first == second)
        {
            return Order::Equal;
        }

        // Sort by order priority of the command in the request. This allows to for instance have cancel request
        // always happen before any other requests.
        auto order = [](auto&& args)
        {
            using Command = AZStd::decay_t<decltype(args)>;
            if constexpr (AZStd::is_same_v<Command, AZStd::monostate>)
            {
                return IStreamerTypes::s_priorityLowest;
            }
            else
            {
                return Command::s_orderPriority;
            }
        };
        IStreamerTypes::Priority firstOrderPriority = AZStd::visit(order, first->GetCommand());
        IStreamerTypes::Priority secondOrderPriority = AZStd::visit(order, second->GetCommand());
        if (firstOrderPriority > secondOrderPriority) { return Order::FirstRequest; }
        if (firstOrderPriority < secondOrderPriority) { return Order::SecondRequest; }

        // Order is the same for both requests, so prioritize the request that are at risk of missing
        // it's deadline.
        const Requests::ReadRequestData* firstRead = first->GetCommandFromChain<Requests::ReadRequestData>();
        const Requests::ReadRequestData* secondRead = second->GetCommandFromChain<Requests::ReadRequestData>();

        if (firstRead == nullptr || secondRead == nullptr)
        {
            // One of the two requests or both don't have information for scheduling so leave them
            // in the same order.
            return Order::Equal;
        }

        bool firstInPanic = first->GetEstimatedCompletion() > firstRead->m_deadline;
        bool secondInPanic = second->GetEstimatedCompletion() > secondRead->m_deadline;
        // Both request are at risk of not completing before their deadline.
        if (firstInPanic && secondInPanic)
        {
            // Let the one with the highest priority go first.
            if (firstRead->m_priority != secondRead->m_priority)
            {
                return firstRead->m_priority < secondRead->m_priority ? Order::FirstRequest : Order::SecondRequest;
            }

            // If neither has started and have the same priority, prefer to start the closest deadline.
            if (firstRead->m_deadline == secondRead->m_deadline)
            {
                return Order::Equal;
            }

            return firstRead->m_deadline < secondRead->m_deadline ? Order::FirstRequest : Order::SecondRequest;
        }

        // Check if one of the requests is in panic and prefer to prioritize that request
        if (firstInPanic) { return Order::FirstRequest; }
        if (secondInPanic) { return Order::SecondRequest; }

        // Both are not in panic so base the order on the number of IO steps (opening files, seeking, etc.) are needed.
        auto sameFile = [this](auto&& args)
        {
            using Command = AZStd::decay_t<decltype(args)>;
            if constexpr (AZStd::is_same_v<Command, Requests::ReadData>)
            {
                return m_threadData.m_lastFilePath == args.m_path;
            }
            else if constexpr (AZStd::is_same_v<Command, Requests::CompressedReadData>)
            {
                return m_threadData.m_lastFilePath == args.m_compressionInfo.m_archiveFilename;
            }
            else
            {
                return false;
            }
        };
        bool firstInSameFile = AZStd::visit(sameFile, first->GetCommand());
        bool secondInSameFile = AZStd::visit(sameFile, second->GetCommand());
        // If both request are in the active file, prefer to pick the file that's closest to the last read.
        if (firstInSameFile && secondInSameFile)
        {
            auto offset = [](auto&& args) -> s64
            {
                using Command = AZStd::decay_t<decltype(args)>;
                if constexpr (AZStd::is_same_v<Command, Requests::ReadData>)
                {
                    return aznumeric_caster(args.m_offset);
                }
                else if constexpr (AZStd::is_same_v<Command, Requests::CompressedReadData>)
                {
                    return aznumeric_caster(args.m_compressionInfo.m_offset);
                }
                else
                {
                    return std::numeric_limits<s64>::max();
                }
            };
            s64 firstReadOffset = AZStd::visit(offset, first->GetCommand());
            s64 secondReadOffset = AZStd::visit(offset, second->GetCommand());
            s64 firstSeekDistance = abs(aznumeric_cast<s64>(m_threadData.m_lastFileOffset) - firstReadOffset);
            s64 secondSeekDistance = abs(aznumeric_cast<s64>(m_threadData.m_lastFileOffset) - secondReadOffset);

            if (firstSeekDistance == secondSeekDistance)
            {
                return Order::Equal;
            }
            
            return firstSeekDistance < secondSeekDistance ? Order::FirstRequest : Order::SecondRequest;
        }

        // Prefer to continue in the same file so prioritize the request that's in the same file
        if (firstInSameFile) { return Order::FirstRequest; }
        if (secondInSameFile) { return Order::SecondRequest; }

        // If both requests need to open a new file, keep them in the same order as there's no information available
        // to indicate which request would be able to load faster or more efficiently.
        return Order::Equal;
    }

    void Scheduler::Thread_ScheduleRequests()
    {
#if AZ_STREAMER_ADD_EXTRA_PROFILING_INFO
        TIMED_AVERAGE_WINDOW_SCOPE(m_schedulingTimeStat);
#endif
        AZ_PROFILE_FUNCTION(AzCore);

        AZStd::chrono::steady_clock::time_point now = AZStd::chrono::steady_clock::now();
        auto& pendingQueue = m_context.GetPreparedRequests();

        m_threadData.m_streamStack->UpdateCompletionEstimates(now, m_threadData.m_internalPendingRequests,
            pendingQueue.begin(), pendingQueue.end());
        m_threadData.m_internalPendingRequests.clear();

        if (m_context.GetNumPreparedRequests() > 1)
        {
            AZ_PROFILE_SCOPE(AzCore,
                "Scheduler::Thread_ScheduleRequests - Sorting %i requests", m_context.GetNumPreparedRequests());
            auto sorter = [this](const FileRequest* lhs, const FileRequest* rhs) -> bool
            {
                if (lhs == rhs)
                {
                    // AZStd::sort may compare an element to itself; 
                    //   it's required this condition remain consistent and return false.
                    return false;
                }

                Order order = Thread_PrioritizeRequests(lhs, rhs);
                switch (order)
                {
                case Order::FirstRequest:
                    return true;
                case Order::SecondRequest:
                    return false;
                case Order::Equal:
                    // If both requests can't be prioritized relative to each other than keep them in the order they
                    // were originally queued.
                    return lhs->GetPendingId() < rhs->GetPendingId();
                default:
                    AZ_Assert(false, "Unsupported request order: %i.", order);
                    return true;
                }
            };

            AZStd::sort(pendingQueue.begin(), pendingQueue.end(), sorter);
        }
    }
} // namespace AZ::IO
