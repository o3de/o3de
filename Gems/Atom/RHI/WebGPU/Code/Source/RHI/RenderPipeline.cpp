/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <RHI/WebGPU.h>
#include <Atom/RHI.Reflect/RenderStates.h>
#include <Atom/RHI.Reflect/InputStreamLayout.h>
#include <RHI/Device.h>
#include <RHI/RenderPipeline.h>
#include <RHI/PipelineLibrary.h>
#include <RHI/ShaderModule.h>

namespace AZ::WebGPU
{
    RHI::Ptr<RenderPipeline> RenderPipeline::Create()
    {
        return aznew RenderPipeline();
    }

    const wgpu::RenderPipeline& RenderPipeline::GetNativeRenderPipeline() const
    {
        return m_wgpuRenderPipeline;
    }

    RHI::ResultCode RenderPipeline::InitInternal(
        [[maybe_unused]] const Descriptor& descriptor, [[maybe_unused]] const PipelineLayout& pipelineLayout)
    {
        AZ_Assert(descriptor.m_pipelineDescritor, "Pipeline State Draw Descriptor is null.");
        AZ_Assert(descriptor.m_pipelineDescritor->GetType() == RHI::PipelineStateType::Draw, "Invalid pipeline descriptor type");

        return BuildNativePipeline(descriptor, pipelineLayout);
    }

    void RenderPipeline::Shutdown()
    {
        Base::Shutdown();
    }

    void RenderPipeline::SetNameInternal(const AZStd::string_view& name)
    {
        if (m_wgpuRenderPipeline && !name.empty())
        {
            m_wgpuRenderPipeline.SetLabel(name.data());
        }
        Base::SetNameInternal(name);
    }

    RHI::ResultCode RenderPipeline::BuildNativePipeline(const Descriptor& descriptor, const PipelineLayout& pipelineLayout)
    {
        auto& device = static_cast<Device&>(GetDevice());
        const auto& drawDescriptor = static_cast<const RHI::PipelineStateDescriptorForDraw&>(*descriptor.m_pipelineDescritor);

        auto* vertexFunction = static_cast<const ShaderStageFunction*>(drawDescriptor.m_vertexFunction.get());
        auto* fragFunction = static_cast<const ShaderStageFunction*>(drawDescriptor.m_vertexFunction.get());
        if ((!vertexFunction || vertexFunction->GetSourceCode().empty()) &&
            (!fragFunction || fragFunction->GetSourceCode().empty()))
        {
            // Temporary until we can compile most of the shaders
            return RHI::ResultCode::Success;
        }

        m_wgpuRenderPipelineDescriptor.layout = pipelineLayout.GetNativePipelineLayout();
        BuildPrimitiveState(drawDescriptor);
        BuildDepthStencilState(drawDescriptor);
        BuildMultisampleState(drawDescriptor);
        BuildVertexState(drawDescriptor);
        BuildFragmentState(drawDescriptor);
        m_wgpuRenderPipelineDescriptor.label = GetName().GetCStr();

        m_wgpuRenderPipeline = device.GetNativeDevice().CreateRenderPipeline(&m_wgpuRenderPipelineDescriptor);
        AZ_Assert(m_wgpuRenderPipeline, "Failed to create render pipeline");
        return m_wgpuRenderPipeline ? RHI::ResultCode::Success : RHI::ResultCode::Fail;
    }

    void RenderPipeline::BuildPrimitiveState(
        const RHI::PipelineStateDescriptorForDraw& descriptor)
    {
        wgpu::PrimitiveState& wgpuPrimtiveState = m_wgpuRenderPipelineDescriptor.primitive;
        wgpuPrimtiveState.topology = ConvertPrimitiveTopology(descriptor.m_inputStreamLayout.GetTopology());
        wgpuPrimtiveState.stripIndexFormat = wgpu::IndexFormat::Undefined;
        wgpuPrimtiveState.frontFace = wgpu::FrontFace::CCW; // o3de only supports Clockwise
        wgpuPrimtiveState.cullMode = ConvertCullMode(descriptor.m_renderStates.m_rasterState.m_cullMode);
        wgpuPrimtiveState.unclippedDepth = !descriptor.m_renderStates.m_rasterState.m_depthClipEnable;
    }

