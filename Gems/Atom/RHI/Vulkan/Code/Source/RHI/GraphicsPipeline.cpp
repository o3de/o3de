/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/std/algorithm.h>
#include <Atom/RHI.Reflect/RenderStates.h>
#include <Atom/RHI.Reflect/InputStreamLayout.h>
#include <Atom/RHI.Reflect/Vulkan/Conversion.h>
#include <RHI/Device.h>
#include <RHI/GraphicsPipeline.h>
#include <RHI/RenderPass.h>
#include <RHI/PipelineLibrary.h>
#include <Atom/RHI.Reflect/VkAllocator.h>

namespace AZ
{
    namespace Vulkan
    {
        RHI::Ptr<GraphicsPipeline> GraphicsPipeline::Create()
        {
            return aznew GraphicsPipeline();
        }

        RHI::ResultCode GraphicsPipeline::InitInternal(const Descriptor& descriptor, const PipelineLayout& pipelineLayout)
        {
            AZ_Assert(descriptor.m_pipelineDescritor, "Pipeline State Draw Descriptor is null.");
            AZ_Assert(descriptor.m_pipelineDescritor->GetType() == RHI::PipelineStateType::Draw, "Invalid pipeline descriptor type");

            const auto* drawDescriptor = static_cast<const RHI::PipelineStateDescriptorForDraw*>(descriptor.m_pipelineDescritor);
            const RHI::RenderAttachmentLayout& renderAttachmentLayout = drawDescriptor->m_renderAttachmentConfiguration.m_renderAttachmentLayout;
            auto renderpassDescriptor = RenderPass::ConvertRenderAttachmentLayout(
                *descriptor.m_device, renderAttachmentLayout, drawDescriptor->m_renderStates.m_multisampleState);
            m_renderPass = descriptor.m_device->AcquireRenderPass(renderpassDescriptor);

            return BuildNativePipeline(descriptor, pipelineLayout);
        }

        void GraphicsPipeline::Shutdown()
        {
            m_renderPass = nullptr;

            Base::Shutdown();
        }

        void GraphicsPipeline::SetNameInternal(const AZStd::string_view& name)
        {
            if (m_renderPass)
            {
                m_renderPass->SetName(AZ::Name(name));
            }
            Base::SetNameInternal(name);
        }

