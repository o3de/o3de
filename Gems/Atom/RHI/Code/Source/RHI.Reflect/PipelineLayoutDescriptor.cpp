/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI.Reflect/PipelineLayoutDescriptor.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Utils/TypeHash.h>

namespace AZ::RHI
{
    void ResourceBindingInfo::Reflect(AZ::ReflectContext* context)
    {
        if (SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context))
        {
            serializeContext->Class<ResourceBindingInfo>()
                ->Version(1)
                ->Field("m_shaderStageMask", &ResourceBindingInfo::m_shaderStageMask)
                ->Field("m_registerId", &ResourceBindingInfo::m_registerId)
                ->Field("m_spaceId", &ResourceBindingInfo::m_spaceId);
        }
    }

    HashValue64 ResourceBindingInfo::GetHash() const
    {
        HashValue64 hash = TypeHash64(static_cast<uint32_t>(m_shaderStageMask));
        hash = TypeHash64(m_registerId, hash);
        return hash;
    }

    void ShaderResourceGroupBindingInfo::Reflect(AZ::ReflectContext* context)
    {
        ResourceBindingInfo::Reflect(context);
        if (SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context))
        {
            serializeContext->Class<ShaderResourceGroupBindingInfo>()
                ->Version(1)
                ->Field("m_constantDataBindingInfo", &ShaderResourceGroupBindingInfo::m_constantDataBindingInfo)
                ->Field("m_resourcesRegisterMap", &ShaderResourceGroupBindingInfo::m_resourcesRegisterMap);
        }
    }

    HashValue64 ShaderResourceGroupBindingInfo::GetHash() const
    {
        HashValue64 seed = TypeHash64(m_constantDataBindingInfo);
        for (const auto& resourceInfo : m_resourcesRegisterMap)
        {
            seed = TypeHash64(resourceInfo.first.GetHash(), seed);
            seed = TypeHash64(resourceInfo.second, seed);
        }
        return seed;
    }

    void PipelineLayoutDescriptor::Reflect(AZ::ReflectContext* context)
    {
        ShaderResourceGroupBindingInfo::Reflect(context);
        ConstantsLayout::Reflect(context);
        if (SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context))
        {
            serializeContext->Class<PipelineLayoutDescriptor>()
                ->Version(4)
                ->Field("m_shaderResourceGroupLayoutsInfo", &PipelineLayoutDescriptor::m_shaderResourceGroupLayoutsInfo)
                ->Field("m_rootConstantLayout", &PipelineLayoutDescriptor::m_rootConstantsLayout)
                ->Field("m_bindingSlotToIndex", &PipelineLayoutDescriptor::m_bindingSlotToIndex)
                ->Field("m_hash", &PipelineLayoutDescriptor::m_hash);
        }
    }

    RHI::Ptr<PipelineLayoutDescriptor> PipelineLayoutDescriptor::Create()
    {
        return aznew PipelineLayoutDescriptor;
    }

    bool PipelineLayoutDescriptor::IsFinalized() const
    {
        return m_hash != InvalidHash;
    }

    void PipelineLayoutDescriptor::Reset()
    {
        m_hash = InvalidHash;
        m_shaderResourceGroupLayoutsInfo.clear();
        m_bindingSlotToIndex.fill(RHI::Limits::Pipeline::ShaderResourceGroupCountMax);
        ResetInternal();
    }

    ResultCode PipelineLayoutDescriptor::Finalize()
    {
        ResultCode resultCode = FinalizeInternal();

        if (resultCode == ResultCode::Success)
        {
            HashValue64 seed = HashValue64{ 0 };
            for (const ShaderResourceGroupLayoutInfo& layoutInfo : m_shaderResourceGroupLayoutsInfo)
            {
                seed = TypeHash64(layoutInfo.first->GetHash(), seed);
                seed = TypeHash64(layoutInfo.second.GetHash(), seed);
            }

            if (m_rootConstantsLayout)
            {
                seed = TypeHash64(m_rootConstantsLayout->GetHash(), seed);
            }

            for (const auto& index : m_bindingSlotToIndex)
            {
                seed = TypeHash64(index, seed);
            }

            m_hash = GetHashInternal(seed);
        }

        return resultCode;
    }

    void PipelineLayoutDescriptor::ResetInternal() {}

    ResultCode PipelineLayoutDescriptor::FinalizeInternal()
    {
        return ResultCode::Success;
    }

    HashValue64 PipelineLayoutDescriptor::GetHashInternal(HashValue64 seed) const
    {
        return seed;
    }

    void PipelineLayoutDescriptor::AddShaderResourceGroupLayoutInfo(const ShaderResourceGroupLayout& layout, const ShaderResourceGroupBindingInfo& shaderResourceGroupInfo)
    {           
        m_bindingSlotToIndex[layout.GetBindingSlot()] = aznumeric_caster(m_shaderResourceGroupLayoutsInfo.size());
        // NOTE: The const_cast is required because serialization does not allow for ConstPtr. However,
        // the layout is always treated as immutable internally, and is only exposed as such externally.
        m_shaderResourceGroupLayoutsInfo.push_back({ const_cast<ShaderResourceGroupLayout*>(&layout), shaderResourceGroupInfo });
    }

    void PipelineLayoutDescriptor::SetRootConstantsLayout(const ConstantsLayout& rootConstantsLayout)
    {
        // NOTE: The const_cast is required because serialization does not allow for ConstPtr.However,
        // the layout is always treated as immutable internally, and is only exposed as such externally.
        m_rootConstantsLayout = const_cast<ConstantsLayout*>(&rootConstantsLayout);
    }

    size_t PipelineLayoutDescriptor::GetShaderResourceGroupLayoutCount() const
    {
        AZ_Assert(IsFinalized(), "Accessor called on a non-finalized pipeline layout. This is not permitted.");

        return m_shaderResourceGroupLayoutsInfo.size();
    }

    const ShaderResourceGroupLayout* PipelineLayoutDescriptor::GetShaderResourceGroupLayout(size_t index) const
    {
        AZ_Assert(IsFinalized(), "Accessor called on a non-finalized pipeline layout. This is not permitted.");

        return m_shaderResourceGroupLayoutsInfo[index].first.get();
    }

    const ShaderResourceGroupBindingInfo& PipelineLayoutDescriptor::GetShaderResourceGroupBindingInfo(size_t index) const
    {
        AZ_Assert(IsFinalized(), "Accessor called on a non-finalized pipeline layout. This is not permitted.");

        return m_shaderResourceGroupLayoutsInfo[index].second;
    }

    const ConstantsLayout* PipelineLayoutDescriptor::GetRootConstantsLayout() const
    {
        AZ_Assert(IsFinalized(), "Accessor called on a non-finalized pipeline layout. This is not permitted.");

        return m_rootConstantsLayout.get();
    }

    HashValue64 PipelineLayoutDescriptor::GetHash() const
    {
        AZ_Assert(IsFinalized(), "Accessor called on a non-finalized pipeline layout. This is not permitted.");
        return m_hash;
    }

    uint32_t PipelineLayoutDescriptor::GetShaderResourceGroupIndexFromBindingSlot(uint32_t bindingSlot) const
    {
        AZ_Assert(IsFinalized(), "Accessor called on a non-finalized pipeline layout. This is not permitted.");

        return m_bindingSlotToIndex[bindingSlot];
    }
}
