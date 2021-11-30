/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/Format.h>
#include <Atom/RHI.Reflect/ClearValue.h>
#include <Atom/RHI.Reflect/AttachmentEnums.h>
#include <Atom/RHI.Reflect/BufferDescriptor.h>
#include <Atom/RHI.Reflect/ClearValue.h>
#include <Atom/RHI.Reflect/Format.h>
#include <Atom/RHI.Reflect/ImageDescriptor.h>
#include <Atom/RHI.Reflect/ImageSubresource.h>
#include <Atom/RHI.Reflect/InputStreamLayout.h>
#include <Atom/RHI.Reflect/MemoryEnums.h>
#include <Atom/RHI.Reflect/MultisampleState.h>
#include <Atom/RHI.Reflect/QueryPoolDescriptor.h>
#include <Atom/RHI.Reflect/RenderAttachmentLayout.h>
#include <Atom/RHI.Reflect/RenderStates.h>
#include <Atom/RHI.Reflect/SamplerState.h>
#include <Atom/RHI.Reflect/ShaderResourceGroupLayoutDescriptor.h>
#include <Atom/RHI.Reflect/Size.h>
#include <Atom/RHI/IndexBufferView.h>
#include <Atom/RHI/Query.h>
#include <Atom/RHI/PipelineStateDescriptor.h>
#include <RHI/RenderPass.h>

namespace AZ
{
    namespace RHI
    {
        class ImageScopeAttachment;
    }

    namespace Vulkan
    {
        RHI::ResultCode ConvertResult(VkResult vkResult);
        RHI::Format ConvertFormat(VkFormat format);

        VkFormat ConvertFormat(RHI::Format format, bool raiseAsserts = true);
        VkImageAspectFlagBits ConvertImageAspect(RHI::ImageAspect imageAspect);
        VkImageAspectFlags ConvertImageAspectFlags(RHI::ImageAspectFlags aspectFlagMask);
        VkPrimitiveTopology ConvertTopology(RHI::PrimitiveTopology topology);
        VkQueueFlags ConvertQueueClass(RHI::HardwareQueueClass queueClass);
        VkMemoryPropertyFlags ConvertHeapMemoryLevel(RHI::HeapMemoryLevel heapMemoryLevel);
        void FillStencilOpState(const RHI::StencilOpState& stencilOpState, VkStencilOpState& vkStencilOpState);
        VkStencilOp ConvertStencilOp(RHI::StencilOp op);
        VkCompareOp ConvertComparisonFunction(RHI::ComparisonFunc func);
        void FillColorBlendAttachmentState(const RHI::TargetBlendState& targetBlendState, VkPipelineColorBlendAttachmentState& colorBlendAttachmentState);
        VkBlendFactor ConvertBlendFactor(const RHI::BlendFactor& blendFactor);
        VkBlendOp ConvertBlendOp(const RHI::BlendOp blendOp);
        VkColorComponentFlags ConvertComponentFlags(uint8_t flags);
        VkSampleCountFlagBits ConvertSampleCount(uint16_t samples);
        VkAttachmentLoadOp ConvertAttachmentLoadAction(RHI::AttachmentLoadAction loadAction);
        VkAttachmentStoreOp ConvertAttachmentStoreAction(RHI::AttachmentStoreAction storeAction);
        VkIndexType ConvertIndexBufferFormat(RHI::IndexFormat indexFormat);
        void FillClearValue(const RHI::ClearValue& rhiClearValue, VkClearValue& vulkanClearValue);
        VkFilter ConvertFilterMode(RHI::FilterMode filterMode);
        VkSamplerAddressMode ConvertAddressMode(RHI::AddressMode addressMode);
        VkImageType ConvertToImageType(RHI::ImageDimension dimension);
        VkExtent3D ConvertToExtent3D(const RHI::Size& size);
        VkQueryType ConvertQueryType(const RHI::QueryType type);
        VkQueryControlFlags ConvertQueryControlFlags(RHI::QueryControlFlags flags);
        VkQueryPipelineStatisticFlags ConvertQueryPipelineStatisticMask(RHI::PipelineStatisticsFlags mask);
        VkShaderStageFlagBits ConvertShaderStage(RHI::ShaderStage stage, uint32_t subStage = 0);
        VkShaderStageFlags ConvertShaderStageMask(uint32_t shaderStageMask);
        RenderPass::Descriptor ConvertRenderAttachmentLayout(const RHI::RenderAttachmentLayout& layout, const RHI::MultisampleState& multisampleState);
        VkPipelineStageFlags GetResourcePipelineStateFlags(const RHI::ScopeAttachment& scopeAttachment);
        VkPipelineStageFlags GetResourcePipelineStateFlags(const RHI::BufferBindFlags& bindFlags);
        VkPipelineStageFlags GetResourcePipelineStateFlags(const RHI::ImageBindFlags& bindFlags);
        VkPipelineStageFlags GetSupportedPipelineStages(RHI::PipelineStateType type);
        VkAccessFlags GetResourceAccessFlags(const RHI::ScopeAttachment& scopeAttachment);
        VkAccessFlags GetResourceAccessFlags(const RHI::BufferBindFlags& bindFlags);
        VkAccessFlags GetResourceAccessFlags(const RHI::ImageBindFlags& bindFlags);
        VkImageLayout GetImageAttachmentLayout(const RHI::ImageScopeAttachment& imageScopeAttachment);
        VkBufferUsageFlags GetBufferUsageFlagBits(RHI::BufferBindFlags bindFlags);
        VkSampleLocationEXT ConvertSampleLocation(const RHI::SamplePosition& position);
        
        VkImageUsageFlags ImageUsageFlagsOfFormatFeatureFlags(VkFormatFeatureFlags formatFeatureFlags);
        VkAccessFlags GetSupportedAccessFlags(VkPipelineStageFlags pipelineStageFlags);
        bool ShouldApplyDeviceAddressBit(RHI::BufferBindFlags bindFlags);
    }
}
