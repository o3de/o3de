/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/IO/Streamer/BlockCache.h>
#include <AzCore/IO/Streamer/FileRange.h>
#include <AzCore/IO/Streamer/Statistics.h>
#include <AzCore/IO/Streamer/StreamStackEntry.h>
#include <AzCore/IO/Streamer/StreamerConfiguration.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/limits.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

namespace AZ::IO
{
    namespace Requests
    {
        struct CreateDedicatedCacheData;
        struct DestroyDedicatedCacheData;
        struct ReportData;
    } // namespace Requests

    struct DedicatedCacheConfig final :
        public IStreamerStackConfig
    {
        AZ_RTTI(AZ::IO::DedicatedCacheConfig, "{DF0F6029-02B0-464C-9846-524654335BCC}", IStreamerStackConfig);
        AZ_CLASS_ALLOCATOR(DedicatedCacheConfig, AZ::SystemAllocator);

        ~DedicatedCacheConfig() override = default;
        AZStd::shared_ptr<StreamStackEntry> AddStreamStackEntry(
            const HardwareInformation& hardware, AZStd::shared_ptr<StreamStackEntry> parent) override;
        static void Reflect(AZ::ReflectContext* context);

        //! The size of the individual blocks inside the cache.
        BlockCacheConfig::BlockSize m_blockSize{ BlockCacheConfig::BlockSize::MemoryAlignment };
        //! The overall size of the cache in megabytes.
        u32 m_cacheSizeMib{ 8 };
        //! If true, only the epilog is written otherwise the prolog and epilog are written. In either case both prolog and epilog are read.
        //! For uses of the cache that read mostly sequentially this flag should be set to true. If reads are more random than it's better
        //! to set this flag to false.
        bool m_writeOnlyEpilog{ true };
    };

    class DedicatedCache
        : public StreamStackEntry
    {
    public:
        DedicatedCache(u64 cacheSize, u32 blockSize, u32 alignment, bool onlyEpilogWrites);

        void SetNext(AZStd::shared_ptr<StreamStackEntry> next) override;
        void SetContext(StreamerContext& context) override;

        void PrepareRequest(FileRequest* request) override;
        void QueueRequest(FileRequest* request) override;
        bool ExecuteRequests() override;

        void UpdateStatus(Status& status) const override;

        void UpdateCompletionEstimates(
            AZStd::chrono::steady_clock::time_point now,
            AZStd::vector<FileRequest*>& internalPending,
            StreamerContext::PreparedQueue::iterator pendingBegin,
            StreamerContext::PreparedQueue::iterator pendingEnd) override;

        void CollectStatistics(AZStd::vector<Statistic>& statistics) const override;

    private:
        void CreateDedicatedCache(FileRequest* request, Requests::CreateDedicatedCacheData& data);
        void DestroyDedicatedCache(FileRequest* request, Requests::DestroyDedicatedCacheData& data);

        void ReadFile(FileRequest* request, Requests::ReadData& data);
        size_t FindCache(const RequestPath& filename, FileRange range);
        size_t FindCache(const RequestPath& filename, u64 offset);

        void FlushCache(const RequestPath& filePath);
        void FlushEntireCache();

        void Report(const Requests::ReportData& data) const;

        AZStd::vector<RequestPath> m_cachedFileNames;
        AZStd::vector<FileRange> m_cachedFileRanges;
        AZStd::vector<AZStd::unique_ptr<BlockCache>> m_cachedFileCaches;
        AZStd::vector<size_t> m_cachedFileRefCounts;

        AZ::Statistics::RunningStatistic m_usagePercentageStat;
#if AZ_STREAMER_ADD_EXTRA_PROFILING_INFO
        AZ::Statistics::RunningStatistic m_overallHitRateStat;
        AZ::Statistics::RunningStatistic m_overallCacheableRateStat;
#endif

        u64 m_cacheSize;
        u32 m_alignment;
        u32 m_blockSize;
        bool m_onlyEpilogWrites;
    };
} // namespace AZ::IO
