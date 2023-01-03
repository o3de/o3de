/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <RHI/Conversions.h>
#include <RHI/Device.h>
#include <RHI/PipelineLayout.h>

namespace AZ
{
    namespace DX12
    {
        PipelineLayout::PipelineLayout(PipelineLayoutCache& parentCache)
            : m_parentCache{&parentCache}
        {}

        PipelineLayout::~PipelineLayout()
        {
            m_signature = nullptr;
        }

        void PipelineLayout::add_ref() const
        {
            AZ_Assert(m_useCount >= 0, "PipelineLayout has been deleted");
            ++m_useCount;
        }

        void PipelineLayout::release() const
        {
            AZ_Assert(m_useCount > 0, "Usecount is already 0!");
            if (m_useCount.fetch_sub(1) == 1)
            {
                // The cache has ownership.
                if (m_parentCache)
                {
                    m_parentCache->TryReleasePipelineLayout(this);
                }

                // We were orphaned by the cache, just delete.
                else
                {
                    delete this;
                }
            }
        }

        bool PipelineLayout::HasRootConstants() const
        {
            return m_hasRootConstants;
        }

        size_t PipelineLayout::GetRootParameterBindingCount() const
        {
            return m_indexToRootParameterBindingTable.size();
        }

        RootParameterBinding PipelineLayout::GetRootParameterBindingByIndex(size_t index) const
        {
            return m_indexToRootParameterBindingTable[index];
        }

        RootParameterIndex PipelineLayout::GetRootConstantsRootParameterIndex() const
        {
            return m_rootConstantsRootParameterIndex;
        }

        size_t PipelineLayout::GetSlotByIndex(size_t index) const
        {
            return m_indexToSlotTable[index];
        }

        size_t PipelineLayout::GetIndexBySlot(size_t slot) const
        {
            return m_slotToIndexTable[slot];
        }

        ID3D12RootSignature* PipelineLayout::Get() const
        {
            return m_signature.get();
        }

        HashValue64 PipelineLayout::GetHash() const
        {
            return m_hash;
        }

