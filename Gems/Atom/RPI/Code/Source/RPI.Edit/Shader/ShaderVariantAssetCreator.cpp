/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RPI.Edit/Shader/ShaderVariantAssetCreator.h>

#include <AzCore/Utils/TypeHash.h>

#include <Atom/RPI.Reflect/Shader/ShaderInputContract.h>
#include <Atom/RPI.Reflect/Shader/ShaderOutputContract.h>
#include <Atom/RPI.Reflect/Shader/ShaderOptionGroup.h>

namespace AZ
{
    namespace RPI
    {
        void ShaderVariantAssetCreator::Begin(const AZ::Data::AssetId& assetId, const ShaderVariantId& shaderVariantId, RPI::ShaderVariantStableId stableId, const ShaderOptionGroupLayout* shaderOptionGroupLayout)
        {
            BeginCommon(assetId);

            if (ValidateIsReady())
            {
                m_asset->m_stableId = stableId;
                m_asset->m_shaderVariantId = shaderVariantId;

                if (shaderOptionGroupLayout)
                {
                    ShaderOptionGroup shaderOptions{shaderOptionGroupLayout, shaderVariantId};
                    m_asset->m_isFullyBaked = shaderOptions.IsFullySpecified();
                }
                else if(shaderVariantId.m_mask.any())
                {
                    ReportError("ShaderVariantId is not empty, but no ShaderOptionGroupLayout was provided");
                }
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

            const ShaderInputContract& shaderInputContract = m_asset->m_inputContract;
            // Validate that each stream ID appears only once.
            for (const auto& channel : shaderInputContract.m_streamChannels)
            {
                int count = 0;

                for (const auto& searchChannel : shaderInputContract.m_streamChannels)
                {
                    if (channel.m_semantic == searchChannel.m_semantic)
                    {
                        ++count;
                    }
                }

                if (count > 1)
                {
                    ReportError("Input stream channel '%s' appears multiple times. For Shader Variant with StableId '%u' ",
                        channel.m_semantic.ToString().c_str(), m_asset->m_stableId);
                    return false;
                }
            }

            m_asset->SetReady();
            return EndCommon(result);
        }


        /////////////////////////////////////////////////////////////////////
        // Methods for all shader variant types

        void ShaderVariantAssetCreator::SetShaderAssetBuildTimestamp(AZStd::sys_time_t shaderAssetBuildTimestamp)
        {
            if (ValidateIsReady())
            {
                m_asset->m_shaderAssetBuildTimestamp = shaderAssetBuildTimestamp;
            }
        }

        void ShaderVariantAssetCreator::SetShaderFunction(RHI::ShaderStage shaderStage, RHI::Ptr<RHI::ShaderStageFunction> shaderStageFunction)
        {
            if (ValidateIsReady())
            {
                m_asset->m_functionsByStage[static_cast<size_t>(shaderStage)] = shaderStageFunction;
            }
        }

        /////////////////////////////////////////////////////////////////////
        // Methods for PipelineStateType::Draw variants.

        void ShaderVariantAssetCreator::SetInputContract(const ShaderInputContract& contract)
        {
            if (ValidateIsReady())
            {
                m_asset->m_inputContract = contract;
            }
        }

        void ShaderVariantAssetCreator::SetOutputContract(const ShaderOutputContract& contract)
        {
            if (ValidateIsReady())
            {
                m_asset->m_outputContract = contract;
            }
        }

        void ShaderVariantAssetCreator::SetRenderStates(const RHI::RenderStates& renderStates)
        {
            if (ValidateIsReady())
            {
                m_asset->m_renderStates = renderStates;
            }
        }


    } // namespace RPI
} // namespace AZ
