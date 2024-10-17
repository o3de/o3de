/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <RHI/WebGPU.h>
#include <RHI/BindGroupLayout.h>
#include <RHI/Device.h>
#include <RHI/PipelineLayout.h>
#include <RHI/RootConstantManager.h>
#include <Atom/RHI.Reflect/PipelineLayoutDescriptor.h>
#include <Atom/RHI.Reflect/ShaderResourceGroupPoolDescriptor.h>

namespace AZ::WebGPU
{
    size_t PipelineLayout::Descriptor::GetHash() const
    {
        return static_cast<size_t>(m_pipelineLayoutDescriptor->GetHash());
    }

    RHI::Ptr<PipelineLayout> PipelineLayout::Create()
    {
        return aznew PipelineLayout();
    }

    RHI::ResultCode PipelineLayout::Init(Device& device, const Descriptor& descriptor)
    {
        Base::Init(device);
        AZ_Assert(descriptor.m_pipelineLayoutDescriptor, "Pipeline layout descriptor is null.");
        m_layoutDescriptor = descriptor.m_pipelineLayoutDescriptor;
        const RHI::PipelineLayoutDescriptor& pipelineLayoutDesc = *m_layoutDescriptor;

        const size_t groupLayoutCount = pipelineLayoutDesc.GetShaderResourceGroupLayoutCount();
        AZ_Assert(
            groupLayoutCount <= RHI::Limits::Pipeline::ShaderResourceGroupCountMax, "Exceeded ShaderResourceGroupLayout count limit.");


        // Initialise mapping tables
        m_slotToIndexTable.fill(static_cast<uint8_t>(RHI::Limits::Pipeline::ShaderResourceGroupCountMax));
        m_indexToSlotTable.resize(groupLayoutCount);
        m_bindGroupLayouts.resize(groupLayoutCount);
        RHI::ResultCode result;
        for (uint32_t groupLayoutIndex = 0; groupLayoutIndex < groupLayoutCount; ++groupLayoutIndex)
        {
            const auto& bindingInfo = pipelineLayoutDesc.GetShaderResourceGroupBindingInfo(groupLayoutIndex);
            const auto* srgLayout = pipelineLayoutDesc.GetShaderResourceGroupLayout(groupLayoutIndex);

            uint32_t spaceId = bindingInfo.m_constantDataBindingInfo.m_spaceId;
            if (spaceId == ~0u)
            {
                AZ_Assert(!bindingInfo.m_resourcesRegisterMap.empty(), "SRG Binding Info has neither constant data nor resources bound");
                spaceId = bindingInfo.m_resourcesRegisterMap.begin()->second.m_spaceId;
            }

            uint32_t bindingSlot = srgLayout->GetBindingSlot();
            m_slotToIndexTable[bindingSlot] = static_cast<uint8_t>(spaceId);
            m_indexToSlotTable[spaceId] = static_cast<uint8_t>(bindingSlot);

            RHI::Ptr<BindGroupLayout> bindGroupLayout = BindGroupLayout::Create();
            BindGroupLayout::Descriptor layoutDesc;
            layoutDesc.m_shaderResouceGroupLayout = srgLayout;
            result = bindGroupLayout->Init(device, layoutDesc);
            RETURN_RESULT_IF_UNSUCCESSFUL(result);
            m_bindGroupLayouts[spaceId] = bindGroupLayout;
        }

        result = BuildNativePipelineLayout();
        RETURN_RESULT_IF_UNSUCCESSFUL(result);

        SetName(GetName());
        return RHI::ResultCode::Success;
    }

    void PipelineLayout::SetNameInternal(const AZStd::string_view& name)
    {
        if (m_wgpuPipelineLayout && !name.empty())
        {
            m_wgpuPipelineLayout.SetLabel(name.data());
        }
    }

    void PipelineLayout::Shutdown()
    {
        m_bindGroupLayouts.clear();
        m_wgpuPipelineLayout = nullptr;
        Base::Shutdown();
    }

    uint32_t PipelineLayout::GetSlotByIndex(uint32_t index) const
    {
        return m_indexToSlotTable[index];
    }

    uint32_t PipelineLayout::GetIndexBySlot(uint32_t slot) const
    {
        return m_slotToIndexTable[slot];
    }

    uint32_t PipelineLayout::GetRootConstantSize() const
    {
        return m_rootConstantSize;
    }

    uint32_t PipelineLayout::GetRootConstantIndex() const
    {
        return m_rootConstantIndex;
    }

    RHI::ResultCode PipelineLayout::BuildNativePipelineLayout()
    {
        AZ_Assert(m_layoutDescriptor, "Pipeline layout descriptor is null.");
        Device& device = static_cast<Device&>(GetDevice());
        size_t numOfBindGroups = m_bindGroupLayouts.size();
        const RHI::ConstantsLayout* rootConstants = m_layoutDescriptor->GetRootConstantsLayout();
        bool usesRootConstants = rootConstants && rootConstants->GetDataSize();
        if (usesRootConstants)
        {
            m_rootConstantSize = rootConstants->GetDataSize();
            // All root constants share the same spaceId
            m_rootConstantIndex = rootConstants->GetShaderInputList().front().m_spaceId;
            numOfBindGroups++;
        }
        AZStd::vector<wgpu::BindGroupLayout> nativeBindGroups(numOfBindGroups);
        for (uint32_t index = 0; index < m_bindGroupLayouts.size(); ++index)
        {
            nativeBindGroups[index] = m_bindGroupLayouts[index]->GetNativeBindGroupLayout();
        }

        if (usesRootConstants)
        {
            nativeBindGroups[m_rootConstantIndex] = device.GetRootConstantManager().GetBindGroupLayout().GetNativeBindGroupLayout();
        }

        wgpu::PipelineLayoutDescriptor descriptor = {};
        descriptor.bindGroupLayoutCount = nativeBindGroups.size();
        descriptor.bindGroupLayouts = nativeBindGroups.data();
        descriptor.label = GetName().GetCStr();

        m_wgpuPipelineLayout = device.GetNativeDevice().CreatePipelineLayout(&descriptor);
        AZ_Assert(m_wgpuPipelineLayout, "Failed to create PipelineLayout");
        return m_wgpuPipelineLayout ? RHI::ResultCode::Success : RHI::ResultCode::Fail;      
    }

    const RHI::PipelineLayoutDescriptor& PipelineLayout::GetPipelineLayoutDescriptor() const
    {
        return *m_layoutDescriptor;
    }

    const wgpu::PipelineLayout& PipelineLayout::GetNativePipelineLayout() const
    {
        return m_wgpuPipelineLayout;
    }

    wgpu::PipelineLayout& PipelineLayout::GetNativePipelineLayout()
    {
        return m_wgpuPipelineLayout;
    }
}
