/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/ImageScopeAttachment.h>
#include <Atom/RHI/Scope.h>
#include <Atom/RHI/ScopeAttachment.h>
#include <Atom/RHI/Image.h>
#include <Atom/RHI/Resource.h>
#include <Atom/RHI/DeviceImageView.h>
#include <RHI/Vulkan.h>
#include <RHI/Conversion.h>
#include <RHI/Device.h>
#include <RHI/Image.h>
#include <RHI/PhysicalDevice.h>

namespace AZ
{
    namespace Vulkan
    {
        VkIndexType ConvertIndexBufferFormat(RHI::IndexFormat indexFormat)
        {
            switch (indexFormat)
            {
            case RHI::IndexFormat::Uint16:
                return VK_INDEX_TYPE_UINT16;
            case RHI::IndexFormat::Uint32:
                return VK_INDEX_TYPE_UINT32;
            default:
                AZ_Assert(false, "IndexFormat is illegal.");
                return VK_INDEX_TYPE_UINT16;
            }
        }

        VkQueryControlFlags ConvertQueryControlFlags(RHI::QueryControlFlags flags)
        {
            VkQueryControlFlags vkFlags = 0;
            if (RHI::CheckBitsAll(flags, RHI::QueryControlFlags::PreciseOcclusion))
            {
                vkFlags |= VK_QUERY_CONTROL_PRECISE_BIT;
            }
            return vkFlags;
        }

        VkPipelineStageFlags GetResourcePipelineStateFlags(
            AZ::RHI::ScopeAttachmentUsage scopeAttachmentUsage,
            AZ::RHI::ScopeAttachmentStage scopeAttachmentStage,
            AZ::RHI::HardwareQueueClass scopeQueueClass,
            VkImageUsageFlags shadingRateAttachmentUsageFlags)
        {
            switch (scopeAttachmentUsage)
            {
            case RHI::ScopeAttachmentUsage::RenderTarget:
                return VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            case RHI::ScopeAttachmentUsage::Resolve:
                return VK_PIPELINE_STAGE_TRANSFER_BIT;
            case RHI::ScopeAttachmentUsage::DepthStencil:
                return RHI::FilterBits(
                    ConvertScopeAttachmentStage(scopeAttachmentStage),
                    static_cast<VkPipelineStageFlags>(
                        VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT));
            case RHI::ScopeAttachmentUsage::SubpassInput:
                {
                    AZ_Assert(scopeQueueClass == RHI::HardwareQueueClass::Graphics,
                        "SubpassInput attachment usage is only supported by the Graphics Queue Class.");
                    // REMARK, We remove VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR in this filter because
                    // when using subpasses only stages for the raster pipeline are supported.
                    return RHI::FilterBits(
                            ConvertScopeAttachmentStage(scopeAttachmentStage),
                            static_cast<VkPipelineStageFlags>(
                                VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT |
                                VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT |
                                VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT | VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT));
                }
            case RHI::ScopeAttachmentUsage::Shader:
                {
                    switch (scopeQueueClass)
                    {
                    case RHI::HardwareQueueClass::Graphics:
                        return RHI::FilterBits(
                            ConvertScopeAttachmentStage(scopeAttachmentStage),
                            static_cast<VkPipelineStageFlags>(
                                VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT |
                                VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT |
                                VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT | VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT |
                                VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR));
                    case RHI::HardwareQueueClass::Compute:
                        return VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
                    default:
                        AZ_Assert(false, "Invalid ScopeAttachment type when getting the resource pipeline stage flags");
                    }
                    break;
                }
            case RHI::ScopeAttachmentUsage::Copy:
                return VK_PIPELINE_STAGE_TRANSFER_BIT;
            case RHI::ScopeAttachmentUsage::Predication:
                return VK_PIPELINE_STAGE_CONDITIONAL_RENDERING_BIT_EXT;
            case RHI::ScopeAttachmentUsage::Indirect:
                return VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT;
            case RHI::ScopeAttachmentUsage::InputAssembly:
                return VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
            case RHI::ScopeAttachmentUsage::ShadingRate:
                {
                    AZ_Assert(shadingRateAttachmentUsageFlags != 0, "shading Rate Attachment UsageFlags can not be 0");
                    return
                        RHI::CheckBitsAll(
                               shadingRateAttachmentUsageFlags,
                               static_cast<VkImageUsageFlags>(VK_IMAGE_USAGE_FRAGMENT_DENSITY_MAP_BIT_EXT))
                        ? VK_PIPELINE_STAGE_FRAGMENT_DENSITY_PROCESS_BIT_EXT
                        : VK_PIPELINE_STAGE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR;
                }
            default:
                break;
            }
            return {};
        }

