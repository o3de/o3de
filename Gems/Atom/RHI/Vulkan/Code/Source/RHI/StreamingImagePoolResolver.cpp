/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/std/containers/fixed_unordered_map.h>
#include <AzCore/std/parallel/lock.h>
#include <RHI/StreamingImagePoolResolver.h>
#include <RHI/Device.h>
#include <RHI/Image.h>
#include <RHI/CommandList.h>
#include <Atom/RHI.Reflect/Vulkan/Conversion.h>

namespace AZ
{
    namespace Vulkan
    {
        StreamingImagePoolResolver::StreamingImagePoolResolver(Device& device)
            : ResourcePoolResolver(device)
        {
        }

        void StreamingImagePoolResolver::QueuePrologueTransitionBarriers(CommandList& commandList)
        {
            auto& device = static_cast<Device&>(commandList.GetDevice());
            AZStd::unordered_map<uint32_t, AZStd::vector<VkImageMemoryBarrier>> barriers;
            // Filter pipeline stage and access flags to the supported ones.
            auto supportedPipelineFlags = device.GetCommandQueueContext().GetSupportedPipelineStages(commandList.GetQueueFamilyIndex());
            for (const BarrierInfo& barrierInfo : m_prologueBarriers)
            {
                const Image& image = *barrierInfo.m_image;
                RHI::ImageBindFlags bindFlags = image.GetDescriptor().m_bindFlags;
                VkPipelineStageFlags destinationStageFlags = RHI::FilterBits(GetResourcePipelineStateFlags(bindFlags), supportedPipelineFlags);
                VkImageMemoryBarrier barrier{};
                barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                barrier.pNext = nullptr;
                barrier.srcAccessMask = 0;
                barrier.dstAccessMask = RHI::FilterBits(GetResourceAccessFlags(bindFlags), GetSupportedAccessFlags(destinationStageFlags));
                barrier.oldLayout = barrierInfo.m_oldLayout;
                barrier.newLayout = barrierInfo.m_newLayout;
                barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.image = image.GetNativeImage();
                barrier.subresourceRange.aspectMask = image.GetImageAspectFlags();
                barrier.subresourceRange.baseMipLevel = barrierInfo.m_baseMipmap;
                barrier.subresourceRange.levelCount = barrierInfo.m_levelCount;
                barrier.subresourceRange.baseArrayLayer = 0;
                barrier.subresourceRange.layerCount = image.GetDescriptor().m_arraySize;
                barriers[static_cast<uint32_t>(destinationStageFlags)].push_back(barrier);
            }            

            for (auto it = barriers.begin(); it != barriers.end(); ++it)
            {
                device.GetContext().CmdPipelineBarrier(
                    commandList.GetNativeCommandBuffer(),
                    VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                    it->first,
                    VK_DEPENDENCY_BY_REGION_BIT,
                    0,
                    nullptr,
                    0,
                    nullptr,
                    static_cast<uint32_t>(it->second.size()),
                    it->second.data());
            }

            m_prologueBarriers.clear();
         }

        void StreamingImagePoolResolver::OnResourceShutdown(const RHI::DeviceResource& resource)
        {
            AZStd::lock_guard<AZStd::mutex> lock(m_barrierLock);
            auto predicate = [&resource](const BarrierInfo& barrierInfo)
            {
                return barrierInfo.m_image == static_cast<const Image*>(&resource);
            };

            AZStd::remove_if(m_prologueBarriers.begin(), m_prologueBarriers.end(), predicate);
        }

        void StreamingImagePoolResolver::AddImageTransitionBarrier(Image& image, uint32_t baseMipmap, uint32_t levelCount)
        {
            AZ_Assert(levelCount > 0, "Invalid level count");
            AZStd::lock_guard<AZStd::mutex> lock(m_barrierLock);
            BarrierInfo barrier;
            barrier.m_image = &image;
            barrier.m_baseMipmap = baseMipmap;
            barrier.m_levelCount = levelCount;
            barrier.m_newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            auto range = RHI::ImageSubresourceRange(
                barrier.m_baseMipmap,
                barrier.m_baseMipmap + barrier.m_levelCount - 1,
                0,
                image.GetDescriptor().m_arraySize - 1);

            auto layouts = image.GetLayouts(range);
            for (const auto& layoutInfo : layouts)
            {
                barrier.m_oldLayout = layoutInfo.m_value;
                barrier.m_baseMipmap = layoutInfo.m_interval.m_mipSliceMin;
                barrier.m_levelCount = layoutInfo.m_interval.m_mipSliceMax - layoutInfo.m_interval.m_mipSliceMin + 1;
                m_prologueBarriers.push_back(barrier);
            }

            image.SetLayout(range, barrier.m_newLayout);
        }
    }
}
