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
#include <RHI/MergedShaderResourceGroup.h>
#include <RHI/MergedShaderResourceGroupPool.h>
#include <RHI/MergedShaderResourceGroupPoolDescriptor.h>
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

    template<class T>
    void AddShaderInput(
        RHI::ShaderResourceGroupLayout& srgLayout,
        const T& shaderInputDesc)
    {
        srgLayout.AddShaderInput(shaderInputDesc);
    }

    template<>
    void AddShaderInput(
        RHI::ShaderResourceGroupLayout& srgLayout,
        const RHI::ShaderInputStaticSamplerDescriptor& shaderInputDesc)
    {
        srgLayout.AddStaticSampler(shaderInputDesc);
    }

    template<class T>
    void AddShaderInputs(
        RHI::ShaderResourceGroupLayout& srgLayout,
        AZStd::span<const T> shaderInputs,
        const uint32_t bindingSlot,
        const RHI::ShaderResourceGroupBindingInfo& srgBindingInfo)
    {
        for (const auto& shaderInputDesc : shaderInputs)
        {
            auto newShaderInputDesc = shaderInputDesc;
            const RHI::ResourceBindingInfo& bindInfo = srgBindingInfo.m_resourcesRegisterMap.find(shaderInputDesc.m_name)->second;
            newShaderInputDesc.m_registerId = bindInfo.m_registerId;
            newShaderInputDesc.m_spaceId = bindInfo.m_spaceId;
            newShaderInputDesc.m_name = MergedShaderResourceGroup::GenerateMergedShaderInputName(shaderInputDesc.m_name, bindingSlot);
            AddShaderInput(srgLayout, newShaderInputDesc);
        }
    }

    RHI::ConstPtr<RHI::ShaderResourceGroupLayout> PipelineLayout::MergeShaderResourceGroupLayouts(
        const AZStd::vector<const RHI::ShaderResourceGroupLayout*>& srgLayoutList, uint32_t spaceId) const
    {
        if (srgLayoutList.empty())
        {
            return nullptr;
        }
        // Check if the root constants buffer was merged into this SRG
        const auto* rootConstantsLayout = m_layoutDescriptor->GetRootConstantsLayout();
        bool mergedRootConstants = rootConstantsLayout && !rootConstantsLayout->GetShaderInputList().empty() &&
            rootConstantsLayout->GetShaderInputList().front().m_spaceId == spaceId;

        if (srgLayoutList.size() == 1 && !mergedRootConstants)
        {
            return srgLayoutList.front();
        }

        AZStd::string layoutName = "[Merged]";
        RHI::Ptr<RHI::ShaderResourceGroupLayout> mergedLayout = RHI::ShaderResourceGroupLayout::Create();
        mergedLayout->SetBindingSlot(srgLayoutList.front()->GetBindingSlot());
        for (const RHI::ShaderResourceGroupLayout* srgLayout : srgLayoutList)
        {
            const uint32_t bindingSlot = srgLayout->GetBindingSlot();
            const auto& srgBindingInfo = m_layoutDescriptor->GetShaderResourceGroupBindingInfo(m_layoutDescriptor->GetShaderResourceGroupIndexFromBindingSlot(bindingSlot));
            // Add all shader inputs to the merged layout.
            AddShaderInputs(*mergedLayout, srgLayout->GetShaderInputListForBuffers(), bindingSlot, srgBindingInfo);
            AddShaderInputs(*mergedLayout, srgLayout->GetShaderInputListForImages(), bindingSlot, srgBindingInfo);
            AddShaderInputs(*mergedLayout, srgLayout->GetShaderInputListForSamplers(), bindingSlot, srgBindingInfo);
            AddShaderInputs(*mergedLayout, srgLayout->GetStaticSamplers(), bindingSlot, srgBindingInfo);

            if (srgLayout->GetConstantDataSize())
            {
                // The merged SRG doesn't have constant data. Instead we replace all constant data of an SRG for a
                // constant buffer entry. This is because we will just use the constant buffer that the original SRG built
                // (the original SRG is one of the source SRGs that was merged together).
                RHI::ShaderInputBufferDescriptor constantsBufferDesc(
                    MergedShaderResourceGroup::GenerateMergedShaderInputName(AZ::Name(MergedShaderResourceGroup::ConstantDataBufferName), bindingSlot),
                    RHI::ShaderInputBufferAccess::Constant,
                    RHI::ShaderInputBufferType::Constant,
                    1,
                    RHI::AlignUp(srgLayout->GetConstantDataSize(), 16u),
                    srgBindingInfo.m_constantDataBindingInfo.m_registerId,
                    srgBindingInfo.m_constantDataBindingInfo.m_spaceId);

                mergedLayout->AddShaderInput(constantsBufferDesc);
            }
            layoutName = AZStd::string::format("%s;%s", layoutName.c_str(), srgLayout->GetName().GetCStr());
        }

        if (mergedRootConstants)
        {
            RHI::ShaderInputBufferDescriptor constantsBufferDesc(
                MergedShaderResourceGroup::GenerateMergedShaderInputName(
                    AZ::Name(MergedShaderResourceGroup::RootConstantBufferName)),
                RHI::ShaderInputBufferAccess::Constant,
                RHI::ShaderInputBufferType::Constant,
                1,
                128,
                rootConstantsLayout->GetShaderInputList().front().m_registerId,
                spaceId);

            mergedLayout->AddShaderInput(constantsBufferDesc);
        }

        if (!mergedLayout->Finalize())
        {
            AZ_Assert(false, "Failed to merge SRG layouts");
            return nullptr;
        }
        mergedLayout->SetName(AZ::Name(layoutName));
        return mergedLayout;
    }

    RHI::ResultCode PipelineLayout::Init(Device& device, const Descriptor& descriptor)
    {
        Base::Init(device);
        AZ_Assert(descriptor.m_pipelineLayoutDescriptor, "Pipeline layout descriptor is null.");
        m_layoutDescriptor = descriptor.m_pipelineLayoutDescriptor;
        const RHI::PipelineLayoutDescriptor& pipelineLayoutDesc = *m_layoutDescriptor;

        const size_t srgCount = pipelineLayoutDesc.GetShaderResourceGroupLayoutCount();
        AZStd::array<AZStd::vector<const RHI::ShaderResourceGroupLayout*>, RHI::Limits::Pipeline::ShaderResourceGroupCountMax> srgLayoutsPerSpace;
        m_slotToIndex.fill(static_cast<uint8_t>(RHI::Limits::Pipeline::ShaderResourceGroupCountMax));

        // Multiple SRGs can have the same "spaceId" (SRGs that need to be merged).
        for (uint32_t srgIndex = 0; srgIndex < srgCount; ++srgIndex)
        {
            const auto& bindingInfo = pipelineLayoutDesc.GetShaderResourceGroupBindingInfo(srgIndex);
            const auto* srgLayout = pipelineLayoutDesc.GetShaderResourceGroupLayout(srgIndex);

            // In contrast to DX12, the "spaceId" in Vulkan (descriptor set index) permits multiple unbounded arrays, and we can assume
            // that all inputs in a given SRG share the same spaceId.
            uint32_t spaceId = bindingInfo.m_constantDataBindingInfo.m_spaceId;
            if (spaceId == ~0u)
            {
                AZ_Assert(!bindingInfo.m_resourcesRegisterMap.empty(), "SRG Binding Info has neither constant data nor resources bound");
                spaceId = bindingInfo.m_resourcesRegisterMap.begin()->second.m_spaceId;
            }

            srgLayoutsPerSpace[spaceId].push_back(srgLayout);

            uint32_t bindingSlot = srgLayout->GetBindingSlot();
            m_indexToSlot[spaceId].set(bindingSlot);
            m_slotToIndex[bindingSlot] = static_cast<uint8_t>(spaceId);
        }

        RHI::ResultCode result;
        for (uint32_t spaceId = 0; spaceId < srgLayoutsPerSpace.size(); ++spaceId)
        {
            if (srgLayoutsPerSpace[spaceId].empty())
            {
                continue;
            }

            RHI::Ptr<BindGroupLayout> bindGroupLayout = BindGroupLayout::Create();
            BindGroupLayout::Descriptor layoutDesc;
            // This will merge all SRG layouts into 1.
            layoutDesc.m_shaderResouceGroupLayout = MergeShaderResourceGroupLayouts(srgLayoutsPerSpace[spaceId], spaceId);
            auto rootConstantsBufferIndex = layoutDesc.m_shaderResouceGroupLayout->FindShaderInputBufferIndex(
                MergedShaderResourceGroup::GenerateMergedShaderInputName(AZ::Name(MergedShaderResourceGroup::RootConstantBufferName)));
            if (rootConstantsBufferIndex.IsValid())
            {
                layoutDesc.m_dynamicOffsetBuffers.push_back(rootConstantsBufferIndex);
            }
            result = bindGroupLayout->Init(device, layoutDesc);
            RETURN_RESULT_IF_UNSUCCESSFUL(result);
            m_bindGroupLayouts.push_back(bindGroupLayout);
        }

        result = BuildNativePipelineLayout();
        RETURN_RESULT_IF_UNSUCCESSFUL(result);

        // Merged SRGs are part of the Pipeline Layout.
        result = BuildMergedShaderResourceGroupPools();
        RETURN_RESULT_IF_UNSUCCESSFUL(result);

        SetName(GetName());
        return result;
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
        m_mergedShaderResourceGroupPools.fill(nullptr);
        m_wgpuPipelineLayout = nullptr;
        Base::Shutdown();
    }

    PipelineLayout::ShaderResourceGroupBitset PipelineLayout::GetSlotsByIndex(uint32_t index) const
    {
        return m_indexToSlot[index];
    }

    uint32_t PipelineLayout::GetIndexBySlot(uint32_t slot) const
    {
        return m_slotToIndex[slot];
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
        const RHI::ConstantsLayout* rootConstants = m_layoutDescriptor->GetRootConstantsLayout();
        bool usesRootConstants = rootConstants && rootConstants->GetDataSize();
        if (usesRootConstants)
        {
            m_rootConstantSize = rootConstants->GetDataSize();
            // All root constants share the same spaceId
            m_rootConstantIndex = rootConstants->GetShaderInputList().front().m_spaceId;
        }
        AZStd::vector<wgpu::BindGroupLayout> nativeBindGroups(m_bindGroupLayouts.size());
        for (uint32_t index = 0; index < m_bindGroupLayouts.size(); ++index)
        {
            nativeBindGroups[index] = m_bindGroupLayouts[index]->GetNativeBindGroupLayout();
        }

        if (usesRootConstants && !IsRootConstantBindGroupMerged())
        {
            if (m_rootConstantIndex >= nativeBindGroups.size())
            {
                nativeBindGroups.resize(m_rootConstantIndex + 1);
            }
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

    RHI::ResultCode PipelineLayout::BuildMergedShaderResourceGroupPools()
    {
        for (uint32_t i = 0; i < m_indexToSlot.size(); ++i)
        {
            const auto& srgBitset = m_indexToSlot[i];
            if (srgBitset.count() > 1)
            {
                MergedShaderResourceGroupPoolDescriptor descriptor;
                descriptor.m_layout = m_bindGroupLayouts[i]->GetShaderResourceGroupLayout();
                descriptor.m_bindGroupLayout = m_bindGroupLayouts[i];

                m_mergedShaderResourceGroupPools[i] = MergedShaderResourceGroupPool::Create();
                auto result = m_mergedShaderResourceGroupPools[i]->Init(GetDevice(), descriptor);
                RETURN_RESULT_IF_UNSUCCESSFUL(result);
            }
        }
        return RHI::ResultCode::Success;
    }

    MergedShaderResourceGroupPool* PipelineLayout::GetMergedShaderResourceGroupPool(uint32_t index) const
    {
        return m_mergedShaderResourceGroupPools[index].get();
    }

    bool PipelineLayout::IsBindGroupMerged(uint32_t index) const
    {
        return m_indexToSlot[index].count() > 1 || m_rootConstantIndex == index;
    }

    bool PipelineLayout::IsRootConstantBindGroupMerged() const
    {
        return m_rootConstantSize && m_indexToSlot[m_rootConstantIndex].any();
    }
}