        VkPipelineStageFlags GetResourcePipelineStateFlags(const RHI::ScopeAttachment& scopeAttachment)
        {
            VkImageUsageFlags shadingRateAttachmentUsageFlags = 0;
            if (scopeAttachment.GetUsage() == RHI::ScopeAttachmentUsage::ShadingRate)
            {
                const RHI::ImageView* imageView = static_cast<const RHI::ImageView*>(scopeAttachment.GetResourceView());
                const int deviceIndex = scopeAttachment.GetScope().GetDeviceIndex();
                const Image& image = static_cast<const Image&>(*imageView->GetImage()->GetDeviceImage(deviceIndex));
                shadingRateAttachmentUsageFlags = image.GetUsageFlags();
            }
            return GetResourcePipelineStateFlags(
                scopeAttachment.GetUsage(), scopeAttachment.GetStage(), scopeAttachment.GetScope().GetHardwareQueueClass(),
                shadingRateAttachmentUsageFlags);
        }

        VkPipelineStageFlags GetResourcePipelineStateFlags(const RHI::BufferBindFlags& bindFlags)
        {
            VkPipelineStageFlags stagesFlags = {};
            if (RHI::CheckBitsAny(bindFlags, RHI::BufferBindFlags::InputAssembly | RHI::BufferBindFlags::DynamicInputAssembly))
            {
                stagesFlags |= VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT | VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
            }

            if (RHI::CheckBitsAny(bindFlags, RHI::BufferBindFlags::Constant) ||
                RHI::CheckBitsAny(bindFlags, RHI::BufferBindFlags::ShaderRead) ||
                RHI::CheckBitsAny(bindFlags, RHI::BufferBindFlags::ShaderWrite))
            {
                stagesFlags |= VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT |
                    VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT |
                    VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT | VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT;
            }

            if (RHI::CheckBitsAny(bindFlags, RHI::BufferBindFlags::CopyRead) ||
                RHI::CheckBitsAny(bindFlags, RHI::BufferBindFlags::CopyWrite))
            {
                stagesFlags |= VK_PIPELINE_STAGE_TRANSFER_BIT;
            }

            if (RHI::CheckBitsAny(bindFlags, RHI::BufferBindFlags::Predication))
            {
                stagesFlags |= VK_PIPELINE_STAGE_CONDITIONAL_RENDERING_BIT_EXT;
            }

            if (RHI::CheckBitsAny(bindFlags, RHI::BufferBindFlags::Indirect))
            {
                stagesFlags |= VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT;
            }

            return stagesFlags;
        }

