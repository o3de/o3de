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
#include <AzCore/Statistics/RunningStatistic.h>
#include <AzCore/std/limits.h>
#include <AzCore/std/chrono/chrono.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/deque.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

namespace AZ::IO
{
    class RequestPath;
    namespace Requests
    {
        struct ReadData;
        struct ReportData;
    }

    struct BlockCacheConfig final :
        public IStreamerStackConfig
    {
        AZ_RTTI(AZ::IO::BlockCacheConfig, "{70120525-88A4-40B6-A75B-BAA7E8FD77F3}", IStreamerStackConfig);
        AZ_CLASS_ALLOCATOR(BlockCacheConfig, AZ::SystemAllocator);

        ~BlockCacheConfig() override = default;
        AZStd::shared_ptr<StreamStackEntry> AddStreamStackEntry(
            const HardwareInformation& hardware, AZStd::shared_ptr<StreamStackEntry> parent) override;
        static void Reflect(AZ::ReflectContext* context);

        //! Dynamic options for the blocks size.
        //! It's possible to set static sizes or use the names from this enum to have AZ::IO::Streamer automatically fill in the sizes.
        //! Fixed sizes are set through the Settings Registry with "BlockSize": 524288, while dynamic values are set like
        //! "BlockSize": "MemoryAlignment". In the latter case AZ::IO::Streamer will use the available hardware information and fill
        //! in the actual value.
        enum BlockSize : u32
        {
            MaxTransfer = AZStd::numeric_limits<u32>::max(), //!< The largest possible block size.
            MemoryAlignment = MaxTransfer - 1, //!< The size of the minimal memory requirement of the storage device.
            SizeAlignment = MemoryAlignment - 1 //!< The minimal read size required by the storage device.
        };

        //! The overall size of the cache in megabytes.
        u32 m_cacheSizeMib{ 8 };
        //! The size of the individual blocks inside the cache.
        BlockSize m_blockSize{ BlockSize::MemoryAlignment };
    };

    class BlockCache
        : public StreamStackEntry
    {
    public:
        BlockCache(u64 cacheSize, u32 blockSize, u32 alignment, bool onlyEpilogWrites);
        BlockCache(BlockCache&& rhs) = delete;
        BlockCache(const BlockCache& rhs) = delete;
        ~BlockCache() override;

        BlockCache& operator=(BlockCache&& rhs) = delete;
        BlockCache& operator=(const BlockCache& rhs) = delete;

        void QueueRequest(FileRequest* request) override;
        bool ExecuteRequests() override;

        void UpdateStatus(Status& status) const override;
        void UpdateCompletionEstimates(AZStd::chrono::steady_clock::time_point now, AZStd::vector<FileRequest*>& internalPending,
            StreamerContext::PreparedQueue::iterator pendingBegin, StreamerContext::PreparedQueue::iterator pendingEnd) override;
        void AddDelayedRequests(AZStd::vector<FileRequest*>& internalPending);
        void UpdatePendingRequestEstimations();

        void FlushCache(const RequestPath& filePath);
        void FlushEntireCache();

        void CollectStatistics(AZStd::vector<Statistic>& statistics) const override;

        double CalculateHitRatePercentage() const;
        double CalculateCacheableRatePercentage() const;
        s32 CalculateAvailableRequestSlots() const;

    protected:
        static constexpr u32 s_fileNotCached = static_cast<u32>(-1);

        enum class CacheResult
        {
            ReadFromCache, //!< Data was found in the cache and reused.
            CacheMiss, //!< Data wasn't found in the cache and no sub request was queued.
            Queued, //!< A sub request was created or appended and queued for processing on the next entry in the streamer stack.
            Delayed //!< There's no more room to queue a new request, so delay the request until a slot becomes available.
        };

        struct Section
        {
            u8* m_output{ nullptr }; //!< The buffer to write the data to.
            FileRequest* m_parent{ nullptr }; //!< If set, the file request that is split up by this section.
            FileRequest* m_wait{ nullptr }; //!< If set, this contains a "wait"-operation that blocks an operation chain from continuing until this section has been loaded.
            u64 m_readOffset{ 0 }; //!< Offset into the file to start reading from.
            u64 m_readSize{ 0 }; //!< Number of bytes to read from file.
            u64 m_blockOffset{ 0 }; //!< Offset into the cache block to start copying from.
            u64 m_copySize{ 0 }; //!< Number of bytes to copy from cache.
            u32 m_cacheBlockIndex{ s_fileNotCached }; //!< If assigned, the index of the cache block assigned to this section.
            bool m_used{ false }; //!< Whether or not this section is used in further processing.

            // Add the provided section in front of this one.
            void Prefix(const Section& section);
        };

        using TimePoint = AZStd::chrono::steady_clock::time_point;

        void ReadFile(FileRequest* request, Requests::ReadData& data);
        void ContinueReadFile(FileRequest* request, u64 fileLength);
        CacheResult ReadFromCache(FileRequest* request, Section& section, const RequestPath& filePath);
        CacheResult ReadFromCache(FileRequest* request, Section& section, u32 cacheBlock);
        CacheResult ServiceFromCache(FileRequest* request, Section& section, const RequestPath& filePath, bool sharedRead);
        void CompleteRead(FileRequest& request);
        bool SplitRequest(Section& prolog, Section& main, Section& epilog, const RequestPath& filePath, u64 fileLength,
            u64 offset, u64 size, u8* buffer) const;

        u8* GetCacheBlockData(u32 index);
        void TouchBlock(u32 index);
        AZ::u32 RecycleOldestBlock(const RequestPath& filePath, u64 offset);
        u32 FindInCache(const RequestPath& filePath, u64 offset) const;
        bool IsCacheBlockInFlight(u32 index) const;
        void ResetCacheEntry(u32 index);
        void ResetCache();

        void Report(const Requests::ReportData& data) const;

        //! Map of the file requests that are being processed and the sections of the parent requests they'll complete.
        AZStd::unordered_multimap<FileRequest*, Section> m_pendingRequests;
        //! List of file sections that were delayed because the cache was full.
        AZStd::deque<Section> m_delayedSections;

        AZ::Statistics::RunningStatistic m_hitRateStat;
        AZ::Statistics::RunningStatistic m_cacheableStat;

        u8* m_cache;
        u64 m_cacheSize;
        u32 m_blockSize;
        u32 m_alignment;
        u32 m_numBlocks;
        s32 m_numInFlightRequests{ 0 };
        //! The file path associated with a cache block.
        AZStd::unique_ptr<RequestPath[]> m_cachedPaths; // Array of m_numBlocks size.
        //! The offset into the file the cache blocks starts at.
        AZStd::unique_ptr<u64[]> m_cachedOffsets; // Array of m_numBlocks size.
        //! The last time the cache block was read from.
        AZStd::unique_ptr<TimePoint[]> m_blockLastTouched; // Array of m_numBlocks size.
        //! The file request that's currently read data into the cache block. If null, the block has been read.
        AZStd::unique_ptr<FileRequest*[]> m_inFlightRequests; // Array of m_numbBlocks size.

        //! The number of requests waiting for meta data to be retrieved.
        s32 m_numMetaDataRetrievalInProgress{ 0 };
        //! Whether or not only the epilog ever writes to the cache.
        bool m_onlyEpilogWrites;
    };
} // namespace AZ::IO

namespace AZ
{
    AZ_TYPE_INFO_SPECIALIZE(AZ::IO::BlockCacheConfig::BlockSize, "{5D4D597D-4605-462D-A27D-8046115C5381}");
} // namespace AZ
