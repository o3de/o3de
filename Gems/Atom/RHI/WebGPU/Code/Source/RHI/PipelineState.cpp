/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <RHI/WebGPU.h>
#include <RHI/Device.h>
#include <RHI/PipelineState.h>
#include <RHI/RenderPipeline.h>
#include <RHI/PipelineLibrary.h>

namespace AZ::WebGPU
{
    RHI::Ptr<PipelineState> PipelineState::Create()
    {
        return aznew PipelineState;
    }

    PipelineLayout* PipelineState::GetPipelineLayout() const
    {
        if (!m_pipeline)
        {
            return nullptr;
        }
        return m_pipeline->GetPipelineLayout();
    }

    Pipeline* PipelineState::GetPipeline() const
    {
        return m_pipeline.get();
    }

    RHI::ResultCode PipelineState::InitInternal(
        RHI::Device& device, const RHI::PipelineStateDescriptorForDraw& descriptor, RHI::DevicePipelineLibrary* pipelineLibrary)
    {
        Pipeline::Descriptor pipelineDescriptor;
        pipelineDescriptor.m_pipelineDescritor = &descriptor;
        pipelineDescriptor.m_pipelineLibrary = static_cast<PipelineLibrary*>(pipelineLibrary);
        RHI::Ptr<RenderPipeline> pipeline = RenderPipeline::Create();
        RHI::ResultCode result = pipeline->Init(static_cast<Device&>(device), pipelineDescriptor);
        RETURN_RESULT_IF_UNSUCCESSFUL(result);

        m_pipeline = pipeline;
        return result;
    }

    void PipelineState::ShutdownInternal()
    {
        m_pipeline = nullptr;
    }

    void PipelineState::SetNameInternal(const AZStd::string_view& name)
    {
        if (m_pipeline)
        {
            m_pipeline->SetName(AZ::Name(name));
        }
    }
}