        RHI::ResultCode GraphicsPipeline::BuildNativePipeline(const Descriptor& descriptor, const PipelineLayout& pipelineLayout)
        {
            AZ_Assert(m_renderPass, "RenderPass is null.");

            auto& device = static_cast<Device&>(*descriptor.m_device);
            const auto& drawDescriptor = static_cast<const RHI::PipelineStateDescriptorForDraw&>(*descriptor.m_pipelineDescritor);

            const RHI::InputStreamLayout& inputStreamLayout = drawDescriptor.m_inputStreamLayout;
            const RHI::RenderAttachmentConfiguration& renderTargetConfig = drawDescriptor.m_renderAttachmentConfiguration;
            const RHI::RasterState& rasterState = drawDescriptor.m_renderStates.m_rasterState;
            const RHI::MultisampleState& multisampleState = drawDescriptor.m_renderStates.m_multisampleState;
            const RHI::DepthStencilState& depthStencilState = drawDescriptor.m_renderStates.m_depthStencilState;
            const RHI::BlendState& blendState = drawDescriptor.m_renderStates.m_blendState;

            auto result = BuildPipelineRasterizationStateCreateInfo(device, rasterState);
            RETURN_RESULT_IF_UNSUCCESSFUL(result);

            BuildPipelineShaderStageCreateInfo(drawDescriptor);
            BuildPipelineVertexInputStateCreateInfo(inputStreamLayout);
            BuildPipelineInputAssemblyStateInfo(drawDescriptor.m_inputStreamLayout.GetTopology());
            BuildPipelineTessellationStateCreatInfo();
            BuildPipelineViewportStateCreateInfo();
            BuildPipelineMultisampleStateCreateInfo(multisampleState, blendState);
            BuildPipelineDepthStencilStateCreateInfo(depthStencilState);
            BuildPipelineColorBlendStateCreateInfo(blendState, renderTargetConfig.GetRenderTargetCount());
            BuildPipelineDynamicStateCreateInfo();

            AZ_Assert(!m_pipelineShaderStageCreateInfos.empty(), "There is no shader stages.");

            VkGraphicsPipelineCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
            createInfo.pNext = nullptr;
            createInfo.flags = 0;
            createInfo.stageCount = static_cast<uint32_t>(m_pipelineShaderStageCreateInfos.size());
            createInfo.pStages = m_pipelineShaderStageCreateInfos.data();
            createInfo.pVertexInputState = &m_pipelineVertexInputStateCreateInfo;
            createInfo.pInputAssemblyState = &m_pipelineInputAssemblyStateCreateInfo;
            createInfo.pTessellationState = &m_pipelineTessellationStateCreateInfo;
            createInfo.pViewportState = &m_pipelineViewportStateCreateInfo;
            createInfo.pRasterizationState = &m_pipelineRasterizationStateCreateInfo;
            createInfo.pMultisampleState = &m_pipelineMultisampleStateCreateInfo;
            createInfo.pDepthStencilState = &m_pipelineDepthStencilStateCreateInfo;
            createInfo.pColorBlendState = &m_pipelineColorBlendStateCreateInfo;
            createInfo.pDynamicState = &m_pipelineDynamicStateCreateInfo;
            createInfo.layout = pipelineLayout.GetNativePipelineLayout();
            createInfo.renderPass = m_renderPass->GetNativeRenderPass();
            createInfo.subpass = drawDescriptor.m_renderAttachmentConfiguration.m_subpassIndex;
            createInfo.basePipelineHandle = VK_NULL_HANDLE;
            createInfo.basePipelineIndex = -1;

            const VkPipelineCache pipelineCache = descriptor.m_pipelineLibrary ? descriptor.m_pipelineLibrary->GetNativePipelineCache() : VK_NULL_HANDLE;

            const VkResult vkResult = descriptor.m_device->GetContext().CreateGraphicsPipelines(
                descriptor.m_device->GetNativeDevice(), pipelineCache, 1, &createInfo, VkSystemAllocator::Get(), &GetNativePipelineRef());

            return ConvertResult(vkResult);
        }

        void GraphicsPipeline::BuildPipelineShaderStageCreateInfo(const RHI::PipelineStateDescriptorForDraw& descriptor)
        {
            m_pipelineShaderStageCreateInfos.reserve(static_cast<size_t>(RHI::ShaderStage::GraphicsCount) * ShaderSubStageCountMax);

            ShaderStageFunction const* func = nullptr;

            func = static_cast<ShaderStageFunction const*>(descriptor.m_vertexFunction.get());
            AZ_Assert(func, "VertexFunction is null.");
            m_pipelineShaderStageCreateInfos.emplace_back();
            FillPipelineShaderStageCreateInfo(*func, RHI::ShaderStage::Vertex, ShaderSubStage::Default, *m_pipelineShaderStageCreateInfos.rbegin());

            if (GetDevice().GetFeatures().m_geometryShader)
            {
                func = static_cast<ShaderStageFunction const*>(descriptor.m_geometryFunction.get());
                if (func)
                {
                    m_pipelineShaderStageCreateInfos.emplace_back();
                    FillPipelineShaderStageCreateInfo(
                        *func, RHI::ShaderStage::Geometry, ShaderSubStage::Default, *m_pipelineShaderStageCreateInfos.rbegin());
                }
            }

            func = static_cast<ShaderStageFunction const*>(descriptor.m_fragmentFunction.get());
            if (func)
            {
                m_pipelineShaderStageCreateInfos.emplace_back();
                FillPipelineShaderStageCreateInfo(*func, RHI::ShaderStage::Fragment, ShaderSubStage::Default, *m_pipelineShaderStageCreateInfos.rbegin());
            }
        }

