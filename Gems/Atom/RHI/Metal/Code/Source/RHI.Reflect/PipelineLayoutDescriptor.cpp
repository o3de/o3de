/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI.Reflect/Metal/PipelineLayoutDescriptor.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Utils/TypeHash.h>

namespace AZ
{
    namespace Metal
    {
        void ShaderResourceGroupVisibility::Reflect(AZ::ReflectContext* context)
        {
            if (SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<ShaderResourceGroupVisibility>()
                    ->Version(0)
                    ->Field("m_resourcesStageMask", &ShaderResourceGroupVisibility::m_resourcesStageMask)
                    ->Field("m_constantDataStageMask", &ShaderResourceGroupVisibility::m_constantDataStageMask)
                    ;
            }
        }

        HashValue64 ShaderResourceGroupVisibility::GetHash(HashValue64 seed) const
        {
            HashValue64 hash = TypeHash64(m_constantDataStageMask, seed);
            for (const auto& it : m_resourcesStageMask)
            {
                hash = TypeHash64(it.first.GetHash(), seed);
                hash = TypeHash64(it.second, seed);
            }

            return hash;
        }

        void RootConstantBinding::Reflect(AZ::ReflectContext* context)
        {
            if (SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<RootConstantBinding>()
                    ->Version(1)
                    ->Field("m_constantRegister", &RootConstantBinding::m_constantRegister)
                    ->Field("m_constantRegisterSpace", &RootConstantBinding::m_constantRegisterSpace);
            }
        }

        RootConstantBinding::RootConstantBinding(
            uint32_t constantRegister,
            uint32_t constantRegisterSpace)
            : m_constantRegister(constantRegister)
            , m_constantRegisterSpace(constantRegisterSpace)
        {
        }

        HashValue64 RootConstantBinding::GetHash(HashValue64 seed) const
        {
            HashValue64 hash = TypeHash64(m_constantRegister, seed);
            hash = TypeHash64(m_constantRegisterSpace, hash);
            return hash;
        }
        void PipelineLayoutDescriptor::Reflect(AZ::ReflectContext* context)
        {
            ShaderResourceGroupVisibility::Reflect(context);
            RootConstantBinding::Reflect(context);
            if (SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<PipelineLayoutDescriptor, Base>()
                ->Version(4)
                ->Field("m_slotToIndexTable", &PipelineLayoutDescriptor::m_slotToIndexTable)
                ->Field("m_indexToSlotTable", &PipelineLayoutDescriptor::m_indexToSlotTable)
                ->Field("m_shaderResourceGroupVisibilities", &PipelineLayoutDescriptor::m_shaderResourceGroupVisibilities)
                ->Field("m_rootConstantBinding", &PipelineLayoutDescriptor::m_rootConstantBinding);
            }
        }
        
        void PipelineLayoutDescriptor::SetBindingTables(const SlotToIndexTable& slotToIndexTable, const IndexToSlotTable& indexToSlotTable)
        {
            m_slotToIndexTable = slotToIndexTable;
            m_indexToSlotTable = indexToSlotTable;
        }
        
        const SlotToIndexTable& PipelineLayoutDescriptor::GetSlotToIndexTable() const
        {
            AZ_Assert(IsFinalized(), "Accessor called on a non-finalized pipeline layout. This is not permitted.");
            
            return m_slotToIndexTable;
        }
        
        const IndexToSlotTable& PipelineLayoutDescriptor::GetIndexToSlotTable() const
        {
            AZ_Assert(IsFinalized(), "Accessor called on a non-finalized pipeline layout. This is not permitted.");
            
            return m_indexToSlotTable;
        }

        void PipelineLayoutDescriptor::AddShaderResourceGroupVisibility(const ShaderResourceGroupVisibility& visibilityInfo)
        {
            m_shaderResourceGroupVisibilities.push_back(visibilityInfo);
        }

        const ShaderResourceGroupVisibility& PipelineLayoutDescriptor::GetShaderResourceGroupVisibility(uint32_t index) const
        {
            return m_shaderResourceGroupVisibilities[index];
        }
        
        RHI::Ptr<PipelineLayoutDescriptor> PipelineLayoutDescriptor::Create()
        {
            return aznew PipelineLayoutDescriptor();
        }

        void PipelineLayoutDescriptor::ResetInternal()
        {
            m_slotToIndexTable.fill(static_cast<uint8_t>(RHI::Limits::Pipeline::ShaderResourceGroupCountMax));
            m_indexToSlotTable.clear();
            m_shaderResourceGroupVisibilities.clear();
        }
        
        HashValue64 PipelineLayoutDescriptor::GetHashInternal(HashValue64 seed) const
        {
            HashValue64 hash = seed;
            
            hash = TypeHash64(m_slotToIndexTable, hash);
            hash = TypeHash64(m_indexToSlotTable.data(), m_indexToSlotTable.size() * sizeof(IndexToSlotTable::value_type), hash);
            for (const auto& visibilityInfo : m_shaderResourceGroupVisibilities)
            {
                hash = TypeHash64(visibilityInfo.GetHash(), hash);
            }
            hash = TypeHash64(m_rootConstantBinding, hash);
            return hash;
        }
    
        void PipelineLayoutDescriptor::SetRootConstantBinding(const RootConstantBinding& rootConstantBinding)
        {
            m_rootConstantBinding = rootConstantBinding;
        }
    
        const RootConstantBinding& PipelineLayoutDescriptor::GetRootConstantBinding() const
        {
            return m_rootConstantBinding;
        }
    }
}
