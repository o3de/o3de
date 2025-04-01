/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <RHI/Image.h>
#include <RHI/ImagePool.h>
#include <RHI/ImagePoolResolver.h>
#include <RHI/MemoryView.h>
#include <RHI/Conversion.h>
#include <Atom/RHI.Reflect/Bits.h>

namespace AZ
{
    namespace Vulkan
    {
        ImagePoolResolver::ImagePoolResolver(Device& device)
            : ResourcePoolResolver(device)
        {
        }

        RHI::ResultCode ImagePoolResolver::UpdateImage(const RHI::DeviceImageUpdateRequest& request, size_t& bytesTransferred)
        {
            auto* image = static_cast<Image*>(request.m_image);
            const auto& subresourceLayout = request.m_sourceSubresourceLayout;

            const uint32_t stagingRowPitch = subresourceLayout.m_bytesPerRow;
            const uint32_t stagingSlicePitch = subresourceLayout.m_bytesPerImage;
            const uint32_t stagingSize = stagingSlicePitch * subresourceLayout.m_size.m_depth;

            bytesTransferred = 0;

            RHI::Ptr<Buffer> stagingBuffer = m_device.AcquireStagingBuffer(stagingSize);
            if (!stagingBuffer)
            {
                return RHI::ResultCode::OutOfMemory;
            }

            m_uploadPacketsLock.lock();
            m_uploadPackets.emplace_back();
            ImageUploadPacket& uploadRequest = m_uploadPackets.back();
            m_uploadPacketsLock.unlock();

            uploadRequest.m_destinationImage = image;
            uploadRequest.m_subresourceLayout = request.m_sourceSubresourceLayout;
            uploadRequest.m_stagingBuffer = stagingBuffer;
            uploadRequest.m_subresource = request.m_imageSubresource;
            uploadRequest.m_offset = request.m_imageSubresourcePixelOffset;

            const uint8_t* subresourceDataStart = reinterpret_cast<const uint8_t*>(request.m_sourceData);
            uint8_t* stagingDataStart = reinterpret_cast<uint8_t*>(stagingBuffer->GetBufferMemoryView()->Map(RHI::HostMemoryAccess::Write));
            AZ_Assert(stagingDataStart, "Failed to map staging buffer for updating image");
            for (uint32_t depth = 0; depth < subresourceLayout.m_size.m_depth; ++depth)
            {
                uint8_t* stagingSliceDataStart = stagingDataStart + depth * stagingSlicePitch;
                const uint8_t* subresourceSliceDataStart = subresourceDataStart + depth * subresourceLayout.m_bytesPerImage;

                for (uint32_t row = 0; row < subresourceLayout.m_rowCount; ++row)
                {
                    ::memcpy(stagingSliceDataStart + row * stagingRowPitch,
                        subresourceSliceDataStart + row * subresourceLayout.m_bytesPerRow,
                        stagingRowPitch);
                }
            }
            stagingBuffer->GetBufferMemoryView()->Unmap(RHI::HostMemoryAccess::Write);
            bytesTransferred = stagingSize;
            return RHI::ResultCode::Success;
        }

        void ImagePoolResolver::Compile(const RHI::HardwareQueueClass hardwareClass)
        {
            for (const auto& request : m_uploadPackets)
            {
                BarrierInfo barrierInfo = {};
                barrierInfo.m_srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                barrierInfo.m_dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;

                RHI::ImageAspectFlags imageAspectFlags = static_cast<RHI::ImageAspectFlags>(AZ_BIT(static_cast<uint32_t>(request.m_subresource.m_aspect)));

                VkImageMemoryBarrier& barrier = barrierInfo.m_barrier;
                barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                barrier.srcAccessMask = 0;
                barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                barrier.image = request.m_destinationImage->GetNativeImage();
                barrier.subresourceRange.aspectMask = RHI::FilterBits(request.m_destinationImage->GetImageAspectFlags(), ConvertImageAspectFlags(imageAspectFlags));
                barrier.subresourceRange.baseMipLevel = request.m_subresource.m_mipSlice;
                barrier.subresourceRange.levelCount = 1;
                barrier.subresourceRange.baseArrayLayer = request.m_subresource.m_arraySlice;
                barrier.subresourceRange.layerCount = 1;

                auto findId = AZStd::find(m_prologueBarriers.begin(), m_prologueBarriers.end(), barrierInfo);
                if (findId != m_prologueBarriers.end())
                {
                    continue;
                }

                m_prologueBarriers.push_back(barrierInfo);
                Image& image = *request.m_destinationImage;
                // If it's an attachment leave it in the transfer destination layout. The graph will take ownership.
                if (!image.IsAttachment())
                {
                    const RHI::ImageBindFlags bindFlags = image.GetDescriptor().m_bindFlags;

                    VkPipelineStageFlags destPipelineFlags = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
                    VkAccessFlags destAccessFlags = 0;
                    // Filter pipeline stage and access flags to the supported ones.
                    auto supportedPipelineFlags = m_device.GetCommandQueueContext().GetCommandQueue(hardwareClass).GetSupportedPipelineStages();
                    destPipelineFlags = RHI::FilterBits(GetResourcePipelineStateFlags(bindFlags), supportedPipelineFlags);
                    destAccessFlags = RHI::FilterBits(GetResourceAccessFlags(bindFlags), GetSupportedAccessFlags(destPipelineFlags));

                    barrierInfo.m_srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
                    barrierInfo.m_dstStageMask = destPipelineFlags;

                    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                    barrier.dstAccessMask = destAccessFlags;

                    m_epiloqueBarriers.push_back(barrierInfo);
                }

                image.SetLayout(barrier.newLayout);
                image.SetPipelineAccess({ barrierInfo.m_dstStageMask, barrier.dstAccessMask });
            }
        }

