/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <RHI/ClearAttachments.h>
#include <RHI/BufferPool.h>
#include <RHI/Buffer.h>
#include <RHI/Device.h>
#include <Atom/RHI/DeviceStreamBufferView.h>
#include <Atom/RHI.Reflect/Metal/PipelineLayoutDescriptor.h>
#include <Atom/RHI.Reflect/Metal/ShaderStageFunction.h>
#include <Atom/RHI.Reflect/ConstantsLayout.h>
#include <Atom/RHI.Reflect/RenderAttachmentLayout.h>
#include <Atom/RHI.Reflect/InputStreamLayoutBuilder.h>

// Metal shader used for doing the clearing.
// It uses a full screen triangle as geometry and push constants for the clearing values.
const char* MetalShaderSource = R"(
#include <metal_stdlib>
using namespace metal;
struct PushConstants
{
    float4 m_color[8];
    float4 m_depth;
};
struct VSOutput
{
    float4 m_position [[position]];
};
vertex VSOutput VSMain(
    uint vertexID [[vertex_id]],
    constant PushConstants& pushConstants [[buffer(0)]])
{
    const float2 vertices[3] = { float2(-1,1), float2(-1, -3), float2(3, 1) };
    VSOutput out = {};
    out.m_position = float4(vertices[vertexID], pushConstants.m_depth.x, 1);
    return out;
}
struct PSOut
{
    float4 m_color0 [[color(0)]];
    float4 m_color1 [[color(1)]];
    float4 m_color2 [[color(2)]];
    float4 m_color3 [[color(3)]];
    float4 m_color4 [[color(4)]];
    float4 m_color5 [[color(5)]];
    float4 m_color6 [[color(6)]];
    float4 m_color7 [[color(7)]];
};
fragment PSOut PSMain(
    constant PushConstants& pushConstants [[buffer(0)]])
{
    PSOut out = {};
    out.m_color0 = pushConstants.m_color[0];
    out.m_color1 = pushConstants.m_color[1];
    out.m_color2 = pushConstants.m_color[2];
    out.m_color3 = pushConstants.m_color[3];
    out.m_color4 = pushConstants.m_color[4];
    out.m_color5 = pushConstants.m_color[5];
    out.m_color6 = pushConstants.m_color[6];
    out.m_color7 = pushConstants.m_color[7];
    return out;
}
)";

namespace AZ::Metal
{
    RHI::ResultCode ClearAttachments::Init(RHI::Device& device)
    {
        RHI::DeviceObject::Init(device);
        
        // No streams required. We calculate the vertices positions in the vertex shader.
        RHI::InputStreamLayout inputStreamLayout;
        inputStreamLayout.SetTopology(RHI::PrimitiveTopology::TriangleList);
        inputStreamLayout.Finalize();
        
        m_pipelineStateDescriptor.m_inputStreamLayout = inputStreamLayout;
        
        AZStd::string_view shaderSource(MetalShaderSource);
        RHI::Ptr<ShaderStageFunction> vertexShader = ShaderStageFunction::Create(RHI::ShaderStage::Vertex);
        vertexShader->SetSourceCode(shaderSource);
        vertexShader->SetEntryFunctionName("VSMain");
        RHI::ResultCode result = vertexShader->Finalize();
        if (result != RHI::ResultCode::Success)
        {
            AZ_Assert(false, "Failed to compile vertex shader for ClearAttachments");
            return result;
        }
        
        RHI::Ptr<ShaderStageFunction> fragmentShader = ShaderStageFunction::Create(RHI::ShaderStage::Fragment);
        fragmentShader->SetSourceCode(shaderSource);
        fragmentShader->SetEntryFunctionName("PSMain");
        if (result != RHI::ResultCode::Success)
        {
            AZ_Assert(false, "Failed to compile fragment shader for ClearAttachments");
            return result;
        }
        
        m_pipelineStateDescriptor.m_vertexFunction = vertexShader;
        m_pipelineStateDescriptor.m_fragmentFunction = fragmentShader;
        
        RHI::Ptr<PipelineLayoutDescriptor> pipelineLayoutDescriptor = PipelineLayoutDescriptor::Create();
        pipelineLayoutDescriptor->Reset();
        RHI::Ptr<RHI::ConstantsLayout> constantLayout = RHI::ConstantsLayout::Create();
        {
            RHI::ShaderInputConstantDescriptor constantDescriptor;
            constantDescriptor.m_name = "Color";
            constantDescriptor.m_constantByteOffset = offsetof(PushConstants, m_color);
            constantDescriptor.m_constantByteCount = sizeof(PushConstants::m_color);
            constantLayout->AddShaderInput(constantDescriptor);
        }
        {
            RHI::ShaderInputConstantDescriptor constantDescriptor;
            constantDescriptor.m_name = "Depth";
            constantDescriptor.m_constantByteOffset = offsetof(PushConstants, m_depth);
            constantDescriptor.m_constantByteCount = sizeof(PushConstants::m_depth);
            constantLayout->AddShaderInput(constantDescriptor);
        }        
        constantLayout->Finalize();
        pipelineLayoutDescriptor->SetRootConstantsLayout(*constantLayout);
        pipelineLayoutDescriptor->SetRootConstantBinding(RootConstantBinding{});
        result = pipelineLayoutDescriptor->Finalize();
        if (result != RHI::ResultCode::Success)
        {
            AZ_Assert(false, "Failed to build PipelineLayoutDescriptor for ClearAttachments");
            return result;
        }
        m_pipelineStateDescriptor.m_pipelineLayoutDescriptor = pipelineLayoutDescriptor;
        return RHI::ResultCode::Success;
    }

