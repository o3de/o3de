/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/IO/Streamer/Statistics.h>
#include <AzCore/IO/Streamer/StreamStackEntry.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Statistics/RunningStatistic.h>
#include <AzCore/std/containers/deque.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/limits.h>

namespace AZ
{
    namespace IO
    {
        namespace Requests
        {
            struct ReportData;
        } // namespace Requests

        struct ReadSplitterConfig final :
            public IStreamerStackConfig
        {
            AZ_RTTI(AZ::IO::ReadSplitterConfig, "{EDDD6CE5-D7BC-4FAB-8EBA-68F5C0390B05}", IStreamerStackConfig);
            AZ_CLASS_ALLOCATOR(ReadSplitterConfig, AZ::SystemAllocator);

            ~ReadSplitterConfig() override = default;
            AZStd::shared_ptr<StreamStackEntry> AddStreamStackEntry(
                const HardwareInformation& hardware, AZStd::shared_ptr<StreamStackEntry> parent) override;
            static void Reflect(AZ::ReflectContext* context);

            //! Dynamic options for the split size.
            //! It's possible to set static sizes or use the names from this enum to have AZ::IO::Streamer automatically fill in the sizes.
            //! Fixed sizes are set through the Settings Registry with "SplitSize": 524288, while dynamic values are set like
            //! "SplitSize": "MemoryAlignment". In the latter case AZ::IO::Streamer will use the available hardware information and fill
            //! in the actual value.
            enum SplitSize : u32
            {
                MaxTransfer = AZStd::numeric_limits<u32>::max(),
                MemoryAlignment = MaxTransfer - 1 //!< The size of the minimal memory requirement of the storage device.
            };

            //! The size of the internal buffer that's used if reads need to be aligned.
            u32 m_bufferSizeMib{ 5 };
            //! The size at which reads are split. This can either be a fixed value that's explicitly supplied or a
            //! dynamic value that's retrieved from the provided hardware.
            SplitSize m_splitSize{ 20 };
            //! If set to true the read splitter will adjust offsets to align to the required size alignment. This should
            //! be disabled if the read splitter is in front of a cache like the block cache as it would negate the cache's
            //! ability to cache data.
            bool m_adjustOffset{ true };
            //! Whether or not to split reads even if they meet the alignment requirements. This is recommended for devices that
            //! can't cancel their requests.
            bool m_splitAlignedRequests{ true };
        };

        class ReadSplitter
            : public StreamStackEntry
        {
        public:
             ReadSplitter(u64 maxReadSize, u32 memoryAlignment, u32 sizeAlignment, size_t bufferSize,
                 bool adjustOffset, bool splitAlignedRequests);
            ~ReadSplitter() override;

            void QueueRequest(FileRequest* request) override;
            void UpdateStatus(Status& status) const override;

            void CollectStatistics(AZStd::vector<Statistic>& statistics) const override;

        private:
            struct PendingRead
            {
                FileRequest* m_request{ nullptr };
                FileRequest* m_wait{ nullptr };
                u8* m_output{ nullptr };
                u64 m_outputSize{ 0 };
                u64 m_readSize{ 0 };
                u64 m_offset : 63;
                u64 m_isBuffered : 1;
            };

            struct BufferCopyInformation
            {
                u8* m_target{ nullptr };
                size_t m_size{ 0 };
                size_t m_bufferOffset{ 0 };
            };

            void QueueAlignedRead(FileRequest* request);
            bool QueueAlignedRead(PendingRead& pending);
            void QueueBufferedRead(FileRequest* request);
            bool QueueBufferedRead(PendingRead& pending);
            void QueuePendingRequest();
            
            void InitializeBuffer();
            u8* GetBufferSlot(size_t index);

            void Report(const Requests::ReportData& data) const;

            AZ::Statistics::RunningStatistic m_averageNumSubReadsStat;
            AZ::Statistics::RunningStatistic m_alignedReadsStat;
            AZStd::unique_ptr<BufferCopyInformation[]> m_bufferCopyInformation;
            AZStd::deque<PendingRead> m_pendingReads;
            AZStd::vector<u32> m_availableBufferSlots;
            u8* m_buffer;
            size_t m_bufferSize;
            u64 m_maxReadSize;
            u32 m_memoryAlignment;
            u32 m_sizeAlignment;
            bool m_adjustOffset;
            bool m_splitAlignedRequests;
        };
    } // namespace IO
    AZ_TYPE_INFO_SPECIALIZE(AZ::IO::ReadSplitterConfig::SplitSize, "{2D6B3695-3F62-42EB-9DFE-FBABED395C58}");
} // namesapce AZ
