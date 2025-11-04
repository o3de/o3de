/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI.Reflect/PipelineLayoutDescriptor.h>
#include <Atom/RHI.Reflect/ShaderResourceGroupPoolDescriptor.h>
#include <Atom/RHI.Reflect/Vulkan/Conversion.h>
#include <RHI/DescriptorSetLayout.h>
#include <RHI/Device.h>
#include <RHI/MergedShaderResourceGroup.h>
#include <RHI/MergedShaderResourceGroupPool.h>
#include <RHI/PipelineLayout.h>
#include <RHI/PhysicalDevice.h>
#include <Atom/RHI.Reflect/VkAllocator.h>

namespace AZ
{
    namespace Vulkan
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

        RHI::ConstPtr<RHI::ShaderResourceGroupLayout> PipelineLayout::MergeShaderResourceGroupLayouts(const AZStd::vector<const RHI::ShaderResourceGroupLayout*>& srgLayoutList) const
        {
            if (srgLayoutList.empty())
            {
                return nullptr;
            }

            if (srgLayoutList.size() == 1)
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
                        srgLayout->GetConstantDataSize(),
                        srgBindingInfo.m_constantDataBindingInfo.m_registerId,
                        srgBindingInfo.m_constantDataBindingInfo.m_spaceId);

                    mergedLayout->AddShaderInput(constantsBufferDesc);
                }
                layoutName = AZStd::string::format("%s;%s", layoutName.c_str(), srgLayout->GetName().GetCStr());
            }

            if (!mergedLayout->Finalize())
            {
                AZ_Assert(false, "Failed to merge SRG layouts");
                return nullptr;
            }
            mergedLayout->SetName(AZ::Name(layoutName));
            return mergedLayout;
        }

        RHI::ResultCode PipelineLayout::Init(const Descriptor& descriptor)
        {
            AZ_Assert(descriptor.m_device, "Device is null.");
            Base::Init(*descriptor.m_device);

            AZ_Assert(descriptor.m_pipelineLayoutDescriptor, "Pipeline layout descriptor is null.");
            m_layoutDescriptor = descriptor.m_pipelineLayoutDescriptor;
            const RHI::PipelineLayoutDescriptor& pipelineLayoutDesc = *m_layoutDescriptor;

            const size_t srgCount = pipelineLayoutDesc.GetShaderResourceGroupLayoutCount();
            AZStd::array<AZStd::vector<const RHI::ShaderResourceGroupLayout*>, RHI::Limits::Pipeline::ShaderResourceGroupCountMax> srgLayoutsPerSpace;
            m_indexToSlot.resize(srgCount);
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

            m_descriptorSetLayouts.reserve(srgCount);
            for (uint32_t spaceId = 0; spaceId < srgLayoutsPerSpace.size(); ++spaceId)
            {
                if (srgLayoutsPerSpace[spaceId].empty())
                {
                    continue;
                }

                DescriptorSetLayout::Descriptor desc;
                desc.m_device = descriptor.m_device;
                // This will merge all SRG layouts into 1.
                desc.m_shaderResouceGroupLayout = MergeShaderResourceGroupLayouts(srgLayoutsPerSpace[spaceId]); 

                RHI::Ptr<DescriptorSetLayout> descSetLayout = descriptor.m_device->AcquireDescriptorSetLayout(desc);
                m_descriptorSetLayouts.push_back(descSetLayout);
            }

            RHI::ResultCode result = BuildNativePipelineLayout();
            RETURN_RESULT_IF_UNSUCCESSFUL(result);

            // Merged SRGs are part of the Pipeline Layout.
            result = BuildMergedShaderResourceGroupPools();
            RETURN_RESULT_IF_UNSUCCESSFUL(result);

            SetName(GetName());
            return result;
        }

        void PipelineLayout::SetNameInternal(const AZStd::string_view& name)
        {
            if (IsInitialized() && !name.empty())
            {
                Debug::SetNameToObject(reinterpret_cast<uint64_t>(m_nativePipelineLayout), name.data(), VK_OBJECT_TYPE_PIPELINE_LAYOUT, static_cast<Device&>(GetDevice()));
            }
        }

        void PipelineLayout::Shutdown()
        {
            m_descriptorSetLayouts.clear();
            m_pushConstantRanges.clear();
            m_mergedShaderResourceGroupPools.fill(nullptr);
            if (m_nativePipelineLayout != VK_NULL_HANDLE)
            {
                auto& device = static_cast<Device&>(GetDevice());
                device.GetContext().DestroyPipelineLayout(device.GetNativeDevice(), m_nativePipelineLayout, VkSystemAllocator::Get());
                m_nativePipelineLayout = VK_NULL_HANDLE;
            }
            m_layoutDescriptor = nullptr;

            Base::Shutdown();
        }

        VkPipelineLayout PipelineLayout::GetNativePipelineLayout() const
        {
            return m_nativePipelineLayout;
        }

        size_t PipelineLayout::GetDescriptorSetLayoutCount() const
        {
            return m_descriptorSetLayouts.size();
        }

        RHI::Ptr<DescriptorSetLayout> PipelineLayout::GetDescriptorSetLayout(uint32_t index) const
        {
            AZ_Assert(index < m_descriptorSetLayouts.size(), "Index of descriptor set layout is illegal.");
            return m_descriptorSetLayouts[index];
        }

        PipelineLayout::ShaderResourceGroupBitset PipelineLayout::GetAZSLBindingSlotsOfIndex(uint32_t index) const
        {
            return m_indexToSlot[index];
        }

        uint32_t PipelineLayout::GetIndexFromAZSLBindingSlot(uint32_t slot) const
        {
            return m_slotToIndex[slot];
        }

        uint32_t PipelineLayout::GetPushContantsSize() const
        {
            return m_pushConstantsSize;
        }

        RHI::ResultCode PipelineLayout::BuildNativePipelineLayout()
        {
            AZ_Assert(m_layoutDescriptor, "Pipeline layout descriptor is null.");

            AZStd::vector<VkDescriptorSetLayout> nativeDescriptorSetLayout(m_descriptorSetLayouts.size());
            for (uint32_t index = 0; index < m_descriptorSetLayouts.size(); ++index)
            {
                nativeDescriptorSetLayout[index] = m_descriptorSetLayouts[index]->GetNativeDescriptorSetLayout();
            }

            BuildPushConstantRanges();

            VkPipelineLayoutCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            createInfo.pNext = nullptr;
            createInfo.flags = 0;
            createInfo.setLayoutCount = static_cast<uint32_t>(m_descriptorSetLayouts.size());
            createInfo.pSetLayouts = nativeDescriptorSetLayout.empty() ? nullptr :  nativeDescriptorSetLayout.data();
            createInfo.pushConstantRangeCount = static_cast<uint32_t>(m_pushConstantRanges.size());
            createInfo.pPushConstantRanges = m_pushConstantRanges.empty() ? nullptr : m_pushConstantRanges.data();

            auto& device = static_cast<Device&>(GetDevice());
            const VkResult result = device.GetContext().CreatePipelineLayout(
                device.GetNativeDevice(), &createInfo, VkSystemAllocator::Get(), &m_nativePipelineLayout);

            return ConvertResult(result);
        }

        void PipelineLayout::BuildPushConstantRanges()
        {
            m_pushConstantRanges.clear();
            m_pushConstantsSize = 0;
            const RHI::ConstantsLayout* rootConstantsLayout = m_layoutDescriptor->GetRootConstantsLayout();
            if (rootConstantsLayout && rootConstantsLayout->GetDataSize() > 0)
            {
                m_pushConstantRanges.emplace_back();
                VkPushConstantRange& range = m_pushConstantRanges.back();
                range = {};
                range.offset = 0;
                range.stageFlags = VK_SHADER_STAGE_ALL; // [GFX TODO][ATOM-2767] Use the proper stages of push constants.
                range.size = rootConstantsLayout->GetDataSize();

                m_pushConstantsSize += range.size;
            }
        }

        const RHI::PipelineLayoutDescriptor& PipelineLayout::GetPipelineLayoutDescriptor() const
        {
            return *m_layoutDescriptor;
        }

        RHI::ResultCode PipelineLayout::BuildMergedShaderResourceGroupPools()
        {
            for (uint32_t i = 0; i < m_indexToSlot.size(); ++i)
            {
                const auto& srgBitset = m_indexToSlot[i];
                if (srgBitset.count() > 1)
                {
                    RHI::ShaderResourceGroupPoolDescriptor descriptor;
                    descriptor.m_layout = m_descriptorSetLayouts[i]->GetShaderResourceGroupLayout();

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

        bool PipelineLayout::IsMergedDescriptorSetLayout(uint32_t index) const
        {
            return m_indexToSlot[index].count() > 1;
        }
    }
}