    void ClearAttachments::Shutdown()
    {
        m_pipelineStateDescriptor = {};
        m_pipelineCache.clear();
    }

    RHI::ResultCode ClearAttachments::Clear(CommandList& commandList, MTLRenderPassDescriptor* renderpassDesc, const AZStd::vector<ClearAttachments::ClearData>& clearAttachmentsData)
    {
        if (renderpassDesc == nil)
        {
            return RHI::ResultCode::InvalidArgument;
        }

        // Need to build a RHI::RenderAttachmentConfiguration and RHI::RenderStates from the
        // MTLRenderPassDescriptor for the PipelineStateDescriptor.
        PushConstants pushConstants{};
        uint32_t stencilRef = 0;
        RHI::RenderStates renderStates;
        renderStates.m_blendState.m_independentBlendEnable = true;
        RHI::RenderAttachmentConfiguration renderConfiguration;
        RHI::RenderAttachmentLayout& layout = renderConfiguration.m_renderAttachmentLayout;
        layout.m_subpassCount = 1;
        RHI::SubpassRenderAttachmentLayout& subpassLayout = layout.m_subpassLayouts[0];
        for(uint32_t i = 0; i < RHI::Limits::Pipeline::AttachmentColorCountMax; ++i)
        {
            MTLRenderPassColorAttachmentDescriptor* colorDescriptor = renderpassDesc.colorAttachments[i];
            if (colorDescriptor.texture)
            {
                RHI::TargetBlendState& blendState = renderStates.m_blendState.m_targets[layout.m_attachmentCount];
                layout.m_attachmentFormats[layout.m_attachmentCount] = ConvertPixelFormat(colorDescriptor.texture.pixelFormat);
                RHI::RenderAttachmentDescriptor& attachmentDescriptor = subpassLayout.m_rendertargetDescriptors[subpassLayout.m_rendertargetCount++];
                attachmentDescriptor.m_attachmentIndex = layout.m_attachmentCount;
                blendState.m_enable = 0;
                blendState.m_writeMask = 0;
                layout.m_attachmentCount++;
            }
        }
        
        MTLRenderPassDepthAttachmentDescriptor* depthAttachment = renderpassDesc.depthAttachment;
        if (depthAttachment.texture)
        {
            layout.m_attachmentFormats[layout.m_attachmentCount] = ConvertPixelFormat(depthAttachment.texture.pixelFormat);
            subpassLayout.m_depthStencilDescriptor.m_attachmentIndex = layout.m_attachmentCount;
            RHI::DepthState& depthState = renderStates.m_depthStencilState.m_depth;
            depthState.m_func = RHI::ComparisonFunc::Always;
            depthState.m_enable = 0;
        }
        
        MTLRenderPassStencilAttachmentDescriptor* stencilAttachment = renderpassDesc.stencilAttachment;
        if (stencilAttachment.texture)
        {
            layout.m_attachmentFormats[layout.m_attachmentCount] = ConvertPixelFormat(stencilAttachment.texture.pixelFormat);
            subpassLayout.m_depthStencilDescriptor.m_attachmentIndex = layout.m_attachmentCount;
            RHI::StencilState& stencilState = renderStates.m_depthStencilState.m_stencil;
            stencilState.m_enable = 0;
        }
        
        for (const ClearData& clearData : clearAttachmentsData)
        {
            if (clearData.m_attachmentIndex < RHI::Limits::Pipeline::AttachmentColorCountMax)
            {
                // Enable the writing to the color attachment that needs clearing.
                RHI::TargetBlendState& blendState = renderStates.m_blendState.m_targets[clearData.m_attachmentIndex];
                blendState.m_enable = 1;
                blendState.m_writeMask = 0xF;
                ::memcpy(pushConstants.m_color[clearData.m_attachmentIndex], clearData.m_clearValue.m_vector4Float.data(), sizeof(pushConstants.m_color[clearData.m_attachmentIndex]));
            }
            else
            {
                if (RHI::CheckBitsAll(clearData.m_imageAspects, RHI::ImageAspectFlags::Depth))
                {
                    // Enable depth write so we can write the clear value
                    RHI::DepthState& depthState = renderStates.m_depthStencilState.m_depth;
                    depthState.m_enable = 1;
                    pushConstants.m_depth[0] = clearData.m_clearValue.m_depthStencil.m_depth;
                }
                
                if (RHI::CheckBitsAll(clearData.m_imageAspects, RHI::ImageAspectFlags::Stencil))
                {
                    // Enable stencil writing so we can write the clear value
                    RHI::StencilState& stencilState = renderStates.m_depthStencilState.m_stencil;
                    stencilState.m_enable = 1;
                    stencilState.m_frontFace.m_failOp = RHI::StencilOp::Replace;
                    stencilState.m_frontFace.m_depthFailOp = RHI::StencilOp::Replace;
                    stencilState.m_frontFace.m_passOp = RHI::StencilOp::Replace;
                    stencilState.m_frontFace.m_func = RHI::ComparisonFunc::Always;
                    stencilState.m_backFace.m_failOp = RHI::StencilOp::Replace;
                    stencilState.m_backFace.m_depthFailOp = RHI::StencilOp::Replace;
                    stencilState.m_backFace.m_passOp = RHI::StencilOp::Replace;
                    stencilState.m_backFace.m_func = RHI::ComparisonFunc::Always;
                    stencilRef = clearData.m_clearValue.m_depthStencil.m_stencil;
                }
            }
        }

        // Check the pipeline cache for the pipeline state
        const RHI::PipelineState* pipelineState = nullptr;
        {
            AZStd::lock_guard<AZStd::mutex> lock(m_pipelineCacheMutex);
            RHI::PipelineStateDescriptorForDraw pipelineDescriptor = m_pipelineStateDescriptor;
            pipelineDescriptor.m_renderAttachmentConfiguration = renderConfiguration;
            pipelineDescriptor.m_renderStates = renderStates;
            auto hash = pipelineDescriptor.GetHash();
            auto findIt = m_pipelineCache.find(RHI::PipelineStateEntry(hash, nullptr, pipelineDescriptor));
            if (findIt != m_pipelineCache.end())
            {
                pipelineState = findIt->m_pipelineState.get();
            }
            
            if (!pipelineState)
            {
                // Need to compile a new PipelineState
                RHI::Ptr<RHI::PipelineState> pipelineStatePtr = aznew RHI::PipelineState;
                RHI::ResultCode result = pipelineStatePtr->Init(static_cast<RHI::MultiDevice::DeviceMask>((1 << GetDevice().GetDeviceIndex())), pipelineDescriptor);
                if(result != RHI::ResultCode::Success)
                {
                    AZ_Assert(false, "Failed to build PipelineState for ClearAttachments");
                    return result;
                }
                m_pipelineCache.insert(RHI::PipelineStateEntry(hash, pipelineStatePtr, pipelineDescriptor));
                pipelineState = pipelineStatePtr.get();
            }
        }
        
        AZ_Assert(pipelineState, "Null PipelineState");
        RHI::DeviceGeometryView geometryView;
        geometryView.SetDrawArguments(RHI::DeviceDrawArguments(RHI::DrawLinear(3, 0)));

        RHI::DeviceDrawItem drawItem;
        drawItem.m_geometryView = &geometryView;
        drawItem.m_pipelineState = pipelineState->GetDevicePipelineState(GetDevice().GetDeviceIndex()).get();
        drawItem.m_rootConstants = reinterpret_cast<uint8_t*>(&pushConstants);
        drawItem.m_rootConstantSize = sizeof(pushConstants);
        drawItem.m_stencilRef = stencilRef;
       
        commandList.Submit(drawItem);
        
        return RHI::ResultCode::Success;
    }
}
