/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Debug/Profiler.h>
#include <AzCore/IO/Streamer/FileRequest.h>
#include <AzCore/IO/Streamer/ReadSplitter.h>
#include <AzCore/IO/Streamer/StreamerContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/smart_ptr/make_shared.h>

namespace AZ::IO
{
    AZStd::shared_ptr<StreamStackEntry> ReadSplitterConfig::AddStreamStackEntry(
        const HardwareInformation& hardware, AZStd::shared_ptr<StreamStackEntry> parent)
    {
        size_t splitSize;
        switch (m_splitSize)
        {
        case SplitSize::MaxTransfer:
            splitSize = hardware.m_maxTransfer;
            break;
        case SplitSize::MemoryAlignment:
            splitSize = hardware.m_maxPhysicalSectorSize;
            break;
        default:
            splitSize = m_splitSize;
            break;
        }

        size_t bufferSize = m_bufferSizeMib * 1_mib;
        if (bufferSize < splitSize)
        {
            AZ_Warning("Streamer", false, "The buffer size for the Read Splitter is smaller than the individual split size. "
                "It will be increased to fit at least one split.");
            bufferSize = splitSize;
        }

        auto stackEntry = AZStd::make_shared<ReadSplitter>(
            splitSize,
            aznumeric_caster(hardware.m_maxPhysicalSectorSize),
            aznumeric_caster(hardware.m_maxLogicalSectorSize),
            bufferSize, m_adjustOffset, m_splitAlignedRequests);
        stackEntry->SetNext(AZStd::move(parent));
        return stackEntry;
    }

