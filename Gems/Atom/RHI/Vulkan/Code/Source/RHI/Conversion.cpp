/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <RHI/Conversion.h>
#include <Atom/RHI.Reflect/ImageDescriptor.h>
#include <Atom/RHI.Reflect/ImageSubresource.h>
#include <Atom/RHI.Reflect/RenderAttachmentLayout.h>
#include <Atom/RHI.Reflect/ShaderResourceGroupLayout.h>
#include <Atom/RHI/ImageScopeAttachment.h>
#include <Atom/RHI/Scope.h>
#include <Atom/RHI/ScopeAttachment.h>
#include <Atom/RHI/Image.h>
#include <Atom/RHI/ImageView.h>
#include <AzCore/std/containers/bitset.h>

namespace AZ
{
    namespace Vulkan
    {
        RHI::ResultCode ConvertResult(VkResult vkResult)
        {
            switch (vkResult)
            {
            case VK_SUCCESS:
            case VK_INCOMPLETE:
                return RHI::ResultCode::Success;
            case VK_ERROR_OUT_OF_HOST_MEMORY:
            case VK_ERROR_OUT_OF_DEVICE_MEMORY:
            case VK_ERROR_OUT_OF_POOL_MEMORY:
                return RHI::ResultCode::OutOfMemory;
            case VK_ERROR_INVALID_SHADER_NV:
            case VK_ERROR_INVALID_EXTERNAL_HANDLE:
                return RHI::ResultCode::InvalidArgument;
             case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
             case VK_ERROR_FRAGMENTATION_EXT:
             case VK_ERROR_FRAGMENTED_POOL:
             case VK_ERROR_TOO_MANY_OBJECTS:
             case VK_ERROR_DEVICE_LOST:
             case VK_ERROR_SURFACE_LOST_KHR:
                 return RHI::ResultCode::InvalidOperation;
             case VK_NOT_READY:
                 return RHI::ResultCode::NotReady;
             default:
                 return RHI::ResultCode::Fail;
            }
        }

        // definition of the macro "RHIVK_EXPAND_FOR_FORMATS" containing formats' names and aspect flags.
#include "Formats.inl"

        VkFormat ConvertFormat(RHI::Format format, [[maybe_unused]] bool raiseAsserts)
        {
#define RHIVK_RHI_TO_VK(_FormatID, _VKFormat, _Color, _Depth, _Stencil) \
    case RHI::Format::_FormatID: \
        return _VKFormat;

            switch (format)
            {
            case RHI::Format::Unknown:
                return VK_FORMAT_UNDEFINED;

            RHIVK_EXPAND_FOR_FORMATS(RHIVK_RHI_TO_VK)

             default:
                AZ_Assert(!raiseAsserts, "unhandled conversion in ConvertFormat");
                return VK_FORMAT_UNDEFINED;
            }

#undef RHIVK_RHI_TO_VK
        }

        RHI::Format ConvertFormat(VkFormat format)
        {
#define RHIVK_VK_TO_RHI(_FormatID, _VKFormat, _Color, _Depth, _Stencil) \
    case _VKFormat: \
        return RHI::Format::_FormatID;

            switch (format)
            {
            case VK_FORMAT_UNDEFINED:
                return RHI::Format::Unknown;

            RHIVK_EXPAND_FOR_FORMATS(RHIVK_VK_TO_RHI)

            default:
                AZ_Assert(false, "unhandled conversion in ConvertFormat");
                return RHI::Format::Unknown;
            }

#undef RHIVK_VK_TO_RHI
        }        

#undef RHIVK_EXPAND_FOR_FORMATS

        VkImageAspectFlagBits ConvertImageAspect(RHI::ImageAspect imageAspect)
        {
            switch (imageAspect)
            {
            case RHI::ImageAspect::Color:
                return VK_IMAGE_ASPECT_COLOR_BIT;
            case RHI::ImageAspect::Depth:
                return VK_IMAGE_ASPECT_DEPTH_BIT;
            case RHI::ImageAspect::Stencil:
                return VK_IMAGE_ASPECT_STENCIL_BIT;
            default:
                AZ_Assert(false, "Invalid image aspect %d", imageAspect);
                return VK_IMAGE_ASPECT_COLOR_BIT;
            }
        }

        VkImageAspectFlags ConvertImageAspectFlags(RHI::ImageAspectFlags aspectFlagMask)
        {
            VkImageAspectFlags flags = 0;
            for (uint32_t i = 0; i < RHI::ImageAspectCount; ++i)
            {
                if (!RHI::CheckBitsAll(aspectFlagMask, static_cast<RHI::ImageAspectFlags>(AZ_BIT(i))))
                {
                    continue;
                }

                flags |= ConvertImageAspect(static_cast<RHI::ImageAspect>(i));
            }

            return flags;
        }