        void ImagePoolResolver::Resolve(CommandList& commandList)
        {
            auto& device = static_cast<Device&>(commandList.GetDevice());
            for (const auto& packet : m_uploadPackets)
            {
                const RHI::DeviceImageSubresourceLayout& subresourceLayout = packet.m_subresourceLayout;
                const uint32_t stagingRowPitch = subresourceLayout.m_bytesPerRow;
                const uint32_t stagingSlicePitch = subresourceLayout.m_rowCount * stagingRowPitch;

                RHI::DeviceCopyBufferToImageDescriptor copyDescriptor;
                copyDescriptor.m_sourceBuffer = packet.m_stagingBuffer.get();
                copyDescriptor.m_sourceOffset = 0;
                copyDescriptor.m_sourceBytesPerRow = stagingRowPitch;
                copyDescriptor.m_sourceBytesPerImage = stagingSlicePitch;
                copyDescriptor.m_sourceFormat = packet.m_destinationImage->GetDescriptor().m_format;
                copyDescriptor.m_sourceSize = subresourceLayout.m_size;
                copyDescriptor.m_destinationImage = packet.m_destinationImage;
                copyDescriptor.m_destinationSubresource.m_mipSlice = packet.m_subresource.m_mipSlice;
                copyDescriptor.m_destinationSubresource.m_arraySlice = packet.m_subresource.m_arraySlice;
                copyDescriptor.m_destinationOrigin.m_left = packet.m_offset.m_left;
                copyDescriptor.m_destinationOrigin.m_top = packet.m_offset.m_top;
                copyDescriptor.m_destinationOrigin.m_front = packet.m_offset.m_front;

                commandList.Submit(RHI::DeviceCopyItem(copyDescriptor));
                
                device.QueueForRelease(packet.m_stagingBuffer);
            }
        }

        void ImagePoolResolver::Deactivate()
        {
            m_uploadPackets.clear();
            m_prologueBarriers.clear();
            m_epiloqueBarriers.clear();
        }

        void ImagePoolResolver::OnResourceShutdown(const RHI::DeviceResource& resource)
        {
            AZStd::lock_guard<AZStd::mutex> lock(m_uploadPacketsLock);
            const Image* image = static_cast<const Image*>(&resource);
            auto predicatePacket = [&image](const ImageUploadPacket& packet)
            {
                return packet.m_destinationImage == image;
            };

            auto predicateBarrier = [&image](const BarrierInfo& barrierInfo)
            {
                return barrierInfo.m_barrier.image == image->GetNativeImage();
            };

            m_uploadPackets.erase(AZStd::remove_if(m_uploadPackets.begin(), m_uploadPackets.end(), predicatePacket), m_uploadPackets.end());
            m_prologueBarriers.erase(AZStd::remove_if(m_prologueBarriers.begin(), m_prologueBarriers.end(), predicateBarrier), m_prologueBarriers.end());
            m_epiloqueBarriers.erase(AZStd::remove_if(m_epiloqueBarriers.begin(), m_epiloqueBarriers.end(), predicateBarrier), m_epiloqueBarriers.end());
        }

        void ImagePoolResolver::QueuePrologueTransitionBarriers(CommandList& commandList, BarrierTypeFlags mask)
        {
            EmmitBarriers(commandList, m_prologueBarriers, mask);
        }

        void ImagePoolResolver::QueueEpilogueTransitionBarriers(CommandList& commandList, BarrierTypeFlags mask)
        {
            EmmitBarriers(commandList, m_epiloqueBarriers, mask);
        }

        void ImagePoolResolver::EmmitBarriers(
            CommandList& commandList, const AZStd::vector<BarrierInfo>& barriers, BarrierTypeFlags mask) const
        {
            for (const auto& barrierInfo : barriers)
            {
                if (!RHI::CheckBitsAll(mask, ConvertBarrierType(VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER)))
                {
                    continue;
                }

                m_device.GetContext().CmdPipelineBarrier(
                    commandList.GetNativeCommandBuffer(),
                    barrierInfo.m_srcStageMask,
                    barrierInfo.m_dstStageMask,
                    VK_DEPENDENCY_BY_REGION_BIT,
                    0,
                    nullptr,
                    0,
                    nullptr,
                    1,
                    &barrierInfo.m_barrier);
            }
        }
    }
}
