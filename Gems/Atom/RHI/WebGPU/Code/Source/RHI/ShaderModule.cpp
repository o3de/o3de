/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <RHI/WebGPU.h>
#include <RHI/Device.h>
#include <RHI/ShaderModule.h>

namespace AZ::WebGPU
{
    RHI::Ptr<ShaderModule> ShaderModule::Create()
    {
        return aznew ShaderModule();
    }

    RHI::ResultCode ShaderModule::Init(Device& device, const Descriptor& descriptor)
    {
        AZ_Assert(!descriptor.m_shaderFunction->GetSourceCode().empty(), "Shader source is empty.");
        Base::Init(device);

        m_descriptor = descriptor;
        m_entryFunctionName = m_descriptor.m_shaderFunction->GetEntryFunctionName();
        wgpu::ShaderModuleWGSLDescriptor wgslDesc;
        wgslDesc.code = descriptor.m_shaderFunction->GetSourceCode().data();
        wgpu::ShaderModuleDescriptor wgpuDescriptor;
        wgpuDescriptor.nextInChain = &wgslDesc;
        wgpuDescriptor.label = GetName().GetCStr();
        m_wgpuShaderModule = device.GetNativeDevice().CreateShaderModule(&wgpuDescriptor);
        AZ_Assert(m_wgpuShaderModule, "Failed to create shader module"); 
        SetName(GetName());
        return RHI::ResultCode::Success;
    }

    const wgpu::ShaderModule& ShaderModule::GetNativeShaderModule() const
    {
        return m_wgpuShaderModule;
    }

    const AZStd::string& ShaderModule::GetEntryFunctionName() const
    {
        return m_entryFunctionName;
    }

    const ShaderStageFunction* ShaderModule::GetStageFunction() const
    {
        return m_descriptor.m_shaderFunction;
    }

    void ShaderModule::SetNameInternal(const AZStd::string_view& name)
    {
        if (m_wgpuShaderModule && !name.empty())
        {
            m_wgpuShaderModule.SetLabel(name.data());
        }
    }

    void ShaderModule::Shutdown()
    {
        m_wgpuShaderModule = nullptr;
        Base::Shutdown();
    }
}