        VkPipelineStageFlags GetResourcePipelineStateFlags(const RHI::ImageBindFlags& bindFlags)
        {
            VkPipelineStageFlags stagesFlags = {};
            if (RHI::CheckBitsAny(bindFlags, RHI::ImageBindFlags::Color))
            {
                stagesFlags |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            }

            if (RHI::CheckBitsAny(bindFlags, RHI::ImageBindFlags::DepthStencil))
            {
                stagesFlags |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
            }

            if (RHI::CheckBitsAny(bindFlags, RHI::ImageBindFlags::CopyWrite | RHI::ImageBindFlags::CopyRead))
            {
                stagesFlags |= VK_PIPELINE_STAGE_TRANSFER_BIT;
            }

            if (RHI::CheckBitsAny(bindFlags, RHI::ImageBindFlags::ShaderRead) ||
                RHI::CheckBitsAny(bindFlags, RHI::ImageBindFlags::ShaderWrite))
            {
                stagesFlags |= VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT |
                    VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT |
                    VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT | VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT;
            }

            if (RHI::CheckBitsAny(bindFlags, RHI::ImageBindFlags::ShadingRate))
            {
                stagesFlags |=
                    VK_PIPELINE_STAGE_FRAGMENT_DENSITY_PROCESS_BIT_EXT | VK_PIPELINE_STAGE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR;
            }

            return stagesFlags;
        }

        VkPipelineStageFlags GetSupportedPipelineStages(RHI::PipelineStateType type)
        {
            // These stages don't need any special queue to be supported.
            VkPipelineStageFlags flags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT | VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT |
                VK_PIPELINE_STAGE_HOST_BIT | VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

            switch (type)
            {
            case RHI::PipelineStateType::Draw:
                flags |= VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT | VK_PIPELINE_STAGE_VERTEX_INPUT_BIT | VK_PIPELINE_STAGE_VERTEX_SHADER_BIT |
                    VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT | VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT |
                    VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT |
                    VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT |
                    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT |
                    VK_PIPELINE_STAGE_CONDITIONAL_RENDERING_BIT_EXT | VK_PIPELINE_STAGE_SHADING_RATE_IMAGE_BIT_NV |
                    VK_PIPELINE_STAGE_TASK_SHADER_BIT_NV | VK_PIPELINE_STAGE_FRAGMENT_DENSITY_PROCESS_BIT_EXT |
                    VK_PIPELINE_STAGE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR | VK_PIPELINE_STAGE_MESH_SHADER_BIT_NV |
                    VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR;
                break;
            case RHI::PipelineStateType::Dispatch:
                flags |= VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
                break;
            default:
                AZ_Assert(false, "Invalid pipeline state type %d", type);
                break;
            }

            return flags;
        }