    void RenderPipeline::BuildDepthStencilState(
        const RHI::PipelineStateDescriptorForDraw& descriptor)
    {
        RHI::Format depthStencilFormat = descriptor.m_renderAttachmentConfiguration.GetDepthStencilFormat();
        if (depthStencilFormat != RHI::Format::Unknown)
        {
            wgpu::DepthStencilState& wgpuDepthStencilState = m_wgpuDepthStencil;
            const auto& depthState = descriptor.m_renderStates.m_depthStencilState.m_depth;
            const auto& stencilState = descriptor.m_renderStates.m_depthStencilState.m_stencil;
            wgpuDepthStencilState.format = ConvertImageFormat(depthStencilFormat);
            if (depthState.m_enable)
            {
                wgpuDepthStencilState.depthWriteEnabled = depthState.m_writeMask == RHI::DepthWriteMask::All;
                wgpuDepthStencilState.depthCompare = ConvertCompareFunction(depthState.m_func);
            }
            else
            {
                wgpuDepthStencilState.depthWriteEnabled = false;
                wgpuDepthStencilState.depthCompare = wgpu::CompareFunction::Always;
            }
            if (stencilState.m_enable)
            {
                wgpuDepthStencilState.stencilFront.compare = ConvertCompareFunction(stencilState.m_frontFace.m_func);
                wgpuDepthStencilState.stencilFront.failOp = ConvertStencilOp(stencilState.m_frontFace.m_failOp);
                wgpuDepthStencilState.stencilFront.depthFailOp = ConvertStencilOp(stencilState.m_frontFace.m_depthFailOp);
                wgpuDepthStencilState.stencilFront.passOp = ConvertStencilOp(stencilState.m_frontFace.m_passOp);
                wgpuDepthStencilState.stencilBack.compare = ConvertCompareFunction(stencilState.m_backFace.m_func);
                wgpuDepthStencilState.stencilBack.failOp = ConvertStencilOp(stencilState.m_backFace.m_failOp);
                wgpuDepthStencilState.stencilBack.depthFailOp = ConvertStencilOp(stencilState.m_backFace.m_depthFailOp);
                wgpuDepthStencilState.stencilBack.passOp = ConvertStencilOp(stencilState.m_backFace.m_passOp);
                wgpuDepthStencilState.stencilReadMask = stencilState.m_readMask;
                wgpuDepthStencilState.stencilWriteMask = stencilState.m_writeMask;
            }

            wgpuDepthStencilState.depthBias = descriptor.m_renderStates.m_rasterState.m_depthBias;
            wgpuDepthStencilState.depthBiasSlopeScale = descriptor.m_renderStates.m_rasterState.m_depthBiasSlopeScale;
            wgpuDepthStencilState.depthBiasClamp = descriptor.m_renderStates.m_rasterState.m_depthBiasClamp;
            m_wgpuRenderPipelineDescriptor.depthStencil = &wgpuDepthStencilState;
        }
        else
        {
            m_wgpuRenderPipelineDescriptor.depthStencil = nullptr;
        }
    }

    void RenderPipeline::BuildMultisampleState(
        const RHI::PipelineStateDescriptorForDraw& descriptor)
    {
        wgpu::MultisampleState& multisampleState = m_wgpuRenderPipelineDescriptor.multisample;
        multisampleState.count = descriptor.m_renderStates.m_multisampleState.m_samples;
        multisampleState.mask = 0xFFFFFFFF;
        multisampleState.alphaToCoverageEnabled = descriptor.m_renderStates.m_blendState.m_alphaToCoverageEnable;
    }