    void ReadSplitterConfig::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context); serializeContext != nullptr)
        {
            serializeContext->Enum<SplitSize>()
                ->Version(1)
                ->Value("MaxTransfer", SplitSize::MaxTransfer)
                ->Value("MemoryAlignment", SplitSize::MemoryAlignment);

            serializeContext->Class<ReadSplitterConfig, IStreamerStackConfig>()
                ->Version(1)
                ->Field("BufferSizeMib", &ReadSplitterConfig::m_bufferSizeMib)
                ->Field("SplitSize", &ReadSplitterConfig::m_splitSize)
                ->Field("AdjustOffset", &ReadSplitterConfig::m_adjustOffset)
                ->Field("SplitAlignedRequests", &ReadSplitterConfig::m_splitAlignedRequests);
        }
    }

    static constexpr char AvgNumSubReadsName[] = "Avg. num sub reads";
    static constexpr char AlignedReadsName[] = "Aligned reads";
    static constexpr char NumAvailableBufferSlotsName[] = "Num available buffer slots";
    static constexpr char NumPendingReadsName[] = "Num pending reads";

    ReadSplitter::ReadSplitter(u64 maxReadSize, u32 memoryAlignment, u32 sizeAlignment, size_t bufferSize,
        bool adjustOffset, bool splitAlignedRequests)
        : StreamStackEntry("Read splitter")
        , m_buffer(nullptr)
        , m_bufferSize(bufferSize)
        , m_maxReadSize(maxReadSize)
        , m_memoryAlignment(memoryAlignment)
        , m_sizeAlignment(sizeAlignment)
        , m_adjustOffset(adjustOffset)
        , m_splitAlignedRequests(splitAlignedRequests)
    {
        AZ_Assert(IStreamerTypes::IsPowerOf2(memoryAlignment), "Memory alignment needs to be a power of 2");
        AZ_Assert(IStreamerTypes::IsPowerOf2(sizeAlignment), "Size alignment needs to be a power of 2");
        AZ_Assert(IStreamerTypes::IsAlignedTo(maxReadSize, sizeAlignment),
            "Maximum read size isn't aligned to a multiple of the size alignment.");

        size_t numBufferSlots = bufferSize / maxReadSize;
        // Don't divide the reads up in more sub-reads than there are dependencies available.
        numBufferSlots = AZStd::min(numBufferSlots, FileRequest::GetMaxNumDependencies());
        m_bufferCopyInformation = AZStd::unique_ptr<BufferCopyInformation[]>(new BufferCopyInformation[numBufferSlots]);
        m_availableBufferSlots.reserve(numBufferSlots);
        for (u32 i = aznumeric_caster(numBufferSlots); i > 0; --i)
        {
            m_availableBufferSlots.push_back(i - 1);
        }
    }

    ReadSplitter::~ReadSplitter()
    {
        if (m_buffer)
        {
            AZ::AllocatorInstance<AZ::SystemAllocator>::Get().DeAllocate(m_buffer, m_bufferSize, m_memoryAlignment);
        }
    }

    void ReadSplitter::QueueRequest(FileRequest* request)
    {
        AZ_Assert(request, "QueueRequest was provided a null request.");
        if (!m_next)
        {
            request->SetStatus(IStreamerTypes::RequestStatus::Failed);
            m_context->MarkRequestAsCompleted(request);
            return;
        }

        auto data = AZStd::get_if<FileRequest::ReadData>(&request->GetCommand());
        if (data == nullptr)
        {
            StreamStackEntry::QueueRequest(request);
            return;
        }

        m_averageNumSubReadsStat.PushSample(aznumeric_cast<double>((data->m_size / m_maxReadSize) + 1));
        Statistic::PlotImmediate(m_name, AvgNumSubReadsName, m_averageNumSubReadsStat.GetMostRecentSample());

        bool isAligned = IStreamerTypes::IsAlignedTo(data->m_output, m_memoryAlignment);
        if (m_adjustOffset)
        {
            isAligned = isAligned && IStreamerTypes::IsAlignedTo(data->m_offset, m_sizeAlignment);
        }

        if (isAligned || m_bufferSize == 0)
        {
            m_alignedReadsStat.PushSample(isAligned ? 1.0 : 0.0);
            if (!m_splitAlignedRequests)
            {
                StreamStackEntry::QueueRequest(request);
            }
            else
            {
                QueueAlignedRead(request);
            }
        }
        else
        {
            m_alignedReadsStat.PushSample(0.0);
            InitializeBuffer();
            QueueBufferedRead(request);
        }
    }

    void ReadSplitter::QueueAlignedRead(FileRequest* request)
    {
        auto data = AZStd::get_if<FileRequest::ReadData>(&request->GetCommand());
        AZ_Assert(data != nullptr, "Provided request to queue by the Read Splitter did not contain a read command.");

        if (data->m_size <= m_maxReadSize)
        {
            StreamStackEntry::QueueRequest(request);
            return;
        }

        PendingRead pendingRead;
        pendingRead.m_request = request;
        pendingRead.m_output = reinterpret_cast<u8*>(data->m_output);
        pendingRead.m_outputSize = data->m_outputSize;
        pendingRead.m_readSize = data->m_size;
        pendingRead.m_offset = data->m_offset;
        pendingRead.m_isBuffered = false;

        if (!m_pendingReads.empty())
        {
            m_pendingReads.push_back(pendingRead);
            return;
        }

        if (!QueueAlignedRead(pendingRead))
        {
            m_pendingReads.push_back(pendingRead);
        }
    }

    bool ReadSplitter::QueueAlignedRead(PendingRead& pending)
    {
        auto data = AZStd::get_if<FileRequest::ReadData>(&pending.m_request->GetCommand());
        AZ_Assert(data != nullptr, "Provided request to queue by the Read Splitter did not contain a read command.");

        while (pending.m_readSize > 0)
        {
            if (pending.m_request->GetNumDependencies() >= FileRequest::GetMaxNumDependencies())
            {
                // Add a wait to make sure the read request isn't completed if all sub-reads completed before
                // the ReadSplitter has had a chance to add new sub-reads to complete the read.
                if (pending.m_wait == nullptr)
                {
                    pending.m_wait = m_context->GetNewInternalRequest();
                    pending.m_wait->CreateWait(pending.m_request);
                }
                return false;
            }

            u64 readSize = m_maxReadSize;
            size_t bufferSize = m_maxReadSize;
            if (pending.m_readSize < m_maxReadSize)
            {
                readSize = pending.m_readSize;
                // This will be the last read so give the remainder of the output buffer to the final request.
                bufferSize = pending.m_outputSize;
            }

            FileRequest* subRequest = m_context->GetNewInternalRequest();
            subRequest->CreateRead(pending.m_request, pending.m_output, bufferSize, data->m_path, pending.m_offset, readSize, data->m_sharedRead);
            subRequest->SetCompletionCallback([this](FileRequest&)
                {
                    AZ_PROFILE_FUNCTION(AzCore);
                    QueuePendingRequest();
                });
            m_next->QueueRequest(subRequest);

            pending.m_offset += readSize;
            pending.m_readSize -= readSize;
            pending.m_outputSize -= bufferSize;
            pending.m_output += readSize;
        }
        if (pending.m_wait != nullptr)
        {
            m_context->MarkRequestAsCompleted(pending.m_wait);
            pending.m_wait = nullptr;
        }
        return true;
    }

    void ReadSplitter::QueueBufferedRead(FileRequest* request)
    {
        auto data = AZStd::get_if<FileRequest::ReadData>(&request->GetCommand());
        AZ_Assert(data != nullptr, "Provided request to queue by the Read Splitter did not contain a read command.");

        PendingRead pendingRead;
        pendingRead.m_request = request;
        pendingRead.m_output = reinterpret_cast<u8*>(data->m_output);
        pendingRead.m_outputSize = data->m_outputSize;
        pendingRead.m_readSize = data->m_size;
        pendingRead.m_offset = data->m_offset;
        pendingRead.m_isBuffered = true;

        if (!m_pendingReads.empty())
        {
            m_pendingReads.push_back(pendingRead);
            return;
        }

        if (!QueueBufferedRead(pendingRead))
        {
            m_pendingReads.push_back(pendingRead);
        }
    }

    bool ReadSplitter::QueueBufferedRead(PendingRead& pending)
    {
        auto data = AZStd::get_if<FileRequest::ReadData>(&pending.m_request->GetCommand());
        AZ_Assert(data != nullptr, "Provided request to queue by the Read Splitter did not contain a read command.");

        while (pending.m_readSize > 0)
        {
            if (!m_availableBufferSlots.empty())
            {
                u32 bufferSlot = m_availableBufferSlots.back();
                m_availableBufferSlots.pop_back();

                u64 readSize;
                u64 copySize;
                u64 offset;
                BufferCopyInformation& copyInfo = m_bufferCopyInformation[bufferSlot];
                copyInfo.m_target = pending.m_output;

                if (m_adjustOffset)
                {
                    offset = AZ_SIZE_ALIGN_DOWN(pending.m_offset, aznumeric_cast<u64>(m_sizeAlignment));
                    size_t bufferOffset = pending.m_offset - offset;
                    copyInfo.m_bufferOffset = bufferOffset;
                    readSize = AZStd::min(pending.m_readSize + bufferOffset, m_maxReadSize);
                    copySize = readSize - bufferOffset;
                }
                else
                {
                    offset = pending.m_offset;
                    readSize = AZStd::min(pending.m_readSize, m_maxReadSize);
                    copySize = readSize;
                }
                AZ_Assert(readSize <= m_maxReadSize, "Read size %llu in read splitter exceeds the maximum split size of %llu.",
                    readSize, m_maxReadSize);
                copyInfo.m_size = copySize;

                FileRequest* subRequest = m_context->GetNewInternalRequest();
                subRequest->CreateRead(pending.m_request, GetBufferSlot(bufferSlot), m_maxReadSize, data->m_path,
                    offset, readSize, data->m_sharedRead);
                subRequest->SetCompletionCallback([this, bufferSlot]([[maybe_unused]] FileRequest& request)
                    {
                        AZ_PROFILE_FUNCTION(AzCore);

                        BufferCopyInformation& copyInfo = m_bufferCopyInformation[bufferSlot];
                        memcpy(copyInfo.m_target, GetBufferSlot(bufferSlot) + copyInfo.m_bufferOffset, copyInfo.m_size);
                        m_availableBufferSlots.push_back(bufferSlot);

                        QueuePendingRequest();
                    });
                m_next->QueueRequest(subRequest);

                pending.m_offset += copySize;
                pending.m_readSize -= copySize;
                pending.m_outputSize -= copySize;
                pending.m_output += copySize;
            }
            else
            {
                // Add a wait to make sure the read request isn't completed if all sub-reads completed before
                // the ReadSplitter has had a chance to add new sub-reads to complete the read.
                if (pending.m_wait == nullptr)
                {
                    pending.m_wait = m_context->GetNewInternalRequest();
                    pending.m_wait->CreateWait(pending.m_request);
                }
                return false;
            }
        }
        if (pending.m_wait != nullptr)
        {
            m_context->MarkRequestAsCompleted(pending.m_wait);
            pending.m_wait = nullptr;
        }
        return true;
    }

    void ReadSplitter::QueuePendingRequest()
    {
        if (!m_pendingReads.empty())
        {
            PendingRead& pendingRead = m_pendingReads.front();
            if (pendingRead.m_isBuffered ? QueueBufferedRead(pendingRead) : QueueAlignedRead(pendingRead))
            {
                m_pendingReads.pop_front();
            }
        }
    }

    void ReadSplitter::UpdateStatus(Status& status) const
    {
        StreamStackEntry::UpdateStatus(status);
        if (m_bufferSize > 0)
        {
            s32 numAvailableSlots = aznumeric_cast<s32>(m_availableBufferSlots.size());
            status.m_numAvailableSlots = AZStd::min(status.m_numAvailableSlots, numAvailableSlots);
            status.m_isIdle = status.m_isIdle && m_pendingReads.empty();
        }
    }

    void ReadSplitter::CollectStatistics(AZStd::vector<Statistic>& statistics) const
    {
        statistics.push_back(Statistic::CreateFloat(m_name, AvgNumSubReadsName, m_averageNumSubReadsStat.GetAverage()));
        statistics.push_back(Statistic::CreatePercentage(m_name, AlignedReadsName, m_alignedReadsStat.GetAverage()));
        statistics.push_back(Statistic::CreateInteger(m_name, NumAvailableBufferSlotsName, aznumeric_caster(m_availableBufferSlots.size())));
        statistics.push_back(Statistic::CreateInteger(m_name, NumPendingReadsName, aznumeric_caster(m_pendingReads.size())));
        StreamStackEntry::CollectStatistics(statistics);
    }

    void ReadSplitter::InitializeBuffer()
    {
        // Lazy initialization to avoid allocating memory if it's not needed.
        if (m_bufferSize != 0 && m_buffer == nullptr)
        {
            m_buffer = reinterpret_cast<u8*>(AZ::AllocatorInstance<AZ::SystemAllocator>::Get().Allocate(
                m_bufferSize, m_memoryAlignment, 0, "AZ::IO::Streamer ReadSplitter", __FILE__, __LINE__));
        }
    }

    u8* ReadSplitter::GetBufferSlot(size_t index)
    {
        AZ_Assert(m_buffer != nullptr, "A buffer slot was requested by the Read Splitter before the buffer was initialized.");
        return m_buffer + (index * m_maxReadSize);
    }
} // namespace AZ::IO
