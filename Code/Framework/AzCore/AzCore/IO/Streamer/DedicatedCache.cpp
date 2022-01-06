/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/IO/Streamer/FileRequest.h>
#include <AzCore/IO/Streamer/DedicatedCache.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/smart_ptr/make_shared.h>

namespace AZ::IO
{
    AZStd::shared_ptr<StreamStackEntry> DedicatedCacheConfig::AddStreamStackEntry(
        const HardwareInformation& hardware, AZStd::shared_ptr<StreamStackEntry> parent)
    {
        size_t blockSize;
        switch (m_blockSize)
        {
        case BlockCacheConfig::BlockSize::MaxTransfer:
            blockSize = hardware.m_maxTransfer;
            break;
        case BlockCacheConfig::BlockSize::MemoryAlignment:
            blockSize = hardware.m_maxPhysicalSectorSize;
            break;
        case BlockCacheConfig::BlockSize::SizeAlignment:
            blockSize = hardware.m_maxLogicalSectorSize;
            break;
        default:
            blockSize = m_blockSize;
            break;
        }

        u32 cacheSize = static_cast<AZ::u32>(m_cacheSizeMib * 1_mib);
        if (blockSize > cacheSize)
        {
            AZ_Warning("Streamer", false, "Size (%u) for DedicatedCache isn't big enough to hold at least one cache blocks of size (%zu). "
                "The cache size will be increased to fit one cache block.", cacheSize, blockSize);
            cacheSize = aznumeric_caster(blockSize);
        }

        auto stackEntry = AZStd::make_shared<DedicatedCache>(
            cacheSize, aznumeric_cast<AZ::u32>(blockSize), aznumeric_cast<AZ::u32>(hardware.m_maxPhysicalSectorSize), m_writeOnlyEpilog);
        stackEntry->SetNext(AZStd::move(parent));
        return stackEntry;
    }

