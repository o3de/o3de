/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Debug/Profiler.h>
#include <AzCore/IO/CompressionBus.h>
#include <AzCore/IO/Streamer/FileRequest.h>
#include <AzCore/IO/Streamer/Streamer.h>
#include <AzCore/IO/Streamer/StreamerConfiguration.h>
#include <AzCore/IO/Streamer/StreamStackEntry.h>
#include <AzCore/std/smart_ptr/make_shared.h>

namespace AZ::IO
{
    FileRequestPtr Streamer::Read(AZStd::string_view relativePath, void* outputBuffer, size_t outputBufferSize, size_t readSize,
        AZStd::chrono::microseconds deadline, IStreamerTypes::Priority priority, size_t offset)
    {
        FileRequestPtr result = CreateRequest();
        Read(result, relativePath, outputBuffer, outputBufferSize, readSize, deadline, priority, offset);
        return result;
    }

    FileRequestPtr& Streamer::Read(FileRequestPtr& request, AZStd::string_view relativePath, void* outputBuffer,
        size_t outputBufferSize, size_t readSize, AZStd::chrono::microseconds deadline, IStreamerTypes::Priority priority, size_t offset)
    {
        AZStd::chrono::system_clock::time_point deadlineTimePoint = (deadline == IStreamerTypes::s_noDeadline)
            ? FileRequest::s_noDeadlineTime
            : AZStd::chrono::system_clock::now() + deadline;
        request->m_request.CreateReadRequest(
            RequestPath(relativePath), outputBuffer, outputBufferSize, offset, readSize, deadlineTimePoint, priority);
        return request;
    }

    FileRequestPtr Streamer::Read(AZStd::string_view relativePath, IStreamerTypes::RequestMemoryAllocator& allocator,
        size_t size, AZStd::chrono::microseconds deadline, IStreamerTypes::Priority priority, size_t offset)
    {
        FileRequestPtr result = CreateRequest();
        Read(result, relativePath, allocator, size, deadline, priority, offset);
        return result;
    }

    FileRequestPtr& Streamer::Read(FileRequestPtr& request, AZStd::string_view relativePath, IStreamerTypes::RequestMemoryAllocator& allocator,
        size_t size, AZStd::chrono::microseconds deadline, IStreamerTypes::Priority priority, size_t offset)
    {
        AZStd::chrono::system_clock::time_point deadlineTimePoint = (deadline == IStreamerTypes::s_noDeadline)
            ? FileRequest::s_noDeadlineTime
            : AZStd::chrono::system_clock::now() + deadline;
        request->m_request.CreateReadRequest(RequestPath(relativePath), &allocator, offset, size, deadlineTimePoint, priority);
        return request;
    }

    FileRequestPtr Streamer::Cancel(FileRequestPtr target)
    {
        FileRequestPtr result = CreateRequest();
        Cancel(result, AZStd::move(target));
        return result;
    }

    FileRequestPtr& Streamer::Cancel(FileRequestPtr& request, FileRequestPtr target)
    {
        request->m_request.CreateCancel(AZStd::move(target));
        return request;
    }

    FileRequestPtr Streamer::RescheduleRequest(FileRequestPtr target, AZStd::chrono::microseconds newDeadline,
        IStreamerTypes::Priority newPriority)
    {
        FileRequestPtr result = CreateRequest();
        RescheduleRequest(result, AZStd::move(target), newDeadline, newPriority);
        return result;
    }

    FileRequestPtr& Streamer::RescheduleRequest(FileRequestPtr& request, FileRequestPtr target, AZStd::chrono::microseconds newDeadline,
        IStreamerTypes::Priority newPriority)
    {
        AZStd::chrono::system_clock::time_point deadlineTimePoint = (newDeadline == IStreamerTypes::s_noDeadline)
            ? FileRequest::s_noDeadlineTime
            : AZStd::chrono::system_clock::now() + newDeadline;
        request->m_request.CreateReschedule(AZStd::move(target), deadlineTimePoint, newPriority);
        return request;
    }