        void GraphicsPipeline::BuildPipelineVertexInputStateCreateInfo(const RHI::InputStreamLayout& inputStreamLayout)
        {
            const auto& streamChannels = inputStreamLayout.GetStreamChannels();
            m_vertexInputAttributeDescriptions.resize(streamChannels.size());
            for (uint32_t index = 0; index < streamChannels.size(); ++index)
            {
                FillVertexInputAttributeDescription(inputStreamLayout, index, m_vertexInputAttributeDescriptions[index]);
            }

            const auto& streamBuffers = inputStreamLayout.GetStreamBuffers();
            m_vertexInputBindingDescriptions.resize(streamBuffers.size());
            for (uint32_t index = 0; index < streamBuffers.size(); ++index)
            {
                FillVertexInputBindingDescription(inputStreamLayout, index, m_vertexInputBindingDescriptions[index]);
            }

            VkPipelineVertexInputStateCreateInfo& createInfo = m_pipelineVertexInputStateCreateInfo;
            createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
            createInfo.pNext = nullptr;
            createInfo.flags = 0;
            createInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(m_vertexInputBindingDescriptions.size());
            createInfo.pVertexBindingDescriptions = m_vertexInputBindingDescriptions.empty() ? nullptr : m_vertexInputBindingDescriptions.data();
            createInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(m_vertexInputAttributeDescriptions.size());
            createInfo.pVertexAttributeDescriptions = m_vertexInputAttributeDescriptions.empty() ? nullptr : m_vertexInputAttributeDescriptions.data();
        }

        void GraphicsPipeline::FillVertexInputAttributeDescription(const RHI::InputStreamLayout& inputStreamLayout,
            uint32_t index, VkVertexInputAttributeDescription& desc)
        {
            AZ_Assert(index < inputStreamLayout.GetStreamChannels().size(), "Index is wrong.");
            const auto streamChannels = inputStreamLayout.GetStreamChannels();
            const RHI::StreamChannelDescriptor& chanDesc = streamChannels[index];

            desc = {};
            desc.location = index;
            desc.binding = chanDesc.m_bufferIndex;
            desc.format = ConvertFormat(chanDesc.m_format);
            desc.offset = chanDesc.m_byteOffset;
        }

        void GraphicsPipeline::FillVertexInputBindingDescription(const RHI::InputStreamLayout& inputStreamLayout,
            uint32_t index, VkVertexInputBindingDescription& desc)
        {
            AZ_Assert(index < inputStreamLayout.GetStreamBuffers().size(), "Index is wrong.");
            const auto streamBuffers = inputStreamLayout.GetStreamBuffers();
            const RHI::StreamBufferDescriptor& buffDesc = streamBuffers[index];

            VkVertexInputRate inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
            switch (buffDesc.m_stepFunction)
            {
            case RHI::StreamStepFunction::PerVertex:
                inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
                break;
            case RHI::StreamStepFunction::PerInstance:
                inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;
                break;
            default:
                AZ_Assert(false, "Cannot recognize Stream Step Function");
            }

            desc = {};
            desc.binding = index;
            desc.stride = buffDesc.m_byteStride;
            desc.inputRate = inputRate;

            if (desc.stride == 0)
            {
                const auto streamChannels = inputStreamLayout.GetStreamChannels();

                for (uint32_t channelIndex = 0; channelIndex < streamChannels.size(); ++channelIndex)
                {
                    const RHI::StreamChannelDescriptor& chanDesc = streamChannels[channelIndex];
                    if (chanDesc.m_bufferIndex == index)
                    {
                        desc.stride += GetFormatSize(chanDesc.m_format);
                    }
                }
            }
        }

        void GraphicsPipeline::BuildPipelineInputAssemblyStateInfo(RHI::PrimitiveTopology topology)
        {
            VkPipelineInputAssemblyStateCreateInfo& info = m_pipelineInputAssemblyStateCreateInfo;
            info = {};
            info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
            info.pNext = nullptr;
            info.flags = 0;
            info.topology = ConvertTopology(topology);
            info.primitiveRestartEnable = VK_FALSE;
        }

        void GraphicsPipeline::BuildPipelineTessellationStateCreatInfo()
        {
            // [GFX TODO][ATOM-365] Tesellation support.
            VkPipelineTessellationStateCreateInfo& info = m_pipelineTessellationStateCreateInfo;
            info = {};
            info.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
        }

