/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "DecompressorStackEntry.h"

#include <AzCore/IO/CompressionBus.h>
#include <AzCore/IO/Streamer/FileRequest.h>
#include <AzCore/IO/Streamer/StreamerContext.h>
#include <AzCore/Task/TaskGraph.h>
#include <AzCore/Math/MathUtils.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/parallel/thread.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/std/typetraits/decay.h>

namespace Compression
{
    AZ_TYPE_INFO_WITH_NAME_IMPL(DecompressorRegistrarConfig, "DecompressorRegistrarConfig", "{763D7F80-0FE1-4084-A165-0CC6A2E57F05}");
    AZ_RTTI_NO_TYPE_INFO_IMPL(DecompressorRegistrarConfig, IStreamerStackConfig);
    AZ_CLASS_ALLOCATOR_IMPL(DecompressorRegistrarConfig, AZ::SystemAllocator);

    AZStd::shared_ptr<AZ::IO::StreamStackEntry> DecompressorRegistrarConfig::AddStreamStackEntry(
        const AZ::IO::HardwareInformation& hardware, AZStd::shared_ptr<AZ::IO::StreamStackEntry> parent)
    {
        auto stackEntry = AZStd::make_shared<DecompressorRegistrarEntry>(
            m_maxNumReads, m_maxNumTasks, aznumeric_caster(hardware.m_maxPhysicalSectorSize));
        stackEntry->SetNext(AZStd::move(parent));
        return stackEntry;
    }

    void DecompressorRegistrarConfig::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            serializeContext != nullptr)
        {
            serializeContext->Class<DecompressorRegistrarConfig, IStreamerStackConfig>()
                ->Field("MaxNumReads", &DecompressorRegistrarConfig::m_maxNumReads)
                ->Field("MaxNumTasks", &DecompressorRegistrarConfig::m_maxNumTasks)
                ;
        }
    }

#if AZ_STREAMER_ADD_EXTRA_PROFILING_INFO
    static constexpr const char* DecompBoundName = "Decompression bound";
    static constexpr const char* ReadBoundName = "Read bound";