        VkAccessFlags GetResourceAccessFlags(
            AZ::RHI::ScopeAttachmentAccess access,
            AZ::RHI::ScopeAttachmentUsage scopeAttachmentUsage,
            VkImageUsageFlags shadingRateAttachmentUsageFlags)
        {
            VkAccessFlags accessFlags = {};
            switch (scopeAttachmentUsage)
            {
            case RHI::ScopeAttachmentUsage::RenderTarget:
                accessFlags |= RHI::CheckBitsAny(access, RHI::ScopeAttachmentAccess::Write)
                    ? VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
                    : accessFlags;
                accessFlags |= RHI::CheckBitsAny(access, RHI::ScopeAttachmentAccess::Read)
                    ? VK_ACCESS_COLOR_ATTACHMENT_READ_BIT
                    : accessFlags;
                break;
            case RHI::ScopeAttachmentUsage::Resolve:
                accessFlags |= VK_ACCESS_TRANSFER_WRITE_BIT;
                break;
            case RHI::ScopeAttachmentUsage::DepthStencil:
                accessFlags |= RHI::CheckBitsAny(access, RHI::ScopeAttachmentAccess::Write)
                    ? VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT
                    : accessFlags;
                accessFlags |= RHI::CheckBitsAny(access, RHI::ScopeAttachmentAccess::Read)
                    ? VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT
                    : accessFlags;
                break;
            case RHI::ScopeAttachmentUsage::SubpassInput:
                // QCOMM is particularly restrictive about this:
                // Starting from the second subpass where the input_attachments field is used,
                // the dstAccessMask must be set to VK_ACCESS_INPUT_ATTACHMENT_READ_BIT.
                accessFlags = VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
                break;
            case RHI::ScopeAttachmentUsage::Shader:
                accessFlags |= RHI::CheckBitsAny(access, RHI::ScopeAttachmentAccess::Write)
                    ? VK_ACCESS_SHADER_WRITE_BIT
                    : accessFlags;
                accessFlags |= RHI::CheckBitsAny(access, RHI::ScopeAttachmentAccess::Read)
                    ? VK_ACCESS_SHADER_READ_BIT
                    : accessFlags;
                break;
            case RHI::ScopeAttachmentUsage::Copy:
                accessFlags |= RHI::CheckBitsAny(access, RHI::ScopeAttachmentAccess::Write)
                    ? VK_ACCESS_TRANSFER_WRITE_BIT
                    : accessFlags;
                accessFlags |= RHI::CheckBitsAny(access, RHI::ScopeAttachmentAccess::Read)
                    ? VK_ACCESS_TRANSFER_READ_BIT
                    : accessFlags;
                break;
            case RHI::ScopeAttachmentUsage::Predication:
                accessFlags |= VK_ACCESS_CONDITIONAL_RENDERING_READ_BIT_EXT;
                break;
            case RHI::ScopeAttachmentUsage::Indirect:
                accessFlags |= VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
                break;
            case RHI::ScopeAttachmentUsage::InputAssembly:
                accessFlags |= VK_ACCESS_INDEX_READ_BIT | VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
                break;
            case RHI::ScopeAttachmentUsage::ShadingRate:
                {
                    AZ_Assert(shadingRateAttachmentUsageFlags != 0, "shading Rate Attachment UsageFlags can not be 0");
                    accessFlags |=
                        RHI::CheckBitsAll(shadingRateAttachmentUsageFlags,
                                          static_cast<VkImageUsageFlags>(VK_IMAGE_USAGE_FRAGMENT_DENSITY_MAP_BIT_EXT))
                        ? VK_ACCESS_FRAGMENT_DENSITY_MAP_READ_BIT_EXT
                        : VK_ACCESS_FRAGMENT_SHADING_RATE_ATTACHMENT_READ_BIT_KHR;
                    break;
                }
            default:
                break;
            }
            return accessFlags;
        }

        VkAccessFlags GetResourceAccessFlags(const RHI::ScopeAttachment& scopeAttachment)
        {
            VkImageUsageFlags shadingRateAttachmentUsageFlags = 0;
            if (scopeAttachment.GetUsage() == RHI::ScopeAttachmentUsage::ShadingRate)
            {
                const RHI::ImageView* imageView = static_cast<const RHI::ImageView*>(scopeAttachment.GetResourceView());
                const int deviceIndex = scopeAttachment.GetScope().GetDeviceIndex();
                const Image& image = static_cast<const Image&>(*imageView->GetImage()->GetDeviceImage(deviceIndex));
                shadingRateAttachmentUsageFlags = image.GetUsageFlags();
            }
            return GetResourceAccessFlags(scopeAttachment.GetAccess(), scopeAttachment.GetUsage(), shadingRateAttachmentUsageFlags);
        }