        VkPrimitiveTopology ConvertTopology(RHI::PrimitiveTopology topology)
        {
            switch (topology)
            {
            case RHI::PrimitiveTopology::PointList:
                return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
            case RHI::PrimitiveTopology::LineList:
                return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
            case RHI::PrimitiveTopology::LineListAdj:
                return VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY;
            case RHI::PrimitiveTopology::LineStrip:
                return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
            case RHI::PrimitiveTopology::LineStripAdj:
                return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY;
            case RHI::PrimitiveTopology::TriangleList:
                return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            case RHI::PrimitiveTopology::TriangleListAdj:
                return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY;
            case RHI::PrimitiveTopology::TriangleStrip:
                return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
            case RHI::PrimitiveTopology::TriangleStripAdj:
                return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY;
            default:
                AZ_Assert(false, "Unknown primitive topology.");
            }
            return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
        }

        VkQueueFlags ConvertQueueClass(RHI::HardwareQueueClass queueClass)
        {
            switch (queueClass)
            {
            case RHI::HardwareQueueClass::Graphics:
                return VK_QUEUE_GRAPHICS_BIT;
            case RHI::HardwareQueueClass::Compute:
                return VK_QUEUE_COMPUTE_BIT;
            case RHI::HardwareQueueClass::Copy:                
                return VK_QUEUE_TRANSFER_BIT;
            default:
                AZ_Assert(false, "Hardware queue class is invalid.");
                return VK_QUEUE_GRAPHICS_BIT;
            }
        }

        VkMemoryPropertyFlags ConvertHeapMemoryLevel(RHI::HeapMemoryLevel heapMemoryLevel)
        {
            switch (heapMemoryLevel)
            {
            case RHI::HeapMemoryLevel::Host:
                return VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
            case RHI::HeapMemoryLevel::Device:
                return VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
            default:
                AZ_Assert(false, "illegal case");
            }
            return VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        }

        void FillStencilOpState(const RHI::StencilOpState& stencilOpState, VkStencilOpState& vkStencilOpState)
        {
            VkStencilOpState& state = vkStencilOpState;
            state.failOp = ConvertStencilOp(stencilOpState.m_failOp);
            state.passOp = ConvertStencilOp(stencilOpState.m_passOp);
            state.depthFailOp = ConvertStencilOp(stencilOpState.m_depthFailOp);
            state.compareOp = ConvertComparisonFunction(stencilOpState.m_func);
        }

        VkStencilOp ConvertStencilOp(RHI::StencilOp op)
        {
            switch (op)
            {
            case RHI::StencilOp::Keep:
                return VK_STENCIL_OP_KEEP;
            case RHI::StencilOp::Zero:
                return VK_STENCIL_OP_ZERO;
            case RHI::StencilOp::Replace:
                return VK_STENCIL_OP_REPLACE;
            case RHI::StencilOp::IncrementSaturate:
                return VK_STENCIL_OP_INCREMENT_AND_CLAMP;
            case RHI::StencilOp::DecrementSaturate:
                return VK_STENCIL_OP_DECREMENT_AND_CLAMP;
            case RHI::StencilOp::Invert:
                return VK_STENCIL_OP_INVERT;
            case RHI::StencilOp::Increment:
                return VK_STENCIL_OP_INCREMENT_AND_WRAP;
            case RHI::StencilOp::Decrement:
                return VK_STENCIL_OP_DECREMENT_AND_WRAP;
            default:
                AZ_Assert(false, "Stencil Op is invalid.");
                return VK_STENCIL_OP_KEEP;
            }
        }

        VkCompareOp ConvertComparisonFunction(RHI::ComparisonFunc func)
        {
            switch (func)
            {
            case RHI::ComparisonFunc::Never:
                return VK_COMPARE_OP_NEVER;
            case RHI::ComparisonFunc::Less:
                return VK_COMPARE_OP_LESS;
            case RHI::ComparisonFunc::Equal:
                return VK_COMPARE_OP_EQUAL;
            case RHI::ComparisonFunc::LessEqual:
                return VK_COMPARE_OP_LESS_OR_EQUAL;
            case RHI::ComparisonFunc::Greater:
                return VK_COMPARE_OP_GREATER;
            case RHI::ComparisonFunc::NotEqual:
                return VK_COMPARE_OP_NOT_EQUAL;
            case RHI::ComparisonFunc::GreaterEqual:
                return VK_COMPARE_OP_GREATER_OR_EQUAL;
            case RHI::ComparisonFunc::Always:
                return VK_COMPARE_OP_ALWAYS;
            default:
                AZ_Assert(false, "Comparison function is invalid.");
            }
            return VK_COMPARE_OP_LESS;
        }

        void FillColorBlendAttachmentState(const RHI::TargetBlendState& targetBlendState, VkPipelineColorBlendAttachmentState& colorBlendAttachmentState)
        {
            VkPipelineColorBlendAttachmentState& state = colorBlendAttachmentState;
            state.blendEnable = (targetBlendState.m_enable != 0);
            state.srcColorBlendFactor = ConvertBlendFactor(targetBlendState.m_blendSource);
            state.dstColorBlendFactor = ConvertBlendFactor(targetBlendState.m_blendDest);
            state.colorBlendOp = ConvertBlendOp(targetBlendState.m_blendOp);
            state.srcAlphaBlendFactor = ConvertBlendFactor(targetBlendState.m_blendAlphaSource);
            state.dstAlphaBlendFactor = ConvertBlendFactor(targetBlendState.m_blendAlphaDest);
            state.alphaBlendOp = ConvertBlendOp(targetBlendState.m_blendAlphaOp);
            state.colorWriteMask = ConvertComponentFlags(static_cast<uint8_t>(targetBlendState.m_writeMask));
        }

