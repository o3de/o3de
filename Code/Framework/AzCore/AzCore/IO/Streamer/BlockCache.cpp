/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Debug/Profiler.h>
#include <AzCore/IO/Streamer/BlockCache.h>
#include <AzCore/IO/Streamer/FileRequest.h>
#include <AzCore/IO/Streamer/StreamerContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/smart_ptr/make_shared.h>

namespace AZ::IO
{
    AZStd::shared_ptr<StreamStackEntry> BlockCacheConfig::AddStreamStackEntry(
        const HardwareInformation& hardware, AZStd::shared_ptr<StreamStackEntry> parent)
    {
        size_t blockSize;
        switch (m_blockSize)
        {
        case BlockSize::MaxTransfer:
            blockSize = hardware.m_maxTransfer;
            break;
        case BlockSize::MemoryAlignment:
            blockSize = hardware.m_maxPhysicalSectorSize;
            break;
        case BlockSize::SizeAlignment:
            blockSize = hardware.m_maxLogicalSectorSize;
            break;
        default:
            blockSize = m_blockSize;
            break;
        }

        u32 cacheSize = static_cast<AZ::u32>(m_cacheSizeMib * 1_mib);
        if (blockSize * 2 > cacheSize)
        {
            AZ_Warning("Streamer", false, "Size (%u) for BlockCache isn't big enough to hold at least two cache blocks of size (%zu). "
                "The cache size will be increased to fit 2 cache blocks.", cacheSize, blockSize);
            cacheSize = aznumeric_caster(blockSize * 2);
        }

        auto stackEntry = AZStd::make_shared<BlockCache>(
            cacheSize, aznumeric_cast<AZ::u32>(blockSize), aznumeric_cast<AZ::u32>(hardware.m_maxPhysicalSectorSize), false);
        stackEntry->SetNext(AZStd::move(parent));
        return stackEntry;
    }

