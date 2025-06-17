/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <RHI/Vulkan.h>
#include <Atom/RHI.Reflect/Vulkan/Conversion.h>
#include <Atom/RHI/DeviceIndexBufferView.h>
#include <Atom/RHI/DeviceQuery.h>
#include <Atom/RHI.Reflect/AttachmentEnums.h>
#include <Atom/RHI/IndexBufferView.h>
#include <Atom/RHI/Query.h>
#include <Atom/RHI/PipelineStateDescriptor.h>

#include <vma/vk_mem_alloc.h>

namespace AZ
{
    namespace RHI
    {
        class ScopeAttachment;
        class ImageScopeAttachment;
        struct ScopeAttachmentDescriptor;
    } // namespace RHI

    namespace Vulkan
    {
        class Image;
        class Device;

        VkIndexType ConvertIndexBufferFormat(RHI::IndexFormat indexFormat);
        VkQueryControlFlags ConvertQueryControlFlags(RHI::QueryControlFlags flags);

        //! @param shadingRateAttachmentUsageFlags Only relevant when @scopeAttachmentUsage == RHI::ScopeAttachmentUsage::ShadingRate.
        VkPipelineStageFlags GetResourcePipelineStateFlags(
            AZ::RHI::ScopeAttachmentUsage scopeAttachmentUsage,
            AZ::RHI::ScopeAttachmentStage scopeAttachmentStage,
            AZ::RHI::HardwareQueueClass scopeQueueClass = AZ::RHI::HardwareQueueClass::Graphics,
            VkImageUsageFlags shadingRateAttachmentUsageFlags = 0);
        VkPipelineStageFlags GetResourcePipelineStateFlags(const RHI::ScopeAttachment& scopeAttachment);
        VkPipelineStageFlags GetResourcePipelineStateFlags(const RHI::BufferBindFlags& bindFlags);
        VkPipelineStageFlags GetResourcePipelineStateFlags(const RHI::ImageBindFlags& bindFlags);
        VkPipelineStageFlags GetSupportedPipelineStages(RHI::PipelineStateType type);

        //! @param shadingRateAttachmentUsageFlags Only relevant when @scopeAttachmentUsage == RHI::ScopeAttachmentUsage::ShadingRate.
        VkAccessFlags GetResourceAccessFlags(
            AZ::RHI::ScopeAttachmentAccess access,
            AZ::RHI::ScopeAttachmentUsage scopeAttachmentUsage,
            VkImageUsageFlags shadingRateAttachmentUsageFlags = 0);
        VkAccessFlags GetResourceAccessFlags(const RHI::ScopeAttachment& scopeAttachment);
        VkAccessFlags GetResourceAccessFlags(const RHI::BufferBindFlags& bindFlags);
        VkAccessFlags GetResourceAccessFlags(const RHI::ImageBindFlags& bindFlags);
        VkImageLayout GetImageAttachmentLayout(const RHI::ImageScopeAttachment& imageScopeAttachment);
        bool HasExplicitClear(const RHI::ScopeAttachment& scopeAttachment);
        VmaAllocationCreateInfo GetVmaAllocationCreateInfo(const RHI::HeapMemoryLevel level);
        VkImageLayout CombineImageLayout(VkImageLayout lhs, VkImageLayout rhs);
        VkImageLayout FilterImageLayout(VkImageLayout layout, RHI::ImageAspectFlags aspectFlags);
        VkAttachmentLoadOp ConvertAttachmentLoadAction(RHI::AttachmentLoadAction loadAction, const Device& device);
        VkAttachmentStoreOp ConvertAttachmentStoreAction(RHI::AttachmentStoreAction storeAction, const Device& device);
        RHI::AttachmentLoadAction CombineLoadOp(RHI::AttachmentLoadAction currentOp, RHI::AttachmentLoadAction newOp);
        RHI::AttachmentStoreAction CombineStoreOp(RHI::AttachmentStoreAction currentOp, RHI::AttachmentStoreAction newOp);
        BarrierTypeFlags ConvertBarrierType(VkStructureType type);

    }
}