        void GraphicsPipeline::BuildPipelineViewportStateCreateInfo()
        {
            VkPipelineViewportStateCreateInfo& info = m_pipelineViewportStateCreateInfo;
            info = {};
            info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
            info.pNext = nullptr;
            info.flags = 0;
            info.viewportCount = 1; // it must be positive even if multiple viewport is not enabled.
            info.pViewports = nullptr; // it will be set dynamically.
            info.scissorCount = 1; // it must be positive even if multiple viewport is not enabled.
            info.pScissors = nullptr; // it will be set dynamically.
        }

        RHI::ResultCode GraphicsPipeline::BuildPipelineRasterizationStateCreateInfo(const Device& device, const RHI::RasterState& rasterState)
        {
            auto& physicalDevice = static_cast<const PhysicalDevice&>(device.GetPhysicalDevice());
            auto& enabledFeatures = device.GetEnabledDevicesFeatures();

            if (rasterState.m_forcedSampleCount != 0)
            {
                AZ_Error("Vulkan", false, "Force sample count is being used but it's not supported on this device");
                return RHI::ResultCode::InvalidArgument;
            }

            VkPipelineRasterizationStateCreateInfo& info = m_pipelineRasterizationStateCreateInfo;
            info = {};
            info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
            info.pNext = nullptr;
            info.depthClampEnable = VK_FALSE;
            info.flags = 0;
            info.rasterizerDiscardEnable = VK_FALSE;

            VkPipelineRasterizationDepthClipStateCreateInfoEXT& depthClipState = m_pipelineRasterizationDepthClipStateInfo;
            depthClipState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_DEPTH_CLIP_STATE_CREATE_INFO_EXT;
            if (physicalDevice.IsFeatureSupported(DeviceFeature::DepthClipEnable))
            {
                depthClipState.depthClipEnable = rasterState.m_depthClipEnable;
                info.pNext = &depthClipState;
            }
            else if (enabledFeatures.depthClamp)
            {
                // This is not 100% correct, but in most cases it will give us the correct result.
                info.depthClampEnable = rasterState.m_depthClipEnable ? VK_FALSE : VK_TRUE;
            }

            switch (rasterState.m_fillMode)
            {
            case RHI::FillMode::Solid:
                info.polygonMode = VK_POLYGON_MODE_FILL;
                break;
            case RHI::FillMode::Wireframe:
            {
                if (!enabledFeatures.fillModeNonSolid)
                {
                    AZ_Error("Vulkan", false, "Wireframe fill mode is being used but it's not supported on this device");
                    return RHI::ResultCode::InvalidArgument;
                }
                info.polygonMode = VK_POLYGON_MODE_LINE;
                break;
            }
            default:
                info.polygonMode = VK_POLYGON_MODE_FILL;
                AZ_Assert(false, "Fill mode is invalid.");
                break;
            }

            switch (rasterState.m_cullMode)
            {
            case RHI::CullMode::None:
                info.cullMode = VK_CULL_MODE_NONE;
                break;
            case RHI::CullMode::Front:
                info.cullMode = VK_CULL_MODE_FRONT_BIT;
                break;
            case RHI::CullMode::Back:
                info.cullMode = VK_CULL_MODE_BACK_BIT;
                break;
            default:
                info.cullMode = VK_CULL_MODE_NONE;
                AZ_Assert(false, "Cull mode is invalid.");
                break;
            }

            info.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

            if (!enabledFeatures.depthBiasClamp && rasterState.m_depthBiasClamp != 0.f)
            {
                AZ_Error("Vulkan", false, "Depth Bias Clamp is being used but it's not supported on this device");
                return RHI::ResultCode::InvalidArgument;
            }

            info.depthBiasEnable = rasterState.m_depthBias == 0 && rasterState.m_depthBiasClamp == 0.f && rasterState.m_depthBiasSlopeScale == 0.f ? VK_FALSE :VK_TRUE;
            info.depthBiasConstantFactor = static_cast<float>(rasterState.m_depthBias);
            info.depthBiasClamp = rasterState.m_depthBiasClamp;
            info.depthBiasSlopeFactor = rasterState.m_depthBiasSlopeScale;

            info.lineWidth = 1;

            VkPipelineRasterizationConservativeStateCreateInfoEXT& conservativeRasterization = m_pipelineRasterizationConservativeInfo;
            conservativeRasterization.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_CONSERVATIVE_STATE_CREATE_INFO_EXT;
            if (physicalDevice.IsFeatureSupported(DeviceFeature::ConservativeRaster))
            {
                conservativeRasterization.conservativeRasterizationMode = rasterState.m_conservativeRasterEnable ? VK_CONSERVATIVE_RASTERIZATION_MODE_OVERESTIMATE_EXT : VK_CONSERVATIVE_RASTERIZATION_MODE_DISABLED_EXT;
                conservativeRasterization.extraPrimitiveOverestimationSize = physicalDevice.GetPhysicalDeviceConservativeRasterProperties().maxExtraPrimitiveOverestimationSize;
                conservativeRasterization.pNext = info.pNext;
                info.pNext = &conservativeRasterization;
            }
            else if (rasterState.m_conservativeRasterEnable)
            {
                AZ_Error("Vulkan", false, "Conservative rasterization is being used but it's not supported on this device");
                return RHI::ResultCode::InvalidArgument;
            }

            return RHI::ResultCode::Success;
        }

