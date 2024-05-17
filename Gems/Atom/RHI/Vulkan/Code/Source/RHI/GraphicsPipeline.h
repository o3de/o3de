/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/PipelineStateDescriptor.h>
#include <Atom/RHI/DeviceDrawItem.h>
#include <Atom/RHI.Reflect/Vulkan/ShaderStageFunction.h>
#include <RHI/Pipeline.h>
#include <RHI/PipelineLayout.h>
#include <RHI/RenderPass.h>

namespace AZ
{
    namespace Vulkan
    {
        class Device;
        class PipelineLibrary;

        class GraphicsPipeline final
            : public Pipeline
        {
            using Base = Pipeline;

        public:
            AZ_CLASS_ALLOCATOR(GraphicsPipeline, AZ::SystemAllocator);
            AZ_RTTI(GraphicsPipeline, "C1152822-AAC0-427B-9200-6370EE8D4545", Base);

            ~GraphicsPipeline() = default;

            static RHI::Ptr<GraphicsPipeline> Create();

        private:
            GraphicsPipeline() = default;

            //////////////////////////////////////////////////////////////////////////
            // RHI::DeviceObject
            void Shutdown() override;
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // RHI::Object
            void SetNameInternal(const AZStd::string_view& name) override;
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            //! Pipeline
            RHI::ResultCode InitInternal(const Descriptor& descriptor, const PipelineLayout& pipelineLayout) override;
            RHI::PipelineStateType GetType() override { return RHI::PipelineStateType::Draw;  }
            //////////////////////////////////////////////////////////////////////////

            /// Create base GraphicsPipeline.
            RHI::ResultCode BuildNativePipeline(const Descriptor& descriptor, const PipelineLayout& pipelineLayout);

            void BuildPipelineShaderStageCreateInfo(const RHI::PipelineStateDescriptorForDraw& descriptor);
            
            void BuildPipelineVertexInputStateCreateInfo(const RHI::InputStreamLayout& inputStreamLayout);
            void FillVertexInputAttributeDescription(const RHI::InputStreamLayout& inputStreamLayout,
                uint32_t index, VkVertexInputAttributeDescription& desc);
            void FillVertexInputBindingDescription(const RHI::InputStreamLayout& inputStreamLayout,
                uint32_t index, VkVertexInputBindingDescription& desc);

            void BuildPipelineInputAssemblyStateInfo(RHI::PrimitiveTopology topology);

            void BuildPipelineTessellationStateCreatInfo();

            void BuildPipelineViewportStateCreateInfo();

            RHI::ResultCode BuildPipelineRasterizationStateCreateInfo(const Device& device, const RHI::RasterState& rasterState);

            void BuildPipelineMultisampleStateCreateInfo(const RHI::MultisampleState& multisampleState, const RHI::BlendState& blendState);

            void BuildPipelineDepthStencilStateCreateInfo(const RHI::DepthStencilState& depthStencilState);

            void BuildPipelineColorBlendStateCreateInfo(const RHI::BlendState& blendState, const uint32_t colorAttachmentCount);

            void BuildPipelineDynamicStateCreateInfo();

            AZStd::vector<VkPipelineShaderStageCreateInfo> m_pipelineShaderStageCreateInfos;

            VkPipelineVertexInputStateCreateInfo m_pipelineVertexInputStateCreateInfo{};
            AZStd::vector<VkVertexInputBindingDescription> m_vertexInputBindingDescriptions;
            AZStd::vector<VkVertexInputAttributeDescription> m_vertexInputAttributeDescriptions;

            VkPipelineInputAssemblyStateCreateInfo m_pipelineInputAssemblyStateCreateInfo{};

            VkPipelineTessellationStateCreateInfo m_pipelineTessellationStateCreateInfo{};

            VkPipelineViewportStateCreateInfo m_pipelineViewportStateCreateInfo{};
            AZStd::vector<VkViewport> m_viewports;
            AZStd::vector<VkRect2D> m_scissors;

            VkPipelineRasterizationStateCreateInfo m_pipelineRasterizationStateCreateInfo{};
            VkPipelineRasterizationDepthClipStateCreateInfoEXT m_pipelineRasterizationDepthClipStateInfo{};
            VkPipelineRasterizationConservativeStateCreateInfoEXT m_pipelineRasterizationConservativeInfo{};

            VkPipelineMultisampleStateCreateInfo m_pipelineMultisampleStateCreateInfo{};
            VkPipelineSampleLocationsStateCreateInfoEXT m_customSampleLocationsCreateInfo{};
            AZStd::vector<VkSampleLocationEXT> m_customSampleLocations;

            VkPipelineDepthStencilStateCreateInfo m_pipelineDepthStencilStateCreateInfo{};

            VkPipelineColorBlendStateCreateInfo m_pipelineColorBlendStateCreateInfo{};
            AZStd::vector<VkPipelineColorBlendAttachmentState> m_colorBlendAttachments;
            static const size_t BlendConstantsCount = 4; // RGBA components
            // [GFX TODO][ATOM-2633] find a way to get blend constants.
            AZStd::array<float, BlendConstantsCount> m_blendConstants{ {0.f, 0.f, 0.f, 0.f} };

            VkPipelineDynamicStateCreateInfo m_pipelineDynamicStateCreateInfo{};
            AZStd::vector<VkDynamicState> m_dynamicStates;

            RHI::Ptr<RenderPass> m_renderPass;
        };
    }
}
