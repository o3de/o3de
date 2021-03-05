/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#pragma once

#include <RHI/ImagePool.h>
#include <RHI/CommandList.h>
#include <RHI/Device.h>
#include <RHI/ResourcePoolResolver.h>

namespace AZ
{
    namespace Vulkan
    {
        class Image;

        //! Provides a way to update an image content using a staging buffer.
        //! This class is in charge of doing the appropiate barrier transitions before and after
        //! updating the image.
        //!
        //! The ImagePoolResolver is part of an ImagePool and the Resolve function must be called
        //! from a scope in order to execute the image updates.
        class ImagePoolResolver final
            : public ResourcePoolResolver
        {
            using Base = RHI::ResourcePoolResolver;

        public:
            AZ_RTTI(ImagePoolResolver, "{8112B50E-E26D-4686-87A3-0757A90BE4EF}", Base);
            AZ_CLASS_ALLOCATOR(ImagePoolResolver, AZ::SystemAllocator, 0);

            ImagePoolResolver(Device& device);

            /// Uploads new content to an image subresource.
            RHI::ResultCode UpdateImage(const RHI::ImageUpdateRequest& request, size_t& bytesTransferred);

            //////////////////////////////////////////////////////////////////////
            ///ResourcePoolResolver
            void Compile(const RHI::HardwareQueueClass hardwareClass) override;
            void Resolve(CommandList& commandList) override;
            void Deactivate() override;
            void OnResourceShutdown(const RHI::Resource& resource) override;
            void QueuePrologueTransitionBarriers(CommandList& commandList) override;
            void QueueEpilogueTransitionBarriers(CommandList& commandList) override;
            //////////////////////////////////////////////////////////////////////

        private:
            struct ImageUploadPacket
            {
                Image* m_destinationImage = nullptr;
                RHI::Ptr<Buffer> m_stagingBuffer;
                RHI::ImageSubresourceLayout m_subresourceLayout;
                RHI::ImageSubresource m_subresource;
                RHI::Origin m_offset;
            };

            AZ_ASSERT_NO_ALIGNMENT_PADDING_BEGIN
            struct BarrierInfo
            {
                VkPipelineStageFlags m_srcStageMask = {};
                VkPipelineStageFlags m_dstStageMask = {};
                VkImageMemoryBarrier m_barrier = {};

                bool operator==(const BarrierInfo& other) { return ::memcmp(this, &other, sizeof(BarrierInfo)) == 0; }
            };
            AZ_ASSERT_NO_ALIGNMENT_PADDING_END

            void EmmitBarriers(CommandList& commandList, const AZStd::vector<BarrierInfo>& barriers) const;

            AZStd::mutex m_uploadPacketsLock;
            AZStd::vector<ImageUploadPacket> m_uploadPackets;
            AZStd::vector<BarrierInfo> m_prologueBarriers;
            AZStd::vector<BarrierInfo> m_epiloqueBarriers;
        };
    }
}