        void PipelineLayout::Init(ID3D12DeviceX* dx12Device, const RHI::PipelineLayoutDescriptor& descriptor)
        {
            m_hash = descriptor.GetHash();

            const uint32_t groupLayoutCount = static_cast<uint32_t>(descriptor.GetShaderResourceGroupLayoutCount());
            AZ_Assert(groupLayoutCount <= RHI::Limits::Pipeline::ShaderResourceGroupCountMax, "Exceeded ShaderResourceGroupLayout count limit.");

            AZStd::vector<D3D12_ROOT_PARAMETER> parameters;
            AZStd::vector<D3D12_DESCRIPTOR_RANGE> descriptorRanges[RHI::Limits::Pipeline::ShaderResourceGroupCountMax];
            AZStd::vector<D3D12_DESCRIPTOR_RANGE> unboundedArraydescriptorRanges[RHI::Limits::Pipeline::ShaderResourceGroupCountMax];
            AZStd::vector<D3D12_DESCRIPTOR_RANGE> samplerDescriptorRanges[RHI::Limits::Pipeline::ShaderResourceGroupCountMax];
            AZStd::vector<D3D12_STATIC_SAMPLER_DESC> staticSamplers;

            m_layoutDescriptor = &descriptor;
            const PipelineLayoutDescriptor* dx12Descriptor = azrtti_cast<const PipelineLayoutDescriptor*>(&descriptor);
            AZ_Assert(dx12Descriptor, "Trying to create a pipeline layout without a DX12 pipeline layout descriptor");

            /**
             * If the pipeline layout uses an inline constant binding, that becomes the very first
             * parameter in the root signature.
             */
            const RootConstantBinding& rootConstantBinding = dx12Descriptor->GetRootConstantBinding();

            // [GFX TODO][ATOM-622] Support Inline Constant in Shader Assets. At the moment inline constants are not saved anywhere in the shader asset.
            m_hasRootConstants = (rootConstantBinding.m_constantCount > 0);

            if (m_hasRootConstants)
            {
                m_rootConstantsRootParameterIndex = RootParameterIndex(parameters.size());
                parameters.emplace_back();
                D3D12_ROOT_PARAMETER& parameter = parameters.back();

                parameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
                parameter.Constants.Num32BitValues = rootConstantBinding.m_constantCount;
                parameter.Constants.ShaderRegister = rootConstantBinding.m_constantRegister;
                parameter.Constants.RegisterSpace = rootConstantBinding.m_constantRegisterSpace;
                parameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
            }

            // Initialise mapping tables
            m_slotToIndexTable.fill(static_cast<uint8_t>(RHI::Limits::Pipeline::ShaderResourceGroupCountMax));
            m_indexToRootParameterBindingTable.resize(groupLayoutCount);
            m_indexToSlotTable.resize(groupLayoutCount);

            for (uint32_t groupLayoutIndex = 0; groupLayoutIndex < groupLayoutCount; ++groupLayoutIndex)
            {
                const RHI::ShaderResourceGroupLayout& groupLayout = *descriptor.GetShaderResourceGroupLayout(groupLayoutIndex);

                const uint32_t srgLayoutSlot = groupLayout.GetBindingSlot();
                m_slotToIndexTable[srgLayoutSlot] = static_cast<uint8_t>(groupLayoutIndex);
                m_indexToSlotTable[groupLayoutIndex] = static_cast<uint8_t>(srgLayoutSlot);
            }

            // Construct a list of indexes sorted by frequency.
            // nVidia recommends to construct the root signature with higher execution frequency first https://developer.nvidia.com/dx12-dos-and-donts#roots
            // [GFX TODO][ATOM-550] Investigate and Modify DX12 RHI to be able to use different Pipeline Layouts depending on GPU.
            AZStd::vector<uint8_t> indexesSortedByFrequency(groupLayoutCount);
            uint32_t usedSlotIndex = 0;
            for (uint32_t slot = 0; slot < m_slotToIndexTable.size(); ++slot)
            {
                const uint8_t groupLayoutIndex = m_slotToIndexTable[slot];
                if (groupLayoutIndex != RHI::Limits::Pipeline::ShaderResourceGroupCountMax)
                {
                    indexesSortedByFrequency[usedSlotIndex] = groupLayoutIndex;
                    ++usedSlotIndex;
                }
            }
            AZ_Assert(usedSlotIndex == groupLayoutCount, "Unexpected number of used slots");

            // Next, front-load by frequency the SRG Constants. Each SRG with Constants adds a constant buffer entry as root parameters of the root signature.
            for (const uint32_t groupLayoutIndex : indexesSortedByFrequency)
            {
                const RHI::ShaderResourceGroupLayout& groupLayout = *dx12Descriptor->GetShaderResourceGroupLayout(groupLayoutIndex);
                const RHI::ShaderResourceGroupBindingInfo& groupBindInfo = dx12Descriptor->GetShaderResourceGroupBindingInfo(groupLayoutIndex);

                if (groupLayout.GetConstantDataSize())
                {
                    const auto& bindingInfo = descriptor.GetShaderResourceGroupBindingInfo(groupLayoutIndex);
                    const uint32_t registerSpace = bindingInfo.m_constantDataBindingInfo.m_spaceId;
                    const uint32_t cbvRegister = groupBindInfo.m_constantDataBindingInfo.m_registerId;

                    D3D12_ROOT_PARAMETER parameter;
                    parameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;

                    parameter.ShaderVisibility = ConvertShaderStageMask(groupBindInfo.m_constantDataBindingInfo.m_shaderStageMask);

                    parameter.Descriptor.ShaderRegister = cbvRegister;
                    parameter.Descriptor.RegisterSpace = registerSpace;

                    m_indexToRootParameterBindingTable[groupLayoutIndex].m_constantBuffer = RootParameterIndex(parameters.size());
                    parameters.push_back(parameter);
                }
            }

            // Next, process the remaining descriptor tables by frequency.
            for (const uint32_t groupLayoutIndex : indexesSortedByFrequency)
            {
                const RHI::ShaderResourceGroupLayout& groupLayout = *descriptor.GetShaderResourceGroupLayout(groupLayoutIndex);
                const RHI::ShaderResourceGroupBindingInfo& groupBindInfo = dx12Descriptor->GetShaderResourceGroupBindingInfo(groupLayoutIndex);
                const ShaderResourceGroupVisibility& groupVisibility = dx12Descriptor->GetShaderResourceGroupVisibility(groupLayoutIndex);

                if (groupLayout.GetGroupSizeForBuffers() || groupLayout.GetGroupSizeForImages())
                {
                    for (const RHI::ShaderInputBufferDescriptor& shaderInputBuffer : groupLayout.GetShaderInputListForBuffers())
                    {
                        auto findIt = groupBindInfo.m_resourcesRegisterMap.find(shaderInputBuffer.m_name);
                        AZ_Assert(findIt != groupBindInfo.m_resourcesRegisterMap.end(), "Could not find register for shader input %s", shaderInputBuffer.m_name.GetCStr());
                        const RHI::ResourceBindingInfo& bindingInfo = findIt->second;
                        D3D12_DESCRIPTOR_RANGE descriptorRange;
                        descriptorRange.RegisterSpace = bindingInfo.m_spaceId;
                        descriptorRange.NumDescriptors = shaderInputBuffer.m_count;
                        descriptorRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
                        descriptorRange.BaseShaderRegister = bindingInfo.m_registerId;

                        switch (shaderInputBuffer.m_access)
                        {
                        case RHI::ShaderInputBufferAccess::Constant:
                            descriptorRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
                            break;

                        case RHI::ShaderInputBufferAccess::Read:
                            descriptorRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
                            break;

                        case RHI::ShaderInputBufferAccess::ReadWrite:
                            descriptorRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
                            break;
                        }

                        descriptorRanges[groupLayoutIndex].push_back(descriptorRange);
                    }

                    for (const RHI::ShaderInputImageDescriptor& shaderInputImage : groupLayout.GetShaderInputListForImages())
                    {
                        auto findIt = groupBindInfo.m_resourcesRegisterMap.find(shaderInputImage.m_name);
                        AZ_Assert(findIt != groupBindInfo.m_resourcesRegisterMap.end(), "Could not find register for shader input %s", shaderInputImage.m_name.GetCStr());

                        const RHI::ResourceBindingInfo& bindingInfo = findIt->second;
                        D3D12_DESCRIPTOR_RANGE descriptorRange;
                        descriptorRange.RegisterSpace = bindingInfo.m_spaceId;
                        descriptorRange.NumDescriptors = shaderInputImage.m_count;
                        descriptorRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
                        descriptorRange.BaseShaderRegister = bindingInfo.m_registerId;

                        switch (shaderInputImage.m_access)
                        {
                        case RHI::ShaderInputImageAccess::Read:
                            descriptorRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
                            break;

                        case RHI::ShaderInputImageAccess::ReadWrite:
                            descriptorRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
                            break;
                        }

                        descriptorRanges[groupLayoutIndex].push_back(descriptorRange);
                    }

                    D3D12_ROOT_PARAMETER parameter;
                    parameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
                    parameter.ShaderVisibility = ConvertShaderStageMask(groupVisibility.m_descriptorTableShaderStageMask);
                    parameter.DescriptorTable.NumDescriptorRanges = static_cast<UINT>(descriptorRanges[groupLayoutIndex].size());
                    parameter.DescriptorTable.pDescriptorRanges = descriptorRanges[groupLayoutIndex].data();

                    m_indexToRootParameterBindingTable[groupLayoutIndex].m_resourceTable = RootParameterIndex(parameters.size());
                    parameters.push_back(parameter);
                }

                for (const RHI::ShaderInputBufferUnboundedArrayDescriptor& shaderInputBufferUnboundedArray :
                     groupLayout.GetShaderInputListForBufferUnboundedArrays())
                {
                    auto findIt = groupBindInfo.m_resourcesRegisterMap.find(shaderInputBufferUnboundedArray.m_name);
                    AZ_Assert(
                        findIt != groupBindInfo.m_resourcesRegisterMap.end(), "Could not find register for shader input %s",
                        shaderInputBufferUnboundedArray.m_name.GetCStr());
                    const RHI::ResourceBindingInfo& bindingInfo = findIt->second;
                    D3D12_DESCRIPTOR_RANGE descriptorRange;
                    descriptorRange.RegisterSpace = shaderInputBufferUnboundedArray.m_spaceId;
                    descriptorRange.NumDescriptors = aznumeric_cast<UINT>(-1);
                    descriptorRange.OffsetInDescriptorsFromTableStart = 0;
                    descriptorRange.BaseShaderRegister = bindingInfo.m_registerId;

                    switch (shaderInputBufferUnboundedArray.m_access)
                    {
                    case RHI::ShaderInputBufferAccess::Read:
                        descriptorRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
                        break;

                    case RHI::ShaderInputBufferAccess::ReadWrite:
                        descriptorRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
                        break;
                    }

                    unboundedArraydescriptorRanges[groupLayoutIndex].push_back(descriptorRange);
                }

                for (const RHI::ShaderInputImageUnboundedArrayDescriptor& shaderInputImageUnboundedArray :
                     groupLayout.GetShaderInputListForImageUnboundedArrays())
                {
                    auto findIt = groupBindInfo.m_resourcesRegisterMap.find(shaderInputImageUnboundedArray.m_name);
                    AZ_Assert(
                        findIt != groupBindInfo.m_resourcesRegisterMap.end(), "Could not find register for shader input %s",
                        shaderInputImageUnboundedArray.m_name.GetCStr());
                    const RHI::ResourceBindingInfo& bindingInfo = findIt->second;
                    D3D12_DESCRIPTOR_RANGE descriptorRange;
                    descriptorRange.RegisterSpace = shaderInputImageUnboundedArray.m_spaceId;
                    descriptorRange.NumDescriptors = aznumeric_cast<UINT>(-1);
                    descriptorRange.OffsetInDescriptorsFromTableStart = 0;
                    descriptorRange.BaseShaderRegister = bindingInfo.m_registerId;

                    switch (shaderInputImageUnboundedArray.m_access)
                    {
                    case RHI::ShaderInputImageAccess::Read:
                        descriptorRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
                        break;

                    case RHI::ShaderInputImageAccess::ReadWrite:
                        descriptorRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
                        break;
                    }

                    unboundedArraydescriptorRanges[groupLayoutIndex].push_back(descriptorRange);
                }

                if (!unboundedArraydescriptorRanges[groupLayoutIndex].empty())
                {
                    D3D12_ROOT_PARAMETER parameter;
                    parameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
                    parameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
                    parameter.DescriptorTable.NumDescriptorRanges = static_cast<uint32_t>(unboundedArraydescriptorRanges[groupLayoutIndex].size());
                    parameter.DescriptorTable.pDescriptorRanges = unboundedArraydescriptorRanges[groupLayoutIndex].data();

                    m_indexToRootParameterBindingTable[groupLayoutIndex].m_bindlessTable = RootParameterIndex(parameters.size());
                    parameters.push_back(parameter);
                }
            }

            // Next, process the dynamic sampler descriptor tables by frequency. Sampler can't be mixed with other resources
            for (const uint32_t groupLayoutIndex : indexesSortedByFrequency)
            {
                const RHI::ShaderResourceGroupLayout& groupLayout = *descriptor.GetShaderResourceGroupLayout(groupLayoutIndex);
                const RHI::ShaderResourceGroupBindingInfo& groupBindInfo = dx12Descriptor->GetShaderResourceGroupBindingInfo(groupLayoutIndex);
                const ShaderResourceGroupVisibility& groupVisibility = dx12Descriptor->GetShaderResourceGroupVisibility(groupLayoutIndex);

                if (groupLayout.GetGroupSizeForSamplers())
                {
                    for (const RHI::ShaderInputSamplerDescriptor& shaderInputSampler : groupLayout.GetShaderInputListForSamplers())
                    {
                        auto findIt = groupBindInfo.m_resourcesRegisterMap.find(shaderInputSampler.m_name);
                        AZ_Assert(findIt != groupBindInfo.m_resourcesRegisterMap.end(), "Could not find register for shader input %s", shaderInputSampler.m_name.GetCStr());

                        const RHI::ResourceBindingInfo& bindingInfo = findIt->second;
                        D3D12_DESCRIPTOR_RANGE descriptorRange;
                        descriptorRange.RegisterSpace = bindingInfo.m_spaceId;
                        descriptorRange.NumDescriptors = shaderInputSampler.m_count;
                        descriptorRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

                        descriptorRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
                        descriptorRange.BaseShaderRegister = bindingInfo.m_registerId;
                        samplerDescriptorRanges[groupLayoutIndex].push_back(descriptorRange);
                    }

                    D3D12_ROOT_PARAMETER parameter;
                    parameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
                    parameter.ShaderVisibility = ConvertShaderStageMask(groupVisibility.m_descriptorTableShaderStageMask);
                    parameter.DescriptorTable.NumDescriptorRanges = static_cast<UINT>(samplerDescriptorRanges[groupLayoutIndex].size());
                    parameter.DescriptorTable.pDescriptorRanges = samplerDescriptorRanges[groupLayoutIndex].data();

                    m_indexToRootParameterBindingTable[groupLayoutIndex].m_samplerTable = RootParameterIndex(parameters.size());
                    parameters.push_back(parameter);
                }
            }

            // Last, process by frequency the static samplers
            for (const uint32_t groupLayoutIndex : indexesSortedByFrequency)
            {
                const RHI::ShaderResourceGroupLayout& groupLayout = *descriptor.GetShaderResourceGroupLayout(groupLayoutIndex);
                const RHI::ShaderResourceGroupBindingInfo& groupBindInfo = dx12Descriptor->GetShaderResourceGroupBindingInfo(groupLayoutIndex);

                for (const RHI::ShaderInputStaticSamplerDescriptor& samplerInput : groupLayout.GetStaticSamplers())
                {
                    auto findRegisterIt = groupBindInfo.m_resourcesRegisterMap.find(samplerInput.m_name);
                    AZ_Assert(findRegisterIt != groupBindInfo.m_resourcesRegisterMap.end(), "Could not find register for shader input %s", samplerInput.m_name.GetCStr());
                    
                    const RHI::ResourceBindingInfo& bindingInfo = findRegisterIt->second;
                    D3D12_STATIC_SAMPLER_DESC desc;
                    ConvertStaticSampler(
                        samplerInput.m_samplerState, bindingInfo.m_registerId, bindingInfo.m_spaceId,
                        ConvertShaderStageMask(bindingInfo.m_shaderStageMask), desc);

                    staticSamplers.push_back(desc);
                }
            }

            D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc;
            rootSignatureDesc.Flags = AZ_DX12_ROOT_SIGNATURE_FLAGS;
            rootSignatureDesc.NumParameters = static_cast<uint32_t>(parameters.size());
            rootSignatureDesc.pParameters = parameters.data();
            rootSignatureDesc.NumStaticSamplers = static_cast<uint32_t>(staticSamplers.size());
            rootSignatureDesc.pStaticSamplers = staticSamplers.data();

            Microsoft::WRL::ComPtr<ID3DBlob> pOutBlob, pErrorBlob;
            D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, pOutBlob.GetAddressOf(), pErrorBlob.GetAddressOf());
            AZ_Assert(pOutBlob, "Failed to serialize root signature: ErrorBlob [%s]", pErrorBlob ? reinterpret_cast<const char*>(pErrorBlob->GetBufferPointer()) : "No error data returned");

            Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature;
            AssertSuccess(dx12Device->CreateRootSignature(1, pOutBlob->GetBufferPointer(), pOutBlob->GetBufferSize(), IID_GRAPHICS_PPV_ARGS(rootSignature.GetAddressOf())));
            m_signature = rootSignature.Get();