        VkAccessFlags GetResourceAccessFlags(const RHI::BufferBindFlags& bindFlags)
        {
            VkAccessFlags accessFlags = {};
            if (RHI::CheckBitsAny(bindFlags, RHI::BufferBindFlags::InputAssembly | RHI::BufferBindFlags::DynamicInputAssembly))
            {
                accessFlags |= VK_ACCESS_INDIRECT_COMMAND_READ_BIT | VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_INDEX_READ_BIT;
            }

            if (RHI::CheckBitsAny(bindFlags, RHI::BufferBindFlags::Constant))
            {
                accessFlags |= VK_ACCESS_UNIFORM_READ_BIT;
            }

            if (RHI::CheckBitsAny(bindFlags, RHI::BufferBindFlags::ShaderRead))
            {
                accessFlags |= VK_ACCESS_SHADER_READ_BIT;
            }

            if (RHI::CheckBitsAny(bindFlags, RHI::BufferBindFlags::ShaderWrite))
            {
                accessFlags |= VK_ACCESS_SHADER_WRITE_BIT;
            }

            if (RHI::CheckBitsAny(bindFlags, RHI::BufferBindFlags::Predication))
            {
                accessFlags |= VK_ACCESS_CONDITIONAL_RENDERING_READ_BIT_EXT;
            }

            if (RHI::CheckBitsAny(bindFlags, RHI::BufferBindFlags::Indirect))
            {
                accessFlags |= VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
            }

            if (RHI::CheckBitsAny(bindFlags, RHI::BufferBindFlags::RayTracingAccelerationStructure))
            {
                accessFlags |= VK_ACCESS_SHADER_READ_BIT;
            }

            return accessFlags;
        }

        VkAccessFlags GetResourceAccessFlags(const RHI::ImageBindFlags& bindFlags)
        {
            VkAccessFlags accessFlags = {};
            if (RHI::CheckBitsAny(bindFlags, RHI::ImageBindFlags::ShaderRead))
            {
                accessFlags |= VK_ACCESS_SHADER_READ_BIT;
            }

            if (RHI::CheckBitsAny(bindFlags, RHI::ImageBindFlags::ShaderWrite))
            {
                accessFlags |= VK_ACCESS_SHADER_WRITE_BIT;
            }

            if (RHI::CheckBitsAny(bindFlags, RHI::ImageBindFlags::Color))
            {
                accessFlags |= VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            }

            if (RHI::CheckBitsAny(bindFlags, RHI::ImageBindFlags::DepthStencil))
            {
                accessFlags |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            }

            if (RHI::CheckBitsAny(bindFlags, RHI::ImageBindFlags::CopyRead))
            {
                accessFlags |= VK_ACCESS_TRANSFER_READ_BIT;
            }

            if (RHI::CheckBitsAny(bindFlags, RHI::ImageBindFlags::CopyWrite))
            {
                accessFlags |= VK_ACCESS_TRANSFER_WRITE_BIT;
            }

            if (RHI::CheckBitsAny(bindFlags, RHI::ImageBindFlags::ShadingRate))
            {
                accessFlags |= VK_ACCESS_FRAGMENT_DENSITY_MAP_READ_BIT_EXT | VK_ACCESS_FRAGMENT_SHADING_RATE_ATTACHMENT_READ_BIT_KHR;
            }

            return accessFlags;
        }

