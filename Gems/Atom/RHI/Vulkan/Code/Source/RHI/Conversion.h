/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/Vulkan/Conversion.h>
#include <Atom/RHI/SingleDeviceIndexBufferView.h>
#include <Atom/RHI/SingleDeviceQuery.h>
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
        VkIndexType ConvertIndexBufferFormat(RHI::IndexFormat indexFormat);
        VkQueryControlFlags ConvertQueryControlFlags(RHI::QueryControlFlags flags);
        VkPipelineStageFlags GetResourcePipelineStateFlags(const RHI::ScopeAttachment& scopeAttachment);
        VkPipelineStageFlags GetResourcePipelineStateFlags(const RHI::BufferBindFlags& bindFlags);
        VkPipelineStageFlags GetResourcePipelineStateFlags(const RHI::ImageBindFlags& bindFlags);
        VkPipelineStageFlags GetSupportedPipelineStages(RHI::PipelineStateType type);
        VkAccessFlags GetResourceAccessFlags(const RHI::ScopeAttachment& scopeAttachment);
        VkAccessFlags GetResourceAccessFlags(const RHI::BufferBindFlags& bindFlags);
        VkAccessFlags GetResourceAccessFlags(const RHI::ImageBindFlags& bindFlags);
        VkImageLayout GetImageAttachmentLayout(const RHI::ImageScopeAttachment& imageScopeAttachment);
        bool HasExplicitClear(const RHI::ScopeAttachment& scopeAttachment, const RHI::ScopeAttachmentDescriptor& descriptor);
        VmaAllocationCreateInfo GetVmaAllocationCreateInfo(const RHI::HeapMemoryLevel level);
    }
}
