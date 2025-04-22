/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RPI.Edit/Shader/ShaderVariantAssetCreator.h>

#include <AzCore/Utils/TypeHash.h>

#include <Atom/RPI.Reflect/Shader/ShaderOptionGroup.h>

namespace AZ
{
    namespace RPI
    {
        void ShaderVariantAssetCreator::Begin(
            const AZ::Data::AssetId& assetId,
            const ShaderVariantId& shaderVariantId,
            RPI::ShaderVariantStableId stableId,
            bool isFullyBaked)
        {
            BeginCommon(assetId);

            if (ValidateIsReady())
            {
                m_asset->m_stableId = stableId;
                m_asset->m_shaderVariantId = shaderVariantId;
                m_asset->m_isFullyBaked = isFullyBaked;
            }
        }

        bool ShaderVariantAssetCreator::End(Data::Asset<ShaderVariantAsset>& result)
        {
            if (!ValidateIsReady())
            {
                return false;
            }

            if (!m_asset->FinalizeAfterLoad())
            {
                ReportError("Failed to finalize the ShaderResourceGroupAsset.");
                return false;
            }

            bool foundDrawFunctions = false;
            bool foundDispatchFunctions = false;

            if (m_asset->GetShaderStageFunction(RHI::ShaderStage::Vertex) ||
                m_asset->GetShaderStageFunction(RHI::ShaderStage::Geometry) ||
                m_asset->GetShaderStageFunction(RHI::ShaderStage::Fragment))
            {
                foundDrawFunctions = true;
            }

            if (m_asset->GetShaderStageFunction(RHI::ShaderStage::Compute))
            {
                foundDispatchFunctions = true;
            }


            if (foundDrawFunctions && foundDispatchFunctions)
            {
                ReportError("ShaderVariant contains both Draw functions and Dispatch functions.");
                return false;
            }

            if (m_asset->GetShaderStageFunction(RHI::ShaderStage::Fragment) &&
                !m_asset->GetShaderStageFunction(RHI::ShaderStage::Vertex))
            {
                ReportError("Shader Variant with StableId '%u' has a fragment function but no vertex function.", m_asset->m_stableId);
                return false;
            }

            if (m_asset->GetShaderStageFunction(RHI::ShaderStage::Geometry) &&
                !m_asset->GetShaderStageFunction(RHI::ShaderStage::Vertex))
            {
                ReportError("Shader Variant with StableId '%u' has a geometry function but no vertex function.", m_asset->m_stableId);
                return false;
            }

            m_asset->SetReady();
            return EndCommon(result);
        }


        /////////////////////////////////////////////////////////////////////
        // Methods for all shader variant types

        void ShaderVariantAssetCreator::SetShaderFunction(RHI::ShaderStage shaderStage, RHI::Ptr<RHI::ShaderStageFunction> shaderStageFunction)
        {
            if (ValidateIsReady())
            {
                m_asset->m_functionsByStage[static_cast<size_t>(shaderStage)] = shaderStageFunction;
            }
        }

    } // namespace RPI
} // namespace AZ