        VkBlendFactor ConvertBlendFactor(const RHI::BlendFactor& blendFactor)
        {
            switch (blendFactor)
            {
            case RHI::BlendFactor::Zero:
                return VK_BLEND_FACTOR_ZERO;
            case RHI::BlendFactor::One:
                return VK_BLEND_FACTOR_ONE;
            case RHI::BlendFactor::ColorSource:
                return VK_BLEND_FACTOR_SRC_COLOR;
            case RHI::BlendFactor::ColorSourceInverse:
                return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
            case RHI::BlendFactor::AlphaSource:
                return VK_BLEND_FACTOR_SRC_ALPHA;
            case RHI::BlendFactor::AlphaSourceInverse:
                return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            case RHI::BlendFactor::AlphaDest:
                return VK_BLEND_FACTOR_DST_ALPHA;
            case RHI::BlendFactor::AlphaDestInverse:
                return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
            case RHI::BlendFactor::ColorDest:
                return VK_BLEND_FACTOR_DST_COLOR;
            case RHI::BlendFactor::ColorDestInverse:
                return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
            case RHI::BlendFactor::AlphaSourceSaturate:
                return VK_BLEND_FACTOR_SRC_ALPHA_SATURATE;
            case RHI::BlendFactor::Factor:
                return VK_BLEND_FACTOR_CONSTANT_COLOR;
            case RHI::BlendFactor::FactorInverse:
                return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR;
            case RHI::BlendFactor::ColorSource1:
                return VK_BLEND_FACTOR_SRC1_COLOR;
            case RHI::BlendFactor::ColorSource1Inverse:
                return VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR;
            case RHI::BlendFactor::AlphaSource1:
                return VK_BLEND_FACTOR_SRC1_ALPHA;
            case RHI::BlendFactor::AlphaSource1Inverse:
                return VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA;
            default:
                AZ_Assert(false, "Blend factor is invalid.");
                break;
            }
            return VK_BLEND_FACTOR_SRC_COLOR;
        }

        VkBlendOp ConvertBlendOp(const RHI::BlendOp blendOp)
        {
            switch (blendOp)
            {
            case RHI::BlendOp::Add:
                return VK_BLEND_OP_ADD;
            case RHI::BlendOp::Subtract:
                return VK_BLEND_OP_SUBTRACT;
            case RHI::BlendOp::SubtractReverse:
                return VK_BLEND_OP_REVERSE_SUBTRACT;
            case RHI::BlendOp::Minimum:
                return VK_BLEND_OP_MIN;
            case RHI::BlendOp::Maximum:
                return VK_BLEND_OP_MAX;
            default:
                AZ_Assert(false, "Blend op is invalid.");
                break;
            }
            return VK_BLEND_OP_ADD;
        }

        VkColorComponentFlags ConvertComponentFlags(uint8_t sflags)
        {
            VkColorComponentFlags dflags = 0;
            
            if(sflags == 0)
            {
                return dflags;
            }
            
            if(RHI::CheckBitsAll(sflags, static_cast<uint8_t>(RHI::WriteChannelMask::ColorWriteMaskAll)))
            {
                return VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
            }
            
            if (RHI::CheckBitsAny(sflags, static_cast<uint8_t>(RHI::WriteChannelMask::ColorWriteMaskRed)))
            {
                dflags |= VK_COLOR_COMPONENT_R_BIT;
            }
            if (RHI::CheckBitsAny(sflags, static_cast<uint8_t>(RHI::WriteChannelMask::ColorWriteMaskGreen)))
            {
                dflags |= VK_COLOR_COMPONENT_G_BIT;
            }
            if (RHI::CheckBitsAny(sflags, static_cast<uint8_t>(RHI::WriteChannelMask::ColorWriteMaskBlue)))
            {
                dflags |= VK_COLOR_COMPONENT_B_BIT;
            }
            if (RHI::CheckBitsAny(sflags, static_cast<uint8_t>(RHI::WriteChannelMask::ColorWriteMaskAlpha)))
            {
                dflags |= VK_COLOR_COMPONENT_A_BIT;
            }
            return dflags;
        }

        VkSampleCountFlagBits ConvertSampleCount(uint16_t samples)
        {
            switch (samples)
            {
            case 1:
                return VK_SAMPLE_COUNT_1_BIT;
            case 2:
                return VK_SAMPLE_COUNT_2_BIT;
            case 4:
                return VK_SAMPLE_COUNT_4_BIT;
            case 8:
                return VK_SAMPLE_COUNT_8_BIT;
            case 16:
                return VK_SAMPLE_COUNT_16_BIT;
            case 32:
                return VK_SAMPLE_COUNT_32_BIT;
            case 64:
                return VK_SAMPLE_COUNT_64_BIT;
            default:
                AZ_Assert(false, "SampleCount is invalid.");
                return VK_SAMPLE_COUNT_1_BIT;
            }
        }

