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
#include <Atom/RHI.Reflect/Limits.h>
#include <RHI/Pipeline.h>
#include <RHI/PipelineLayout.h>

namespace AZ::WebGPU
{
    class Device;
    class PipelineLibrary;

    //! Encapsulates a WebGPU RenderPipeline object
    class RenderPipeline final
        : public Pipeline
    {
        using Base = Pipeline;

    public:
        AZ_CLASS_ALLOCATOR(RenderPipeline, AZ::SystemAllocator);
        AZ_RTTI(RenderPipeline, "{E1178F82-964E-41F0-A7C6-90AE4C3E917D}", Base);

        ~RenderPipeline() = default;

        static RHI::Ptr<RenderPipeline> Create();
        const wgpu::RenderPipeline& GetNativeRenderPipeline() const;

    private:
        RenderPipeline() = default;

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

        //! Creates RenderPipeline.
        RHI::ResultCode BuildNativePipeline(const Descriptor& descriptor, const PipelineLayout& pipelineLayout);
        void BuildPrimitiveState(const RHI::PipelineStateDescriptorForDraw& descriptor);
        void BuildDepthStencilState(const RHI::PipelineStateDescriptorForDraw& descriptor);
        void BuildMultisampleState(const RHI::PipelineStateDescriptorForDraw& descriptor);
        void BuildVertexState(const RHI::PipelineStateDescriptorForDraw& descriptor);
        void BuildFragmentState(const RHI::PipelineStateDescriptorForDraw& descriptor);
        void FillColorBlendAttachmentState(const RHI::TargetBlendState& blendState, wgpu::ColorTargetState& targetState);
      
        //! Native wgpu renderpipeline
        wgpu::RenderPipeline m_wgpuRenderPipeline;

        // Descriptors used for creating the WebGPU RenderPipeline object
        AZStd::fixed_vector<wgpu::VertexBufferLayout, RHI::Limits::Pipeline::StreamCountMax> m_wgpuBuffers;
        AZStd::fixed_vector<wgpu::VertexAttribute, RHI::Limits::Pipeline::StreamChannelCountMax> m_wgpuAttributes;
        AZStd::fixed_vector<wgpu::ColorTargetState, RHI::Limits::Pipeline::AttachmentColorCountMax> m_wgpuTargets;
        AZStd::fixed_vector<wgpu::BlendState, RHI::Limits::Pipeline::AttachmentColorCountMax> m_wgpuBlends;
        AZStd::vector<wgpu::ConstantEntry> m_vertexConstants;
        AZStd::vector<wgpu::ConstantEntry> m_fragmentConstants;

        wgpu::FragmentState m_wgpuFragment = {};
        wgpu::DepthStencilState m_wgpuDepthStencil = {};
        wgpu::RenderPipelineDescriptor m_wgpuRenderPipelineDescriptor = {};
    };
}