            AZStd::wstring name = AZStd::wstring::format(L"RootSig (%d %d %d)", rootSignatureDesc.NumParameters, rootSignatureDesc.NumStaticSamplers, rootSignatureDesc.Flags);
            m_signature->SetName(name.data());

            // This will signal other waiting threads that the root signature is now ready.
            m_isCompiled = true;
        }

        const RHI::PipelineLayoutDescriptor& PipelineLayout::GetPipelineLayoutDescriptor() const
        {
            return *m_layoutDescriptor;
        }

        RHI::ConstPtr<PipelineLayout> PipelineLayoutCache::Allocate(const RHI::PipelineLayoutDescriptor& descriptor)
        {
            AZ_Assert(m_parentDevice, "Cache is not initialized.");

            const uint64_t hashCode = static_cast<uint64_t>(descriptor.GetHash());
            bool isFirstCompile = false;

            RHI::Ptr<PipelineLayout> layout;

            {
                AZStd::lock_guard<AZStd::mutex> lock(m_mutex);
                auto iter = m_pipelineLayouts.find(hashCode);

                // Reserve space so the next inquiry will find that someone got here first.
                if (iter == m_pipelineLayouts.end())
                {
                    iter = m_pipelineLayouts.emplace(hashCode, aznew PipelineLayout(*this)).first;
                    isFirstCompile = true;
                }

                layout = iter->second;
            }

            if (isFirstCompile)
            {
                layout->Init(m_parentDevice->GetDevice(), descriptor);
            }
            else
            {
                // If we are another thread that has requested an uncompiled root signature, but we lost the race
                // to compile it, we now have to wait until the other thread finishes compilation.
                while (!layout->m_isCompiled)
                {
                    AZStd::this_thread::yield();
                }
            }

            return layout;
        }