        void GraphicsPipeline::BuildPipelineMultisampleStateCreateInfo(const RHI::MultisampleState& multisampleState, const RHI::BlendState& blendState)
        {
            VkPipelineMultisampleStateCreateInfo& info = m_pipelineMultisampleStateCreateInfo;
            info = {};
            info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
            info.flags = 0;
            info.rasterizationSamples = ConvertSampleCount(multisampleState.m_samples);

            info.sampleShadingEnable = VK_FALSE;
            info.pSampleMask = nullptr;

            info.alphaToCoverageEnable = blendState.m_alphaToCoverageEnable ? VK_TRUE : VK_FALSE;
            info.alphaToOneEnable = VK_FALSE;

            if (multisampleState.m_customPositionsCount > 0)
            {
                auto& physicalDevice = static_cast<const PhysicalDevice&>(GetDevice().GetPhysicalDevice());
                if (physicalDevice.IsFeatureSupported(DeviceFeature::CustomSampleLocation))
                {
                    AZStd::transform(
                        multisampleState.m_customPositions.begin(),
                        multisampleState.m_customPositions.begin() + multisampleState.m_customPositionsCount,
                        AZStd::back_inserter(m_customSampleLocations), [&](const auto& item)
                    {
                        return ConvertSampleLocation(item);
                    });

                    AZ_Assert(
                        multisampleState.m_customPositionsCount >= static_cast<uint32_t>(info.rasterizationSamples),
                        "Sample locations is smaller than rasterization samples %d < %d",
                        static_cast<int>(multisampleState.m_customPositionsCount),
                        static_cast<int>(info.rasterizationSamples));

                    VkSampleLocationsInfoEXT sampleLocations = {};
                    sampleLocations.sType = VK_STRUCTURE_TYPE_SAMPLE_LOCATIONS_INFO_EXT;
                    sampleLocations.sampleLocationGridSize = VkExtent2D{ 1, 1 };
                    sampleLocations.sampleLocationsCount = info.rasterizationSamples;
                    sampleLocations.sampleLocationsPerPixel = info.rasterizationSamples;
                    sampleLocations.pSampleLocations = m_customSampleLocations.data();

                    m_customSampleLocationsCreateInfo = {};
                    m_customSampleLocationsCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SAMPLE_LOCATIONS_STATE_CREATE_INFO_EXT;
                    m_customSampleLocationsCreateInfo.sampleLocationsEnable = VK_TRUE;
                    m_customSampleLocationsCreateInfo.sampleLocationsInfo = sampleLocations;
                    info.pNext = &m_customSampleLocationsCreateInfo;
                }
                else
                {
                    AZ_Error("GraphicsPipeline", false, "Custom sampling positions is not supported on this device");
                }
            }
        }