    void RenderPipeline::BuildVertexState(const RHI::PipelineStateDescriptorForDraw& descriptor)
    {
        ShaderModule* module = BuildShaderModule(descriptor.m_vertexFunction.get());
        if (module)
        {
            BuildConstants(
                descriptor,
                static_cast<const ShaderStageFunction*>(module->GetStageFunction())->GetSourceCode().data(),
                m_vertexConstants);
        }
        wgpu::VertexState& vertexState = m_wgpuRenderPipelineDescriptor.vertex;
        vertexState.module = module ? module->GetNativeShaderModule() : nullptr;
        vertexState.entryPoint = module ? module->GetEntryFunctionName().c_str() : nullptr;
        vertexState.constantCount = m_vertexConstants.size();
        vertexState.constants = m_vertexConstants.empty() ? nullptr : m_vertexConstants.data();

        const auto& buffers = descriptor.m_inputStreamLayout.GetStreamBuffers();
        const auto& attributtes = descriptor.m_inputStreamLayout.GetStreamChannels();
        vertexState.bufferCount = buffers.size();
        vertexState.buffers = m_wgpuBuffers.data();
        for (uint32_t i = 0; i < buffers.size(); ++i)
        {
            const auto& streamBuffer = buffers[i];
            wgpu::VertexBufferLayout& bufferLayout = m_wgpuBuffers.emplace_back(wgpu::VertexBufferLayout{});
            bufferLayout.arrayStride = streamBuffer.m_byteStride;
            bufferLayout.stepMode = ConvertVertexStep(streamBuffer.m_stepFunction);
            bufferLayout.attributes = m_wgpuAttributes.data() + m_wgpuAttributes.size();
            for (uint32_t j = 0; j < attributtes.size(); ++j)
            {
                const auto& attribute = attributtes[j];
                if (attribute.m_bufferIndex == i)
                {
                    wgpu::VertexAttribute& vertexAttribute = m_wgpuAttributes.emplace_back(wgpu::VertexAttribute{});
                    vertexAttribute.format = ConvertVertexFormat(attribute.m_format);
                    vertexAttribute.offset = attribute.m_byteOffset;
                    vertexAttribute.shaderLocation = j;
                    bufferLayout.attributeCount++;
                }
            }
        }
    }

    void RenderPipeline::FillColorBlendAttachmentState(const RHI::TargetBlendState& blendState, wgpu::ColorTargetState& targetState)
    {
        if (blendState.m_enable)
        {
            wgpu::BlendState& blend = m_wgpuBlends.emplace_back(wgpu::BlendState{});
            blend.color.operation = ConvertBlendOp(blendState.m_blendOp);
            blend.color.srcFactor = ConvertBlendFactor(blendState.m_blendSource);
            blend.color.dstFactor = ConvertBlendFactor(blendState.m_blendDest);
            blend.alpha.operation = ConvertBlendOp(blendState.m_blendAlphaOp);
            blend.alpha.srcFactor = ConvertBlendFactor(blendState.m_blendAlphaSource);
            blend.alpha.dstFactor = ConvertBlendFactor(blendState.m_blendAlphaDest);
            targetState.blend = &blend;
        }
        else
        {
            targetState.blend = nullptr;
        }
        targetState.writeMask = ConvertWriteMask(static_cast<uint8_t>(blendState.m_writeMask));
    }

    void RenderPipeline::BuildFragmentState(const RHI::PipelineStateDescriptorForDraw& descriptor)
    {
        ShaderModule* module = BuildShaderModule(descriptor.m_fragmentFunction.get());
        uint32_t targetCount = descriptor.m_renderAttachmentConfiguration.GetRenderTargetCount();
        if (targetCount)
        {
            if (module)
            {
                BuildConstants(
                    descriptor,
                    static_cast<const ShaderStageFunction*>(module->GetStageFunction())->GetSourceCode().data(),
                    m_fragmentConstants);
            }
            wgpu::FragmentState& fragmentState = m_wgpuFragment;
            fragmentState.module = module ? module->GetNativeShaderModule() : nullptr;
            fragmentState.entryPoint = module ? module->GetEntryFunctionName().c_str() : nullptr;
            fragmentState.constantCount = m_fragmentConstants.size();
            fragmentState.constants = m_fragmentConstants.empty() ? nullptr : m_fragmentConstants.data();
            fragmentState.targetCount = descriptor.m_renderAttachmentConfiguration.GetRenderTargetCount();
            for (uint32_t i = 0; i < fragmentState.targetCount; ++i)
            {
                wgpu::ColorTargetState& targetState = m_wgpuTargets.emplace_back(wgpu::ColorTargetState{});
                targetState.format = ConvertImageFormat(descriptor.m_renderAttachmentConfiguration.GetRenderTargetFormat(i));

                // If m_independentBlendEnable is not enabled, we use the values from attachment 0 (same as D3D12)
                if (i == 0 || descriptor.m_renderStates.m_blendState.m_independentBlendEnable)
                {
                    FillColorBlendAttachmentState(descriptor.m_renderStates.m_blendState.m_targets[0], targetState);
                }
                else
                {
                    FillColorBlendAttachmentState(descriptor.m_renderStates.m_blendState.m_targets[i], targetState);
                }               
            }
            fragmentState.targets = m_wgpuTargets.data();
            m_wgpuRenderPipelineDescriptor.fragment = &m_wgpuFragment;
        }
        else
        {
            m_wgpuRenderPipelineDescriptor.fragment = nullptr;
        }
    }    
}
