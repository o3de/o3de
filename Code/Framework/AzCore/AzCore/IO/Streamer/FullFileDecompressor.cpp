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
#include <AzCore/IO/Streamer/FullFileDecompressor.h>
#include <AzCore/IO/Streamer/StreamerContext.h>
#include <AzCore/Jobs/JobContext.h>
#include <AzCore/Jobs/JobFunction.h>
#include <AzCore/Jobs/JobManager.h>
#include <AzCore/Math/MathUtils.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/parallel/thread.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/std/typetraits/decay.h>

namespace AZ::IO
{
    AZStd::shared_ptr<StreamStackEntry> FullFileDecompressorConfig::AddStreamStackEntry(
        const HardwareInformation& hardware, AZStd::shared_ptr<StreamStackEntry> parent)
    {
        auto stackEntry = AZStd::make_shared<FullFileDecompressor>(
            m_maxNumReads, m_maxNumJobs, aznumeric_caster(hardware.m_maxPhysicalSectorSize));
        stackEntry->SetNext(AZStd::move(parent));
        return stackEntry;
    }

    void FullFileDecompressorConfig::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context); serializeContext != nullptr)
        {
            serializeContext->Class<FullFileDecompressorConfig, IStreamerStackConfig>()
                ->Version(1)
                ->Field("MaxNumReads", &FullFileDecompressorConfig::m_maxNumReads)
                ->Field("MaxNumJobs", &FullFileDecompressorConfig::m_maxNumJobs);
        }
    }

#if AZ_STREAMER_ADD_EXTRA_PROFILING_INFO
    static constexpr char DecompBoundName[] = "Decompression bound";
    static constexpr char ReadBoundName[] = "Read bound";