        VkImageLayout GetImageAttachmentLayout(const RHI::ImageScopeAttachment& imageAttachment)
        {
            const RHI::DeviceImageView* imageView = imageAttachment.GetImageView()->GetDeviceImageView(imageAttachment.GetScope().GetDeviceIndex()).get();
            auto imageAspects = RHI::FilterBits(imageView->GetImage().GetAspectFlags(), imageView->GetDescriptor().m_aspectFlags);
            RHI::ScopeAttachmentAccess access = imageAttachment.GetAccess();
            switch (imageAttachment.GetUsage())
            {
            case RHI::ScopeAttachmentUsage::RenderTarget:
                return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            case RHI::ScopeAttachmentUsage::Resolve:
                return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            case RHI::ScopeAttachmentUsage::DepthStencil:
                if (RHI::CheckBitsAll(imageAspects, RHI::ImageAspectFlags::DepthStencil))
                {
                    return RHI::CheckBitsAny(access, RHI::ScopeAttachmentAccess::Write) ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
                                                                                        : VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
                }
                else if (RHI::CheckBitsAll(imageAspects, RHI::ImageAspectFlags::Depth))
                {
                    return RHI::CheckBitsAny(access, RHI::ScopeAttachmentAccess::Write) ? VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL
                                                                                        : VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
                }
                else
                {
                    return RHI::CheckBitsAny(access, RHI::ScopeAttachmentAccess::Write) ? VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL
                                                                                        : VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
                }                
            case RHI::ScopeAttachmentUsage::Shader:
                if (RHI::CheckBitsAny(access, RHI::ScopeAttachmentAccess::Write))
                {
                    return VK_IMAGE_LAYOUT_GENERAL;
                }
                [[fallthrough]];
            case RHI::ScopeAttachmentUsage::SubpassInput:
                {
                    // if we are reading from a depth/stencil texture, then we use the depth/stencil read optimal layout instead of the
                    // generic shader read one
                    AZ_Error(
                        "Vulkan",
                        !RHI::CheckBitsAll(imageAspects, RHI::ImageAspectFlags::DepthStencil),
                        "Please specify depth or stencil aspect mask for ScopeAttachment %s in Scope %s",
                        imageAttachment.GetDescriptor().m_attachmentId.GetCStr(),
                        imageAttachment.GetScope().GetId().GetCStr());

                    if (RHI::CheckBitsAny(imageAspects, RHI::ImageAspectFlags::DepthStencil))
                    {
                        if (imageAspects == RHI::ImageAspectFlags::Depth)
                        {
                            return VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL;
                        }
                        else if (imageAspects == RHI::ImageAspectFlags::Stencil)
                        {
                            return VK_IMAGE_LAYOUT_STENCIL_READ_ONLY_OPTIMAL;
                        }

                        return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
                    }

                    return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                }
            case RHI::ScopeAttachmentUsage::Copy:
                return RHI::CheckBitsAny(access, RHI::ScopeAttachmentAccess::Write)
                    ? VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
                    : VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            case RHI::ScopeAttachmentUsage::ShadingRate:
                {
                    const Image& image = static_cast<const Image&>(imageView->GetImage());
                    return RHI::CheckBitsAll(
                               image.GetUsageFlags(), static_cast<VkImageUsageFlags>(VK_IMAGE_USAGE_FRAGMENT_DENSITY_MAP_BIT_EXT))
                        ? VK_IMAGE_LAYOUT_FRAGMENT_DENSITY_MAP_OPTIMAL_EXT
                        : VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR;
                }
            default:
                return VK_IMAGE_LAYOUT_GENERAL;
            }
        }

        bool HasExplicitClear(const RHI::ScopeAttachment& scopeAttachment)
        {
            const RHI::ScopeAttachmentDescriptor& descriptor = scopeAttachment.GetScopeAttachmentDescriptor();
            const bool isClearAction = descriptor.m_loadStoreAction.m_loadAction == RHI::AttachmentLoadAction::Clear;
            const bool isClearActionStencil = descriptor.m_loadStoreAction.m_loadActionStencil == RHI::AttachmentLoadAction::Clear;
            return ((isClearAction || isClearActionStencil) &&
                scopeAttachment.GetUsage() == RHI::ScopeAttachmentUsage::Shader);
        }

        VmaAllocationCreateInfo GetVmaAllocationCreateInfo(const RHI::HeapMemoryLevel level)
        {
            VmaAllocationCreateInfo allocInfo = {};
            allocInfo.flags = VMA_ALLOCATION_CREATE_STRATEGY_MIN_MEMORY_BIT;
            switch (level)
            {
            case RHI::HeapMemoryLevel::Host:
                allocInfo.flags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
                allocInfo.requiredFlags |= VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
                allocInfo.requiredFlags |= VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
                break;
            case RHI::HeapMemoryLevel::Device:
                allocInfo.requiredFlags |= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
                break;
            default:
                break;
            }
            return allocInfo;
        }

