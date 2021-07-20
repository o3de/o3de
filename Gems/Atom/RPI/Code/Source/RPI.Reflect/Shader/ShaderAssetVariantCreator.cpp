/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RPI.Reflect/Shader/ShaderAssetVariantCreator.h>

#include <AzCore/Utils/TypeHash.h>

namespace AZ
{
    namespace RPI
    {
        ShaderAssetVariantCreator::ShaderAssetVariantCreator(ShaderVariantId id)
        {
            m_variant.m_shaderVariantId = AZStd::move(id);
        }

        ShaderAssetVariant ShaderAssetVariantCreator::End()
        {
            const uint8_t*  functionIdData = reinterpret_cast<const uint8_t*>(m_variant.m_functionIdsByStage.data());
            const size_t functionIdDataSize = m_variant.m_functionIdsByStage.size() * sizeof(RHI::ShaderStageFunctionId);

            HashValue64 hash = HashValue64{ 0 };
            hash = TypeHash64(functionIdData, functionIdDataSize, hash);
            hash = TypeHash64(m_variant.m_inputContract.GetHash(), hash);
            hash = TypeHash64(m_variant.m_outputContract.GetHash(), hash);
            hash = m_variant.m_renderStates.GetHash(hash);
            m_variant.m_hash = hash;

            return m_variant;
        }

        void ShaderAssetVariantCreator::SetShaderFunctionId(RHI::ShaderStage shaderStage, RHI::ShaderStageFunctionId functionId)
        {
            m_variant.m_functionIdsByStage[static_cast<size_t>(shaderStage)] = functionId;
        }

        void ShaderAssetVariantCreator::SetInputContract(const ShaderInputContract& contract)
        {
            m_variant.m_inputContract = contract;
        }

        void ShaderAssetVariantCreator::SetOutputContract(const ShaderOutputContract& contract)
        {
            m_variant.m_outputContract = contract;
        }

        void ShaderAssetVariantCreator::SetRenderStates(const RHI::RenderStates& renderStates)
        {
            m_variant.m_renderStates = renderStates;
        }
    } // namespace RPI
} // namespace AZ