        VkAttachmentLoadOp ConvertAttachmentLoadAction(RHI::AttachmentLoadAction loadAction)
        {
            switch (loadAction)
            {
            case RHI::AttachmentLoadAction::Load:
                return VK_ATTACHMENT_LOAD_OP_LOAD;
            case RHI::AttachmentLoadAction::Clear:
                return VK_ATTACHMENT_LOAD_OP_CLEAR;
            case RHI::AttachmentLoadAction::DontCare:
                return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            default:    
                AZ_Assert(false, "AttachmentLoadAction is illegal.");
                return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            }
        }

        VkAttachmentStoreOp ConvertAttachmentStoreAction(RHI::AttachmentStoreAction storeAction)
        {
            switch (storeAction)
            {
            case RHI::AttachmentStoreAction::Store:
                return VK_ATTACHMENT_STORE_OP_STORE;
            case RHI::AttachmentStoreAction::DontCare:
                return VK_ATTACHMENT_STORE_OP_DONT_CARE;
            default:
                AZ_Assert(false, "AttachmentStoreAction is illegal.");
                return VK_ATTACHMENT_STORE_OP_DONT_CARE;
            }
        }

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

        void FillClearValue(const RHI::ClearValue& rhiClearValue, VkClearValue& vulkanClearValue)
        {
            switch (rhiClearValue.m_type)
            {
            case RHI::ClearValueType::Vector4Float:
                vulkanClearValue.color.float32[0] = rhiClearValue.m_vector4Float[0];
                vulkanClearValue.color.float32[1] = rhiClearValue.m_vector4Float[1];
                vulkanClearValue.color.float32[2] = rhiClearValue.m_vector4Float[2];
                vulkanClearValue.color.float32[3] = rhiClearValue.m_vector4Float[3];
                return;
            case RHI::ClearValueType::Vector4Uint:
                vulkanClearValue.color.uint32[0] = rhiClearValue.m_vector4Uint[0];
                vulkanClearValue.color.uint32[1] = rhiClearValue.m_vector4Uint[1];
                vulkanClearValue.color.uint32[2] = rhiClearValue.m_vector4Uint[2];
                vulkanClearValue.color.uint32[3] = rhiClearValue.m_vector4Uint[3];
                return;
            case RHI::ClearValueType::DepthStencil:
                vulkanClearValue.depthStencil.depth = rhiClearValue.m_depthStencil.m_depth;
                vulkanClearValue.depthStencil.stencil = rhiClearValue.m_depthStencil.m_stencil;
                return;
            default:
                AZ_Assert(false, "ClearValueType is invalid.");
            }
        }

        VkFilter ConvertFilterMode(RHI::FilterMode filterMode)
        {
            switch (filterMode)
            {
            case RHI::FilterMode::Point:
                return VK_FILTER_NEAREST;
            case RHI::FilterMode::Linear:
                return VK_FILTER_LINEAR;
            default:
                AZ_Assert(false, "SamplerFilterMode is illegal.");
                return VK_FILTER_NEAREST;
            }
        }

        VkSamplerAddressMode ConvertAddressMode(RHI::AddressMode addressMode)
        {
            switch (addressMode)
            {
            case RHI::AddressMode::Wrap:
                return VK_SAMPLER_ADDRESS_MODE_REPEAT;
            case RHI::AddressMode::Mirror:
                return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
            case RHI::AddressMode::Clamp:
                return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            case RHI::AddressMode::Border:
                return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
            case RHI::AddressMode::MirrorOnce:
                return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            default:
                AZ_Assert(false, "SamplerAddressMode is illegal.");
                return VK_SAMPLER_ADDRESS_MODE_REPEAT;
            }
        }            

        VkImageType ConvertToImageType(RHI::ImageDimension dimension)
        {
            switch (dimension)
            {
            case RHI::ImageDimension::Image1D:
                return VK_IMAGE_TYPE_1D;
            case RHI::ImageDimension::Image2D:
                return VK_IMAGE_TYPE_2D;
            case RHI::ImageDimension::Image3D:
                return VK_IMAGE_TYPE_3D;
            default:
                AZ_Assert(false, "Invalid dimension type.");
                return VK_IMAGE_TYPE_2D;
            }
        }

        VkExtent3D ConvertToExtent3D(const RHI::Size& size)
        {
            VkExtent3D extent{};
            extent.width = size.m_width;
            extent.height = size.m_height;
            extent.depth = size.m_depth;
            return extent;
        }

