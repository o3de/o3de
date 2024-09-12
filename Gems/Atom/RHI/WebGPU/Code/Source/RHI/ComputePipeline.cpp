/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <RHI/WebGPU.h>
#include <RHI/ComputePipeline.h>
#include <RHI/Device.h>
#include <RHI/PipelineLayout.h>
#include <RHI/PipelineLibrary.h>

namespace AZ::WebGPU
{
    RHI::Ptr<ComputePipeline> ComputePipeline::Create()
    {
        return aznew ComputePipeline();
    }

    const wgpu::ComputePipeline& ComputePipeline::GetNativeComputePipeline() const
    {
        return m_wgpuComputePipeline;
    }

    void ComputePipeline::Shutdown()
    {
        m_wgpuComputePipeline = nullptr;
        Base::Shutdown();
    }

    void ComputePipeline::SetNameInternal(const AZStd::string_view& name)
    {
        if (m_wgpuComputePipeline && !name.empty())
        {
            m_wgpuComputePipeline.SetLabel(name.data());
        }
        Base::SetNameInternal(name);
    }

    RHI::ResultCode ComputePipeline::InitInternal(const Descriptor& descriptor, const PipelineLayout& pipelineLayout)
    {
        AZ_Assert(descriptor.m_pipelineDescritor, "Pipeline State Dispatch Descriptor is null.");
        AZ_Assert(descriptor.m_pipelineDescritor->GetType() == RHI::PipelineStateType::Dispatch, "Invalid pipeline descriptor type");

        RHI::ResultCode result = BuildNativePipeline(descriptor, pipelineLayout);
        RETURN_RESULT_IF_UNSUCCESSFUL(result);

        return result;
    }

    RHI::ResultCode ComputePipeline::BuildNativePipeline(
        const Descriptor& descriptor, const PipelineLayout& pipelineLayout)
    {
        auto& device = static_cast<Device&>(GetDevice());
        const auto& dispatchDescriptor = static_cast<const RHI::PipelineStateDescriptorForDispatch&>(*descriptor.m_pipelineDescritor);

        auto* computeFunction = static_cast<const ShaderStageFunction*>(dispatchDescriptor.m_computeFunction.get());
        if (!computeFunction || computeFunction->GetSourceCode().empty())
        {
            // Temporary until we can compile most of the shaders
            return RHI::ResultCode::Success;
        }

        ShaderModule* module = BuildShaderModule(computeFunction);
        if (module)
        {
            BuildConstants(
                dispatchDescriptor,
                static_cast<const ShaderStageFunction*>(module->GetStageFunction())->GetSourceCode().data(),
                m_computeConstants);
        }

        wgpu::ComputePipelineDescriptor wgpuDescriptor = {};
        wgpuDescriptor.layout = pipelineLayout.GetNativePipelineLayout();
        wgpuDescriptor.label = GetName().GetCStr();
        wgpuDescriptor.compute.module = module ? module->GetNativeShaderModule() : nullptr;
        wgpuDescriptor.compute.entryPoint = module ? module->GetEntryFunctionName().c_str() : nullptr;
        wgpuDescriptor.compute.constantCount = m_computeConstants.size();
        wgpuDescriptor.compute.constants = m_computeConstants.empty() ? nullptr : m_computeConstants.data();

        m_wgpuComputePipeline = device.GetNativeDevice().CreateComputePipeline(&wgpuDescriptor);
        AZ_Assert(m_wgpuComputePipeline, "Failed to create compute pipeline");
        return m_wgpuComputePipeline ? RHI::ResultCode::Success : RHI::ResultCode::Fail;
    }
}
