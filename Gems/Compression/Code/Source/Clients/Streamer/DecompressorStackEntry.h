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
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/chrono/chrono.h>
#include <AzCore/std/containers/deque.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/Statistics/RunningStatistic.h>
#include <AzCore/Task/TaskExecutor.h>
#include <AzCore/Task/TaskGraph.h>

namespace AZ::IO
{
    class RequestPath;
}
namespace AZ::IO::Requests
{
    struct ReadRequestData;
    struct ReportData;
}

namespace Compression
{
    //! Thunk structure used to create a DecompressorRegistrarEntry instance
    //! and add it to the StreamerStack without needing public API access
    //! to this DecompressorRegistrarEntry outside of this gem
    //! Streamer uses the SerializeContext to load any derived IStreamerStackConfig
    //! classes listed under the "/Amazon/AzCore/Streamer/Profiles" keys
    //! from the merged Settings registry(include .setreg files)
    //! and invokes the virtual AddStreamStackEntry function on it to create the actual instance
    struct DecompressorRegistrarConfig final
        : public AZ::IO::IStreamerStackConfig
    {
        AZ_TYPE_INFO_WITH_NAME_DECL(DecompressorRegistrarConfig);
        AZ_RTTI_NO_TYPE_INFO_DECL();
        AZ_CLASS_ALLOCATOR_DECL;

        ~DecompressorRegistrarConfig() override = default;
        AZStd::shared_ptr<AZ::IO::StreamStackEntry> AddStreamStackEntry(
            const AZ::IO::HardwareInformation& hardware, AZStd::shared_ptr<AZ::IO::StreamStackEntry> parent) override;
        static void Reflect(AZ::ReflectContext* context);

        //! Maximum number of reads that are kept in flight.
        AZ::u32 m_maxNumReads{ 2 };
        //! Maximum number of decompression tasks that can run simultaneously.
        AZ::u32 m_maxNumTasks{ 2 };
    };

    //! Decompression Entry in the streamer stack that is used to look up registered compression interfaces
    //! The decompression is performed in a temporary buffer on a separate thread using the Task system
    //! as single files and without equally distributed seek points.
    //! Because the target archive has compressed the entire file, it needs to be decompressed
    //! completely, so even if the file is partially read, it needs to be fully loaded. This
    //! also means that there's no upper limit to the memory so every decompression job will
    //! need to allocate memory as a temporary buffer (in-place decompression is not supported).
    //! Finally, the lack of an upper limit also means that the duration of the decompression job
    //! can vary largely so a dedicated job system is used to decompress on to avoid blocking
    //! the main job system from working.
    class DecompressorRegistrarEntry
        : public AZ::IO::StreamStackEntry
    {
    public:
        DecompressorRegistrarEntry(AZ::u32 maxNumReads, AZ::u32 maxNumTasks, AZ::u32 alignment);
        ~DecompressorRegistrarEntry() override = default;

        void PrepareRequest(AZ::IO::FileRequest* request) override;
        void QueueRequest(AZ::IO::FileRequest* request) override;
        bool ExecuteRequests() override;

        void UpdateStatus(Status& status) const override;
        void UpdateCompletionEstimates(AZStd::chrono::steady_clock::time_point now, AZStd::vector<AZ::IO::FileRequest*>& internalPending,
            AZ::IO::StreamerContext::PreparedQueue::iterator pendingBegin, AZ::IO::StreamerContext::PreparedQueue::iterator pendingEnd) override;

        void CollectStatistics(AZStd::vector<AZ::IO::Statistic>& statistics) const override;

    private:
        using Buffer = AZ::u8*;

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
            AZ::IO::FileRequest* m_waitRequest{ nullptr };
            AZ::u32 m_alignmentOffset{ 0 };
        };

        bool IsIdle() const;

        void PrepareReadRequest(AZ::IO::FileRequest* request, AZ::IO::Requests::ReadRequestData& data);
        void PrepareDedicatedCache(AZ::IO::FileRequest* request, const AZ::IO::RequestPath& path);
        void FileExistsCheck(AZ::IO::FileRequest* checkRequest);

        void EstimateCompressedReadRequest(AZ::IO::FileRequest* request, AZStd::chrono::microseconds& cumulativeDelay,
            AZStd::chrono::microseconds decompressionDelay, double totalDecompressionDurationUs, double totalBytesDecompressed) const;

        void StartArchiveRead(AZ::IO::FileRequest* compressedReadRequest);
        void FinishArchiveRead(AZ::IO::FileRequest* readRequest, AZ::u32 readSlot);
        bool StartDecompressions();
        void FinishDecompression(AZ::IO::FileRequest* waitRequest, AZ::u32 jobSlot);

        static void FullDecompression(AZ::IO::StreamerContext* context, DecompressionInformation& info);
        static void PartialDecompression(AZ::IO::StreamerContext* context, DecompressionInformation& info);

        void Report(const AZ::IO::Requests::ReportData& data) const;

        AZStd::deque<AZ::IO::FileRequest*> m_pendingReads;
        AZStd::deque<AZ::IO::FileRequest*> m_pendingFileExistChecks;

        AZ::IO::AverageWindow<size_t, double, AZ::IO::s_statisticsWindowSize> m_decompressionJobDelayMicroSec;
        AZ::IO::AverageWindow<size_t, double, AZ::IO::s_statisticsWindowSize> m_decompressionDurationMicroSec;
        AZ::IO::AverageWindow<size_t, double, AZ::IO::s_statisticsWindowSize> m_bytesDecompressed;
#if AZ_STREAMER_ADD_EXTRA_PROFILING_INFO
        AZ::Statistics::RunningStatistic m_decompressionBoundStat;
        AZ::Statistics::RunningStatistic m_readBoundStat;
#endif
        // Task executor for decompressor
        AZ::TaskExecutor m_taskExecutor;
        AZStd::unique_ptr<AZ::TaskGraphEvent> m_taskGraphEvent;

        AZStd::unique_ptr<Buffer[]> m_readBuffers;
        // Nullptr if not reading, the read request if reading the file and the wait request for decompression when waiting on decompression.
        AZStd::unique_ptr<AZ::IO::FileRequest*[]> m_readRequests;
        AZStd::unique_ptr<ReadBufferStatus[]> m_readBufferStatus;

        AZStd::unique_ptr<DecompressionInformation[]> m_processingJobs;

        size_t m_memoryUsage{ 0 }; //!< Amount of memory used for buffers by the decompressor.
        AZ::u32 m_maxNumReads{ 2 };
        AZ::u32 m_numInFlightReads{ 0 };
        AZ::u32 m_numPendingDecompression{ 0 };
        AZ::u32 m_maxNumTasks{ 1 };
        AZ::u32 m_numRunningTasks{ 0 };
        AZ::u32 m_alignment{ 0 };
    };
} // namespace AZ::IO
