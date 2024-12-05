/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <vulkan/vulkan.h>
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
#include <Atom/RHI.Reflect/QueryPoolDescriptor.h>
#include <Atom/RHI.Reflect/RenderStates.h>
#include <Atom/RHI.Reflect/SamplerState.h>
#include <Atom/RHI.Reflect/ShaderResourceGroupLayoutDescriptor.h>
#include <Atom/RHI.Reflect/ScopeAttachmentDescriptor.h>
#include <Atom/RHI.Reflect/Size.h>
#include <Atom/RHI.Reflect/VariableRateShadingEnums.h>
#include <Atom/RHI.Reflect/Vulkan/ImageViewDescriptor.h>

namespace AZ
{
    namespace Vulkan
    {
        RHI::ResultCode ConvertResult(VkResult vkResult);
        RHI::Format ConvertFormat(VkFormat format);

        VkFormat ConvertFormat(RHI::Format format, bool raiseAsserts = true);
        VkImageAspectFlagBits ConvertImageAspect(RHI::ImageAspect imageAspect);
        VkImageAspectFlags ConvertImageAspectFlags(RHI::ImageAspectFlags aspectFlagMask);
        RHI::ImageAspectFlags ConvertImageAspectFlags(VkImageAspectFlags aspectFlagMask);
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
        void FillClearValue(const RHI::ClearValue& rhiClearValue, VkClearValue& vulkanClearValue);
        VkFilter ConvertFilterMode(RHI::FilterMode filterMode);
        VkSamplerAddressMode ConvertAddressMode(RHI::AddressMode addressMode);
        VkImageType ConvertToImageType(RHI::ImageDimension dimension);
        VkExtent3D ConvertToExtent3D(const RHI::Size& size);
        VkQueryType ConvertQueryType(const RHI::QueryType type);
        VkQueryPipelineStatisticFlags ConvertQueryPipelineStatisticMask(RHI::PipelineStatisticsFlags mask);
        VkShaderStageFlagBits ConvertShaderStage(RHI::ShaderStage stage, uint32_t subStage = 0);
        VkShaderStageFlags ConvertShaderStageMask(uint32_t shaderStageMask);
        VkBufferUsageFlags GetBufferUsageFlagBits(RHI::BufferBindFlags bindFlags);
        VkSampleLocationEXT ConvertSampleLocation(const RHI::SamplePosition& position);
        VkFragmentShadingRateCombinerOpKHR ConvertShadingRateCombiner(const RHI::ShadingRateCombinerOp op);
        VkExtent2D ConvertFragmentShadingRate(const RHI::ShadingRate rate);
        RHI::ShadingRate ConvertFragmentShadingRate(const VkExtent2D rate);
        VkImageUsageFlags ImageUsageFlagsOfFormatFeatureFlags(VkFormatFeatureFlags formatFeatureFlags);
        VkAccessFlags GetSupportedAccessFlags(VkPipelineStageFlags pipelineStageFlags);
        VkComponentSwizzle ConvertComponentSwizzle(const ImageComponentMapping::Swizzle swizzle);
        VkComponentMapping ConvertComponentMapping(const ImageComponentMapping& mapping);
        RHI::ImageSubresourceRange ConvertSubresourceRange(const VkImageSubresourceRange& range);
        VkImageSubresourceRange ConvertSubresourceRange(const RHI::ImageSubresourceRange& range);
        VkPipelineStageFlags ConvertScopeAttachmentStage(const RHI::ScopeAttachmentStage& stage);
    }
}
