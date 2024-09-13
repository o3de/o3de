/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <RHI/WebGPU.h>
#include <RHI/BindGroup.h>
#include <RHI/BindGroupLayout.h>
#include <RHI/Device.h>
#include <RHI/ShaderResourceGroup.h>
#include <RHI/ShaderResourceGroupPool.h>

namespace AZ::WebGPU
{
    RHI::Ptr<ShaderResourceGroupPool> ShaderResourceGroupPool::Create()
    {
        return aznew ShaderResourceGroupPool();
    }

    RHI::ResultCode ShaderResourceGroupPool::InitInternal(RHI::Device& deviceBase, const RHI::ShaderResourceGroupPoolDescriptor& descriptor)
    {
        auto& device = static_cast<Device&>(deviceBase);
        const RHI::ShaderResourceGroupLayout& layout = *descriptor.m_layout;

        m_bindGroupCount = RHI::Limits::Device::FrameCountMax;

        m_bindGroupLayout = BindGroupLayout::Create();
        BindGroupLayout::Descriptor layoutDescriptor;
        layoutDescriptor.m_shaderResouceGroupLayout = &layout;
        RHI::ResultCode result = m_bindGroupLayout->Init(device, layoutDescriptor);
        RETURN_RESULT_IF_UNSUCCESSFUL(result);
        m_bindGroupLayout->SetName(GetName());
        return RHI::ResultCode::Success;
    }

    RHI::ResultCode ShaderResourceGroupPool::InitGroupInternal(RHI::DeviceShaderResourceGroup& groupBase)
    {
        RHI::ResultCode result = RHI::ResultCode::Success;
        Device& device = static_cast<Device&>(GetDevice());
        auto& group = static_cast<ShaderResourceGroup&>(groupBase);
        BindGroup::Descriptor bindGroupDesc;
        bindGroupDesc.m_bindGroupLayout = m_bindGroupLayout.get();
        for (size_t i = 0; i < m_bindGroupCount; ++i)
        {
            RHI::Ptr<BindGroup> bindGroup = BindGroup::Create();
            result = bindGroup->Init(device, bindGroupDesc);
            RETURN_RESULT_IF_UNSUCCESSFUL(result);
            AZStd::string name = AZStd::string::format("%s_%d", GetName().GetCStr(), static_cast<int>(i));
            bindGroup->SetName(AZ::Name(name));
            group.m_compiledData.push_back(bindGroup);
        }

        return result;
    }

    RHI::ResultCode ShaderResourceGroupPool::CompileGroupInternal(
        RHI::DeviceShaderResourceGroup& groupBase, const RHI::DeviceShaderResourceGroupData& groupData)
    {
        auto& group = static_cast<ShaderResourceGroup&>(groupBase);

        group.UpdateCompiledDataIndex(m_currentIteration);
        BindGroup& bindGroup = *group.m_compiledData[group.GetCompileDataIndex()];

        const RHI::ShaderResourceGroupLayout* layout = groupData.GetLayout();

        auto const& shaderBufferList = layout->GetShaderInputListForBuffers();
        for (uint32_t groupIndex = 0; groupIndex < static_cast<uint32_t>(shaderBufferList.size()); ++groupIndex)
        {
            const RHI::ShaderInputBufferIndex index(groupIndex);
            auto bufViews = groupData.GetBufferViewArray(index);
            uint32_t binding = shaderBufferList[groupIndex].m_registerId;
            bindGroup.UpdateBufferViews(binding, bufViews);
        }

        auto const& shaderImageList = layout->GetShaderInputListForImages();
        for (uint32_t groupIndex = 0; groupIndex < static_cast<uint32_t>(shaderImageList.size()); ++groupIndex)
        {
            const RHI::ShaderInputImageIndex index(groupIndex);
            auto imgViews = groupData.GetImageViewArray(index);
            uint32_t binding = shaderImageList[groupIndex].m_registerId;
            bindGroup.UpdateImageViews(groupIndex, binding, imgViews, shaderImageList[groupIndex].m_type);
        }        

        auto const& shaderSamplerList = layout->GetShaderInputListForSamplers();
        for (uint32_t groupIndex = 0; groupIndex < static_cast<uint32_t>(shaderSamplerList.size()); ++groupIndex)
        {
            const RHI::ShaderInputSamplerIndex index(groupIndex);
            auto samplerArray = groupData.GetSamplerArray(index);
            uint32_t binding = shaderSamplerList[groupIndex].m_registerId;
            bindGroup.UpdateSamplers(binding, samplerArray);
        }

        // WebGPU doesn't support static sampler, so we just use normal samplers
        auto const& shaderStaticSamplerList = layout->GetStaticSamplers();
        for (uint32_t groupIndex = 0; groupIndex < static_cast<uint32_t>(shaderStaticSamplerList.size()); ++groupIndex)
        {
            uint32_t binding = shaderStaticSamplerList[groupIndex].m_registerId;
            bindGroup.UpdateSamplers(binding, AZStd::span<const RHI::SamplerState>(&shaderStaticSamplerList[groupIndex].m_samplerState, 1));
        }

        AZStd::span<const uint8_t> constantData = groupData.GetConstantData();
        if (!constantData.empty())
        {
            bindGroup.UpdateConstantData(constantData);
        }
        bindGroup.CommitUpdates();

        return RHI::ResultCode::Success;
    }

    void ShaderResourceGroupPool::OnFrameEnd()
    {
        m_currentIteration++;
        Base::OnFrameEnd();
    }

    void ShaderResourceGroupPool::SetNameInternal(const AZStd::string_view& name)
    {
        if (m_bindGroupLayout)
        {
            m_bindGroupLayout->SetName(AZ::Name(name));
        }
    }

    void ShaderResourceGroupPool::ShutdownInternal()
    {
        m_currentIteration = 0;
        m_bindGroupCount = 0;
        m_bindGroupLayout = nullptr;
        Base::ShutdownInternal();
    }

    void ShaderResourceGroupPool::ShutdownResourceInternal(RHI::DeviceResource& resourceBase)
    {
        ShaderResourceGroup& group = static_cast<ShaderResourceGroup&>(resourceBase);
        group.m_compiledData.clear();
        Base::ShutdownResourceInternal(resourceBase);
    }
}
