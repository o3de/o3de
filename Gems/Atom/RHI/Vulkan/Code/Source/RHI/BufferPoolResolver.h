/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/containers/list.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/parallel/mutex.h>
#include <RHI/Buffer.h>
#include <RHI/BufferPool.h>
#include <RHI/CommandList.h>
#include <RHI/Device.h>
#include <RHI/ResourcePoolResolver.h>

namespace AZ
{
    namespace Vulkan
    {
        class BufferPoolResolver final
            : public ResourcePoolResolver
        {
            using Base = RHI::ResourcePoolResolver;

        public:
            AZ_RTTI(BufferPoolResolver, "A8752FD0-9832-4015-B3B8-25853C6E9BF7", Base);
            AZ_CLASS_ALLOCATOR(BufferPoolResolver, AZ::SystemAllocator);

            BufferPoolResolver(Device& device, const RHI::BufferPoolDescriptor& descriptor);

            ///Get a pointer to write a content to upload to GPU.
            void* MapBuffer(const RHI::DeviceBufferMapRequest& request);

            //////////////////////////////////////////////////////////////////////
            ///ResourcePoolResolver
            void Compile(const RHI::HardwareQueueClass hardwareClass) override;
            void Resolve(CommandList& commandList) override;
            void Deactivate() override;
            void OnResourceShutdown(const RHI::DeviceResource& resource) override;
            void QueuePrologueTransitionBarriers(CommandList&, BarrierTypeFlags mask) override;
            void QueueEpilogueTransitionBarriers(CommandList&, BarrierTypeFlags mask) override;
            //////////////////////////////////////////////////////////////////////

        private:
            struct BufferUploadPacket
            {
                Buffer* m_attachmentBuffer = nullptr;
                RHI::Ptr<Buffer> m_stagingBuffer;
                VkDeviceSize m_byteOffset = 0;
                VkDeviceSize m_byteSize = 0;
            };

            struct BarrierInfo
            {
                VkPipelineStageFlags m_srcStageMask = {};
                VkPipelineStageFlags m_dstStageMask = {};
                VkBufferMemoryBarrier m_barrier = {};
            };

            void EmmitBarriers(CommandList& commandList, const AZStd::vector<BarrierInfo>& barriers, BarrierTypeFlags mask) const;

            AZStd::mutex m_uploadPacketsLock;
            AZStd::vector<BufferUploadPacket> m_uploadPackets;
            AZStd::vector<BarrierInfo> m_prologueBarriers;
            AZStd::vector<BarrierInfo> m_epilogueBarriers;
        };
    }
}
