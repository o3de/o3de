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
#include <Atom/RHI/SingleDeviceImage.h>
#include <Atom/RHI/ImageView.h>
#include <RHI/Conversion.h>
#include <RHI/Image.h>

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
        VkPipelineStageFlags GetResourcePipelineStateFlags(const RHI::ScopeAttachment& scopeAttachment)
        {
            VkPipelineStageFlags mergedStateFlags = {};
            const AZStd::vector<RHI::ScopeAttachmentUsageAndAccess>& usagesAndAccesses = scopeAttachment.GetUsageAndAccess();
            for (const RHI::ScopeAttachmentUsageAndAccess& usageAndAccess : usagesAndAccesses)
            {
                switch (usageAndAccess.m_usage)
                {
                case RHI::ScopeAttachmentUsage::RenderTarget:
                    mergedStateFlags |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                    break;
                case RHI::ScopeAttachmentUsage::Resolve:
                    mergedStateFlags |= VK_PIPELINE_STAGE_TRANSFER_BIT;
                    break;
                case RHI::ScopeAttachmentUsage::DepthStencil:
                    mergedStateFlags |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
                    break;
                case RHI::ScopeAttachmentUsage::SubpassInput:
                case RHI::ScopeAttachmentUsage::Shader:
                    {
                        switch (scopeAttachment.GetScope().GetHardwareQueueClass())
                        {
                        case RHI::HardwareQueueClass::Graphics:
                            mergedStateFlags |= VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT |
                                VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT |
                                VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT | VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT;
                            break;
                        case RHI::HardwareQueueClass::Compute:
                            mergedStateFlags |= VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
                            break;
                        default:
                            AZ_Assert(false, "Invalid ScopeAttachment type when getting the resource pipeline stage flags");
                        }
                        break;
                    }
                case RHI::ScopeAttachmentUsage::Copy:
                    mergedStateFlags |= VK_PIPELINE_STAGE_TRANSFER_BIT;
                    break;
                case RHI::ScopeAttachmentUsage::Predication:
                    mergedStateFlags |= VK_PIPELINE_STAGE_CONDITIONAL_RENDERING_BIT_EXT;
                    break;
                case RHI::ScopeAttachmentUsage::Indirect:
                    mergedStateFlags |= VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT;
                    break;
                case RHI::ScopeAttachmentUsage::InputAssembly:
                    mergedStateFlags |= VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
                    break;
                case RHI::ScopeAttachmentUsage::ShadingRate:
                    {
                        const Image& image =
                            static_cast<const Image&>((static_cast<const RHI::ImageView*>(scopeAttachment.GetResourceView()))->GetImage());
                        mergedStateFlags |=
                            RHI::CheckBitsAll(
                                image.GetUsageFlags(), static_cast<VkImageUsageFlags>(VK_IMAGE_USAGE_FRAGMENT_DENSITY_MAP_BIT_EXT))
                            ? VK_PIPELINE_STAGE_FRAGMENT_DENSITY_PROCESS_BIT_EXT
                            : VK_PIPELINE_STAGE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR;
                        break;
                    }
                default:
                    break;
                }
            }
            return mergedStateFlags;
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
                    VK_PIPELINE_STAGE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR | VK_PIPELINE_STAGE_MESH_SHADER_BIT_NV;
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

        VkAccessFlags GetResourceAccessFlags(const RHI::ScopeAttachment& scopeAttachment)
        {
            VkAccessFlags accessFlags = {};

            const AZStd::vector<RHI::ScopeAttachmentUsageAndAccess>& usagesAndAccesses = scopeAttachment.GetUsageAndAccess();
            for (const RHI::ScopeAttachmentUsageAndAccess& usageAndAccess : usagesAndAccesses)
            {
                switch (usageAndAccess.m_usage)
                {
                case RHI::ScopeAttachmentUsage::RenderTarget:
                    accessFlags |= RHI::CheckBitsAny(usageAndAccess.m_access, RHI::ScopeAttachmentAccess::Write)
                        ? VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
                        : accessFlags;
                    accessFlags |= RHI::CheckBitsAny(usageAndAccess.m_access, RHI::ScopeAttachmentAccess::Read)
                        ? VK_ACCESS_COLOR_ATTACHMENT_READ_BIT
                        : accessFlags;
                    break;
                case RHI::ScopeAttachmentUsage::Resolve:
                    accessFlags |= VK_ACCESS_TRANSFER_WRITE_BIT;
                    break;
                case RHI::ScopeAttachmentUsage::DepthStencil:
                    accessFlags |= RHI::CheckBitsAny(usageAndAccess.m_access, RHI::ScopeAttachmentAccess::Write)
                        ? VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT
                        : accessFlags;
                    accessFlags |= RHI::CheckBitsAny(usageAndAccess.m_access, RHI::ScopeAttachmentAccess::Read)
                        ? VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT
                        : accessFlags;
                    break;
                case RHI::ScopeAttachmentUsage::SubpassInput:
                case RHI::ScopeAttachmentUsage::Shader:
                    accessFlags |= RHI::CheckBitsAny(usageAndAccess.m_access, RHI::ScopeAttachmentAccess::Write)
                        ? VK_ACCESS_SHADER_WRITE_BIT
                        : accessFlags;
                    accessFlags |= RHI::CheckBitsAny(usageAndAccess.m_access, RHI::ScopeAttachmentAccess::Read) ? VK_ACCESS_SHADER_READ_BIT
                                                                                                                : accessFlags;
                    break;
                case RHI::ScopeAttachmentUsage::Copy:
                    accessFlags |= RHI::CheckBitsAny(usageAndAccess.m_access, RHI::ScopeAttachmentAccess::Write)
                        ? VK_ACCESS_TRANSFER_WRITE_BIT
                        : accessFlags;
                    accessFlags |= RHI::CheckBitsAny(usageAndAccess.m_access, RHI::ScopeAttachmentAccess::Read)
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
                        const Image& image =
                            static_cast<const Image&>((static_cast<const RHI::ImageView*>(scopeAttachment.GetResourceView()))->GetImage());
                        accessFlags |=
                            RHI::CheckBitsAll(
                                image.GetUsageFlags(), static_cast<VkImageUsageFlags>(VK_IMAGE_USAGE_FRAGMENT_DENSITY_MAP_BIT_EXT))
                            ? VK_ACCESS_FRAGMENT_DENSITY_MAP_READ_BIT_EXT
                            : VK_ACCESS_FRAGMENT_SHADING_RATE_ATTACHMENT_READ_BIT_KHR;
                        break;
                    }
                default:
                    break;
                }
            }
            return accessFlags;
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
            const AZStd::vector<RHI::ScopeAttachmentUsageAndAccess>& usagesAndAccesses = imageAttachment.GetUsageAndAccess();

            if (usagesAndAccesses.size() > 1)
            {
                // The Attachment is used multiple times: If all usages/accesses are the same type, we can determine the
                // vk image layout from the first usage. If not, we check if all usages are depth/stencil read only (because we can use
                // the VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL for all those cases).
                // Finally we just use the fallback VK_IMAGE_LAYOUT_GENERAL that can be applied for multiple uses.
                // [GFX TODO][ATOM-4779] -Multiple Usage/Access can be further optimized.
                bool sameUsageAndAccess = true;
                bool readOnlyDepthStencil = true;
                const auto& first = usagesAndAccesses.front();
                for (int i = 0; i < usagesAndAccesses.size(); ++i)
                {
                    if (usagesAndAccesses[i].m_access != first.m_access || usagesAndAccesses[i].m_usage != first.m_usage)
                    {
                        sameUsageAndAccess = false;
                    }

                    if (usagesAndAccesses[i].m_access != RHI::ScopeAttachmentAccess::Read ||
                        (usagesAndAccesses[i].m_usage != RHI::ScopeAttachmentUsage::DepthStencil && usagesAndAccesses[i].m_usage != RHI::ScopeAttachmentUsage::Shader))
                    {
                        readOnlyDepthStencil = false;
                    }
                }

                if (!sameUsageAndAccess)
                {
                    return readOnlyDepthStencil ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_GENERAL;
                }
            }

            const RHI::ImageView* imageView = imageAttachment.GetImageView();
            auto imageAspects = RHI::FilterBits(imageView->GetImage().GetAspectFlags(), imageView->GetDescriptor().m_aspectFlags);
            switch (usagesAndAccesses.front().m_usage)
            {
            case RHI::ScopeAttachmentUsage::RenderTarget:
                return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            case RHI::ScopeAttachmentUsage::Resolve:
                return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            case RHI::ScopeAttachmentUsage::DepthStencil:
                return RHI::CheckBitsAny(usagesAndAccesses.front().m_access, RHI::ScopeAttachmentAccess::Write)
                    ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
                    : VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
            case RHI::ScopeAttachmentUsage::Shader:
            case RHI::ScopeAttachmentUsage::SubpassInput:
                {
                    // always set VK_IMAGE_LAYOUT_GENERAL if the Image is ShaderWrite, even in a read scope
                    if (RHI::CheckBitsAny(usagesAndAccesses.front().m_access, RHI::ScopeAttachmentAccess::Write) ||
                        RHI::CheckBitsAny(imageView->GetImage().GetDescriptor().m_bindFlags, RHI::ImageBindFlags::ShaderWrite))
                    {
                        return VK_IMAGE_LAYOUT_GENERAL;
                    }
                    else
                    {
                        // if we are reading from a depth/stencil texture, then we use the depth/stencil read optimal layout instead of the
                        // generic shader read one
                        return RHI::CheckBitsAny(imageAspects, RHI::ImageAspectFlags::DepthStencil)
                            ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL
                            : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                    }
                }
            case RHI::ScopeAttachmentUsage::Copy:
                return RHI::CheckBitsAny(usagesAndAccesses.front().m_access, RHI::ScopeAttachmentAccess::Write)
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

        bool HasExplicitClear(const RHI::ScopeAttachment& scopeAttachment, const RHI::ScopeAttachmentDescriptor& descriptor)
        {
            const auto& usageAndAccess = scopeAttachment.GetUsageAndAccess();
            const bool isClearAction = descriptor.m_loadStoreAction.m_loadAction == RHI::AttachmentLoadAction::Clear;
            const bool isClearActionStencil = descriptor.m_loadStoreAction.m_loadActionStencil == RHI::AttachmentLoadAction::Clear;
            if ((isClearAction || isClearActionStencil) &&
                AZStd::any_of(
                    usageAndAccess.begin(),
                    usageAndAccess.end(),
                    [](auto& usage)
                    {
                        return usage.m_usage == RHI::ScopeAttachmentUsage::Shader;
                    }))
            {
                return true;
            }
            return false;
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
                break;
            case RHI::HeapMemoryLevel::Device:
                allocInfo.requiredFlags |= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
                break;
            default:
                break;
            }
            return allocInfo;
        }
    }
}
