/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI.Reflect/DX12/PipelineLayoutDescriptor.h>
#include <AzCore/Utils/TypeHash.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AZ
{
    namespace DX12
    {
        void RootParameterBinding::Reflect(AZ::ReflectContext* context)
        {
            if (SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<RootParameterBinding>()
                    ->Version(1)
                    ->Field("m_constantBuffer", &RootParameterBinding::m_constantBuffer)
                    ->Field("m_resourceTable", &RootParameterBinding::m_resourceTable)
                    ->Field("m_samplerTable", &RootParameterBinding::m_samplerTable);
            }
        }

        void RootConstantBinding::Reflect(AZ::ReflectContext* context)
        {
            if (SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<RootConstantBinding>()
                    ->Version(2)
                    ->Field("m_constantCount", &RootConstantBinding::m_constantCount)
                    ->Field("m_constantRegister", &RootConstantBinding::m_constantRegister)
                    ->Field("m_constantRegisterSpace", &RootConstantBinding::m_constantRegisterSpace);
            }
        }

        RootConstantBinding::RootConstantBinding(
            uint32_t constantCount,
            uint32_t constantRegister,
            uint32_t constantRegisterSpace)
            : m_constantCount(constantCount)
            , m_constantRegister(constantRegister)
            , m_constantRegisterSpace(constantRegisterSpace)
        {
        }

        HashValue64 RootConstantBinding::GetHash(HashValue64 seed) const
        {
            HashValue64 hash = TypeHash64(m_constantCount, seed);
            hash = TypeHash64(m_constantRegister, hash);
            hash = TypeHash64(m_constantRegisterSpace, hash);
            return hash;
        }

        void PipelineLayoutDescriptor::Reflect(AZ::ReflectContext* context)
        {
            RootParameterBinding::Reflect(context);
            RootConstantBinding::Reflect(context);
            ShaderResourceGroupVisibility::Reflect(context);

            if (SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<PipelineLayoutDescriptor, Base>()
                    ->Version(3)
                    ->Field("m_rootConstantBinding", &PipelineLayoutDescriptor::m_rootConstantBinding)
                    ->Field("m_shaderResourceGroupVisibilities", &PipelineLayoutDescriptor::m_shaderResourceGroupVisibilities)
                    ;
            }
        }

        void ShaderResourceGroupVisibility::Reflect(AZ::ReflectContext* context)
        {
            if (SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<ShaderResourceGroupVisibility>()
                    ->Version(0)
                    ->Field("m_descriptorTableShaderStageMask", &ShaderResourceGroupVisibility::m_descriptorTableShaderStageMask)
                    ;
            }
        }

        HashValue64 ShaderResourceGroupVisibility::GetHash(HashValue64 seed) const
        {
            return TypeHash64(m_descriptorTableShaderStageMask, seed);
        }

        RHI::Ptr<PipelineLayoutDescriptor> PipelineLayoutDescriptor::Create()
        {
            return aznew PipelineLayoutDescriptor();
        }

        void PipelineLayoutDescriptor::SetRootConstantBinding(const RootConstantBinding& rootConstantBinding)
        {
            m_rootConstantBinding = rootConstantBinding;
        }

        const RootConstantBinding& PipelineLayoutDescriptor::GetRootConstantBinding() const
        {
            return m_rootConstantBinding;
        }

        HashValue64 PipelineLayoutDescriptor::GetHashInternal(HashValue64 seed) const
        {
            HashValue64 hash = TypeHash64(m_rootConstantBinding, seed);
            for (const auto& visibility : m_shaderResourceGroupVisibilities)
            {
                hash = TypeHash64(visibility.GetHash(), hash);
            }
            return hash;
        }

        void PipelineLayoutDescriptor::AddShaderResourceGroupVisibility(const ShaderResourceGroupVisibility& shaderResourceGroupVisibility)
        {
            m_shaderResourceGroupVisibilities.push_back(shaderResourceGroupVisibility);
        }

        const ShaderResourceGroupVisibility& PipelineLayoutDescriptor::GetShaderResourceGroupVisibility(uint32_t index) const
        {
            return m_shaderResourceGroupVisibilities[index];
        }
    }
}
