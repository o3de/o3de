/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI.Reflect/Vulkan/Conversion.h>
#include <Atom/RHI.Reflect/ImageDescriptor.h>
#include <Atom/RHI.Reflect/ImageSubresource.h>
#include <Atom/RHI.Reflect/RenderAttachmentLayout.h>
#include <Atom/RHI.Reflect/ShaderResourceGroupLayout.h>
#include <AzCore/std/algorithm.h>
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

        RHI::ImageAspectFlags ConvertImageAspectFlags(VkImageAspectFlags imageAspect)
        {
            RHI::ImageAspectFlags flags = {};
            if (RHI::CheckBitsAll(imageAspect, static_cast<VkImageAspectFlags>(VK_IMAGE_ASPECT_COLOR_BIT)))
            {
                flags |= RHI::ImageAspectFlags::Color;
            }

            if (RHI::CheckBitsAll(imageAspect, static_cast<VkImageAspectFlags>(VK_IMAGE_ASPECT_DEPTH_BIT)))
            {
                flags |= RHI::ImageAspectFlags::Depth;
            }

            if (RHI::CheckBitsAll(imageAspect, static_cast<VkImageAspectFlags>(VK_IMAGE_ASPECT_STENCIL_BIT)))
            {
                flags |= RHI::ImageAspectFlags::Stencil;
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
                return VK_QUEUE_TRANSFER_BIT|VK_QUEUE_SPARSE_BINDING_BIT;
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
                return VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
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

        VkShaderStageFlagBits ConvertShaderStage(RHI::ShaderStage stage, [[maybe_unused]] uint32_t subStageIndex /* = 0 */)
        {
            switch (stage)
            {
            case RHI::ShaderStage::Vertex:
                return VK_SHADER_STAGE_VERTEX_BIT;
            case RHI::ShaderStage::Fragment:
                return VK_SHADER_STAGE_FRAGMENT_BIT;
            case RHI::ShaderStage::Compute:
                return VK_SHADER_STAGE_COMPUTE_BIT;
            case RHI::ShaderStage::Geometry:
                return VK_SHADER_STAGE_GEOMETRY_BIT;
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
                usageFlags |= VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;
            }

            if (RHI::CheckBitsAny(bindFlags, BindFlags::RayTracingShaderTable))
            {
                usageFlags |= VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR;
            }

            if (RHI::CheckBitsAny(
                    bindFlags,
                    RHI::BufferBindFlags::InputAssembly | RHI::BufferBindFlags::DynamicInputAssembly |
                        RHI::BufferBindFlags::RayTracingShaderTable | RHI::BufferBindFlags::RayTracingAccelerationStructure |
                        RHI::BufferBindFlags::RayTracingScratchBuffer | RHI::BufferBindFlags::Indirect))
            {
                usageFlags |=  VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
            }

            return usageFlags;
        }

        VkSampleLocationEXT ConvertSampleLocation(const RHI::SamplePosition& position)
        {
            const static float cellSize = 1.0f / RHI::Limits::Pipeline::MultiSampleCustomLocationGridSize;
            return VkSampleLocationEXT{ position.m_x * cellSize, position.m_y * cellSize };
        }

        VkFragmentShadingRateCombinerOpKHR ConvertShadingRateCombiner(const RHI::ShadingRateCombinerOp op)
        {
            switch (op)
            {
                case RHI::ShadingRateCombinerOp::Max: return VK_FRAGMENT_SHADING_RATE_COMBINER_OP_MAX_KHR;
                case RHI::ShadingRateCombinerOp::Min: return VK_FRAGMENT_SHADING_RATE_COMBINER_OP_MIN_KHR;
                case RHI::ShadingRateCombinerOp::Override: return VK_FRAGMENT_SHADING_RATE_COMBINER_OP_REPLACE_KHR;
                case RHI::ShadingRateCombinerOp::Passthrough: return VK_FRAGMENT_SHADING_RATE_COMBINER_OP_KEEP_KHR;
                default:
                    AZ_Assert(false, "Invalid ShadingRateCombinerOp %d", op);
                    return VK_FRAGMENT_SHADING_RATE_COMBINER_OP_KEEP_KHR;
            }
        }

        VkExtent2D ConvertFragmentShadingRate(const RHI::ShadingRate rate)
        {
            VkExtent2D fragmentSize;
            switch (rate)
            {
            case RHI::ShadingRate::Rate1x1:
                fragmentSize.width = fragmentSize.height = 1;
                break;
            case RHI::ShadingRate::Rate1x2:
                fragmentSize.width = 1;
                fragmentSize.height = 2;
                break;
            case RHI::ShadingRate::Rate2x1:
                fragmentSize.width = 2;
                fragmentSize.height = 1;
                break;
            case RHI::ShadingRate::Rate2x2:
                fragmentSize.width = fragmentSize.height = 2;
                break;
            case RHI::ShadingRate::Rate2x4:
                fragmentSize.width = 2;
                fragmentSize.height = 4;
                break;
            case RHI::ShadingRate::Rate4x2:
                fragmentSize.width = 4;
                fragmentSize.height = 2;
                break;
            case RHI::ShadingRate::Rate4x1:
                fragmentSize.width = 4;
                fragmentSize.height = 1;
                break;
            case RHI::ShadingRate::Rate1x4:
                fragmentSize.width = 1;
                fragmentSize.height = 4;
                break;
            case RHI::ShadingRate::Rate4x4:
                fragmentSize.width = fragmentSize.height = 4;
                break;
            default:
                AZ_Assert(false, "Invalid shading rate %d", rate);
                fragmentSize.width = fragmentSize.height = 1;
                break;
            }

            return fragmentSize;
        }

        RHI::ShadingRate ConvertFragmentShadingRate(const VkExtent2D rate)
        {
            switch (rate.width)
            {
            case 1:
                switch (rate.height)
                {
                case 1: return RHI::ShadingRate::Rate1x1;
                case 2: return RHI::ShadingRate::Rate1x2;
                case 4: return RHI::ShadingRate::Rate1x4;
                default:
                    break;
                }
            case 2:
                switch (rate.height)
                {
                case 1: return RHI::ShadingRate::Rate2x1;
                case 2: return RHI::ShadingRate::Rate2x2;
                case 4: return RHI::ShadingRate::Rate2x4;
                default:
                    break;
                }
            case 4:
                switch (rate.height)
                {
                case 1: return RHI::ShadingRate::Rate4x1;
                case 2: return RHI::ShadingRate::Rate4x2;
                case 4: return RHI::ShadingRate::Rate4x4;
                default:
                    break;
                }
            }
            AZ_Assert(false, "Invalid rate for conversion (%d, %d)", rate.width, rate.height);
            return RHI::ShadingRate::Rate1x1;
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
            if (RHI::CheckBitsAny(formatFeatureFlags, static_cast<VkFormatFeatureFlags>(VK_FORMAT_FEATURE_FRAGMENT_DENSITY_MAP_BIT_EXT)))
            {
                usageFlags |= VK_IMAGE_USAGE_FRAGMENT_DENSITY_MAP_BIT_EXT;
            }
            if (RHI::CheckBitsAny(
                    formatFeatureFlags, static_cast<VkFormatFeatureFlags>(VK_FORMAT_FEATURE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR)))
            {
                usageFlags |= VK_IMAGE_USAGE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR;
            }
            return usageFlags;
        }

        VkAccessFlags GetSupportedAccessFlags(VkPipelineStageFlags pipelineStageFlags)
        {
            if (RHI::CheckBitsAny(pipelineStageFlags, static_cast<VkPipelineStageFlags>(VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT)) ||
                RHI::CheckBitsAny(pipelineStageFlags, static_cast<VkPipelineStageFlags>(VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT)))
            {
                return VK_ACCESS_NONE;
            }

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

            if (RHI::CheckBitsAny(
                    pipelineStageFlags, static_cast<VkPipelineStageFlags>(VK_PIPELINE_STAGE_FRAGMENT_DENSITY_PROCESS_BIT_EXT)))
            {
                accessFlagBits |= VK_ACCESS_FRAGMENT_DENSITY_MAP_READ_BIT_EXT;
            }

            if (RHI::CheckBitsAny(
                    pipelineStageFlags, static_cast<VkPipelineStageFlags>(VK_PIPELINE_STAGE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR)))
            {
                accessFlagBits |= VK_ACCESS_FRAGMENT_SHADING_RATE_ATTACHMENT_READ_BIT_KHR;
            }

            return accessFlagBits;
        }

        VkComponentSwizzle ConvertComponentSwizzle(const ImageComponentMapping::Swizzle swizzle)
        {
            switch (swizzle)
            {
            case ImageComponentMapping::Swizzle::Identity:
                return VkComponentSwizzle::VK_COMPONENT_SWIZZLE_IDENTITY;
            case ImageComponentMapping::Swizzle::Zero:
                return VkComponentSwizzle::VK_COMPONENT_SWIZZLE_ZERO;
            case ImageComponentMapping::Swizzle::One:
                return VkComponentSwizzle::VK_COMPONENT_SWIZZLE_ONE;
            case ImageComponentMapping::Swizzle::R:
                return VkComponentSwizzle::VK_COMPONENT_SWIZZLE_R;
            case ImageComponentMapping::Swizzle::G:
                return VkComponentSwizzle::VK_COMPONENT_SWIZZLE_G;
            case ImageComponentMapping::Swizzle::B:
                return VkComponentSwizzle::VK_COMPONENT_SWIZZLE_B;
            case ImageComponentMapping::Swizzle::A:
                return VkComponentSwizzle::VK_COMPONENT_SWIZZLE_A;
            default:
                AZ_Assert(false, "Invalid component swizzle %d", swizzle);
                return VkComponentSwizzle::VK_COMPONENT_SWIZZLE_IDENTITY;
            }
        }

        VkComponentMapping ConvertComponentMapping(const ImageComponentMapping& mapping)
        {
            VkComponentMapping vkMapping;
            vkMapping.r = ConvertComponentSwizzle(mapping.m_red);
            vkMapping.g = ConvertComponentSwizzle(mapping.m_green);
            vkMapping.b = ConvertComponentSwizzle(mapping.m_blue);
            vkMapping.a = ConvertComponentSwizzle(mapping.m_alpha);
            return vkMapping;
        }

        RHI::ImageSubresourceRange ConvertSubresourceRange(const VkImageSubresourceRange& range)
        {
            RHI::ImageSubresourceRange rhiRange;
            rhiRange.m_aspectFlags = ConvertImageAspectFlags(range.aspectMask);
            rhiRange.m_mipSliceMin = static_cast<uint16_t>(range.baseMipLevel);
            rhiRange.m_mipSliceMax = static_cast<uint16_t>(range.baseMipLevel + range.levelCount - 1);
            rhiRange.m_arraySliceMin = static_cast<uint16_t>(range.baseArrayLayer);
            rhiRange.m_arraySliceMax = static_cast<uint16_t>(range.baseArrayLayer + range.layerCount - 1);
            return rhiRange;
        }

        VkImageSubresourceRange ConvertSubresourceRange(const RHI::ImageSubresourceRange& range)
        {
            VkImageSubresourceRange vkRange = {};
            vkRange.aspectMask = ConvertImageAspectFlags(range.m_aspectFlags);
            vkRange.baseMipLevel = range.m_mipSliceMin;
            vkRange.levelCount = range.m_mipSliceMax - range.m_mipSliceMin + 1;
            vkRange.baseArrayLayer = range.m_arraySliceMin;
            vkRange.layerCount = range.m_arraySliceMax - range.m_arraySliceMin + 1;
            return vkRange;
        }

        VkPipelineStageFlags ConvertScopeAttachmentStage(const RHI::ScopeAttachmentStage& stage)
        {
            VkPipelineStageFlags flags = {};
            if (RHI::CheckBitsAll(stage, RHI::ScopeAttachmentStage::VertexShader))
            {
                flags |= VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
            }
            if (RHI::CheckBitsAll(stage, RHI::ScopeAttachmentStage::FragmentShader))
            {
                flags |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            }
            if (RHI::CheckBitsAll(stage, RHI::ScopeAttachmentStage::ComputeShader))
            {
                flags |= VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
            }
            if (RHI::CheckBitsAll(stage, RHI::ScopeAttachmentStage::RayTracingShader))
            {
                flags |= VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR;
            }
            if (RHI::CheckBitsAll(stage, RHI::ScopeAttachmentStage::EarlyFragmentTest))
            {
                flags |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
            }
            if (RHI::CheckBitsAll(stage, RHI::ScopeAttachmentStage::LateFragmentTest))
            {
                flags |= VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
            }
            if (RHI::CheckBitsAll(stage, RHI::ScopeAttachmentStage::ColorAttachmentOutput))
            {
                flags |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            }
            if (RHI::CheckBitsAll(stage, RHI::ScopeAttachmentStage::Copy))
            {
                flags |= VK_PIPELINE_STAGE_TRANSFER_BIT;
            }
            if (RHI::CheckBitsAll(stage, RHI::ScopeAttachmentStage::Predication))
            {
                flags |= VK_PIPELINE_STAGE_CONDITIONAL_RENDERING_BIT_EXT;
            }
            if (RHI::CheckBitsAll(stage, RHI::ScopeAttachmentStage::DrawIndirect))
            {
                flags |= VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT;
            }
            if (RHI::CheckBitsAll(stage, RHI::ScopeAttachmentStage::VertexInput))
            {
                flags |= VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
            }
            if (RHI::CheckBitsAll(stage, RHI::ScopeAttachmentStage::ShadingRate))
            {
                flags |= VK_PIPELINE_STAGE_FRAGMENT_DENSITY_PROCESS_BIT_EXT | VK_PIPELINE_STAGE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR;
            }
            return flags;
        }
    }
}