        void GraphicsPipeline::BuildPipelineDepthStencilStateCreateInfo(const RHI::DepthStencilState& depthStencilState)
        {
            const RHI::DepthState& depthState = depthStencilState.m_depth;
            const RHI::StencilState& stencilState = depthStencilState.m_stencil;

            VkPipelineDepthStencilStateCreateInfo& info = m_pipelineDepthStencilStateCreateInfo;
            info = {};
            info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
            info.pNext = nullptr;
            info.flags = 0;
            info.depthTestEnable = (depthState.m_enable != 0);

            switch (depthState.m_writeMask)
            {
            case RHI::DepthWriteMask::Zero:
                info.depthWriteEnable = VK_FALSE;
                break;
            case RHI::DepthWriteMask::All:
                info.depthWriteEnable = VK_TRUE;
                break;
            default:
                info.depthWriteEnable = VK_FALSE;
                AZ_Assert(false, "Depth write mask is invalid.");
                break;
            }

            info.depthCompareOp = ConvertComparisonFunction(depthState.m_func);
            info.depthBoundsTestEnable = VK_FALSE;

            info.stencilTestEnable = (stencilState.m_enable != 0);

            FillStencilOpState(stencilState.m_frontFace, info.front);
            info.front.compareMask = stencilState.m_readMask;
            info.front.writeMask = stencilState.m_writeMask;
            info.front.reference = 0; // It will be written by dynamic state.

            FillStencilOpState(stencilState.m_backFace, info.back);
            info.back.compareMask = stencilState.m_readMask;
            info.back.writeMask = stencilState.m_writeMask;
            info.back.reference = 0; // It will be written by dynamic state.

            // minDepthBounds and maxDepthBounds are valid if depthBoudsTestEnable is true.
            info.minDepthBounds = -1.f;
            info.maxDepthBounds = 1.f;
        }

        void GraphicsPipeline::BuildPipelineColorBlendStateCreateInfo(const RHI::BlendState& blendState, const uint32_t colorAttachmentCount)
        {
            VkPipelineColorBlendStateCreateInfo& info = m_pipelineColorBlendStateCreateInfo;
            info = {};
            info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
            info.pNext = nullptr;
            info.flags = 0;
            info.logicOpEnable = VK_FALSE;
            info.logicOp = VK_LOGIC_OP_SET;
            info.attachmentCount = colorAttachmentCount;
            m_colorBlendAttachments.resize(info.attachmentCount);
            for (uint32_t index = 0; index < info.attachmentCount; ++index)
            {
                // If m_independentBlendEnable is not enabled, we use the values from attachment 0 (same as D3D12)
                if (index == 0 || blendState.m_independentBlendEnable)
                {
                    FillColorBlendAttachmentState(blendState.m_targets[index], m_colorBlendAttachments[index]);
                }
                else
                {
                    m_colorBlendAttachments[index] = m_colorBlendAttachments[0];
                }
            }
            info.pAttachments = m_colorBlendAttachments.empty() ? nullptr : m_colorBlendAttachments.data();
            for (uint32_t index = 0; index < BlendConstantsCount; ++index)
            {
                info.blendConstants[index] = m_blendConstants[index];
            }
        }

        void GraphicsPipeline::BuildPipelineDynamicStateCreateInfo()
        {
            m_dynamicStates =
            {
                VK_DYNAMIC_STATE_VIEWPORT,
                VK_DYNAMIC_STATE_SCISSOR,
                VK_DYNAMIC_STATE_STENCIL_REFERENCE
            };

            auto& physicalDevice = static_cast<const PhysicalDevice&>(GetDevice().GetPhysicalDevice());
            if (RHI::CheckBitsAll(GetDevice().GetFeatures().m_shadingRateTypeMask, RHI::ShadingRateTypeFlags::PerDraw) &&
                physicalDevice.IsOptionalDeviceExtensionSupported(OptionalDeviceExtension::FragmentShadingRate))
            {
                m_dynamicStates.push_back(VK_DYNAMIC_STATE_FRAGMENT_SHADING_RATE_KHR);
            }

            VkPipelineDynamicStateCreateInfo& info = m_pipelineDynamicStateCreateInfo;
            info = {};
            info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
            info.pNext = nullptr;
            info.flags = 0;
            info.dynamicStateCount = aznumeric_caster(m_dynamicStates.size());
            info.pDynamicStates = m_dynamicStates.data();
        }
    }
}
