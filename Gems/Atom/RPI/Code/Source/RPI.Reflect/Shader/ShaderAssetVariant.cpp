/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RPI.Reflect/Shader/ShaderAssetVariant.h>

#include <AzCore/Serialization/SerializeContext.h>

namespace AZ
{
    namespace RPI
    {
        void ShaderAssetVariant::Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<ShaderAssetVariant>()
                    ->Version(2)
                    ->Field("shaderVariantId", &ShaderAssetVariant::m_shaderVariantId)
                    ->Field("functionIdsByStage", &ShaderAssetVariant::m_functionIdsByStage)
                    ->Field("inputContract", &ShaderAssetVariant::m_inputContract)
                    ->Field("outputContract", &ShaderAssetVariant::m_outputContract)
                    ->Field("renderStates", &ShaderAssetVariant::m_renderStates)
                    ->Field("hash", &ShaderAssetVariant::m_hash)
                    ;
            }
        }

        const ShaderVariantId& ShaderAssetVariant::GetId() const
        {
            return m_shaderVariantId;
        }

        RHI::ShaderStageFunctionId ShaderAssetVariant::GetShaderStageFunctionId(RHI::ShaderStage shaderStage) const
        {
            return m_functionIdsByStage[static_cast<size_t>(shaderStage)];
        }

        const ShaderInputContract& ShaderAssetVariant::GetInputContract() const
        {
            return m_inputContract;
        }

        const ShaderOutputContract& ShaderAssetVariant::GetOutputContract() const
        {
            return m_outputContract;
        }

        RHI::RenderStates ShaderAssetVariant::GetRenderStates() const
        {
            return m_renderStates;
        }

        HashValue64 ShaderAssetVariant::GetHash() const
        {
            return m_hash;
        }
    } // namespace RPI
} // namespace AZ
