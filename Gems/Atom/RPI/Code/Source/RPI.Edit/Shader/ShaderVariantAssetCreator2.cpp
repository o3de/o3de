/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#include <Atom/RPI.Edit/Shader/ShaderVariantAssetCreator2.h>

#include <AzCore/Utils/TypeHash.h>

#include <Atom/RPI.Reflect/Shader/ShaderInputContract.h>
#include <Atom/RPI.Reflect/Shader/ShaderOutputContract.h>
#include <Atom/RPI.Reflect/Shader/ShaderOptionGroup.h>

namespace AZ
{
    namespace RPI
    {
        void ShaderVariantAssetCreator2::Begin(const AZ::Data::AssetId& assetId, const ShaderVariantId& shaderVariantId, RPI::ShaderVariantStableId stableId, bool isFullyBaked)
        {
            BeginCommon(assetId);

            if (ValidateIsReady())
            {
                m_asset->m_stableId = stableId;
                m_asset->m_shaderVariantId = shaderVariantId;
                m_asset->m_isFullyBaked = isFullyBaked;
            }
        }

        bool ShaderVariantAssetCreator2::End(Data::Asset<ShaderVariantAsset2>& result)
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
                m_asset->GetShaderStageFunction(RHI::ShaderStage::Tessellation) ||
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

            if (m_asset->GetShaderStageFunction(RHI::ShaderStage::Tessellation) &&
                !m_asset->GetShaderStageFunction(RHI::ShaderStage::Vertex))
            {
                ReportError("Shader Variant with StableId '%u' has a tessellation function but no vertex function.", m_asset->m_stableId);
                return false;
            }



            m_asset->SetReady();
            return EndCommon(result);
        }


        /////////////////////////////////////////////////////////////////////
        // Methods for all shader variant types

        void ShaderVariantAssetCreator2::SetBuildTimestamp(AZStd::sys_time_t buildTimestamp)
        {
            if (ValidateIsReady())
            {
                m_asset->m_buildTimestamp = buildTimestamp;
            }
        }

        void ShaderVariantAssetCreator2::SetShaderFunction(RHI::ShaderStage shaderStage, RHI::Ptr<RHI::ShaderStageFunction> shaderStageFunction)
        {
            if (ValidateIsReady())
            {
                m_asset->m_functionsByStage[static_cast<size_t>(shaderStage)] = shaderStageFunction;
            }
        }

    } // namespace RPI
} // namespace AZ