        VkImageLayout CombineImageLayout(VkImageLayout lhs, VkImageLayout rhs)
        {
            auto isSameFunc = [&](VkImageLayout compareValue1, VkImageLayout compareValue2)
            {
                return (lhs == compareValue1 && rhs == compareValue2) || (lhs == compareValue2 && rhs == compareValue1);
            };

            if (isSameFunc(VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL))
            {
                return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            }
            else if (isSameFunc(VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_STENCIL_READ_ONLY_OPTIMAL))
            {
                return VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL;
            }
            else if (isSameFunc(VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL))
            {
                return VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL;
            }
            else if (isSameFunc(VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_STENCIL_READ_ONLY_OPTIMAL))
            {
                return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
            }
            else if (
                isSameFunc(VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL) ||
                isSameFunc(VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_STENCIL_READ_ONLY_OPTIMAL))
            {
                return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
            }
            else if (
                isSameFunc(VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL) ||
                isSameFunc(VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL))
            {
                return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            }
            else if (
                isSameFunc(VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL) ||
                isSameFunc(VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL))
            {
                return VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL;
            }
            else if (
                isSameFunc(VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL) ||
                isSameFunc(VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_STENCIL_READ_ONLY_OPTIMAL))
            {
                return VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL;
            }
            // We always add both depth and stencil aspects for image layouts, even when dealing with depth only or stencil only layouts.
            // Using a depth only or stencil only layouts requires an extension and more complicated logic.
            else if (
                isSameFunc(VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL) ||
                isSameFunc(VK_IMAGE_LAYOUT_STENCIL_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_STENCIL_READ_ONLY_OPTIMAL))   
            {
                return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
            }
            // We always add both depth and stencil aspects for image layouts, even when dealing with depth only or stencil only layouts.
            // Using a depth only or stencil only layouts requires an extension and more complicated logic.
            else if (
                isSameFunc(VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL) ||
                isSameFunc(VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL))  
            {
                return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            }

            return lhs;
        }

        VkImageLayout FilterImageLayout(VkImageLayout layout, RHI::ImageAspectFlags aspectFlags)
        {
            RHI::ScopeAttachmentAccess depthAccess = {};
            RHI::ScopeAttachmentAccess stencilAccess = {};
            switch (layout)
            {
            case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
                depthAccess = RHI::ScopeAttachmentAccess::ReadWrite;
                stencilAccess = RHI::ScopeAttachmentAccess::ReadWrite;
                break;
            case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL:
                depthAccess = RHI::ScopeAttachmentAccess::ReadWrite;
                stencilAccess = RHI::ScopeAttachmentAccess::Read;
                break;
            case VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL:
                depthAccess = RHI::ScopeAttachmentAccess::Read;
                stencilAccess = RHI::ScopeAttachmentAccess::ReadWrite;
                break;
            case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL:
                depthAccess = RHI::ScopeAttachmentAccess::Read;
                stencilAccess = RHI::ScopeAttachmentAccess::Read;
                break;
            default:
                break;
            }
            if (aspectFlags == RHI::ImageAspectFlags::Depth)
            {
                // Depth only
                if (RHI::CheckBitsAny(depthAccess, RHI::ScopeAttachmentAccess::Write))
                {
                    return VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
                }
                else if (RHI::CheckBitsAny(depthAccess, RHI::ScopeAttachmentAccess::Read))
                {
                    return VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL;
                }
            }
            else if (aspectFlags == RHI::ImageAspectFlags::Stencil)
            {
                // Stencil only
                if (RHI::CheckBitsAny(stencilAccess, RHI::ScopeAttachmentAccess::Write))
                {
                    return VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL;
                }
                else if (RHI::CheckBitsAny(stencilAccess, RHI::ScopeAttachmentAccess::Read))
                {
                    return VK_IMAGE_LAYOUT_STENCIL_READ_ONLY_OPTIMAL;
                }
            }

            return layout;
        }