#endif // AZ_STREAMER_ADD_EXTRA_PROFILING_INFO

    bool DecompressorRegistrarEntry::DecompressionInformation::IsProcessing() const
    {
        return m_compressedData != nullptr;
    }

    DecompressorRegistrarEntry::DecompressorRegistrarEntry(AZ::u32 maxNumReads, AZ::u32 maxNumTasks, AZ::u32 alignment)
        : AZ::IO::StreamStackEntry("Compression Gem decompressor registrar")
        , m_maxNumReads(maxNumReads)
        , m_maxNumTasks(maxNumTasks)
        , m_alignment(alignment)
    {

        m_processingJobs = AZStd::make_unique<DecompressionInformation[]>(m_maxNumTasks);

        m_readBuffers = AZStd::make_unique<Buffer[]>(maxNumReads);
        m_readRequests = AZStd::make_unique<AZ::IO::FileRequest*[]>(maxNumReads);
        m_readBufferStatus = AZStd::make_unique<ReadBufferStatus[]>(maxNumReads);
        for (AZ::u32 i = 0; i < maxNumReads; ++i)
        {
            m_readBufferStatus[i] = ReadBufferStatus::Unused;
        }

        // Add initial dummy values to the stats to avoid division by zero later on and avoid needing branches.
        m_bytesDecompressed.PushEntry(1);
        m_decompressionDurationMicroSec.PushEntry(1);
    }

    void DecompressorRegistrarEntry::PrepareRequest(AZ::IO::FileRequest* request)
    {
        AZ_Assert(request, "PrepareRequest was provided a null request.");

        auto RunCommand = [this, request](auto&& args)
        {
            using Command = AZStd::decay_t<decltype(args)>;
            if constexpr (AZStd::is_same_v<Command, AZ::IO::Requests::ReadRequestData>)
            {
                PrepareReadRequest(request, args);
            }
            else if constexpr (AZStd::is_same_v<Command, AZ::IO::Requests::CreateDedicatedCacheData> ||
                AZStd::is_same_v<Command, AZ::IO::Requests::DestroyDedicatedCacheData>)
            {
                PrepareDedicatedCache(request, args.m_path);
            }
            else
            {
                AZ::IO::StreamStackEntry::PrepareRequest(request);
            }
        };

        AZStd::visit(AZStd::move(RunCommand), request->GetCommand());
    }

    void DecompressorRegistrarEntry::QueueRequest(AZ::IO::FileRequest* request)
    {
        AZ_Assert(request, "QueueRequest was provided a null request.");

        auto QueueCommand = [this, request](auto&& args)
        {
            using Command = AZStd::decay_t<decltype(args)>;
            if constexpr (AZStd::is_same_v<Command, AZ::IO::Requests::CompressedReadData>)
            {
                m_pendingReads.push_back(request);
            }
            else if constexpr (AZStd::is_same_v<Command, AZ::IO::Requests::FileExistsCheckData>)
            {
                m_pendingFileExistChecks.push_back(request);
            }
            else
            {
                if constexpr (AZStd::is_same_v<Command, AZ::IO::Requests::ReportData>)
                {
                    Report(args);
                }
                AZ::IO::StreamStackEntry::QueueRequest(request);
            }
        };

        AZStd::visit(AZStd::move(QueueCommand), request->GetCommand());
    }

    bool DecompressorRegistrarEntry::ExecuteRequests()
    {
        bool result = false;
        // First queue jobs as this might open up new read slots.
        if (m_numInFlightReads > 0 && m_numRunningTasks < m_maxNumTasks)
        {
            result = StartDecompressions();
        }

        // Queue as many new reads as possible.
        while (!m_pendingReads.empty() && m_numInFlightReads < m_maxNumReads)
        {
            StartArchiveRead(m_pendingReads.front());
            m_pendingReads.pop_front();
            result = true;
        }

        // If nothing else happened and there is at least one pending file exist check request, run one of those.
        if (!result && !m_pendingFileExistChecks.empty())
        {
            FileExistsCheck(m_pendingFileExistChecks.front());
            m_pendingFileExistChecks.pop_front();
            result = true;
        }

#if AZ_STREAMER_ADD_EXTRA_PROFILING_INFO
        bool allPendingDecompression = true;
        bool allReading = true;
        for (AZ::u32 i = 0; i < m_maxNumReads; ++i)
        {
            allPendingDecompression =
                allPendingDecompression && (m_readBufferStatus[i] == ReadBufferStatus::PendingDecompression);
            allReading =
                allReading && (m_readBufferStatus[i] == ReadBufferStatus::ReadInFlight);
        }

        m_decompressionBoundStat.PushSample(allPendingDecompression ? 1.0 : 0.0);
        AZ::IO::Statistic::PlotImmediate(m_name, DecompBoundName, m_decompressionBoundStat.GetMostRecentSample());

        m_readBoundStat.PushSample(allReading && (m_numRunningTasks < m_maxNumTasks) ? 1.0 : 0.0);
        AZ::IO::Statistic::PlotImmediate(m_name, ReadBoundName, m_readBoundStat.GetMostRecentSample());
#endif

        return AZ::IO::StreamStackEntry::ExecuteRequests() || result;
    }

    void DecompressorRegistrarEntry::UpdateStatus(Status& status) const
    {
        AZ::IO::StreamStackEntry::UpdateStatus(status);
        AZ::s32 numAvailableSlots = static_cast<AZ::s32>(m_maxNumReads - m_numInFlightReads);
        status.m_numAvailableSlots = AZStd::min(status.m_numAvailableSlots, numAvailableSlots);
        status.m_isIdle = status.m_isIdle && IsIdle();
    }

    void DecompressorRegistrarEntry::UpdateCompletionEstimates(AZStd::chrono::steady_clock::time_point now, AZStd::vector<AZ::IO::FileRequest*>& internalPending,
        AZ::IO::StreamerContext::PreparedQueue::iterator pendingBegin, AZ::IO::StreamerContext::PreparedQueue::iterator pendingEnd)
    {
        // Create predictions for all pending requests. Some will be further processed after this.
        AZStd::reverse_copy(m_pendingFileExistChecks.begin(), m_pendingFileExistChecks.end(), AZStd::back_inserter(internalPending));
        AZStd::reverse_copy(m_pendingReads.begin(), m_pendingReads.end(), AZStd::back_inserter(internalPending));

        AZ::IO::StreamStackEntry::UpdateCompletionEstimates(now, internalPending, pendingBegin, pendingEnd);

        double totalBytesDecompressed = aznumeric_caster(m_bytesDecompressed.GetTotal());
        double totalDecompressionDuration = aznumeric_caster(m_decompressionDurationMicroSec.GetTotal());
        AZStd::chrono::microseconds cumulativeDelay = AZStd::chrono::microseconds::max();

        // Check the number of jobs that are processing.
        for (AZ::u32 i = 0; i < m_maxNumTasks; ++i)
        {
            if (m_processingJobs[i].IsProcessing())
            {
                AZ::IO::FileRequest* compressedRequest = m_processingJobs[i].m_waitRequest->GetParent();
                AZ_Assert(compressedRequest, "A wait request attached to DecompressorRegistrarEntry was completed but didn't have a parent compressed request.");
                auto data = AZStd::get_if<AZ::IO::Requests::CompressedReadData>(&compressedRequest->GetCommand());
                AZ_Assert(data, "Compressed request in the decompression queue in DecompressorRegistrarEntry didn't contain compression read data.");

                size_t bytesToDecompress = data->m_compressionInfo.m_compressedSize;
                auto decompressionDuration = AZStd::chrono::microseconds(
                    static_cast<AZ::u64>((bytesToDecompress * totalDecompressionDuration) / totalBytesDecompressed));
                auto timeInProcessing = now - m_processingJobs[i].m_jobStartTime;
                auto timeLeft = decompressionDuration > timeInProcessing ? decompressionDuration - timeInProcessing : AZStd::chrono::microseconds(0);
                // Get the shortest time as this indicates the next decompression to become available.
                cumulativeDelay = AZStd::min(AZStd::chrono::duration_cast<AZStd::chrono::microseconds>(timeLeft), cumulativeDelay);
                m_processingJobs[i].m_waitRequest->SetEstimatedCompletion(now + timeLeft);
            }
        }
        if (cumulativeDelay == AZStd::chrono::microseconds::max())
        {
            cumulativeDelay = AZStd::chrono::microseconds(0);
        }

        // Next update all reads that are in flight. These will have an estimation for the read to complete, but will then be queued
        // for decompression, so add the time needed decompression. Assume that decompression happens in parallel.
        AZStd::chrono::microseconds decompressionDelay =
            AZStd::chrono::microseconds(static_cast<AZ::u64>(m_decompressionJobDelayMicroSec.CalculateAverage()));
        AZStd::chrono::microseconds smallestDecompressionDuration = AZStd::chrono::microseconds::max();
        for (AZ::u32 i = 0; i < m_maxNumReads; ++i)
        {
            AZStd::chrono::steady_clock::time_point baseTime;
            switch (m_readBufferStatus[i])
            {
            case ReadBufferStatus::Unused:
                continue;
            case ReadBufferStatus::ReadInFlight:
                // Internal read requests can start and complete but pending finalization before they're ever scheduled in which case
                // the estimated time is not set.
                baseTime = m_readRequests[i]->GetEstimatedCompletion();
                if (baseTime == AZStd::chrono::steady_clock::time_point())
                {
                    baseTime = now;
                }
                break;
            case ReadBufferStatus::PendingDecompression:
                baseTime = now;
                break;
            default:
                AZ_Assert(false, "Unsupported buffer type: %i.", m_readBufferStatus[i]);
                continue;
            }

            baseTime += cumulativeDelay; // Delay until the first decompression slot becomes available.
            baseTime += decompressionDelay; // The average time it takes for the job system to pick up the decompression job.

            // Calculate the amount of time it will take to decompress the data.
            AZ::IO::FileRequest* compressedRequest = m_readRequests[i]->GetParent();
            auto data = AZStd::get_if<AZ::IO::Requests::CompressedReadData>(&compressedRequest->GetCommand());

            size_t bytesToDecompress = data->m_compressionInfo.m_compressedSize;
            auto decompressionDuration = AZStd::chrono::microseconds(
                static_cast<AZ::u64>((bytesToDecompress * totalDecompressionDuration) / totalBytesDecompressed));
            smallestDecompressionDuration = AZStd::min(smallestDecompressionDuration, decompressionDuration);
            baseTime += decompressionDuration;

            m_readRequests[i]->SetEstimatedCompletion(baseTime);
        }
        if (smallestDecompressionDuration != AZStd::chrono::microseconds::max())
        {
            cumulativeDelay += smallestDecompressionDuration; // Time after which the decompression jobs and pending reads have completed.
        }

        // For all internally pending compressed reads add the decompression time. The read time will have already been added downstream.
        // Because this call will go from the top of the stack to the bottom, but estimation is calculated from the bottom to the top, this
        // list should be processed in reverse order.
        for (auto pendingIt = internalPending.rbegin(); pendingIt != internalPending.rend(); ++pendingIt)
        {
            EstimateCompressedReadRequest(*pendingIt, cumulativeDelay, decompressionDelay,
                totalDecompressionDuration, totalBytesDecompressed);
        }

        // Finally add a prediction for all the requests that are waiting to be queued.
        for (auto requestIt = pendingBegin; requestIt != pendingEnd; ++requestIt)
        {
            EstimateCompressedReadRequest(*requestIt, cumulativeDelay, decompressionDelay,
                totalDecompressionDuration, totalBytesDecompressed);
        }
    }

    void DecompressorRegistrarEntry::EstimateCompressedReadRequest(AZ::IO::FileRequest* request, AZStd::chrono::microseconds& cumulativeDelay,
        AZStd::chrono::microseconds decompressionDelay, double totalDecompressionDurationUs, double totalBytesDecompressed) const
    {
        auto data = AZStd::get_if<AZ::IO::Requests::CompressedReadData>(&request->GetCommand());
        if (data)
        {
            AZStd::chrono::microseconds processingTime = decompressionDelay;
            size_t bytesToDecompress = data->m_compressionInfo.m_compressedSize;
            processingTime += AZStd::chrono::microseconds(
                static_cast<AZ::u64>((bytesToDecompress * totalDecompressionDurationUs) / totalBytesDecompressed));

            cumulativeDelay += processingTime;
            request->SetEstimatedCompletion(request->GetEstimatedCompletion() + processingTime);
        }
    }

    void DecompressorRegistrarEntry::CollectStatistics(AZStd::vector<AZ::IO::Statistic>& statistics) const
    {
        constexpr double usToSec = 1.0 / (1000.0 * 1000.0);
        constexpr double usToMs = 1.0 / 1000.0;

        if (m_bytesDecompressed.GetNumRecorded() > 1) // There's always a default added.
        {
            //It only makes sense to add decompression statistics when reading from PAK files.
            statistics.push_back(AZ::IO::Statistic::CreateInteger(
                m_name, "Available decompression slots", m_maxNumTasks - m_numRunningTasks,
                "The number of available slots to decompress files with. Increasing the number of slots will require more hardware "
                "resources and may negatively impact other cpu utilization but improves performance of Streamer."));
            statistics.push_back(AZ::IO::Statistic::CreateInteger(
                m_name, "Available read slots", m_maxNumReads - m_numInFlightReads,
                "The number of slots available to queue read requests into. Increasing this number will allow more read requests to be "
                "processed but new slots will not become available until a read file can queued in a decompression slot. Increasing this "
                "number will only be helpful if decompressing is faster than reading, otherwise the number of slots can be kept around the "
                "same number as there are decompression slots."));
            statistics.push_back(AZ::IO::Statistic::CreateInteger(
                m_name, "Pending decompression", m_numPendingDecompression,
                "The number of requests that have completed reading and are waiting for a decompression slot to become available. If this "
                "value is frequently more than zero than the number of decompression slots may need to be increased, a faster decompressor "
                "is needed or the number of read slots can be reduced."));
            statistics.push_back(AZ::IO::Statistic::CreateByteSize(
                m_name, "Buffer memory", m_memoryUsage,
                "The total amount of memory in megabytes used by the decompressor. This is dependent on the compressed file sizes and may "
                "improve by reducing the file sizes of the largest files in the archive."));

            double averageJobStartDelay = m_decompressionJobDelayMicroSec.CalculateAverage() * usToMs;
            statistics.push_back(AZ::IO::Statistic::CreateFloat(
                m_name, "Decompression job delay (avg. ms)", averageJobStartDelay,
                "The amount of time in milliseconds between queuing a decompression job and it starting. If this is too long it may "
                "indicate that the job system is too saturated to pick decompression jobs."));

            AZ::u64 totalBytesDecompressed = m_bytesDecompressed.GetTotal();
            double totalDecompressionTimeSec = m_decompressionDurationMicroSec.GetTotal() * usToSec;
            statistics.push_back(AZ::IO::Statistic::CreateBytesPerSecond(
                m_name, "Decompression Speed per job", totalBytesDecompressed / totalDecompressionTimeSec,
                "The average speed that the decompressor can handle. If this is not higher than the average read "
                "speed than decompressing can't keep up with file reads. Increasing the number of jobs can help hide this issue, but only "
                "for parallel reads, while individual reads will still remain decompression bound."));

#if AZ_STREAMER_ADD_EXTRA_PROFILING_INFO
            statistics.push_back(AZ::IO::Statistic::CreatePercentageRange(
                m_name, DecompBoundName, m_decompressionBoundStat.GetAverage(), m_decompressionBoundStat.GetMinimum(),
                m_decompressionBoundStat.GetMaximum(),
                "The percentage of time that Streamer was decompression bound. High values mean that more jobs are needed, although this "
                "may only help if there are a sufficient number of requests."));
            statistics.push_back(AZ::IO::Statistic::CreatePercentageRange(
                m_name, ReadBoundName, m_readBoundStat.GetAverage(), m_readBoundStat.GetMinimum(),
                m_readBoundStat.GetMaximum(),
                "The percentage of time that Streamer was read bound. High values are generally good if there is a sufficient number of "
                "requests."));
#endif
        }

        AZ::IO::StreamStackEntry::CollectStatistics(statistics);
    }

    bool DecompressorRegistrarEntry::IsIdle() const
    {
        return m_pendingReads.empty()
            && m_pendingFileExistChecks.empty()
            && m_numInFlightReads == 0
            && m_numPendingDecompression == 0
            && m_numRunningTasks == 0;
    }

    void DecompressorRegistrarEntry::PrepareReadRequest(AZ::IO::FileRequest* request, AZ::IO::Requests::ReadRequestData& data)
    {
        if (AZ::IO::CompressionInfo info; AZ::IO::CompressionUtils::FindCompressionInfo(info, data.m_path.GetRelativePath()))
        {
            AZ::IO::FileRequest* nextRequest = m_context->GetNewInternalRequest();
            if (info.m_isCompressed)
            {
                AZ_Assert(info.m_decompressor,
                    "DecompressorRegistrarEntry::PrepareRequest found a compressed file, but no decompressor to decompress with.");
                nextRequest->CreateCompressedRead(request, AZStd::move(info), data.m_output, data.m_offset, data.m_size);
            }
            else
            {
                AZ::IO::FileRequest* pathStorageRequest = m_context->GetNewInternalRequest();
                pathStorageRequest->CreateRequestPathStore(request, AZStd::move(info.m_archiveFilename));
                auto& pathStorage = AZStd::get<AZ::IO::Requests::RequestPathStoreData>(pathStorageRequest->GetCommand());

                nextRequest->CreateRead(pathStorageRequest, data.m_output, data.m_outputSize, pathStorage.m_path,
                    info.m_offset + data.m_offset, data.m_size, info.m_isSharedPak);
            }

            if (info.m_conflictResolution == AZ::IO::ConflictResolution::PreferFile)
            {
                auto callback = [this, nextRequest](const AZ::IO::FileRequest& checkRequest)
                {
                    auto check = AZStd::get_if<AZ::IO::Requests::FileExistsCheckData>(&checkRequest.GetCommand());
                    AZ_Assert(check,
                        "Callback in DecompressorRegistrarEntry::PrepareReadRequest expected FileExistsCheck but got another command.");
                    if (check->m_found)
                    {
                        AZ::IO::FileRequest* originalRequest = m_context->RejectRequest(nextRequest);
                        if (AZStd::holds_alternative<AZ::IO::Requests::RequestPathStoreData>(originalRequest->GetCommand()))
                        {
                            originalRequest = m_context->RejectRequest(originalRequest);
                        }
                        AZ::IO::StreamStackEntry::PrepareRequest(originalRequest);
                    }
                    else
                    {
                        m_context->PushPreparedRequest(nextRequest);
                    }
                };
                AZ::IO::FileRequest* fileCheckRequest = m_context->GetNewInternalRequest();
                fileCheckRequest->CreateFileExistsCheck(data.m_path);
                fileCheckRequest->SetCompletionCallback(AZStd::move(callback));
                AZ::IO::StreamStackEntry::QueueRequest(fileCheckRequest);
            }
            else
            {
                m_context->PushPreparedRequest(nextRequest);
            }
        }
        else
        {
            AZ::IO::StreamStackEntry::PrepareRequest(request);
        }
    }

    void DecompressorRegistrarEntry::PrepareDedicatedCache(AZ::IO::FileRequest* request, const AZ::IO::RequestPath& path)
    {
        if (AZ::IO::CompressionInfo info;
            AZ::IO::CompressionUtils::FindCompressionInfo(info, path.GetRelativePath()))
        {
            AZ::IO::FileRequest* nextRequest = m_context->GetNewInternalRequest();
            auto RunCacheCommand = [request, &info, nextRequest](auto&& args)
            {
                using Command = AZStd::decay_t<decltype(args)>;
                if constexpr (AZStd::is_same_v<Command, AZ::IO::Requests::CreateDedicatedCacheData>)
                {
                    nextRequest->CreateDedicatedCacheCreation(AZStd::move(info.m_archiveFilename),
                        AZ::IO::FileRange::CreateRange(info.m_offset, info.m_compressedSize), request);
                }
                else if constexpr (AZStd::is_same_v<Command, AZ::IO::Requests::DestroyDedicatedCacheData>)
                {
                    nextRequest->CreateDedicatedCacheDestruction(AZStd::move(info.m_archiveFilename),
                        AZ::IO::FileRange::CreateRange(info.m_offset, info.m_compressedSize), request);
                }
            };
            AZStd::visit(AZStd::move(RunCacheCommand), request->GetCommand());

            if (info.m_conflictResolution == AZ::IO::ConflictResolution::PreferFile)
            {
                auto callback = [this, nextRequest](const AZ::IO::FileRequest& checkRequest)
                {

                    auto check = AZStd::get_if<AZ::IO::Requests::FileExistsCheckData>(&checkRequest.GetCommand());
                    AZ_Assert(check,
                        "Callback in DecompressorRegistrarEntry::PrepareDedicatedCache expected FileExistsCheck but got another command.");
                    if (check->m_found)
                    {
                        AZ::IO::FileRequest* originalRequest = nextRequest->GetParent();
                        m_context->RejectRequest(nextRequest);
                        AZ::IO::StreamStackEntry::PrepareRequest(originalRequest);
                    }
                    else
                    {
                        m_context->PushPreparedRequest(nextRequest);
                    }
                };
                AZ::IO::FileRequest* fileCheckRequest = m_context->GetNewInternalRequest();
                fileCheckRequest->CreateFileExistsCheck(path);
                fileCheckRequest->SetCompletionCallback(AZStd::move(callback));
                AZ::IO::StreamStackEntry::QueueRequest(fileCheckRequest);
            }
            else
            {
                m_context->PushPreparedRequest(nextRequest);
            }
        }
        else
        {
            AZ::IO::StreamStackEntry::PrepareRequest(request);
        }
    }

    void DecompressorRegistrarEntry::FileExistsCheck(AZ::IO::FileRequest* checkRequest)
    {
        auto& fileCheckRequest = AZStd::get<AZ::IO::Requests::FileExistsCheckData>(checkRequest->GetCommand());
        AZ::IO::CompressionInfo info;
        if (AZ::IO::CompressionUtils::FindCompressionInfo(info, fileCheckRequest.m_path.GetRelativePath()))
        {
            fileCheckRequest.m_found = true;
        }
        else
        {
            // The file isn't in the archive but might still exist as a loose file, so let the next node have a shot.
            AZ::IO::StreamStackEntry::QueueRequest(checkRequest);
        }
    }

    void DecompressorRegistrarEntry::StartArchiveRead(AZ::IO::FileRequest* compressedReadRequest)
    {
        if (!m_next)
        {
            compressedReadRequest->SetStatus(AZ::IO::IStreamerTypes::RequestStatus::Failed);
            m_context->MarkRequestAsCompleted(compressedReadRequest);
            return;
        }

        for (AZ::u32 i = 0; i < m_maxNumReads; ++i)
        {
            if (m_readBufferStatus[i] == ReadBufferStatus::Unused)
            {
                auto data = AZStd::get_if<AZ::IO::Requests::CompressedReadData>(&compressedReadRequest->GetCommand());
                AZ_Assert(data, "Compressed request that's starting a read in DecompressorRegistrarEntry didn't contain compression read data.");
                AZ_Assert(data->m_compressionInfo.m_decompressor,
                    "FileRequest for DecompressorRegistrarEntry is missing a decompression callback.");

                AZ::IO::CompressionInfo& info = data->m_compressionInfo;
                AZ_Assert(info.m_decompressor, "DecompressorRegistrarEntry is planning to a queue a request for reading but couldn't find a decompressor.");

                // The buffer is aligned down but the offset is not corrected. If the offset was adjusted it would mean the same data is read
                // multiple times and negates the block cache's ability to detect these cases. By still adjusting it means that the reads between
                // the BlockCache's prolog and epilog are read into aligned buffers.
                size_t offsetAdjustment = info.m_offset - AZ_SIZE_ALIGN_DOWN(info.m_offset, aznumeric_cast<size_t>(m_alignment));
                size_t bufferSize = AZ_SIZE_ALIGN_UP((info.m_compressedSize + offsetAdjustment), aznumeric_cast<size_t>(m_alignment));
                m_readBuffers[i] = reinterpret_cast<Buffer>(AZ::AllocatorInstance<AZ::SystemAllocator>::Get().Allocate(
                    bufferSize, m_alignment));
                m_memoryUsage += bufferSize;

                AZ::IO::FileRequest* archiveReadRequest = m_context->GetNewInternalRequest();
                archiveReadRequest->CreateRead(compressedReadRequest, m_readBuffers[i] + offsetAdjustment, bufferSize, info.m_archiveFilename,
                    info.m_offset, info.m_compressedSize, info.m_isSharedPak);

                auto ArchiveReadCommandComplete = [this, readSlot = i](AZ::IO::FileRequest& request)
                {
                    FinishArchiveRead(&request, readSlot);
                };
                archiveReadRequest->SetCompletionCallback(AZStd::move(ArchiveReadCommandComplete));
                m_next->QueueRequest(archiveReadRequest);

                m_readRequests[i] = archiveReadRequest;
                m_readBufferStatus[i] = ReadBufferStatus::ReadInFlight;

                AZ_Assert(m_numInFlightReads < m_maxNumReads,
                    "A FileRequest was queued for reading in DecompressorRegistrarEntry, but there's no slots available.");
                m_numInFlightReads++;

                return;
            }
        }
        AZ_Assert(false, "%u of %u read slots are used in the DecompressorRegistrarEntry, but no empty slot was found.", m_numInFlightReads, m_maxNumReads);
    }

    void DecompressorRegistrarEntry::FinishArchiveRead(AZ::IO::FileRequest* readRequest, AZ::u32 readSlot)
    {
        AZ_Assert(m_readRequests[readSlot] == readRequest,
            "Request in the archive read slot isn't the same as request that's being completed.");

        AZ::IO::FileRequest* compressedRequest = readRequest->GetParent();
        AZ_Assert(compressedRequest, "Read requests started by DecompressorRegistrarEntry is missing a parent request.");

        if (readRequest->GetStatus() == AZ::IO::IStreamerTypes::RequestStatus::Completed)
        {
            m_readBufferStatus[readSlot] = ReadBufferStatus::PendingDecompression;
            ++m_numPendingDecompression;

            // Add this wait so the compressed request isn't fully completed yet as only the read part is done. The
            // job thread will finish this wait, which in turn will trigger this function again on the main streaming thread.
            AZ::IO::FileRequest* waitRequest = m_context->GetNewInternalRequest();
            waitRequest->CreateWait(compressedRequest);
            m_readRequests[readSlot] = waitRequest;
        }
        else
        {
            auto data = AZStd::get_if<AZ::IO::Requests::CompressedReadData>(&compressedRequest->GetCommand());
            AZ_Assert(data, "Compressed request in DecompressorRegistrarEntry that finished unsuccessfully didn't contain compression read data.");
            AZ::IO::CompressionInfo& info = data->m_compressionInfo;
            size_t offsetAdjustment = info.m_offset - AZ_SIZE_ALIGN_DOWN(info.m_offset, aznumeric_cast<size_t>(m_alignment));
            size_t bufferSize = AZ_SIZE_ALIGN_UP((info.m_compressedSize + offsetAdjustment), aznumeric_cast<size_t>(m_alignment));
            m_memoryUsage -= bufferSize;

            if (m_readBuffers[readSlot] != nullptr)
            {
                AZ::AllocatorInstance<AZ::SystemAllocator>::Get().DeAllocate(m_readBuffers[readSlot], bufferSize, m_alignment);
                m_readBuffers[readSlot] = nullptr;
            }
            m_readRequests[readSlot] = nullptr;
            m_readBufferStatus[readSlot] = ReadBufferStatus::Unused;
            AZ_Assert(m_numInFlightReads > 0,
                "Trying to decrement a read request after it was canceled or failed in DecompressorRegistrarEntry, "
                "but no read requests are supposed to be queued.");
            m_numInFlightReads--;
        }
    }

    bool DecompressorRegistrarEntry::StartDecompressions()
    {
        bool submittedTask = false;
        for (AZ::u32 readSlot = 0; readSlot < m_maxNumReads; ++readSlot)
        {
            // Find completed read.
            if (m_readBufferStatus[readSlot] != ReadBufferStatus::PendingDecompression)
            {
                continue;
            }

            auto DecompressionCompleteTask = []()
            {
                AZ_Trace("Decompression Registrar Streamer", "All current decompression task are complete");
            };

            AZ::TaskGraph taskGraph{ "Decompression Tasks" };
            AZ::TaskToken finishToken = taskGraph.AddTask(
                AZ::TaskDescriptor{"Decompress Gather All", "Compression"}, AZStd::move(DecompressionCompleteTask));

            // Find decompression slot
            for (size_t decompressionSlotIndex = 0; decompressionSlotIndex < m_maxNumTasks; ++decompressionSlotIndex)
            {
                if (m_processingJobs[decompressionSlotIndex].IsProcessing())
                {
                    continue;
                }

                AZ::IO::FileRequest* waitRequest = m_readRequests[readSlot];
                AZ_Assert(AZStd::holds_alternative<AZ::IO::Requests::WaitData>(waitRequest->GetCommand()),
                    "File request waiting for decompression wasn't marked as being a wait operation.");
                AZ::IO::FileRequest* compressedRequest = waitRequest->GetParent();
                AZ_Assert(compressedRequest, "Read requests started by DecompressorRegistrarEntry is missing a parent request.");

                auto DecompressionRequestFinishedCB = [this, taskSlot = AZ::u32(decompressionSlotIndex)](AZ::IO::FileRequest& request)
                {
                    FinishDecompression(&request, taskSlot);
                };
                waitRequest->SetCompletionCallback(AZStd::move(DecompressionRequestFinishedCB));

                DecompressionInformation& info = m_processingJobs[decompressionSlotIndex];
                info.m_waitRequest = waitRequest;
                info.m_queueStartTime = AZStd::chrono::steady_clock::now();
                info.m_jobStartTime = info.m_queueStartTime; // Set these to the same in case the scheduler requests an update before the job has started.
                info.m_compressedData = m_readBuffers[readSlot]; // Transfer ownership of the pointer.
                m_readBuffers[readSlot] = nullptr;

                if (m_taskGraphEvent == nullptr || m_taskGraphEvent->IsSignaled())
                {
                    m_taskGraphEvent = AZStd::make_unique<AZ::TaskGraphEvent>("Decompressor Registrar Wait");

                    auto data = AZStd::get_if<AZ::IO::Requests::CompressedReadData>(&compressedRequest->GetCommand());
                    AZ_Assert(data, "Compressed request in DecompressorRegistrarEntry that's starting decompression didn't contain compression read data.");
                    AZ_Assert(data->m_compressionInfo.m_decompressor, "DecompressorRegistrarEntry is queuing a decompression job but couldn't find a decompressor.");

                    info.m_alignmentOffset = aznumeric_caster(data->m_compressionInfo.m_offset -
                        AZ_SIZE_ALIGN_DOWN(data->m_compressionInfo.m_offset, aznumeric_cast<size_t>(m_alignment)));

                    AZ::TaskDescriptor taskDescriptor{ "Decompress file", "Compression" };
                    if (data->m_readOffset == 0 && data->m_readSize == data->m_compressionInfo.m_uncompressedSize)
                    {
                        auto decompressTask = [this, &info]()
                        {
                            FullDecompression(m_context, info);
                        };
                        AZ::TaskToken token = taskGraph.AddTask(taskDescriptor, AZStd::move(decompressTask));
                        token.Precedes(finishToken);
                    }
                    else
                    {
                        m_memoryUsage += data->m_compressionInfo.m_uncompressedSize;
                        auto decompressTask = [this, &info]()
                        {
                            PartialDecompression(m_context, info);
                        };

                        AZ::TaskToken token = taskGraph.AddTask(taskDescriptor, AZStd::move(decompressTask));
                        token.Precedes(finishToken);
                    }

                    --m_numPendingDecompression;
                    ++m_numRunningTasks;
                }

                AZ_Assert(m_taskGraphEvent->IsSignaled() == false, "Decompression has been started on another thread"
                    " while executing this function");
                taskGraph.SubmitOnExecutor(m_taskExecutor, m_taskGraphEvent.get());

                m_readRequests[readSlot] = nullptr;
                m_readBufferStatus[readSlot] = ReadBufferStatus::Unused;
                AZ_Assert(m_numInFlightReads > 0, "Trying to decrement a read request after it's queued for decompression in DecompressorRegistrarEntry, but no read requests are supposed to be queued.");
                m_numInFlightReads--;

                submittedTask = true;
                break;
            }

            if (m_numInFlightReads == 0 || m_numRunningTasks == m_maxNumTasks)
            {
                return submittedTask;
            }
        }
        return submittedTask;
    }

    void DecompressorRegistrarEntry::FinishDecompression([[maybe_unused]] AZ::IO::FileRequest* waitRequest, AZ::u32 jobSlot)
    {
        DecompressionInformation& jobInfo = m_processingJobs[jobSlot];
        AZ_Assert(jobInfo.m_waitRequest == waitRequest, "Job slot didn't contain the expected wait request.");

        auto endTime = AZStd::chrono::steady_clock::now();

        AZ::IO::FileRequest* compressedRequest = jobInfo.m_waitRequest->GetParent();
        AZ_Assert(compressedRequest, "A wait request attached to DecompressorRegistrarEntry was completed but didn't have a parent compressed request.");
        auto data = AZStd::get_if<AZ::IO::Requests::CompressedReadData>(&compressedRequest->GetCommand());
        AZ_Assert(data, "Compressed request in DecompressorRegistrarEntry that completed decompression didn't contain compression read data.");
        AZ::IO::CompressionInfo& info = data->m_compressionInfo;
        size_t offsetAdjustment = info.m_offset - AZ_SIZE_ALIGN_DOWN(info.m_offset, aznumeric_cast<size_t>(m_alignment));
        size_t bufferSize = AZ_SIZE_ALIGN_UP((info.m_compressedSize + offsetAdjustment), aznumeric_cast<size_t>(m_alignment));
        m_memoryUsage -= bufferSize;
        if (data->m_readOffset != 0 || data->m_readSize != data->m_compressionInfo.m_uncompressedSize)
        {
            m_memoryUsage -= data->m_compressionInfo.m_uncompressedSize;
        }

        m_decompressionJobDelayMicroSec.PushEntry(AZStd::chrono::duration_cast<AZStd::chrono::microseconds>(
            jobInfo.m_jobStartTime - jobInfo.m_queueStartTime).count());
        m_decompressionDurationMicroSec.PushEntry(AZStd::chrono::duration_cast<AZStd::chrono::microseconds>(
            endTime - jobInfo.m_jobStartTime).count());
        m_bytesDecompressed.PushEntry(data->m_compressionInfo.m_compressedSize);

        AZ::AllocatorInstance<AZ::SystemAllocator>::Get().DeAllocate(jobInfo.m_compressedData, bufferSize, m_alignment);
        jobInfo.m_compressedData = nullptr;
        AZ_Assert(m_numRunningTasks > 0, "About to complete a decompression job, but the internal count doesn't see a running job.");
        --m_numRunningTasks;
        return;
    }

    void DecompressorRegistrarEntry::FullDecompression(AZ::IO::StreamerContext* context, DecompressionInformation& info)
    {
        info.m_jobStartTime = AZStd::chrono::steady_clock::now();

        AZ::IO::FileRequest* compressedRequest = info.m_waitRequest->GetParent();
        AZ_Assert(compressedRequest, "A wait request attached to DecompressorRegistrarEntry was completed but didn't have a parent compressed request.");
        auto request = AZStd::get_if<AZ::IO::Requests::CompressedReadData>(&compressedRequest->GetCommand());
        AZ_Assert(request, "Compressed request in DecompressorRegistrarEntry that's running full decompression didn't contain compression read data.");
        AZ::IO::CompressionInfo& compressionInfo = request->m_compressionInfo;
        AZ_Assert(compressionInfo.m_decompressor, "Full decompressor job started, but there's no decompressor callback assigned.");

        AZ_Assert(request->m_readOffset == 0, "DecompressorRegistrarEntry is doing a full decompression on a file request with an offset (%zu).",
            request->m_readOffset);
        AZ_Assert(compressionInfo.m_uncompressedSize == request->m_readSize,
            "DecompressorRegistrarEntry is doing a full decompression, but the target buffer size (%llu) doesn't match the decompressed size (%zu).",
            request->m_readSize, compressionInfo.m_uncompressedSize);

        bool success = compressionInfo.m_decompressor(compressionInfo, info.m_compressedData + info.m_alignmentOffset,
            compressionInfo.m_compressedSize, request->m_output, compressionInfo.m_uncompressedSize);
        info.m_waitRequest->SetStatus(success ? AZ::IO::IStreamerTypes::RequestStatus::Completed : AZ::IO::IStreamerTypes::RequestStatus::Failed);

        context->MarkRequestAsCompleted(info.m_waitRequest);
        context->WakeUpSchedulingThread();
    }

    void DecompressorRegistrarEntry::PartialDecompression(AZ::IO::StreamerContext* context, DecompressionInformation& info)
    {
        info.m_jobStartTime = AZStd::chrono::steady_clock::now();

        AZ::IO::FileRequest* compressedRequest = info.m_waitRequest->GetParent();
        AZ_Assert(compressedRequest, "A wait request attached to DecompressorRegistrarEntry was completed but didn't have a parent compressed request.");
        auto request = AZStd::get_if<AZ::IO::Requests::CompressedReadData>(&compressedRequest->GetCommand());
        AZ_Assert(request, "Compressed request in DecompressorRegistrarEntry that's running partial decompression didn't contain compression read data.");
        AZ::IO::CompressionInfo& compressionInfo = request->m_compressionInfo;
        AZ_Assert(compressionInfo.m_decompressor, "Partial decompressor job started, but there's no decompressor callback assigned.");

        auto decompressionBuffer = AZStd::make_unique<AZStd::byte[]>(compressionInfo.m_uncompressedSize);
        bool success = compressionInfo.m_decompressor(compressionInfo, info.m_compressedData + info.m_alignmentOffset,
            compressionInfo.m_compressedSize, decompressionBuffer.get(), compressionInfo.m_uncompressedSize);
        info.m_waitRequest->SetStatus(success ? AZ::IO::IStreamerTypes::RequestStatus::Completed : AZ::IO::IStreamerTypes::RequestStatus::Failed);

        memcpy(request->m_output, decompressionBuffer.get() + request->m_readOffset, request->m_readSize);

        context->MarkRequestAsCompleted(info.m_waitRequest);
        context->WakeUpSchedulingThread();
    }

    void DecompressorRegistrarEntry::Report(const AZ::IO::Requests::ReportData& data) const
    {
        switch (data.m_reportType)
        {
        case AZ::IO::IStreamerTypes::ReportType::Config:
            data.m_output.push_back(AZ::IO::Statistic::CreateInteger(
                m_name, "Max number of reads", m_maxNumReads, "The maximum number of parallel reads this decompressor node will support."));
            data.m_output.push_back(AZ::IO::Statistic::CreateInteger(
                m_name, "Max number of jobs", m_maxNumTasks,
                "The maximum number of decompression jobs that can run in parallel. A thread per job will be used. A dedicated job system "
                "is used as not to interfere with the regular job/task system, but this does add additional thread scheduling work to the "
                "operating system and may impact how stable the performance on the rest of the engine is. If there are functions that "
                "periodically take much longer, look for excessive context switches by the operating systems and if found lowering this "
                "value may help reduce those at the cost or streaming speeds."));
            data.m_output.push_back(AZ::IO::Statistic::CreateByteSize(
                m_name, "Alignment", m_alignment,
                "The alignment for read buffer. This allows enough memory to be reserved in the read buffer to allow for alignment to "
                "happen by later nodes without requiring additional temporary buffers. This does not adjust the offset or read size in "
                "order to allow cache nodes to remain effective."));
            data.m_output.push_back(AZ::IO::Statistic::CreateReferenceString(
                m_name, "Next node", m_next ? AZStd::string_view(m_next->GetName()) : AZStd::string_view("<None>"),
                "The name of the node that follows this node or none."));
            break;
        };
    }
} // namespace AZ::IO