    void BlockCacheConfig::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context); serializeContext != nullptr)
        {
            serializeContext->Enum<BlockSize>()
                ->Version(1)
                ->Value("MaxTransfer", BlockSize::MaxTransfer)
                ->Value("MemoryAlignment", BlockSize::MemoryAlignment)
                ->Value("SizeAlignment", BlockSize::SizeAlignment);

            serializeContext->Class<BlockCacheConfig, IStreamerStackConfig>()
                ->Version(1)
                ->Field("CacheSizeMib", &BlockCacheConfig::m_cacheSizeMib)
                ->Field("BlockSize", &BlockCacheConfig::m_blockSize);
        }
    }

    static constexpr char CacheHitRateName[] = "Cache hit rate";
    static constexpr char CacheableName[] = "Cacheable";

    void BlockCache::Section::Prefix(const Section& section)
    {
        AZ_Assert(section.m_used, "Trying to prefix an unused section");
        AZ_Assert(!m_wait && !section.m_wait, "Can't merge two section that are already waiting for data to be loaded.");

        if (m_used)
        {
            AZ_Assert(m_blockOffset == 0, "Unable to add a block cache to this one as this block requires an offset upon completion.");

            AZ_Assert(section.m_readOffset < m_readOffset, "The block that's being merged needs to come before this block.");
            m_readOffset = section.m_readOffset + section.m_blockOffset; // Remove any alignment that might have been added.
            m_readSize += section.m_readSize - section.m_blockOffset;

            AZ_Assert(section.m_output < m_output, "The block that's being merged needs to come before this block.");
            m_output = section.m_output;
            m_copySize += section.m_copySize;
        }
        else
        {
            m_used = true;
            m_readOffset = section.m_readOffset + section.m_blockOffset;
            m_readSize = section.m_readSize - section.m_blockOffset;
            m_output = section.m_output;
            m_copySize = section.m_copySize;
        }
        m_blockOffset = 0; // Two merged sections do not support caching.
    }

    BlockCache::BlockCache(u64 cacheSize, u32 blockSize, u32 alignment, bool onlyEpilogWrites)
        : StreamStackEntry("Block cache")
        , m_alignment(alignment)
        , m_onlyEpilogWrites(onlyEpilogWrites)
    {
        AZ_Assert(IStreamerTypes::IsPowerOf2(alignment), "Alignment needs to be a power of 2.");
        AZ_Assert(IStreamerTypes::IsAlignedTo(blockSize, alignment), "Block size needs to be a multiple of the alignment.");

        m_numBlocks = aznumeric_caster(cacheSize / blockSize);
        m_cacheSize = cacheSize - (cacheSize % blockSize); // Only use the amount needed for the cache.
        m_blockSize = blockSize;
        if (m_numBlocks == 1)
        {
            m_onlyEpilogWrites = true;
        }

        m_cache = reinterpret_cast<u8*>(AZ::AllocatorInstance<AZ::SystemAllocator>::Get().Allocate(
            m_cacheSize, alignment, 0, "AZ::IO::Streamer BlockCache", __FILE__, __LINE__));
        m_cachedPaths = AZStd::unique_ptr<RequestPath[]>(new RequestPath[m_numBlocks]);
        m_cachedOffsets = AZStd::unique_ptr<u64[]>(new u64[m_numBlocks]);
        m_blockLastTouched = AZStd::unique_ptr<TimePoint[]>(new TimePoint[m_numBlocks]);
        m_inFlightRequests = AZStd::unique_ptr<FileRequest*[]>(new FileRequest*[m_numBlocks]);

        ResetCache();
    }

    BlockCache::~BlockCache()
    {
        AZ::AllocatorInstance<AZ::SystemAllocator>::Get().DeAllocate(m_cache, m_cacheSize, m_alignment);
    }

    void BlockCache::QueueRequest(FileRequest* request)
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

    bool BlockCache::ExecuteRequests()
    {
        size_t delayedCount = m_delayedSections.size();

        bool delayedRequestProcessed = false;
        for (size_t i = 0; i < delayedCount; ++i)
        {
            Section& delayed = m_delayedSections.front();
            AZ_Assert(delayed.m_parent, "Delayed section doesn't have a reference to the original request.");
            auto data = AZStd::get_if<FileRequest::ReadData>(&delayed.m_parent->GetCommand());
            AZ_Assert(data, "A request in the delayed queue of the BlockCache didn't have a parent with read data.");
            // This call can add the same section to the back of the queue if there's not
            // enough space. Because of this the entry needs to be removed from the delayed
            // list no matter what the result is of ServiceFromCache.
            if (ServiceFromCache(delayed.m_parent, delayed, data->m_path, data->m_sharedRead) != CacheResult::Delayed)
            {
                delayedRequestProcessed = true;
            }
            m_delayedSections.pop_front();
        }
        bool nextResult = StreamStackEntry::ExecuteRequests();
        return nextResult || delayedRequestProcessed;
    }

    void BlockCache::UpdateStatus(Status& status) const
    {
        StreamStackEntry::UpdateStatus(status);
        s32 numAvailableSlots = CalculateAvailableRequestSlots();
        status.m_numAvailableSlots = AZStd::min(status.m_numAvailableSlots, numAvailableSlots);
        status.m_isIdle = status.m_isIdle &&
            static_cast<u32>(numAvailableSlots) == m_numBlocks &&
            m_delayedSections.empty();
    }

    void BlockCache::UpdateCompletionEstimates(AZStd::chrono::system_clock::time_point now, AZStd::vector<FileRequest*>& internalPending,
        StreamerContext::PreparedQueue::iterator pendingBegin, StreamerContext::PreparedQueue::iterator pendingEnd)
    {
        // Have the stack downstream estimate the completion time for the requests that are waiting for a slot to execute in.
        AddDelayedRequests(internalPending);

        StreamStackEntry::UpdateCompletionEstimates(now, internalPending, pendingBegin, pendingEnd);

        // The in-flight requests don't have to be updated because the subdivided request will bubble up in order so the final
        // write will be the latest completion time. Requests that have a wait on another request though will need to be update
        // as the estimation of the in-flight request needs to be copied to the wait request to get an accurate prediction.
        UpdatePendingRequestEstimations();

        // Technically here the wait commands for the delayed sections should be updated as well, but it's the parent that's interesting,
        // not the wait so don't waste cycles updating the wait.
    }

    void BlockCache::AddDelayedRequests(AZStd::vector<FileRequest*>& internalPending)
    {
        for (auto& section : m_delayedSections)
        {
            internalPending.push_back(section.m_parent);
        }
    }

    void BlockCache::UpdatePendingRequestEstimations()
    {
        for (auto it : m_pendingRequests)
        {
            Section& section = it.second;
            AZ_Assert(section.m_cacheBlockIndex != s_fileNotCached, "An in-flight cache section doesn't have a cache block associated with it.");
            AZ_Assert(m_inFlightRequests[section.m_cacheBlockIndex],
                "Cache block %i is reported as being in-flight but has no request.", section.m_cacheBlockIndex);
            if (section.m_wait)
            {
                AZ_Assert(section.m_parent, "A cache section with a wait request pending is missing a parent to wait on.");
                auto largestTime = AZStd::max(section.m_parent->GetEstimatedCompletion(), it.first->GetEstimatedCompletion());
                section.m_wait->SetEstimatedCompletion(largestTime);
            }
        }
    }

    void BlockCache::ReadFile(FileRequest* request, FileRequest::ReadData& data)
    {
        if (!m_next)
        {
            request->SetStatus(IStreamerTypes::RequestStatus::Failed);
            m_context->MarkRequestAsCompleted(request);
            return;
        }

        auto continueReadFile = [this, request](FileRequest& fileSizeRequest)
        {
            AZ_PROFILE_FUNCTION(AzCore);
            AZ_Assert(m_numMetaDataRetrievalInProgress > 0,
                "More requests have completed meta data retrieval in the Block Cache than were requested.");
            m_numMetaDataRetrievalInProgress--;
            if (fileSizeRequest.GetStatus() == IStreamerTypes::RequestStatus::Completed)
            {
                auto& requestInfo = AZStd::get<FileRequest::FileMetaDataRetrievalData>(fileSizeRequest.GetCommand());
                if (requestInfo.m_found)
                {
                    ContinueReadFile(request, requestInfo.m_fileSize);
                    return;
                }
            }
            // Couldn't find the file size so don't try to split and pass the request to the next entry in the stack.
            StreamStackEntry::QueueRequest(request);
        };
        m_numMetaDataRetrievalInProgress++;
        FileRequest* fileSizeRequest = m_context->GetNewInternalRequest();
        fileSizeRequest->CreateFileMetaDataRetrieval(data.m_path);
        fileSizeRequest->SetCompletionCallback(AZStd::move(continueReadFile));
        StreamStackEntry::QueueRequest(fileSizeRequest);
    }
    void BlockCache::ContinueReadFile(FileRequest* request, u64 fileLength)
    {
        Section prolog;
        Section main;
        Section epilog;

        auto& data = AZStd::get<FileRequest::ReadData>(request->GetCommand());

        if (!SplitRequest(prolog, main, epilog, data.m_path, fileLength, data.m_offset, data.m_size,
            reinterpret_cast<u8*>(data.m_output)))
        {
            m_context->MarkRequestAsCompleted(request);
            return;
        }

        if (prolog.m_used || epilog.m_used)
        {
            m_cacheableStat.PushSample(1.0);
            Statistic::PlotImmediate(m_name, CacheableName, m_cacheableStat.GetMostRecentSample());
        }
        else
        {
            // Nothing to cache so simply forward the call to the next entry in the stack for direct reading.
            m_cacheableStat.PushSample(0.0);
            Statistic::PlotImmediate(m_name, CacheableName, m_cacheableStat.GetMostRecentSample());
            m_next->QueueRequest(request);
            return;
        }

        bool fullyCached = true;
        if (prolog.m_used)
        {
            if (m_onlyEpilogWrites && (main.m_used || epilog.m_used))
            {
                // Only the epilog is allowed to write to the cache, but a previous read could
                // still have cached the prolog, so check the cache and use the data if it's there
                // otherwise merge the section with the main section to have the data read.
                if (ReadFromCache(request, prolog, data.m_path) == CacheResult::CacheMiss)
                {
                    // The data isn't cached so put the prolog in front of the main section
                    // so it's read in one read request. If main wasn't used, prefixing the prolog
                    // will cause it to be filled in and used.
                    main.Prefix(prolog);
                    m_hitRateStat.PushSample(0.0);
                    Statistic::PlotImmediate(m_name, CacheHitRateName, m_hitRateStat.GetMostRecentSample());
                }
                else
                {
                    m_hitRateStat.PushSample(1.0);
                    Statistic::PlotImmediate(m_name, CacheHitRateName, m_hitRateStat.GetMostRecentSample());
                }
            }
            else
            {
                // If m_onlyEpilogWrites is set but main and epilog are not filled in, it means that
                // the request was so small it fits in one cache block, in which case the prolog and
                // epilog are practically the same. Or this code is reached because both prolog and
                // epilog are allowed to write.
                bool readFromCache = (ServiceFromCache(request, prolog, data.m_path, data.m_sharedRead) == CacheResult::ReadFromCache);
                fullyCached = readFromCache && fullyCached;

                m_hitRateStat.PushSample(readFromCache ? 1.0 : 0.0);
                Statistic::PlotImmediate(m_name, CacheHitRateName, m_hitRateStat.GetMostRecentSample());
            }
        }

        if (main.m_used)
        {
            FileRequest* mainRequest = m_context->GetNewInternalRequest();
            // No need for a callback as there's nothing to do after the read has been completed.
            mainRequest->CreateRead(request, main.m_output, main.m_readSize, data.m_path,
                main.m_readOffset, main.m_readSize, data.m_sharedRead);
            m_next->QueueRequest(mainRequest);
            fullyCached = false;
        }

        if (epilog.m_used)
        {
            bool readFromCache = (ServiceFromCache(request, epilog, data.m_path, data.m_sharedRead) == CacheResult::ReadFromCache);
            fullyCached = readFromCache && fullyCached;

            m_hitRateStat.PushSample(readFromCache ? 1.0 : 0.0);
            Statistic::PlotImmediate(m_name, CacheHitRateName, m_hitRateStat.GetMostRecentSample());
        }

        if (fullyCached)
        {
            request->SetStatus(IStreamerTypes::RequestStatus::Completed);
            m_context->MarkRequestAsCompleted(request);
        }
    }

    void BlockCache::FlushCache(const RequestPath& filePath)
    {
        for (u32 i = 0; i < m_numBlocks; ++i)
        {
            if (m_cachedPaths[i] == filePath)
            {
                ResetCacheEntry(i);
            }
        }
    }

    void BlockCache::FlushEntireCache()
    {
        ResetCache();
    }

    void BlockCache::CollectStatistics(AZStd::vector<Statistic>& statistics) const
    {
        statistics.push_back(Statistic::CreatePercentage(m_name, CacheHitRateName, CalculateHitRatePercentage()));
        statistics.push_back(Statistic::CreatePercentage(m_name, CacheableName, CalculateCacheableRatePercentage()));
        statistics.push_back(Statistic::CreateInteger(m_name, "Available slots", CalculateAvailableRequestSlots()));

        StreamStackEntry::CollectStatistics(statistics);
    }

    double BlockCache::CalculateHitRatePercentage() const
    {
        return m_hitRateStat.GetAverage();
    }

    double BlockCache::CalculateCacheableRatePercentage() const
    {
        return m_cacheableStat.GetAverage();
    }

    s32 BlockCache::CalculateAvailableRequestSlots() const
    {
        return  aznumeric_cast<s32>(m_numBlocks) - m_numInFlightRequests - m_numMetaDataRetrievalInProgress -
            aznumeric_cast<s32>(m_delayedSections.size());
    }

    BlockCache::CacheResult BlockCache::ReadFromCache(FileRequest* request, Section& section, const RequestPath& filePath)
    {
        u32 cacheLocation = FindInCache(filePath, section.m_readOffset);
        if (cacheLocation != s_fileNotCached)
        {
            return ReadFromCache(request, section, cacheLocation);
        }
        else
        {
            return CacheResult::CacheMiss;
        }
    }

    BlockCache::CacheResult BlockCache::ReadFromCache(FileRequest* request, Section& section, u32 cacheBlock)
    {
        if (!IsCacheBlockInFlight(cacheBlock))
        {
            TouchBlock(cacheBlock);
            memcpy(section.m_output, GetCacheBlockData(cacheBlock) + section.m_blockOffset, section.m_copySize);
            return CacheResult::ReadFromCache;
        }
        else
        {
            AZ_Assert(section.m_wait == nullptr, "A wait request has to be set on a block cache section, but one has already been assigned.");
            FileRequest* wait = m_context->GetNewInternalRequest();
            wait->CreateWait(request);
            section.m_cacheBlockIndex = cacheBlock;
            section.m_parent = request;
            section.m_wait = wait;
            m_pendingRequests.emplace(m_inFlightRequests[cacheBlock], section);
            return CacheResult::Queued;
        }
    }

    BlockCache::CacheResult BlockCache::ServiceFromCache(
        FileRequest* request, Section& section, const RequestPath& filePath, bool sharedRead)
    {
        AZ_Assert(m_next, "ServiceFromCache in BlockCache was called when the cache doesn't have a way to read files.");

        u32 cacheLocation = FindInCache(filePath, section.m_readOffset);
        if (cacheLocation == s_fileNotCached)
        {
            m_hitRateStat.PushSample(0.0);
            Statistic::PlotImmediate(m_name, CacheHitRateName, m_hitRateStat.GetMostRecentSample());

            section.m_parent = request;
            cacheLocation = RecycleOldestBlock(filePath, section.m_readOffset);
            if (cacheLocation != s_fileNotCached)
            {
                FileRequest* readRequest = m_context->GetNewInternalRequest();
                readRequest->CreateRead(request, GetCacheBlockData(cacheLocation), m_blockSize, filePath, section.m_readOffset,
                    section.m_readSize, sharedRead);
                readRequest->SetCompletionCallback([this](FileRequest& request)
                    {
                        AZ_PROFILE_FUNCTION(AzCore);
                        CompleteRead(request);
                    });
                section.m_cacheBlockIndex = cacheLocation;
                m_inFlightRequests[cacheLocation] = readRequest;
                m_numInFlightRequests++;

                // If set, this is the wait added by the delay.
                if (section.m_wait)
                {
                    m_context->MarkRequestAsCompleted(section.m_wait);
                    section.m_wait = nullptr;
                }

                m_pendingRequests.emplace(readRequest, section);
                m_next->QueueRequest(readRequest);
                return CacheResult::Queued;
            }
            else
            {
                // There's no more space in the cache to store this request to. This is because there are more in-flight requests than
                // there are slots in the cache. Delay the request until there's a slot available but add a wait for the section to
                // make sure the request can't complete if some parts are read.
                if (!section.m_wait)
                {
                    section.m_wait = m_context->GetNewInternalRequest();
                    section.m_wait->CreateWait(request);
                }
                m_delayedSections.push_back(section);
                return CacheResult::Delayed;
            }
        }
        else
        {
            // If set, this is the wait added by the delay when the cache was full.
            if (section.m_wait)
            {
                m_context->MarkRequestAsCompleted(section.m_wait);
                section.m_wait = nullptr;
            }

            m_hitRateStat.PushSample(1.0);
            Statistic::PlotImmediate(m_name, CacheHitRateName, m_hitRateStat.GetMostRecentSample());

            return ReadFromCache(request, section, cacheLocation);
        }
    }

    void BlockCache::CompleteRead(FileRequest& request)
    {
        auto requestInfo = m_pendingRequests.equal_range(&request);
        AZ_Assert(requestInfo.first != requestInfo.second, "Block cache was asked to complete a file request it never queued.");

        IStreamerTypes::RequestStatus requestStatus = request.GetStatus();
        bool requestWasSuccessful = requestStatus == IStreamerTypes::RequestStatus::Completed;
        u32 cacheBlockIndex = requestInfo.first->second.m_cacheBlockIndex;

        for (auto it = requestInfo.first; it != requestInfo.second; ++it)
        {
            Section& section = it->second;
            AZ_Assert(section.m_cacheBlockIndex == cacheBlockIndex,
                "Section associated with the file request is referencing the incorrect cache block (%u vs %u).", cacheBlockIndex, section.m_cacheBlockIndex);
            if (section.m_wait)
            {
                section.m_wait->SetStatus(requestStatus);
                m_context->MarkRequestAsCompleted(section.m_wait);
                section.m_wait = nullptr;
            }

            if (requestWasSuccessful)
            {
                memcpy(section.m_output, GetCacheBlockData(cacheBlockIndex) + section.m_blockOffset, section.m_copySize);
            }
        }

        if (requestWasSuccessful)
        {
            TouchBlock(cacheBlockIndex);
            m_inFlightRequests[cacheBlockIndex] = nullptr;
        }
        else
        {
            ResetCacheEntry(cacheBlockIndex);
        }
        AZ_Assert(m_numInFlightRequests > 0, "Clearing out an in-flight request, but there shouldn't be any in flight according to records.");
        m_numInFlightRequests--;
        m_pendingRequests.erase(&request);
    }

    bool BlockCache::SplitRequest(Section& prolog, Section& main, Section& epilog,
        [[maybe_unused]] const RequestPath& filePath, u64 fileLength,
        u64 offset, u64 size, u8* buffer) const
    {
        AZ_Assert(offset + size <= fileLength, "File at path '%s' is being read past the end of the file.", filePath.GetRelativePath());

        //
        // Prolog
        // This looks at the request and sees if there's anything in front of the file that should be cached. This also
        // deals with the situation where the entire file request fits inside the cache which could mean there's data
        // left after the file as well that could be cached.
        //
        u64 roundedOffsetStart = AZ_SIZE_ALIGN_DOWN(offset, aznumeric_cast<u64>(m_blockSize));

        u64 blockReadSizeStart = AZStd::min(fileLength - roundedOffsetStart, aznumeric_cast<u64>(m_blockSize));
        // Check if the request is on the left edge of the cache block, which means there's nothing in front of it
        // that could be cached.
        if (roundedOffsetStart == offset)
        {
            if (offset + size >= fileLength)
            {
                // The entire (remainder) of the file is read so there's nothing to cache
                main.m_readOffset = offset;
                main.m_readSize = size;
                main.m_output = buffer;
                main.m_used = true;
                return true;
            }
            else if (size < blockReadSizeStart)
            {
                // The entire request fits inside a single cache block, but there's more file to read.
                prolog.m_readOffset = offset;
                prolog.m_readSize = blockReadSizeStart;
                prolog.m_blockOffset = 0;
                prolog.m_output = buffer;
                prolog.m_copySize = size;
                prolog.m_used = true;
                return true;
            }
            // In any other case it means that the entire block would be read so caching has no effect.
        }
        else
        {
            // There is a portion of the file before that's not requested so always cache this block.
            const u64 blockOffset = offset - roundedOffsetStart;
            prolog.m_readOffset = roundedOffsetStart;
            prolog.m_blockOffset = blockOffset;
            prolog.m_output = buffer;
            prolog.m_used = true;

            const bool isEntirelyInCache = blockOffset + size <= blockReadSizeStart;
            if (isEntirelyInCache)
            {
                // The read size is already clamped to the file size above when blockReadSizeStart is set.
                AZ_Assert(roundedOffsetStart + blockReadSizeStart <= fileLength,
                    "Read size in block cache was set to %llu but this is beyond the file length of %llu.",
                    roundedOffsetStart + blockReadSizeStart, fileLength);
                prolog.m_readSize = blockReadSizeStart;
                prolog.m_copySize = size;

                // There won't be anything else coming after this so continue reading.
                return true;
            }
            else
            {
                prolog.m_readSize = blockReadSizeStart;
                prolog.m_copySize = blockReadSizeStart - blockOffset;
            }
        }


        //
        // Epilog
        // Since the prolog already takes care of the situation where the file fits entirely in the cache the epilog is
        // much simpler as it only has to look at the case where there is more file after the request to read for caching.
        //
        u64 roundedOffsetEnd = AZ_SIZE_ALIGN_DOWN(offset + size, aznumeric_cast<u64>(m_blockSize));
        u64 copySize = offset + size - roundedOffsetEnd;
        u64 blockReadSizeEnd = m_blockSize;
        if ((roundedOffsetEnd + blockReadSizeEnd) > fileLength)
        {
            blockReadSizeEnd = fileLength - roundedOffsetEnd;
        }

        // If the read doesn't align with the edge of the cache
        if (copySize != 0 && copySize < blockReadSizeEnd)
        {
            epilog.m_readOffset = roundedOffsetEnd;
            epilog.m_readSize = blockReadSizeEnd;
            epilog.m_blockOffset = 0;
            epilog.m_output = buffer + (roundedOffsetEnd - offset);
            epilog.m_copySize = copySize;
            epilog.m_used = true;
        }

        //
        // Main
        // If this point is reached there's potentially a block between the prolog and epilog that can be directly read.
        //
        u64 adjustedOffset = offset;
        if (prolog.m_used)
        {
            adjustedOffset += prolog.m_copySize;
            size -= prolog.m_copySize;
        }
        if (epilog.m_used)
        {
            size -= epilog.m_copySize;
        }
        AZ_Assert(IStreamerTypes::IsAlignedTo(adjustedOffset, m_blockSize),
            "The adjustments made by the prolog should guarantee the offset is aligned to a cache block.");
        if (size != 0)
        {
            main.m_readOffset = adjustedOffset;
            main.m_readSize = size;
            main.m_output = buffer + (adjustedOffset - offset);
            main.m_used = true;
        }

        return true;
    }

    u8* BlockCache::GetCacheBlockData(u32 index)
    {
        AZ_Assert(index < m_numBlocks, "Index for touch a cache entry in the BlockCache is out of bounds.");
        return m_cache + (index * m_blockSize);
    }

    void BlockCache::TouchBlock(u32 index)
    {
        AZ_Assert(index < m_numBlocks, "Index for touch a cache entry in the BlockCache is out of bounds.");
        m_blockLastTouched[index] = AZStd::chrono::high_resolution_clock::now();
    }

    u32 BlockCache::RecycleOldestBlock(const RequestPath& filePath, u64 offset)
    {
        AZ_Assert((offset & (m_blockSize - 1)) == 0, "The offset used to recycle a block cache needs to be a multiple of the block size.");

        // Find the oldest cache block.
        TimePoint oldest = m_blockLastTouched[0];
        u32 oldestIndex = 0;
        for (u32 i = 1; i < m_numBlocks; ++i)
        {
            if (m_blockLastTouched[i] < oldest && !m_inFlightRequests[i])
            {
                oldest = m_blockLastTouched[i];
                oldestIndex = i;
            }
        }

        if (!IsCacheBlockInFlight(oldestIndex))
        {
            // Recycle the block.
            m_cachedPaths[oldestIndex] = filePath;
            m_cachedOffsets[oldestIndex] = offset;
            TouchBlock(oldestIndex);
            return oldestIndex;
        }
        else
        {
            return s_fileNotCached;
        }
    }

    u32 BlockCache::FindInCache(const RequestPath& filePath, u64 offset) const
    {
        AZ_Assert((offset & (m_blockSize - 1)) == 0, "The offset used to find a block in the block cache needs to be a multiple of the block size.");
        for (u32 i = 0; i < m_numBlocks; ++i)
        {
            if (m_cachedPaths[i] == filePath && m_cachedOffsets[i] == offset)
            {
                return i;
            }
        }

        return s_fileNotCached;
    }

    bool BlockCache::IsCacheBlockInFlight(u32 index) const
    {
        AZ_Assert(index < m_numBlocks, "Index for checking if a cache block is in flight is out of bounds.");
        return m_inFlightRequests[index] != nullptr;
    }

    void BlockCache::ResetCacheEntry(u32 index)
    {
        AZ_Assert(index < m_numBlocks, "Index for resetting a cache entry in the BlockCache is out of bounds.");

        m_cachedPaths[index].Clear();
        m_cachedOffsets[index] = 0;
        m_blockLastTouched[index] = TimePoint::min();
        m_inFlightRequests[index] = nullptr;
    }

    void BlockCache::ResetCache()
    {
        for (u32 i = 0; i < m_numBlocks; ++i)
        {
            ResetCacheEntry(i);
        }
        m_numInFlightRequests = 0;
    }
} // namespace AZ::IO