        void PipelineLayoutCache::Init(Device& device)
        {
            m_parentDevice = &device;
        }

        void PipelineLayoutCache::Shutdown()
        {
            /**
             * Orphan any remaining pipeline layouts from the cache so they don't
             * de-reference a garbage parent pool pointer when they shutdown.
             */
            for (auto& keyValue : m_pipelineLayouts)
            {
                keyValue.second->m_parentCache = nullptr;
            }
            m_pipelineLayouts.clear();
            m_parentDevice = nullptr;
        }

        void PipelineLayoutCache::TryReleasePipelineLayout(const PipelineLayout* pipelineLayout)
        {
            if (!pipelineLayout)
            {
                return;
            }

            AZStd::lock_guard<AZStd::mutex> lock(m_mutex);

            /**
             * need to check the count again in here in case
             * someone was trying to get the name on another thread
             * Set it to -1 so only this thread will attempt to clean up the
             * cache and delete the pipeline layout.
             */

            int32_t expectedRefCount = 0;
            if (pipelineLayout->m_useCount.compare_exchange_strong(expectedRefCount, -1))
            {
                auto iter = m_pipelineLayouts.find(static_cast<uint64_t>(pipelineLayout->GetHash()));
                if (iter != m_pipelineLayouts.end())
                {
                    m_pipelineLayouts.erase(iter);
                }

                m_parentDevice->QueueForRelease(pipelineLayout->m_signature);
                delete pipelineLayout;
            }
        }
    }
}