    void DedicatedCacheConfig::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context); serializeContext != nullptr)
        {
            serializeContext->Class<DedicatedCacheConfig, IStreamerStackConfig>()
                ->Version(1)
                ->Field("CacheSizeMib", &DedicatedCacheConfig::m_cacheSizeMib)
                ->Field("BlockSize", &DedicatedCacheConfig::m_blockSize)
                ->Field("WriteOnlyEpilog", &DedicatedCacheConfig::m_writeOnlyEpilog);
        }
    }



    //
    // DedicatedCache
    //

    DedicatedCache::DedicatedCache(u64 cacheSize, u32 blockSize, u32 alignment, bool onlyEpilogWrites)
        : StreamStackEntry("Dedicated cache")
        , m_cacheSize(cacheSize)
        , m_alignment(alignment)
        , m_blockSize(blockSize)
        , m_onlyEpilogWrites(onlyEpilogWrites)
    {
    }

    void DedicatedCache::SetNext(AZStd::shared_ptr<StreamStackEntry> next)
    {
        m_next = AZStd::move(next);
        for (AZStd::unique_ptr<BlockCache>& cache : m_cachedFileCaches)
        {
            cache->SetNext(m_next);
        }
    }

    void DedicatedCache::SetContext(StreamerContext& context)
    {
        StreamStackEntry::SetContext(context);
        for (AZStd::unique_ptr<BlockCache>& cache : m_cachedFileCaches)
        {
            cache->SetContext(context);
        }
    }

    void DedicatedCache::PrepareRequest(FileRequest* request)
    {
        AZ_Assert(request, "PrepareRequest was provided a null request.");

        // Claim the requests so other entries can't claim it and make updates.
        AZStd::visit([this, request](auto&& args)
        {
            using Command = AZStd::decay_t<decltype(args)>;
            if constexpr (AZStd::is_same_v<Command, FileRequest::CreateDedicatedCacheData>)
            {
                args.m_range = FileRange::CreateRangeForEntireFile();
                m_context->PushPreparedRequest(request);
            }
            else if constexpr (AZStd::is_same_v<Command, FileRequest::DestroyDedicatedCacheData>)
            {
                args.m_range = FileRange::CreateRangeForEntireFile();
                m_context->PushPreparedRequest(request);
            }
            else
            {
                StreamStackEntry::PrepareRequest(request);
            }
        }, request->GetCommand());
    }

    void DedicatedCache::QueueRequest(FileRequest* request)
    {
        AZ_Assert(request, "QueueRequest was provided a null request.");

        AZStd::visit([this, request](auto&& args)
        {
            using Command = AZStd::decay_t<decltype(args)>;
            if constexpr (AZStd::is_same_v<Command, FileRequest::ReadData>)
            {
                ReadFile(request, args);
                return;
            }
            else if constexpr (AZStd::is_same_v<Command, FileRequest::CreateDedicatedCacheData>)
            {
                CreateDedicatedCache(request, args);
                return;
            }
            else if constexpr (AZStd::is_same_v<Command, FileRequest::DestroyDedicatedCacheData>)
            {
                DestroyDedicatedCache(request, args);
                return;
            }
            else
            {
                if constexpr (AZStd::is_same_v<Command, FileRequest::FlushData>)
                {
                    FlushCache(args.m_path);
                }
                else if constexpr (AZStd::is_same_v<Command, FileRequest::FlushAllData>)
                {
                    FlushEntireCache();
                }
                StreamStackEntry::QueueRequest(request);
            }
        }, request->GetCommand());
    }

    bool DedicatedCache::ExecuteRequests()
    {
        bool hasProcessedRequest = false;
        for (AZStd::unique_ptr<BlockCache>& cache : m_cachedFileCaches)
        {
            hasProcessedRequest = cache->ExecuteRequests() || hasProcessedRequest;
        }
        return StreamStackEntry::ExecuteRequests() || hasProcessedRequest;
    }

    void DedicatedCache::UpdateStatus(Status& status) const
    {
        // Available slots are not updated because the dedicated caches are often
        // small and specific to a tiny subset of files that are loaded. It would therefore
        // return a small number of slots that would needlessly hamper streaming as it doesn't
        // apply to the majority of files.

        bool isIdle = true;
        for (auto& cache : m_cachedFileCaches)
        {
            Status blockStatus;
            cache->UpdateStatus(blockStatus);
            isIdle = isIdle && blockStatus.m_isIdle;
        }
        status.m_isIdle = status.m_isIdle && isIdle;
        StreamStackEntry::UpdateStatus(status);
    }

    void DedicatedCache::UpdateCompletionEstimates(AZStd::chrono::system_clock::time_point now,
        AZStd::vector<FileRequest*>& internalPending, StreamerContext::PreparedQueue::iterator pendingBegin,
        StreamerContext::PreparedQueue::iterator pendingEnd)
    {
        for (auto& cache : m_cachedFileCaches)
        {
            cache->AddDelayedRequests(internalPending);
        }

        StreamStackEntry::UpdateCompletionEstimates(now, internalPending, pendingBegin, pendingEnd);

        for (auto& cache : m_cachedFileCaches)
        {
            cache->UpdatePendingRequestEstimations();
        }
    }

    void DedicatedCache::ReadFile(FileRequest* request, FileRequest::ReadData& data)
    {
        size_t index = FindCache(data.m_path, data.m_offset);
        if (index == s_fileNotFound)
        {
            m_usagePercentageStat.PushSample(0.0);
            if (m_next)
            {
                m_next->QueueRequest(request);
            }
        }
        else
        {
            m_usagePercentageStat.PushSample(1.0);
            BlockCache& cache = *m_cachedFileCaches[index];
            cache.QueueRequest(request);
#if AZ_STREAMER_ADD_EXTRA_PROFILING_INFO
            m_overallHitRateStat.PushSample(cache.CalculateHitRatePercentage());
            m_overallCacheableRateStat.PushSample(cache.CalculateCacheableRatePercentage());
#endif
        }
    }

    void DedicatedCache::FlushCache(const RequestPath& filePath)
    {
        size_t count = m_cachedFileNames.size();
        for (size_t i = 0; i < count; ++i)
        {
            if (m_cachedFileNames[i] == filePath)
            {
                // Flush the entire block cache as it's entirely dedicated to the found file.
                m_cachedFileCaches[i]->FlushEntireCache();
            }
        }
    }

    void DedicatedCache::FlushEntireCache()
    {
        for (AZStd::unique_ptr<BlockCache>& cache : m_cachedFileCaches)
        {
            cache->FlushEntireCache();
        }
    }

    void DedicatedCache::CollectStatistics(AZStd::vector<Statistic>& statistics) const
    {
        statistics.push_back(Statistic::CreatePercentage(m_name, "Reads from dedicated cache", m_usagePercentageStat.GetAverage()));
#if AZ_STREAMER_ADD_EXTRA_PROFILING_INFO
        statistics.push_back(Statistic::CreatePercentage(m_name, "Overall cacheable rate", m_overallCacheableRateStat.GetAverage()));
        statistics.push_back(Statistic::CreatePercentage(m_name, "Overall hit rate", m_overallHitRateStat.GetAverage()));
#endif
        statistics.push_back(Statistic::CreateInteger(m_name, "Num dedicated caches", aznumeric_caster(m_cachedFileNames.size())));
        StreamStackEntry::CollectStatistics(statistics);
    }

    void DedicatedCache::CreateDedicatedCache(FileRequest* request, FileRequest::CreateDedicatedCacheData& data)
    {
        size_t index = FindCache(data.m_path, data.m_range);
        if (index == s_fileNotFound)
        {
            index = m_cachedFileCaches.size();
            m_cachedFileNames.push_back(data.m_path);
            m_cachedFileRanges.push_back(data.m_range);
            m_cachedFileCaches.push_back(AZStd::make_unique<BlockCache>(m_cacheSize, m_blockSize, m_alignment, m_onlyEpilogWrites));
            m_cachedFileCaches[index]->SetNext(m_next);
            m_cachedFileCaches[index]->SetContext(*m_context);
            m_cachedFileRefCounts.push_back(1);
        }
        else
        {
            ++m_cachedFileRefCounts[index];
        }
        request->SetStatus(IStreamerTypes::RequestStatus::Completed);
        m_context->MarkRequestAsCompleted(request);
    }

    void DedicatedCache::DestroyDedicatedCache(FileRequest* request, FileRequest::DestroyDedicatedCacheData& data)
    {
        size_t index = FindCache(data.m_path, data.m_range);
        if (index != s_fileNotFound)
        {
            if (m_cachedFileRefCounts[index] > 0)
            {
                --m_cachedFileRefCounts[index];
                if (m_cachedFileRefCounts[index] == 0)
                {
                    m_cachedFileNames.erase(m_cachedFileNames.begin() + index);
                    m_cachedFileRanges.erase(m_cachedFileRanges.begin() + index);
                    m_cachedFileCaches.erase(m_cachedFileCaches.begin() + index);
                    m_cachedFileRefCounts.erase(m_cachedFileRefCounts.begin() + index);
                }
                request->SetStatus(IStreamerTypes::RequestStatus::Completed);
                m_context->MarkRequestAsCompleted(request);
                return;
            }
        }
        request->SetStatus(IStreamerTypes::RequestStatus::Failed);
        m_context->MarkRequestAsCompleted(request);
    }

    size_t DedicatedCache::FindCache(const RequestPath& filename, FileRange range)
    {
        size_t count = m_cachedFileNames.size();
        for (size_t i = 0; i < count; ++i)
        {
            if (m_cachedFileNames[i] == filename && m_cachedFileRanges[i] == range)
            {
                return i;
            }
        }
        return s_fileNotFound;
    }

    size_t DedicatedCache::FindCache(const RequestPath& filename, u64 offset)
    {
        size_t count = m_cachedFileNames.size();
        for (size_t i = 0; i < count; ++i)
        {
            if (m_cachedFileNames[i] == filename && m_cachedFileRanges[i].IsInRange(offset))
            {
                return i;
            }
        }
        return s_fileNotFound;
    }
} // namespace AZ::IO
