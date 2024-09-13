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
        m_bindGroupLayouts.reserve(groupLayoutCount);
        RHI::ResultCode result;
        for (uint32_t groupLayoutIndex = 0; groupLayoutIndex < groupLayoutCount; ++groupLayoutIndex)
        {
            const auto* srgLayout = pipelineLayoutDesc.GetShaderResourceGroupLayout(groupLayoutIndex);

            uint32_t bindingSlot = srgLayout->GetBindingSlot();
            m_slotToIndexTable[bindingSlot] = static_cast<uint8_t>(groupLayoutIndex);
            m_indexToSlotTable[groupLayoutIndex] = static_cast<uint8_t>(bindingSlot);

            RHI::Ptr<BindGroupLayout> bindGroupLayout = BindGroupLayout::Create();
            BindGroupLayout::Descriptor layoutDesc;
            layoutDesc.m_shaderResouceGroupLayout = srgLayout;
            result = bindGroupLayout->Init(device, layoutDesc);
            RETURN_RESULT_IF_UNSUCCESSFUL(result);
            m_bindGroupLayouts.push_back(bindGroupLayout);
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

    uint32_t PipelineLayout::GetPushContantsSize() const
    {
        return m_pushConstantsSize;
    }

    RHI::ResultCode PipelineLayout::BuildNativePipelineLayout()
    {
        AZ_Assert(m_layoutDescriptor, "Pipeline layout descriptor is null.");
        AZStd::vector<wgpu::BindGroupLayout> nativeBindGroups(m_bindGroupLayouts.size());
        for (uint32_t index = 0; index < m_bindGroupLayouts.size(); ++index)
        {
            nativeBindGroups[index] = m_bindGroupLayouts[index]->GetNativeBindGroupLayout();
        }

        wgpu::PipelineLayoutDescriptor descriptor = {};
        descriptor.bindGroupLayoutCount = nativeBindGroups.size();
        descriptor.bindGroupLayouts = nativeBindGroups.data();
        descriptor.label = GetName().GetCStr();

        m_wgpuPipelineLayout = static_cast<Device&>(GetDevice()).GetNativeDevice().CreatePipelineLayout(&descriptor);
        AZ_Assert(m_wgpuPipelineLayout, "Failed to create PipelineLayout");
        return m_wgpuPipelineLayout ? RHI::ResultCode::Success : RHI::ResultCode::Fail;      
    }

    void PipelineLayout::BuildPushConstantRanges()
    {
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
