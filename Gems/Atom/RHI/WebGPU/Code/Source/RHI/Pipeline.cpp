/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <RHI/WebGPU.h>
#include <RHI/Pipeline.h>
#include <RHI/Device.h>
#include <RHI/PipelineLayout.h>
#include <RHI/PipelineLibrary.h>

namespace AZ::WebGPU
{
    RHI::ResultCode Pipeline::Init(Device& device, const Descriptor& descriptor)
    {
        Base::Init(device);
        AZ_Assert(descriptor.m_pipelineDescritor, "Pipeline descriptor is null.");
        AZ_Assert(descriptor.m_pipelineDescritor->m_pipelineLayoutDescriptor, "Pipeline layout descriptor is null.");

        PipelineLayout::Descriptor layoutDescriptor;
        layoutDescriptor.m_pipelineLayoutDescriptor = descriptor.m_pipelineDescritor->m_pipelineLayoutDescriptor;
        RHI::Ptr<PipelineLayout> layout = device.AcquirePipelineLayout(layoutDescriptor);

        if (!layout)
        {
            AZ_Assert(false, "Failed to acquire PipelineLayout");
            return RHI::ResultCode::Fail;
        }

        Base::Init(device);

        RHI::ResultCode result = InitInternal(descriptor, *layout);
        RETURN_RESULT_IF_UNSUCCESSFUL(result);
        m_pipelineLayout = layout;
        m_pipelineLibrary = descriptor.m_pipelineLibrary;

        SetName(GetName());
        return result;
    }

    PipelineLayout* Pipeline::GetPipelineLayout() const
    {
        return m_pipelineLayout.get();
    }

    PipelineLibrary* Pipeline::GetPipelineLibrary() const
    {
        return m_pipelineLibrary;
    }

    void Pipeline::SetNameInternal([[maybe_unused]] const AZStd::string_view& name)
    {
        for (auto& shaderModule : m_shaderModules)
        {
            shaderModule->SetName(AZ::Name(name));
        }
        m_pipelineLayout->SetName(AZ::Name(name));
    }

    void Pipeline::Shutdown()
    {
        Base::Shutdown();
    }

    ShaderModule* Pipeline::BuildShaderModule(const RHI::ShaderStageFunction* function)
    {
        if (!function)
        {
            return nullptr;
        }

        ShaderModule::Descriptor shaderModuleDesc;
        shaderModuleDesc.m_shaderFunction = static_cast<const ShaderStageFunction*>(function);
        shaderModuleDesc.m_shaderStage = function->GetShaderStage();
        RHI::Ptr<ShaderModule> shaderModule = ShaderModule::Create();
        shaderModule->Init(static_cast<Device&>(GetDevice()), shaderModuleDesc);
        m_shaderModules.emplace_back(shaderModule);
        return shaderModule.get();
    }
}