        VkAttachmentLoadOp ConvertAttachmentLoadAction(RHI::AttachmentLoadAction loadAction, const Device& device)
        {
            const auto& physicalDevice = static_cast<const PhysicalDevice&>(device.GetPhysicalDevice());
            switch (loadAction)
            {
            case RHI::AttachmentLoadAction::Load:
                return VK_ATTACHMENT_LOAD_OP_LOAD;
            case RHI::AttachmentLoadAction::Clear:
                return VK_ATTACHMENT_LOAD_OP_CLEAR;
            case RHI::AttachmentLoadAction::DontCare:
                return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            case RHI::AttachmentLoadAction::None:
                return physicalDevice.IsFeatureSupported(DeviceFeature::LoadNoneOp) ? VK_ATTACHMENT_LOAD_OP_NONE_EXT
                                                                                    : VK_ATTACHMENT_LOAD_OP_LOAD;
            default:
                AZ_Assert(false, "AttachmentLoadAction is invalid.");
                return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            }
        }

        VkAttachmentStoreOp ConvertAttachmentStoreAction(RHI::AttachmentStoreAction storeAction, const Device& device)
        {
            const auto& physicalDevice = static_cast<const PhysicalDevice&>(device.GetPhysicalDevice());
            switch (storeAction)
            {
            case RHI::AttachmentStoreAction::Store:
                return VK_ATTACHMENT_STORE_OP_STORE;
            case RHI::AttachmentStoreAction::DontCare:
                return VK_ATTACHMENT_STORE_OP_DONT_CARE;
            case RHI::AttachmentStoreAction::None:
                return physicalDevice.IsFeatureSupported(DeviceFeature::StoreNoneOp) ? VK_ATTACHMENT_STORE_OP_NONE
                                                                                     : VK_ATTACHMENT_STORE_OP_STORE;
            default:
                AZ_Assert(false, "AttachmentStoreAction is invalid.");
                return VK_ATTACHMENT_STORE_OP_DONT_CARE;
            }
        }

        RHI::AttachmentLoadAction CombineLoadOp(RHI::AttachmentLoadAction currentOp, RHI::AttachmentLoadAction newOp)
        {
            switch (currentOp)
            {
            case RHI::AttachmentLoadAction::Load:
                return RHI::AttachmentLoadAction::Load;
            case RHI::AttachmentLoadAction::DontCare:
                return RHI::AttachmentLoadAction::DontCare;
            case RHI::AttachmentLoadAction::Clear:
                return RHI::AttachmentLoadAction::Clear;
            case RHI::AttachmentLoadAction::None:
                return newOp != RHI::AttachmentLoadAction::None ? RHI::AttachmentLoadAction::Load : RHI::AttachmentLoadAction::None;
            default:
                AZ_Assert(false, "AttachmentLoadAction is invalid.");
                return RHI::AttachmentLoadAction::Load;
            }
        }

        RHI::AttachmentStoreAction CombineStoreOp(RHI::AttachmentStoreAction currentOp, RHI::AttachmentStoreAction newOp)
        {
            switch (currentOp)
            {
            case RHI::AttachmentStoreAction::DontCare:
                return newOp;
            case RHI::AttachmentStoreAction::Store:
                return newOp == RHI::AttachmentStoreAction::None ? RHI::AttachmentStoreAction::Store : newOp;
            case RHI::AttachmentStoreAction::None:
                return newOp;
            default:
                AZ_Assert(false, "AttachmentStoreAction is invalid.");
                return RHI::AttachmentStoreAction::Store;
            }
        }

        BarrierTypeFlags ConvertBarrierType(VkStructureType type)
        {
            switch (type)
            {
            case VK_STRUCTURE_TYPE_MEMORY_BARRIER:
                return BarrierTypeFlags::Memory;
            case VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER:
                return BarrierTypeFlags::Buffer;
            case VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER:
                return BarrierTypeFlags::Image;
            default:
                AZ_Assert(false, "Invalid memory barrier type.");
                return BarrierTypeFlags::None;
            }
        }
    }
}