    FileRequestPtr Streamer::CreateDedicatedCache(AZStd::string_view relativePath)
    {
        FileRequestPtr result = CreateRequest();
        CreateDedicatedCache(result, relativePath);
        return result;
    }

    FileRequestPtr& Streamer::CreateDedicatedCache(FileRequestPtr& request, AZStd::string_view relativePath)
    {
        RequestPath path(relativePath);
        request->m_request.CreateDedicatedCacheCreation(AZStd::move(path));
        return request;
    }

    FileRequestPtr Streamer::DestroyDedicatedCache(AZStd::string_view relativePath)
    {
        FileRequestPtr result = CreateRequest();
        DestroyDedicatedCache(result, relativePath);
        return result;
    }

    FileRequestPtr& Streamer::DestroyDedicatedCache(FileRequestPtr& request, AZStd::string_view relativePath)
    {
        request->m_request.CreateDedicatedCacheDestruction(RequestPath(relativePath));
        return request;
    }

    FileRequestPtr Streamer::FlushCache(AZStd::string_view relativePath)
    {
        FileRequestPtr result = CreateRequest();
        FlushCache(result, relativePath);
        return result;
    }

    FileRequestPtr& Streamer::FlushCache(FileRequestPtr& request, AZStd::string_view relativePath)
    {
        request->m_request.CreateFlush(RequestPath(relativePath));
        return request;
    }

    FileRequestPtr Streamer::FlushCaches()
    {
        FileRequestPtr result = CreateRequest();
        FlushCaches(result);
        return result;
    }

    FileRequestPtr& Streamer::FlushCaches(FileRequestPtr& request)
    {
        request->m_request.CreateFlushAll();
        return request;
    }

    FileRequestPtr Streamer::Custom(AZStd::any data)
    {
        FileRequestPtr result = CreateRequest();
        Custom(result, AZStd::move(data));
        return result;
    }

    FileRequestPtr& Streamer::Custom(FileRequestPtr& request, AZStd::any data)
    {
        request->m_request.CreateCustom(AZStd::move(data));
        return request;
    }

    FileRequestPtr& Streamer::SetRequestCompleteCallback(FileRequestPtr& request, OnCompleteCallback callback)
    {
        request->m_request.SetCompletionCallback(AZStd::move(callback));
        return request;
    }

    FileRequestPtr Streamer::CreateRequest()
    {
        return m_streamStack->CreateRequest();
    }

    void Streamer::CreateRequestBatch(AZStd::vector<FileRequestPtr>& requests, size_t count)
    {
        m_streamStack->CreateRequestBatch(requests, count);
    }

    void Streamer::QueueRequest(const FileRequestPtr& request)
    {
        m_streamStack->QueueRequest(request);
    }

    void Streamer::QueueRequestBatch(const AZStd::vector<FileRequestPtr>& requests)
    {
        m_streamStack->QueueRequestBatch(requests);
    }

    void Streamer::QueueRequestBatch(AZStd::vector<FileRequestPtr>&& requests)
    {
        m_streamStack->QueueRequestBatch(AZStd::move(requests));
    }

    bool Streamer::HasRequestCompleted(FileRequestHandle request) const
    {
        return GetRequestStatus(request) >= IStreamerTypes::RequestStatus::Completed;
    }

    IStreamerTypes::RequestStatus Streamer::GetRequestStatus(FileRequestHandle request) const
    {
        AZ_Assert(request.m_request, "The request handle provided to Streamer::GetRequestStatus is invalid.");
        return request.m_request->GetStatus();
    }

    AZStd::chrono::system_clock::time_point Streamer::GetEstimatedRequestCompletionTime(FileRequestHandle request) const
    {
        AZ_Assert(request.m_request, "The request handle provided to Streamer::GetEstimatedRequestCompletionTime is invalid.");
        return request.m_request->GetEstimatedCompletion();
    }

