/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/IO/Streamer/Statistics.h>
#include <AzCore/IO/Streamer/StreamerConfiguration.h>
#include <AzCore/IO/Streamer/StreamStackEntry.h>
#include <AzCore/Jobs/JobManager.h>
#include <AzCore/Jobs/JobContext.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/chrono/chrono.h>
#include <AzCore/std/containers/deque.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/Statistics/RunningStatistic.h>

namespace AZ::IO
{
    namespace Requests
    {
        struct ReadRequestData;
        struct ReportData;
    }

    struct FullFileDecompressorConfig final :
        public IStreamerStackConfig
    {
        AZ_RTTI(AZ::IO::FullFileDecompressorConfig, "{C96B7EC1-8C73-4493-A7CB-66F5D550FC3A}", IStreamerStackConfig);
        AZ_CLASS_ALLOCATOR(FullFileDecompressorConfig, AZ::SystemAllocator);

        ~FullFileDecompressorConfig() override = default;
        AZStd::shared_ptr<StreamStackEntry> AddStreamStackEntry(
            const HardwareInformation& hardware, AZStd::shared_ptr<StreamStackEntry> parent) override;
        static void Reflect(AZ::ReflectContext* context);

        //! Maximum number of reads that are kept in flight.
        u32 m_maxNumReads{ 2 };
        //! Maximum number of decompression jobs that can run simultaneously.
        u32 m_maxNumJobs{ 2 };
    };

    //! Entry in the streaming stack that decompresses files from an archive that are stored
    //! as single files and without equally distributed seek points.
    //! Because the target archive has compressed the entire file, it needs to be decompressed
    //! completely, so even if the file is partially read, it needs to be fully loaded. This
    //! also means that there's no upper limit to the memory so every decompression job will
    //! need to allocate memory as a temporary buffer (in-place decompression is not supported).
    //! Finally, the lack of an upper limit also means that the duration of the decompression job
    //! can vary largely so a dedicated job system is used to decompress on to avoid blocking
    //! the main job system from working.
    class FullFileDecompressor
        : public StreamStackEntry
    {
    public:
        FullFileDecompressor(u32 maxNumReads, u32 maxNumJobs, u32 alignment);
        ~FullFileDecompressor() override = default;

        void PrepareRequest(FileRequest* request) override;
        void QueueRequest(FileRequest* request) override;
        bool ExecuteRequests() override;

        void UpdateStatus(Status& status) const override;
        void UpdateCompletionEstimates(AZStd::chrono::steady_clock::time_point now, AZStd::vector<FileRequest*>& internalPending,
            StreamerContext::PreparedQueue::iterator pendingBegin, StreamerContext::PreparedQueue::iterator pendingEnd) override;

        void CollectStatistics(AZStd::vector<Statistic>& statistics) const override;

    private:
        using Buffer = u8*;

        enum class ReadBufferStatus : uint8_t
        {
            Unused,
            ReadInFlight,
            PendingDecompression
        };

        struct DecompressionInformation
        {
            bool IsProcessing() const;

            AZStd::chrono::steady_clock::time_point m_queueStartTime;
            AZStd::chrono::steady_clock::time_point m_jobStartTime;
            Buffer m_compressedData{ nullptr };
            FileRequest* m_waitRequest{ nullptr };
            u32 m_alignmentOffset{ 0 };
        };

        bool IsIdle() const;

        void PrepareReadRequest(FileRequest* request, Requests::ReadRequestData& data);
        void PrepareDedicatedCache(FileRequest* request, const RequestPath& path);
        void FileExistsCheck(FileRequest* checkRequest);

        void EstimateCompressedReadRequest(FileRequest* request, AZStd::chrono::microseconds& cumulativeDelay,
            AZStd::chrono::microseconds decompressionDelay, double totalDecompressionDurationUs, double totalBytesDecompressed) const;

        void StartArchiveRead(FileRequest* compressedReadRequest);
        void FinishArchiveRead(FileRequest* readRequest, u32 readSlot);
        bool StartDecompressions();
        void FinishDecompression(FileRequest* waitRequest, u32 jobSlot);

        static void FullDecompression(StreamerContext* context, DecompressionInformation& info);
        static void PartialDecompression(StreamerContext* context, DecompressionInformation& info);

        void Report(const Requests::ReportData& data) const;

        AZStd::deque<FileRequest*> m_pendingReads;
        AZStd::deque<FileRequest*> m_pendingFileExistChecks;

        AverageWindow<size_t, double, s_statisticsWindowSize> m_decompressionJobDelayMicroSec;
        AverageWindow<size_t, double, s_statisticsWindowSize> m_decompressionDurationMicroSec;
        AverageWindow<size_t, double, s_statisticsWindowSize> m_bytesDecompressed;
#if AZ_STREAMER_ADD_EXTRA_PROFILING_INFO
        AZ::Statistics::RunningStatistic m_decompressionBoundStat;
        AZ::Statistics::RunningStatistic m_readBoundStat;
#endif

        AZStd::unique_ptr<Buffer[]> m_readBuffers;
        // Nullptr if not reading, the read request if reading the file and the wait request for decompression when waiting on decompression.
        AZStd::unique_ptr<FileRequest*[]> m_readRequests;
        AZStd::unique_ptr<ReadBufferStatus[]> m_readBufferStatus;

        AZStd::unique_ptr<DecompressionInformation[]> m_processingJobs;
        AZStd::unique_ptr<JobManager> m_decompressionJobManager;
        AZStd::unique_ptr<JobContext> m_decompressionjobContext;

        size_t m_memoryUsage{ 0 }; //!< Amount of memory used for buffers by the decompressor.
        u32 m_maxNumReads{ 2 };
        u32 m_numInFlightReads{ 0 };
        u32 m_numPendingDecompression{ 0 };
        u32 m_maxNumJobs{ 1 };
        u32 m_numRunningJobs{ 0 };
        u32 m_alignment{ 0 };
    };
} // namespace AZ::IO