        VkQueryType ConvertQueryType(const RHI::QueryType type)
        {
            switch (type)
            {
            case RHI::QueryType::Occlusion:
                return VK_QUERY_TYPE_OCCLUSION;
            case RHI::QueryType::PipelineStatistics:
                return VK_QUERY_TYPE_PIPELINE_STATISTICS;
            case RHI::QueryType::Timestamp:
                return VK_QUERY_TYPE_TIMESTAMP;
            default:
                AZ_Assert(false, "Invalid query type");
                return VK_QUERY_TYPE_OCCLUSION;
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

        VkQueryPipelineStatisticFlags ConvertQueryPipelineStatisticMask(RHI::PipelineStatisticsFlags mask)
        {
            VkQueryPipelineStatisticFlags flags = 0;
            if (RHI::CheckBitsAll(mask, RHI::PipelineStatisticsFlags::IAVertices))
            {
                flags |= VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_VERTICES_BIT;
            }
            if (RHI::CheckBitsAll(mask, RHI::PipelineStatisticsFlags::IAPrimitives))
            {
                flags |= VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_PRIMITIVES_BIT;
            }
            if (RHI::CheckBitsAll(mask, RHI::PipelineStatisticsFlags::VSInvocations))
            {
                flags |= VK_QUERY_PIPELINE_STATISTIC_VERTEX_SHADER_INVOCATIONS_BIT;
            }
            if (RHI::CheckBitsAll(mask, RHI::PipelineStatisticsFlags::GSInvocations))
            {
                flags |= VK_QUERY_PIPELINE_STATISTIC_GEOMETRY_SHADER_INVOCATIONS_BIT;
            }
            if (RHI::CheckBitsAll(mask, RHI::PipelineStatisticsFlags::GSPrimitives))
            {
                flags |= VK_QUERY_PIPELINE_STATISTIC_GEOMETRY_SHADER_PRIMITIVES_BIT;
            }
            if (RHI::CheckBitsAll(mask, RHI::PipelineStatisticsFlags::CInvocations))
            {
                flags |= VK_QUERY_PIPELINE_STATISTIC_CLIPPING_INVOCATIONS_BIT;
            }
            if (RHI::CheckBitsAll(mask, RHI::PipelineStatisticsFlags::CPrimitives))
            {
                flags |= VK_QUERY_PIPELINE_STATISTIC_CLIPPING_PRIMITIVES_BIT;
            }
            if (RHI::CheckBitsAll(mask, RHI::PipelineStatisticsFlags::PSInvocations))
            {
                flags |= VK_QUERY_PIPELINE_STATISTIC_FRAGMENT_SHADER_INVOCATIONS_BIT;
            }
            if (RHI::CheckBitsAll(mask, RHI::PipelineStatisticsFlags::HSInvocations))
            {
                flags |= VK_QUERY_PIPELINE_STATISTIC_TESSELLATION_CONTROL_SHADER_PATCHES_BIT;
            }
            if (RHI::CheckBitsAll(mask, RHI::PipelineStatisticsFlags::DSInvocations))
            {
                flags |= VK_QUERY_PIPELINE_STATISTIC_TESSELLATION_EVALUATION_SHADER_INVOCATIONS_BIT;
            }
            if (RHI::CheckBitsAll(mask, RHI::PipelineStatisticsFlags::CSInvocations))
            {
                flags |= VK_QUERY_PIPELINE_STATISTIC_COMPUTE_SHADER_INVOCATIONS_BIT;
            }
            return flags;
        }

        VkShaderStageFlagBits ConvertShaderStage(RHI::ShaderStage stage, uint32_t subStageIndex /* = 0 */)
        {
            switch (stage)
            {
            case RHI::ShaderStage::Vertex:
                return VK_SHADER_STAGE_VERTEX_BIT;
            case RHI::ShaderStage::Fragment:
                return VK_SHADER_STAGE_FRAGMENT_BIT;
            case RHI::ShaderStage::Compute:
                return VK_SHADER_STAGE_COMPUTE_BIT;
            case RHI::ShaderStage::Tessellation:
                return subStageIndex == 0 ? VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT : VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
            default:
                AZ_Assert(false, "Invalid shader stage %d", stage);
                return VkShaderStageFlagBits(0);
            }
        }

        VkShaderStageFlags ConvertShaderStageMask(uint32_t shaderStageMask)
        {
            VkShaderStageFlags flags = 0;
            for (uint32_t i = 0; i < RHI::ShaderStageCount; ++i)
            {
                if (RHI::CheckBitsAll(shaderStageMask, AZ_BIT(i)))
                {
                    flags |= ConvertShaderStage(static_cast<RHI::ShaderStage>(i));
                }
            }
            return flags;
        }

        RenderPass::Descriptor ConvertRenderAttachmentLayout(const RHI::RenderAttachmentLayout& layout, const RHI::MultisampleState& multisampleState)
        {
            RenderPass::Descriptor renderPassDesc;
            renderPassDesc.m_attachmentCount = layout.m_attachmentCount;

            for (uint32_t index = 0; index < renderPassDesc.m_attachmentCount; ++index)
            {
                // Only fill up the necessary information to get a compatible render pass
                renderPassDesc.m_attachments[index].m_format = layout.m_attachmentFormats[index];
                renderPassDesc.m_attachments[index].m_initialLayout = VK_IMAGE_LAYOUT_GENERAL;
                renderPassDesc.m_attachments[index].m_finalLayout = VK_IMAGE_LAYOUT_GENERAL;
                renderPassDesc.m_attachments[index].m_multisampleState = multisampleState;
            }

            renderPassDesc.m_subpassCount = layout.m_subpassCount;
            for (uint32_t subpassIndex = 0; subpassIndex < layout.m_subpassCount; ++subpassIndex)
            {
                AZStd::bitset<RHI::Limits::Pipeline::RenderAttachmentCountMax> usedAttachments;
                const auto& subpassLayout = layout.m_subpassLayouts[subpassIndex];
                auto& subpassDescriptor = renderPassDesc.m_subpassDescriptors[subpassIndex];
                subpassDescriptor.m_rendertargetCount = subpassLayout.m_rendertargetCount;
                subpassDescriptor.m_subpassInputCount = subpassLayout.m_subpassInputCount;
                if (subpassLayout.m_depthStencilDescriptor.IsValid())
                {
                    subpassDescriptor.m_depthStencilAttachment = RenderPass::SubpassAttachment{
                        subpassLayout.m_depthStencilDescriptor.m_attachmentIndex,
                        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };
                    usedAttachments.set(subpassLayout.m_depthStencilDescriptor.m_attachmentIndex, true);
                }

                for (uint32_t colorAttachmentIndex = 0; colorAttachmentIndex < subpassLayout.m_rendertargetCount; ++colorAttachmentIndex)
                {
                    RenderPass::SubpassAttachment& subpassAttachment = subpassDescriptor.m_rendertargetAttachments[colorAttachmentIndex];
                    subpassAttachment.m_attachmentIndex = subpassLayout.m_rendertargetDescriptors[colorAttachmentIndex].m_attachmentIndex;
                    subpassAttachment.m_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                    usedAttachments.set(subpassAttachment.m_attachmentIndex);
                    RenderPass::SubpassAttachment& resolveSubpassAttachment = subpassDescriptor.m_resolveAttachments[colorAttachmentIndex];
                    resolveSubpassAttachment.m_attachmentIndex = subpassLayout.m_rendertargetDescriptors[colorAttachmentIndex].m_resolveAttachmentIndex;
                    resolveSubpassAttachment.m_layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                    if (resolveSubpassAttachment.IsValid())
                    {
                        // Set the number of samples for resolve attachments to 1.
                        renderPassDesc.m_attachments[resolveSubpassAttachment.m_attachmentIndex].m_multisampleState.m_samples = 1;
                        usedAttachments.set(resolveSubpassAttachment.m_attachmentIndex);
                    }
                }

                for (uint32_t inputAttachmentIndex = 0; inputAttachmentIndex < subpassLayout.m_subpassInputCount; ++inputAttachmentIndex)
                {
                    RenderPass::SubpassAttachment& subpassAttachment = subpassDescriptor.m_subpassInputAttachments[inputAttachmentIndex];
                    subpassAttachment.m_attachmentIndex = subpassLayout.m_subpassInputIndices[inputAttachmentIndex];
                    subpassAttachment.m_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                    usedAttachments.set(subpassAttachment.m_attachmentIndex);
                }

                // [GFX_TODO][ATOM-3948] Implement preserve attachments. For now preserve all attachments.
                for (uint32_t attachmentIndex = 0; attachmentIndex < renderPassDesc.m_attachmentCount; ++attachmentIndex)
                {
                    if (!usedAttachments[attachmentIndex])
                    {
                        subpassDescriptor.m_preserveAttachments[subpassDescriptor.m_preserveAttachmentCount++] = attachmentIndex;
                    }
                }
            }

            return renderPassDesc;
        }

        VkBufferUsageFlags GetBufferUsageFlagBits(RHI::BufferBindFlags bindFlags)
        {
            using BindFlags = RHI::BufferBindFlags;
            VkBufferUsageFlags usageFlags{ 0 };

            if (RHI::CheckBitsAny(bindFlags, BindFlags::InputAssembly | BindFlags::DynamicInputAssembly))
            {
                usageFlags |=
                    VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
                    VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                    VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;
            }

            if (RHI::CheckBitsAny(bindFlags, BindFlags::Constant))
            {
                usageFlags |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
            }

            if (RHI::CheckBitsAny(bindFlags, BindFlags::ShaderRead))
            {
                usageFlags |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT;
            }

            if (RHI::CheckBitsAny(bindFlags, BindFlags::ShaderWrite))
            {
                usageFlags |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT;
            }

            if (RHI::CheckBitsAny(bindFlags, BindFlags::CopyRead))
            {
                usageFlags |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
            }

            if (RHI::CheckBitsAny(bindFlags, BindFlags::CopyWrite))
            {
                usageFlags |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
            }

            if (RHI::CheckBitsAny(bindFlags, BindFlags::Predication))
            {
                usageFlags |= VK_BUFFER_USAGE_CONDITIONAL_RENDERING_BIT_EXT;
            }

            if (RHI::CheckBitsAny(bindFlags, BindFlags::Indirect))
            {
                usageFlags |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
            }

            if (RHI::CheckBitsAny(bindFlags, BindFlags::RayTracingAccelerationStructure))
            {
                usageFlags |= VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR;
            }

            if (RHI::CheckBitsAny(bindFlags, BindFlags::RayTracingShaderTable))
            {
                usageFlags |= VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR;
            }

            if (ShouldApplyDeviceAddressBit(bindFlags))
            {
                usageFlags |=  VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
            }

            return usageFlags;
        }

        bool ShouldApplyDeviceAddressBit(RHI::BufferBindFlags bindFlags)
        {
            return RHI::CheckBitsAny(
                bindFlags,
                RHI::BufferBindFlags::InputAssembly | RHI::BufferBindFlags::DynamicInputAssembly | RHI::BufferBindFlags::RayTracingShaderTable);
        }

        VkPipelineStageFlags GetSupportedPipelineStages(RHI::PipelineStateType type)
        {           
            // These stages don't need any special queue to be supported.
            VkPipelineStageFlags flags =
                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT | VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT |
                VK_PIPELINE_STAGE_HOST_BIT | VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

            switch (type)
            {
            case RHI::PipelineStateType::Draw:
                flags |=
                    VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT | VK_PIPELINE_STAGE_VERTEX_INPUT_BIT | VK_PIPELINE_STAGE_VERTEX_SHADER_BIT |
                    VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT | VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT |
                    VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
                    VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                    VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT | VK_PIPELINE_STAGE_CONDITIONAL_RENDERING_BIT_EXT |
                    VK_PIPELINE_STAGE_SHADING_RATE_IMAGE_BIT_NV | VK_PIPELINE_STAGE_TASK_SHADER_BIT_NV |
                    VK_PIPELINE_STAGE_MESH_SHADER_BIT_NV;
                break;
            case RHI::PipelineStateType::Dispatch:
                flags |=
                     VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
                break;
            default:
                AZ_Assert(false, "Invalid pipeline state type %d", type);
                break;
            }

            return flags;
        }

        VkSampleLocationEXT ConvertSampleLocation(const RHI::SamplePosition& position)
        {
            const static float cellSize = 1.0f / RHI::Limits::Pipeline::MultiSampleCustomLocationGridSize;
            return VkSampleLocationEXT{ position.m_x * cellSize, position.m_y * cellSize };
        }

        VkImageUsageFlags ImageUsageFlagsOfFormatFeatureFlags(VkFormatFeatureFlags formatFeatureFlags)
        {
            VkImageUsageFlags usageFlags{};

            if (RHI::CheckBitsAny(formatFeatureFlags, static_cast<VkFormatFeatureFlags>(VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT)))
            {
                usageFlags |= VK_IMAGE_USAGE_SAMPLED_BIT;
            }
            if (RHI::CheckBitsAny(formatFeatureFlags, static_cast<VkFormatFeatureFlags>(VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT)))
            {
                usageFlags |= VK_IMAGE_USAGE_STORAGE_BIT;
            }
            if (RHI::CheckBitsAny(formatFeatureFlags, static_cast<VkFormatFeatureFlags>(VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT)))
            {
                usageFlags |= VK_IMAGE_USAGE_STORAGE_BIT;
            }
            if (RHI::CheckBitsAny(formatFeatureFlags, static_cast<VkFormatFeatureFlags>(VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT)))
            {
                usageFlags |= (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT);
            }
            if (RHI::CheckBitsAny(formatFeatureFlags, static_cast<VkFormatFeatureFlags>(VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)))
            {
                usageFlags |= (VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT);
            }
            if (RHI::CheckBitsAny(formatFeatureFlags, static_cast<VkFormatFeatureFlags>(VK_FORMAT_FEATURE_TRANSFER_SRC_BIT)))
            {
                usageFlags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
            }
            if (RHI::CheckBitsAny(formatFeatureFlags, static_cast<VkFormatFeatureFlags>(VK_FORMAT_FEATURE_TRANSFER_DST_BIT)))
            {
                usageFlags |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
            }

            return usageFlags;
        }

        VkAccessFlags GetSupportedAccessFlags(VkPipelineStageFlags pipelineStageFlags)
        {
            // The initial access flags don't need special stages.
            VkAccessFlags accessFlagBits = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
            if (RHI::CheckBitsAny(pipelineStageFlags, static_cast<VkPipelineStageFlags>(VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT)))
            {
                accessFlagBits |= VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
            }

            if (RHI::CheckBitsAny(pipelineStageFlags, static_cast<VkPipelineStageFlags>(VK_PIPELINE_STAGE_VERTEX_INPUT_BIT)))
            {
                accessFlagBits |= VK_ACCESS_INDEX_READ_BIT | VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
            }

            if (RHI::CheckBitsAny(
                pipelineStageFlags, 
                static_cast<VkPipelineStageFlags>(VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT |
                    VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT | VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT |
                    VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_TASK_SHADER_BIT_NV | VK_PIPELINE_STAGE_MESH_SHADER_BIT_NV)))
            {
                accessFlagBits |= VK_ACCESS_UNIFORM_READ_BIT | VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
            }

            if (RHI::CheckBitsAny(pipelineStageFlags, static_cast<VkPipelineStageFlags>(VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT)))
            {
                accessFlagBits |= VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
            }

            if (RHI::CheckBitsAny(pipelineStageFlags, static_cast<VkPipelineStageFlags>(VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT)))
            {
                accessFlagBits |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            }

            if (RHI::CheckBitsAny(pipelineStageFlags, static_cast<VkPipelineStageFlags>(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT)))
            {
                accessFlagBits |= VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_NONCOHERENT_BIT_EXT;
            }

            if (RHI::CheckBitsAny(pipelineStageFlags, static_cast<VkPipelineStageFlags>(VK_PIPELINE_STAGE_TRANSFER_BIT)))
            {
                accessFlagBits |= VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
            }

            if (RHI::CheckBitsAny(pipelineStageFlags, static_cast<VkPipelineStageFlags>(VK_PIPELINE_STAGE_HOST_BIT)))
            {
                accessFlagBits |= VK_ACCESS_HOST_READ_BIT | VK_ACCESS_HOST_WRITE_BIT;
            }

            if (RHI::CheckBitsAny(pipelineStageFlags, static_cast<VkPipelineStageFlags>(VK_PIPELINE_STAGE_CONDITIONAL_RENDERING_BIT_EXT)))
            {
                accessFlagBits |= VK_ACCESS_CONDITIONAL_RENDERING_READ_BIT_EXT;
            }

            if (RHI::CheckBitsAny(pipelineStageFlags, static_cast<VkPipelineStageFlags>(VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT)))
            {
                accessFlagBits |= VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
            }

            return accessFlagBits;
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

            return stagesFlags;
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
                    accessFlags |= RHI::CheckBitsAny(usageAndAccess.m_access, RHI::ScopeAttachmentAccess::Write) ? VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT : accessFlags;
                    accessFlags |= RHI::CheckBitsAny(usageAndAccess.m_access, RHI::ScopeAttachmentAccess::Read) ? VK_ACCESS_COLOR_ATTACHMENT_READ_BIT : accessFlags;
                    break;
                case RHI::ScopeAttachmentUsage::Resolve:
                    accessFlags |= VK_ACCESS_TRANSFER_WRITE_BIT;
                    break;
                case RHI::ScopeAttachmentUsage::DepthStencil:
                    accessFlags |= RHI::CheckBitsAny(usageAndAccess.m_access, RHI::ScopeAttachmentAccess::Write) ? VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT : accessFlags;
                    accessFlags |= RHI::CheckBitsAny(usageAndAccess.m_access, RHI::ScopeAttachmentAccess::Read) ? VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT : accessFlags;
                    break;
                case RHI::ScopeAttachmentUsage::SubpassInput:
                case RHI::ScopeAttachmentUsage::Shader:
                    accessFlags |= RHI::CheckBitsAny(usageAndAccess.m_access, RHI::ScopeAttachmentAccess::Write) ? VK_ACCESS_SHADER_WRITE_BIT : accessFlags;
                    accessFlags |= RHI::CheckBitsAny(usageAndAccess.m_access, RHI::ScopeAttachmentAccess::Read) ? VK_ACCESS_SHADER_READ_BIT : accessFlags;
                    break;
                case RHI::ScopeAttachmentUsage::Copy:
                    accessFlags |= RHI::CheckBitsAny(usageAndAccess.m_access, RHI::ScopeAttachmentAccess::Write) ? VK_ACCESS_TRANSFER_WRITE_BIT : accessFlags;
                    accessFlags |= RHI::CheckBitsAny(usageAndAccess.m_access, RHI::ScopeAttachmentAccess::Read) ? VK_ACCESS_TRANSFER_READ_BIT : accessFlags;
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

            return accessFlags;
        }

        VkImageLayout GetImageAttachmentLayout(const RHI::ImageScopeAttachment& imageAttachment)
        {
            const AZStd::vector<RHI::ScopeAttachmentUsageAndAccess>& usagesAndAccesses = imageAttachment.GetUsageAndAccess();
            if (usagesAndAccesses.size() > 1)
            {
                //[GFX TODO][ATOM-4779] -Multiple Usage/Access can be further optimized. For now VK_IMAGE_LAYOUT_GENERAL is the fallback.
                return VK_IMAGE_LAYOUT_GENERAL;
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
                return RHI::CheckBitsAny(usagesAndAccesses.front().m_access, RHI::ScopeAttachmentAccess::Write) ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
            case RHI::ScopeAttachmentUsage::Shader:
            case RHI::ScopeAttachmentUsage::SubpassInput:
                // If we are reading from a depth/stencil texture, then we use the depth/stencil read optimal layout instead of the generic shader read one.
                if (RHI::CheckBitsAny(imageAspects, RHI::ImageAspectFlags::DepthStencil))
                {
                    return RHI::CheckBitsAny(usagesAndAccesses.front().m_access, RHI::ScopeAttachmentAccess::Write) ? VK_IMAGE_LAYOUT_GENERAL : VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
                }
                else
                {
                    return RHI::CheckBitsAny(usagesAndAccesses.front().m_access, RHI::ScopeAttachmentAccess::Write) ? VK_IMAGE_LAYOUT_GENERAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                }                
            case RHI::ScopeAttachmentUsage::Copy:
                return RHI::CheckBitsAny(usagesAndAccesses.front().m_access, RHI::ScopeAttachmentAccess::Write) ? VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL : VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            default:
                return VK_IMAGE_LAYOUT_GENERAL;
            }

        }
    }
}