    bool Streamer::GetReadRequestResult(FileRequestHandle request, void*& buffer, u64& numBytesRead,
        IStreamerTypes::ClaimMemory claimMemory) const
    {
        AZ_Assert(request.m_request, "The request handle provided to Streamer::GetReadRequestResult is invalid.");
        auto readRequest = AZStd::get_if<Requests::ReadRequestData>(&request.m_request->GetCommand());
        if (readRequest != nullptr)
        {
            buffer = readRequest->m_output;
            numBytesRead = readRequest->m_size;
            if (claimMemory == IStreamerTypes::ClaimMemory::Yes)
            {
                AZ_Assert(HasRequestCompleted(request), "Claiming memory from a read request that's still in progress. "
                    "This can lead to crashing if data is still being streamed to the request's buffer.");
                // The caller has claimed the buffer and is now responsible for clearing it.
                readRequest->m_allocator->UnlockAllocator();
                readRequest->m_allocator = nullptr;
            }
            return true;
        }
        else
        {
            AZ_Assert(false, "Provided file request did not contain read information");
            buffer = nullptr;
            numBytesRead = 0;
            return false;
        }
    }

    void Streamer::CollectStatistics(AZStd::vector<Statistic>& statistics)
    {
        m_streamStack->CollectStatistics(statistics);
    }

    const IStreamerTypes::Recommendations& Streamer::GetRecommendations() const
    {
        return m_recommendations;
    }

    void Streamer::SuspendProcessing()
    {
        m_streamStack->SuspendProcessing();
    }

    void Streamer::ResumeProcessing()
    {
        m_streamStack->ResumeProcessing();
    }

    void Streamer::RecordStatistics()
    {
        AZStd::vector<Statistic> statistics;
        m_streamStack->CollectStatistics(statistics);
        for (Statistic& stat : statistics)
        {
            switch (stat.GetType())
            {
            case Statistic::Type::FloatingPoint:
                AZ_PROFILE_DATAPOINT(AzCore, stat.GetFloatValue(), "Streamer/%.*s/%.*s",
                    aznumeric_cast<int>(stat.GetOwner().length()), stat.GetOwner().data(), aznumeric_cast<int>(stat.GetName().length()), stat.GetName().data());
                break;
            case Statistic::Type::Integer:
                AZ_PROFILE_DATAPOINT(AzCore, stat.GetIntegerValue(), "Streamer/%.*s/%.*s",
                    aznumeric_cast<int>(stat.GetOwner().length()), stat.GetOwner().data(), aznumeric_cast<int>(stat.GetName().length()), stat.GetName().data());
                break;
            case Statistic::Type::Percentage:
                AZ_PROFILE_DATAPOINT_PERCENT(AzCore, stat.GetPercentage(), "Streamer/%.*s/%.*s (percent)",
                    aznumeric_cast<int>(stat.GetOwner().length()), stat.GetOwner().data(), aznumeric_cast<int>(stat.GetName().length()), stat.GetName().data());
                break;
            default:
                AZ_Assert(false, "Unsupported statistic type: %i", stat.GetType());
                break;
            }
        }
    }

    FileRequestPtr Streamer::Report(Requests::ReportType reportType)
    {
        FileRequestPtr result = CreateRequest();
        Report(result, reportType);
        return result;
    }

    FileRequestPtr& Streamer::Report(FileRequestPtr& request, Requests::ReportType reportType)
    {
        request->m_request.CreateReport(reportType);
        return request;
    }

    Streamer::Streamer(const AZStd::thread_desc& threadDesc, AZStd::unique_ptr<Scheduler> streamStack)
        : m_streamStack(AZStd::move(streamStack))
    {
        m_streamStack->GetRecommendations(m_recommendations);
        m_streamStack->Start(threadDesc);
    }

    Streamer::~Streamer()
    {
        m_streamStack->Stop();
    }
} // namespace AZ::IO