#endif // AZ_STREAMER_ADD_EXTRA_PROFILING_INFO

    bool FullFileDecompressor::DecompressionInformation::IsProcessing() const
    {
        return !!m_compressedData;
    }

    FullFileDecompressor::FullFileDecompressor(u32 maxNumReads, u32 maxNumJobs, u32 alignment)
        : StreamStackEntry("Full file decompressor")
        , m_maxNumReads(maxNumReads)
        , m_maxNumJobs(maxNumJobs)
        , m_alignment(alignment)
    {
        JobManagerDesc jobDesc;
            jobDesc.m_jobManagerName = "Full File Decompressor";
        u32 numThreads = AZ::GetMin(maxNumJobs, AZStd::thread::hardware_concurrency());
        for (u32 i = 0; i < numThreads; ++i)
        {
            jobDesc.m_workerThreads.push_back(JobManagerThreadDesc());
        }
        m_decompressionJobManager = AZStd::make_unique<JobManager>(jobDesc);
        m_decompressionjobContext = AZStd::make_unique<JobContext>(*m_decompressionJobManager);

        m_processingJobs = AZStd::make_unique<DecompressionInformation[]>(maxNumJobs);

        m_readBuffers = AZStd::make_unique<Buffer[]>(maxNumReads);
        m_readRequests = AZStd::make_unique<FileRequest*[]>(maxNumReads);
        m_readBufferStatus = AZStd::make_unique<ReadBufferStatus[]>(maxNumReads);
        for (u32 i = 0; i < maxNumReads; ++i)
        {
            m_readBufferStatus[i] = ReadBufferStatus::Unused;
        }

        // Add initial dummy values to the stats to avoid division by zero later on and avoid needing branches.
        m_bytesDecompressed.PushEntry(1);
        m_decompressionDurationMicroSec.PushEntry(1);
    }

    void FullFileDecompressor::PrepareRequest(FileRequest* request)
    {
        AZ_Assert(request, "PrepareRequest was provided a null request.");

        AZStd::visit([this, request](auto&& args)
        {
            using Command = AZStd::decay_t<decltype(args)>;
            if constexpr (AZStd::is_same_v<Command, FileRequest::ReadRequestData>)
            {
                PrepareReadRequest(request, args);
            }
            else if constexpr (AZStd::is_same_v<Command, FileRequest::CreateDedicatedCacheData> ||
                AZStd::is_same_v<Command, FileRequest::DestroyDedicatedCacheData>)
            {
                PrepareDedicatedCache(request, args.m_path);
            }
            else
            {
                StreamStackEntry::PrepareRequest(request);
            }
        }, request->GetCommand());
    }

    void FullFileDecompressor::QueueRequest(FileRequest* request)
    {
        AZ_Assert(request, "QueueRequest was provided a null request.");

        AZStd::visit([this, request](auto&& args)
        {
            using Command = AZStd::decay_t<decltype(args)>;
            if constexpr (AZStd::is_same_v<Command, FileRequest::CompressedReadData>)
            {
                m_pendingReads.push_back(request);
            }
            else if constexpr (AZStd::is_same_v<Command, FileRequest::FileExistsCheckData>)
            {
                m_pendingFileExistChecks.push_back(request);
            }
            else
            {
                StreamStackEntry::QueueRequest(request);
            }
        }, request->GetCommand());
    }

    bool FullFileDecompressor::ExecuteRequests()
    {
        bool result = false;
        // First queue jobs as this might open up new read slots.
        if (m_numInFlightReads > 0 && m_numRunningJobs < m_maxNumJobs)
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
        for (u32 i = 0; i < m_maxNumReads; ++i)
        {
            allPendingDecompression =
                allPendingDecompression && (m_readBufferStatus[i] == ReadBufferStatus::PendingDecompression);
            allReading =
                allReading && (m_readBufferStatus[i] == ReadBufferStatus::ReadInFlight);
        }

        m_decompressionBoundStat.PushSample(allPendingDecompression ? 1.0 : 0.0);
        Statistic::PlotImmediate(m_name, DecompBoundName, m_decompressionBoundStat.GetMostRecentSample());

        m_readBoundStat.PushSample(allReading && (m_numRunningJobs < m_maxNumJobs) ? 1.0 : 0.0);
        Statistic::PlotImmediate(m_name, ReadBoundName, m_readBoundStat.GetMostRecentSample());
#endif

        return StreamStackEntry::ExecuteRequests() || result;
    }

    void FullFileDecompressor::UpdateStatus(Status& status) const
    {
        StreamStackEntry::UpdateStatus(status);
        s32 numAvailableSlots = aznumeric_cast<s32>(m_maxNumReads - m_numInFlightReads);
        status.m_numAvailableSlots = AZStd::min(status.m_numAvailableSlots, numAvailableSlots);
        status.m_isIdle = status.m_isIdle && IsIdle();
    }

    void FullFileDecompressor::UpdateCompletionEstimates(AZStd::chrono::system_clock::time_point now, AZStd::vector<FileRequest*>& internalPending,
        StreamerContext::PreparedQueue::iterator pendingBegin, StreamerContext::PreparedQueue::iterator pendingEnd)
    {
        // Create predictions for all pending requests. Some will be further processed after this.
        AZStd::reverse_copy(m_pendingFileExistChecks.begin(), m_pendingFileExistChecks.end(), AZStd::back_inserter(internalPending));
        AZStd::reverse_copy(m_pendingReads.begin(), m_pendingReads.end(), AZStd::back_inserter(internalPending));

        StreamStackEntry::UpdateCompletionEstimates(now, internalPending, pendingBegin, pendingEnd);

        double totalBytesDecompressed = aznumeric_caster(m_bytesDecompressed.GetTotal());
        double totalDecompressionDuration = aznumeric_caster(m_decompressionDurationMicroSec.GetTotal());
        AZStd::chrono::microseconds cumulativeDelay = AZStd::chrono::microseconds::max();

        // Check the number of jobs that are processing.
        for (u32 i = 0; i < m_maxNumJobs; ++i)
        {
            if (m_processingJobs[i].IsProcessing())
            {
                FileRequest* compressedRequest = m_processingJobs[i].m_waitRequest->GetParent();
                AZ_Assert(compressedRequest, "A wait request attached to FullFileDecompressor was completed but didn't have a parent compressed request.");
                auto data = AZStd::get_if<FileRequest::CompressedReadData>(&compressedRequest->GetCommand());
                AZ_Assert(data, "Compressed request in the decompression queue in FullFileDecompressor didn't contain compression read data.");

                size_t bytesToDecompress = data->m_compressionInfo.m_compressedSize;
                auto decompressionDuration = AZStd::chrono::microseconds(
                    aznumeric_cast<u64>((bytesToDecompress * totalDecompressionDuration) / totalBytesDecompressed));
                auto timeInProcessing = now - m_processingJobs[i].m_jobStartTime;
                auto timeLeft = decompressionDuration > timeInProcessing ? decompressionDuration - timeInProcessing : AZStd::chrono::microseconds(0);
                // Get the shortest time as this indicates the next decompression to become available.
                cumulativeDelay = AZStd::min(timeLeft, cumulativeDelay);
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
            AZStd::chrono::microseconds(aznumeric_cast<u64>(m_decompressionJobDelayMicroSec.CalculateAverage()));
        AZStd::chrono::microseconds smallestDecompressionDuration = AZStd::chrono::microseconds::max();
        for (u32 i = 0; i < m_maxNumReads; ++i)
        {
            AZStd::chrono::system_clock::time_point baseTime;
            switch (m_readBufferStatus[i])
            {
            case ReadBufferStatus::Unused:
                continue;
            case ReadBufferStatus::ReadInFlight:
                // Internal read requests can start and complete but pending finalization before they're ever scheduled in which case
                // the estimated time is not set.
                baseTime = m_readRequests[i]->GetEstimatedCompletion();
                if (baseTime == AZStd::chrono::system_clock::time_point())
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
            FileRequest* compressedRequest = m_readRequests[i]->GetParent();
            auto data = AZStd::get_if<FileRequest::CompressedReadData>(&compressedRequest->GetCommand());

            size_t bytesToDecompress = data->m_compressionInfo.m_compressedSize;
            auto decompressionDuration = AZStd::chrono::microseconds(
                aznumeric_cast<u64>((bytesToDecompress * totalDecompressionDuration) / totalBytesDecompressed));
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

    void FullFileDecompressor::EstimateCompressedReadRequest(FileRequest* request, AZStd::chrono::microseconds& cumulativeDelay,
        AZStd::chrono::microseconds decompressionDelay, double totalDecompressionDurationUs, double totalBytesDecompressed) const
    {
        auto data = AZStd::get_if<FileRequest::CompressedReadData>(&request->GetCommand());
        if (data)
        {
            AZStd::chrono::microseconds processingTime = decompressionDelay;
            size_t bytesToDecompress = data->m_compressionInfo.m_compressedSize;
            processingTime += AZStd::chrono::microseconds(
                aznumeric_cast<u64>((bytesToDecompress * totalDecompressionDurationUs) / totalBytesDecompressed));

            cumulativeDelay += processingTime;
            request->SetEstimatedCompletion(request->GetEstimatedCompletion() + processingTime);
        }
    }

    void FullFileDecompressor::CollectStatistics(AZStd::vector<Statistic>& statistics) const
    {
        constexpr double bytesToMB = 1.0 / (1024.0 * 1024.0);
        constexpr double usToSec = 1.0 / (1000.0 * 1000.0);
        constexpr double usToMs = 1.0 / 1000.0;

        if (m_bytesDecompressed.GetNumRecorded() > 1) // There's always a default added.
        {
            //It only makes sense to add decompression statistics when reading from PAK files.
            statistics.push_back(Statistic::CreateInteger(m_name, "Available decompression slots", m_maxNumJobs - m_numRunningJobs));
            statistics.push_back(Statistic::CreateInteger(m_name, "Available read slots", m_maxNumReads - m_numInFlightReads));
            statistics.push_back(Statistic::CreateInteger(m_name, "Pending decompression", m_numPendingDecompression));
            statistics.push_back(Statistic::CreateFloat(m_name, "Buffer memory (MB)", m_memoryUsage * bytesToMB));

            double averageJobStartDelay = m_decompressionJobDelayMicroSec.CalculateAverage() * usToMs;
            statistics.push_back(Statistic::CreateFloat(m_name, "Decompression job delay (avg. ms)", averageJobStartDelay));

            double totalBytesDecompressedMB = m_bytesDecompressed.GetTotal() * bytesToMB;
            double totalDecompressionTimeSec = m_decompressionDurationMicroSec.GetTotal() * usToSec;
            statistics.push_back(Statistic::CreateFloat(m_name, "Decompression Speed per job (avg. mbps)", totalBytesDecompressedMB / totalDecompressionTimeSec));

#if AZ_STREAMER_ADD_EXTRA_PROFILING_INFO
            statistics.push_back(Statistic::CreatePercentage(m_name, DecompBoundName, m_decompressionBoundStat.GetAverage()));
            statistics.push_back(Statistic::CreatePercentage(m_name, ReadBoundName, m_readBoundStat.GetAverage()));
#endif
        }

        StreamStackEntry::CollectStatistics(statistics);
    }

    bool FullFileDecompressor::IsIdle() const
    {
        return
            m_pendingReads.empty() &&
            m_pendingFileExistChecks.empty() &&
            m_numInFlightReads == 0 &&
            m_numPendingDecompression == 0 &&
            m_numRunningJobs == 0;
    }

    void FullFileDecompressor::PrepareReadRequest(FileRequest* request, FileRequest::ReadRequestData& data)
    {
        CompressionInfo info;
        if (CompressionUtils::FindCompressionInfo(info, data.m_path.GetRelativePath()))
        {
            FileRequest* nextRequest = m_context->GetNewInternalRequest();
            if (info.m_isCompressed)
            {
                AZ_Assert(info.m_decompressor,
                    "FullFileDecompressor::PrepareRequest found a compressed file, but no decompressor to decompress with.");
                nextRequest->CreateCompressedRead(request, AZStd::move(info), data.m_output, data.m_offset, data.m_size);
            }
            else
            {
                FileRequest* pathStorageRequest = m_context->GetNewInternalRequest();
                pathStorageRequest->CreateRequestPathStore(request, AZStd::move(info.m_archiveFilename));
                auto& pathStorage = AZStd::get<FileRequest::RequestPathStoreData>(pathStorageRequest->GetCommand());

                nextRequest->CreateRead(pathStorageRequest, data.m_output, data.m_outputSize, pathStorage.m_path,
                    info.m_offset + data.m_offset, data.m_size, info.m_isSharedPak);
            }

            if (info.m_conflictResolution == ConflictResolution::PreferFile)
            {
                auto callback = [this, nextRequest](const FileRequest& checkRequest)
                {
                    AZ_PROFILE_FUNCTION(AzCore);
                    auto check = AZStd::get_if<FileRequest::FileExistsCheckData>(&checkRequest.GetCommand());
                    AZ_Assert(check,
                        "Callback in FullFileDecompressor::PrepareReadRequest expected FileExistsCheck but got another command.");
                    if (check->m_found)
                    {
                        FileRequest* originalRequest = m_context->RejectRequest(nextRequest);
                        if (AZStd::holds_alternative<FileRequest::RequestPathStoreData>(originalRequest->GetCommand()))
                        {
                            originalRequest = m_context->RejectRequest(originalRequest);
                        }
                        StreamStackEntry::PrepareRequest(originalRequest);
                    }
                    else
                    {
                        m_context->PushPreparedRequest(nextRequest);
                    }
                };
                FileRequest* fileCheckRequest = m_context->GetNewInternalRequest();
                fileCheckRequest->CreateFileExistsCheck(data.m_path);
                fileCheckRequest->SetCompletionCallback(AZStd::move(callback));
                StreamStackEntry::QueueRequest(fileCheckRequest);
            }
            else
            {
                m_context->PushPreparedRequest(nextRequest);
            }
        }
        else
        {
            StreamStackEntry::PrepareRequest(request);
        }
    }

    void FullFileDecompressor::PrepareDedicatedCache(FileRequest* request, const RequestPath& path)
    {
        CompressionInfo info;
        if (CompressionUtils::FindCompressionInfo(info, path.GetRelativePath()))
        {
            FileRequest* nextRequest = m_context->GetNewInternalRequest();
            AZStd::visit([request, &info, nextRequest](auto&& args)
            {
                using Command = AZStd::decay_t<decltype(args)>;
                if constexpr (AZStd::is_same_v<Command, FileRequest::CreateDedicatedCacheData>)
                {
                    nextRequest->CreateDedicatedCacheCreation(AZStd::move(info.m_archiveFilename),
                        FileRange::CreateRange(info.m_offset, info.m_compressedSize), request);
                }
                else if constexpr (AZStd::is_same_v<Command, FileRequest::DestroyDedicatedCacheData>)
                {
                    nextRequest->CreateDedicatedCacheDestruction(AZStd::move(info.m_archiveFilename),
                        FileRange::CreateRange(info.m_offset, info.m_compressedSize), request);
                }
            }, request->GetCommand());

            if (info.m_conflictResolution == ConflictResolution::PreferFile)
            {
                auto callback = [this, nextRequest](const FileRequest& checkRequest)
                {
                    AZ_PROFILE_FUNCTION(AzCore);
                    auto check = AZStd::get_if<FileRequest::FileExistsCheckData>(&checkRequest.GetCommand());
                    AZ_Assert(check,
                        "Callback in FullFileDecompressor::PrepareDedicatedCache expected FileExistsCheck but got another command.");
                    if (check->m_found)
                    {
                        FileRequest* originalRequest = nextRequest->GetParent();
                        m_context->RejectRequest(nextRequest);
                        StreamStackEntry::PrepareRequest(originalRequest);
                    }
                    else
                    {
                        m_context->PushPreparedRequest(nextRequest);
                    }
                };
                FileRequest* fileCheckRequest = m_context->GetNewInternalRequest();
                fileCheckRequest->CreateFileExistsCheck(path);
                fileCheckRequest->SetCompletionCallback(AZStd::move(callback));
                StreamStackEntry::QueueRequest(fileCheckRequest);
            }
            else
            {
                m_context->PushPreparedRequest(nextRequest);
            }
        }
        else
        {
            StreamStackEntry::PrepareRequest(request);
        }
    }

    void FullFileDecompressor::FileExistsCheck(FileRequest* checkRequest)
    {
        auto& fileCheckRequest = AZStd::get<FileRequest::FileExistsCheckData>(checkRequest->GetCommand());
        CompressionInfo info;
        if (CompressionUtils::FindCompressionInfo(info, fileCheckRequest.m_path.GetRelativePath()))
        {
            fileCheckRequest.m_found = true;
        }
        else
        {
            // The file isn't in the archive but might still exist as a loose file, so let the next node have a shot.
            StreamStackEntry::QueueRequest(checkRequest);
        }
    }

    void FullFileDecompressor::StartArchiveRead(FileRequest* compressedReadRequest)
    {
        if (!m_next)
        {
            compressedReadRequest->SetStatus(IStreamerTypes::RequestStatus::Failed);
            m_context->MarkRequestAsCompleted(compressedReadRequest);
            return;
        }

        for (u32 i = 0; i < m_maxNumReads; ++i)
        {
            if (m_readBufferStatus[i] == ReadBufferStatus::Unused)
            {
                auto data = AZStd::get_if<FileRequest::CompressedReadData>(&compressedReadRequest->GetCommand());
                AZ_Assert(data, "Compressed request that's starting a read in FullFileDecompressor didn't contain compression read data.");
                AZ_Assert(data->m_compressionInfo.m_decompressor,
                    "FileRequest for FullFileDecompressor is missing a decompression callback.");

                CompressionInfo& info = data->m_compressionInfo;
                AZ_Assert(info.m_decompressor, "FullFileDecompressor is planning to a queue a request for reading but couldn't find a decompressor.");

                // The buffer is aligned down but the offset is not corrected. If the offset was adjusted it would mean the same data is read
                // multiple times and negates the block cache's ability to detect these cases. By still adjusting it means that the reads between
                // the BlockCache's prolog and epilog are read into aligned buffers.
                size_t offsetAdjustment = info.m_offset - AZ_SIZE_ALIGN_DOWN(info.m_offset, aznumeric_cast<size_t>(m_alignment));
                size_t bufferSize = AZ_SIZE_ALIGN_UP((info.m_compressedSize + offsetAdjustment), aznumeric_cast<size_t>(m_alignment));
                m_readBuffers[i] = reinterpret_cast<Buffer>(AZ::AllocatorInstance<AZ::SystemAllocator>::Get().Allocate(
                    bufferSize, m_alignment, 0, "AZ::IO::Streamer FullFileDecompressor", __FILE__, __LINE__));
                m_memoryUsage += bufferSize;

                FileRequest* archiveReadRequest = m_context->GetNewInternalRequest();
                archiveReadRequest->CreateRead(compressedReadRequest, m_readBuffers[i] + offsetAdjustment, bufferSize, info.m_archiveFilename,
                    info.m_offset, info.m_compressedSize, info.m_isSharedPak);
                archiveReadRequest->SetCompletionCallback(
                    [this, readSlot = i](FileRequest& request)
                    {
                        AZ_PROFILE_FUNCTION(AzCore);
                        FinishArchiveRead(&request, readSlot);
                    });
                m_next->QueueRequest(archiveReadRequest);

                m_readRequests[i] = archiveReadRequest;
                m_readBufferStatus[i] = ReadBufferStatus::ReadInFlight;

                AZ_Assert(m_numInFlightReads < m_maxNumReads,
                    "A FileRequest was queued for reading in FullFileDecompressor, but there's no slots available.");
                m_numInFlightReads++;

                return;
            }
        }
        AZ_Assert(false, "%u of %u read slots are use in the FullFileDecompressor, but no empty slot was found.", m_numInFlightReads, m_maxNumReads);
    }

    void FullFileDecompressor::FinishArchiveRead(FileRequest* readRequest, u32 readSlot)
    {
        AZ_Assert(m_readRequests[readSlot] == readRequest,
            "Request in the archive read slot isn't the same as request that's being completed.");

        FileRequest* compressedRequest = readRequest->GetParent();
        AZ_Assert(compressedRequest, "Read requests started by FullFileDecompressor is missing a parent request.");

        if (readRequest->GetStatus() == IStreamerTypes::RequestStatus::Completed)
        {
            m_readBufferStatus[readSlot] = ReadBufferStatus::PendingDecompression;
            ++m_numPendingDecompression;

            // Add this wait so the compressed request isn't fully completed yet as only the read part is done. The
            // job thread will finish this wait, which in turn will trigger this function again on the main streaming thread.
            FileRequest* waitRequest = m_context->GetNewInternalRequest();
            waitRequest->CreateWait(compressedRequest);
            m_readRequests[readSlot] = waitRequest;
        }
        else
        {
            auto data = AZStd::get_if<FileRequest::CompressedReadData>(&compressedRequest->GetCommand());
            AZ_Assert(data, "Compressed request in FullFileDecompressor that finished unsuccessfully didn't contain compression read data.");
            CompressionInfo& info = data->m_compressionInfo;
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
                "Trying to decrement a read request after it was canceled or failed in FullFileDecompressor, "
                "but no read requests are supposed to be queued.");
            m_numInFlightReads--;
        }
    }

    bool FullFileDecompressor::StartDecompressions()
    {
        bool queuedJobs = false;
        u32 jobSlot = 0;
        for (u32 readSlot = 0; readSlot < m_maxNumReads; ++readSlot)
        {
            // Find completed read.
            if (m_readBufferStatus[readSlot] != ReadBufferStatus::PendingDecompression)
            {
                continue;
            }

            // Find decompression slot
            for (; jobSlot < m_maxNumJobs; ++jobSlot)
            {
                if (m_processingJobs[jobSlot].IsProcessing())
                {
                    continue;
                }

                FileRequest* waitRequest = m_readRequests[readSlot];
                AZ_Assert(AZStd::holds_alternative<FileRequest::WaitData>(waitRequest->GetCommand()),
                    "File request waiting for decompression wasn't marked as being a wait operation.");
                FileRequest* compressedRequest = waitRequest->GetParent();
                AZ_Assert(compressedRequest, "Read requests started by FullFileDecompressor is missing a parent request.");

                waitRequest->SetCompletionCallback([this, jobSlot](FileRequest& request)
                    {
                        AZ_PROFILE_FUNCTION(AzCore);
                        FinishDecompression(&request, jobSlot);
                    });

                DecompressionInformation& info = m_processingJobs[jobSlot];
                info.m_waitRequest = waitRequest;
                info.m_queueStartTime = AZStd::chrono::high_resolution_clock::now();
                info.m_jobStartTime = info.m_queueStartTime; // Set these to the same in case the scheduler requests an update before the job has started.
                info.m_compressedData = m_readBuffers[readSlot]; // Transfer ownership of the pointer.
                m_readBuffers[readSlot] = nullptr;

                AZ::Job* decompressionJob;
                auto data = AZStd::get_if<FileRequest::CompressedReadData>(&compressedRequest->GetCommand());
                AZ_Assert(data, "Compressed request in FullFileDecompressor that's starting decompression didn't contain compression read data.");
                AZ_Assert(data->m_compressionInfo.m_decompressor, "FullFileDecompressor is queuing a decompression job but couldn't find a decompressor.");

                info.m_alignmentOffset = aznumeric_caster(data->m_compressionInfo.m_offset -
                    AZ_SIZE_ALIGN_DOWN(data->m_compressionInfo.m_offset, aznumeric_cast<size_t>(m_alignment)));

                if (data->m_readOffset == 0 && data->m_readSize == data->m_compressionInfo.m_uncompressedSize)
                {
                    auto job = [this, &info]()
                    {
                        FullDecompression(m_context, info);
                    };
                    decompressionJob = AZ::CreateJobFunction(job, true, m_decompressionjobContext.get());
                }
                else
                {
                    m_memoryUsage += data->m_compressionInfo.m_uncompressedSize;
                    auto job = [this, &info]()
                    {
                        PartialDecompression(m_context, info);
                    };
                    decompressionJob = AZ::CreateJobFunction(job, true, m_decompressionjobContext.get());
                }
                --m_numPendingDecompression;
                ++m_numRunningJobs;
                decompressionJob->Start();

                m_readRequests[readSlot] = nullptr;
                m_readBufferStatus[readSlot] = ReadBufferStatus::Unused;
                AZ_Assert(m_numInFlightReads > 0, "Trying to decrement a read request after it's queued for decompression in FullFileDecompressor, but no read requests are supposed to be queued.");
                m_numInFlightReads--;

                queuedJobs = true;
                break;
            }

            if (m_numInFlightReads == 0 || m_numRunningJobs == m_maxNumJobs)
            {
                return queuedJobs;
            }
        }
        return queuedJobs;
    }

    void FullFileDecompressor::FinishDecompression([[maybe_unused]] FileRequest* waitRequest, u32 jobSlot)
    {
        DecompressionInformation& jobInfo = m_processingJobs[jobSlot];
        AZ_Assert(jobInfo.m_waitRequest == waitRequest, "Job slot didn't contain the expected wait request.");

        auto endTime = AZStd::chrono::high_resolution_clock::now();

        FileRequest* compressedRequest = jobInfo.m_waitRequest->GetParent();
        AZ_Assert(compressedRequest, "A wait request attached to FullFileDecompressor was completed but didn't have a parent compressed request.");
        auto data = AZStd::get_if<FileRequest::CompressedReadData>(&compressedRequest->GetCommand());
        AZ_Assert(data, "Compressed request in FullFileDecompressor that completed decompression didn't contain compression read data.");
        CompressionInfo& info = data->m_compressionInfo;
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
        AZ_Assert(m_numRunningJobs > 0, "About to complete a decompression job, but the internal count doesn't see a running job.");
        --m_numRunningJobs;
        return;
    }

    void FullFileDecompressor::FullDecompression(StreamerContext* context, DecompressionInformation& info)
    {
        info.m_jobStartTime = AZStd::chrono::high_resolution_clock::now();

        FileRequest* compressedRequest = info.m_waitRequest->GetParent();
        AZ_Assert(compressedRequest, "A wait request attached to FullFileDecompressor was completed but didn't have a parent compressed request.");
        auto request = AZStd::get_if<FileRequest::CompressedReadData>(&compressedRequest->GetCommand());
        AZ_Assert(request, "Compressed request in FullFileDecompressor that's running full decompression didn't contain compression read data.");
        CompressionInfo& compressionInfo = request->m_compressionInfo;
        AZ_Assert(compressionInfo.m_decompressor, "Full decompressor job started, but there's no decompressor callback assigned.");

        AZ_Assert(request->m_readOffset == 0, "FullFileDecompressor is doing a full decompression on a file request with an offset (%zu).",
            request->m_readOffset);
        AZ_Assert(compressionInfo.m_uncompressedSize == request->m_readSize,
            "FullFileDecompressor is doing a full decompression, but the target buffer size (%llu) doesn't match the decompressed size (%zu).",
            request->m_readSize, compressionInfo.m_uncompressedSize);

        bool success = compressionInfo.m_decompressor(compressionInfo, info.m_compressedData + info.m_alignmentOffset,
            compressionInfo.m_compressedSize, request->m_output, compressionInfo.m_uncompressedSize);
        info.m_waitRequest->SetStatus(success ? IStreamerTypes::RequestStatus::Completed : IStreamerTypes::RequestStatus::Failed);

        context->MarkRequestAsCompleted(info.m_waitRequest);
        context->WakeUpSchedulingThread();
    }

    void FullFileDecompressor::PartialDecompression(StreamerContext* context, DecompressionInformation& info)
    {
        info.m_jobStartTime = AZStd::chrono::high_resolution_clock::now();

        FileRequest* compressedRequest = info.m_waitRequest->GetParent();
        AZ_Assert(compressedRequest, "A wait request attached to FullFileDecompressor was completed but didn't have a parent compressed request.");
        auto request = AZStd::get_if<FileRequest::CompressedReadData>(&compressedRequest->GetCommand());
        AZ_Assert(request, "Compressed request in FullFileDecompressor that's running partial decompression didn't contain compression read data.");
        CompressionInfo& compressionInfo = request->m_compressionInfo;
        AZ_Assert(compressionInfo.m_decompressor, "Partial decompressor job started, but there's no decompressor callback assigned.");

        AZStd::unique_ptr<u8[]> decompressionBuffer = AZStd::unique_ptr<u8[]>(new u8[compressionInfo.m_uncompressedSize]);
        bool success = compressionInfo.m_decompressor(compressionInfo, info.m_compressedData + info.m_alignmentOffset,
            compressionInfo.m_compressedSize, decompressionBuffer.get(), compressionInfo.m_uncompressedSize);
        info.m_waitRequest->SetStatus(success ? IStreamerTypes::RequestStatus::Completed : IStreamerTypes::RequestStatus::Failed);

        memcpy(request->m_output, decompressionBuffer.get() + request->m_readOffset, request->m_readSize);

        context->MarkRequestAsCompleted(info.m_waitRequest);
        context->WakeUpSchedulingThread();
    }
} // namespace AZ::IO
