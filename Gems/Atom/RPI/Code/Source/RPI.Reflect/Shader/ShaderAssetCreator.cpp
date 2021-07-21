/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Reflect/Shader/ShaderAssetCreator.h>

namespace AZ
{
    namespace RPI
    {
        void ShaderAssetCreator::Begin(const Data::AssetId& assetId)
        {
            BeginCommon(assetId);
        }

        void ShaderAssetCreator::SetShaderAssetBuildTimestamp(AZStd::sys_time_t shaderAssetBuildTimestamp)
        {
            if (ValidateIsReady())
            {
                m_asset->m_shaderAssetBuildTimestamp = shaderAssetBuildTimestamp;
            }
        }

        void ShaderAssetCreator::SetName(const Name& name)
        {
            if (ValidateIsReady())
            {
                m_asset->m_name = name;
            }
        }
        
        void ShaderAssetCreator::SetDrawListName(const Name& name)
        {
            if (ValidateIsReady())
            {
                m_asset->m_drawListName = name;
            }
        }

        void ShaderAssetCreator::AddShaderResourceGroupAsset(const Data::Asset<ShaderResourceGroupAsset>& shaderResourceGroupAsset)
        {
            if (!shaderResourceGroupAsset)
            {
                ReportError("ShaderResourceGroupAsset '%s' is not loaded.", shaderResourceGroupAsset.GetId().ToString<AZStd::string>().c_str());
                return;
            }

            if (ValidateIsReady())
            {
                auto& shaderResourceGroupAssets = m_asset->m_shaderResourceGroupAssets;

                const auto findIt = AZStd::find(shaderResourceGroupAssets.begin(), shaderResourceGroupAssets.end(), shaderResourceGroupAsset);
                if (findIt == shaderResourceGroupAssets.end())
                {
                    shaderResourceGroupAssets.push_back(shaderResourceGroupAsset);
                }
            }
        }

        void ShaderAssetCreator::SetShaderOptionGroupLayout(const Ptr<ShaderOptionGroupLayout>& shaderOptionGroupLayout)
        {
            if (ValidateIsReady())
            {
                m_asset->m_shaderOptionGroupLayout = shaderOptionGroupLayout;
            }
        }

        void ShaderAssetCreator::BeginAPI(RHI::APIType type)
        {
            if (ValidateIsReady())
            {
                ShaderAsset::ShaderApiDataContainer shaderData;
                shaderData.m_APIType = type;
                m_asset->m_currentAPITypeIndex = m_asset->m_perAPIShaderData.size();
                m_asset->m_perAPIShaderData.push_back(shaderData);
            }
        }

        void ShaderAssetCreator::SetRootShaderVariantAsset(Data::Asset<ShaderVariantAsset> shaderVariantAsset)
        {
            if (ValidateIsReady())
            {
                m_asset->GetCurrentShaderApiData().m_rootShaderVariantAsset = shaderVariantAsset;
            }
        }

        void ShaderAssetCreator::SetShaderStageAttributeMapList(const RHI::ShaderStageAttributeMapList& shaderStageAttributeMapList)
        {
            if (ValidateIsReady())
            {
                m_asset->GetCurrentShaderApiData().m_attributeMaps = shaderStageAttributeMapList;
            }
        }

        void ShaderAssetCreator::SetPipelineLayout(const Ptr<RHI::PipelineLayoutDescriptor>& pipelineLayoutDescriptor)
        {
            if (ValidateIsReady())
            {
                m_asset->GetCurrentShaderApiData().m_pipelineLayoutDescriptor = pipelineLayoutDescriptor;
            }
        }

        static RHI::PipelineStateType GetPipelineStateType(const Data::Asset<ShaderVariantAsset>& shaderVariantAsset)
        {
            if (shaderVariantAsset->GetShaderStageFunction(RHI::ShaderStage::Vertex) ||
                shaderVariantAsset->GetShaderStageFunction(RHI::ShaderStage::Tessellation) ||
                shaderVariantAsset->GetShaderStageFunction(RHI::ShaderStage::Fragment))
            {
                return RHI::PipelineStateType::Draw;
            }

            if (shaderVariantAsset->GetShaderStageFunction(RHI::ShaderStage::Compute))
            {
                return RHI::PipelineStateType::Dispatch;
            }

            if (shaderVariantAsset->GetShaderStageFunction(RHI::ShaderStage::RayTracing))
            {
                return RHI::PipelineStateType::RayTracing;
            }

            // For practical purposes We should never get to this point because  a Data::Asset<ShaderVariantAsset> is always validated.
            AZ_Assert(false, "Invalid Shader Variant Asset. Couldn't deduce the pipeline state type.");
            return RHI::PipelineStateType::Count;
        }

        bool ShaderAssetCreator::EndAPI()
        {
            if (!ValidateIsReady())
            {
                return false;
            }

            // Shared resources by all RHI API versions
            auto shaderOptionGroupLayout = m_asset->m_shaderOptionGroupLayout;
            auto& shaderResourceGroupAssets = m_asset->m_shaderResourceGroupAssets;

            // RHI API specific resources
            auto& pipelineLayoutDescriptor = m_asset->GetCurrentShaderApiData().m_pipelineLayoutDescriptor;
            auto& rootShaderVariantAsset = m_asset->GetCurrentShaderApiData().m_rootShaderVariantAsset;
            if (!rootShaderVariantAsset)
            {
                ReportError("Invalid root variant");
                return false;
            }

            if (pipelineLayoutDescriptor)
            {
                if (!pipelineLayoutDescriptor->IsFinalized())
                {
                    if (pipelineLayoutDescriptor->Finalize() != RHI::ResultCode::Success)
                    {
                        ReportError("Failed to finalize pipeline layout descriptor.");
                        return false;
                    }
                }

                // Validate that the SRG layouts match between the local SRG assets and the pipeline layout.

                if (pipelineLayoutDescriptor->GetShaderResourceGroupLayoutCount() != shaderResourceGroupAssets.size())
                {
                    ReportError(
                        "Number of shader resource group layouts specified in pipeline layout does not match the "
                        "number of shader resource group assets added to ShaderAssetCreator.");
                    return false;
                }

                for (size_t i = 0; i < shaderResourceGroupAssets.size(); ++i)
                {
                    const RHI::ShaderResourceGroupLayout* layoutForAsset = shaderResourceGroupAssets[i]->GetLayout(m_asset->GetCurrentShaderApiData().m_APIType);
                    const RHI::ShaderResourceGroupLayout* layoutForPipeline = pipelineLayoutDescriptor->GetShaderResourceGroupLayout(i);

                    if (layoutForAsset->GetHash() != layoutForPipeline->GetHash())
                    {
                        ReportError(
                            "ShaderResourceGroupAsset '%s' at index '%d' does not match ShaderResourceGroupLayout specified in PipelineLayoutDescriptor",
                            shaderResourceGroupAssets[i]->GetName().GetCStr(), i);
                        return false;
                    }
                }
            }
            else
            {
                ReportError("PipelineLayoutDescriptor not specified.");
                return false;
            }

            if (!shaderOptionGroupLayout)
            {
                ReportError("ShaderOptionGroupLayout not specified.");
                return false;
            }

            m_asset->m_pipelineStateType = GetPipelineStateType(rootShaderVariantAsset);
            
            m_asset->m_currentAPITypeIndex = ShaderAsset::InvalidAPITypeIndex;
            return true;
        }

        bool ShaderAssetCreator::End(Data::Asset<ShaderAsset>& shaderAsset)
        {
            if (!ValidateIsReady())
            {
                return false;
            }           

            if (m_asset->m_perAPIShaderData.empty())
            {
                ReportError("Empty shader data. Check that a valid RHI is enabled for this platform.");
                return false;
            }
            
            if (!m_asset->SelectShaderApiData())
            {
                ReportError("Failed to finalize the ShaderAsset.");
                return false;
            }

            m_asset->SetReady();

            return EndCommon(shaderAsset);
        }

        void ShaderAssetCreator::Clone(const Data::AssetId& assetId,
                                       const ShaderAsset* sourceShaderAsset,
                                       const ShaderResourceGroupAssets& srgAssets,
                                       const ShaderRootVariantAssets& rootVariantAssets)
        {
            BeginCommon(assetId);

            m_asset->m_name = sourceShaderAsset->m_name;
            m_asset->m_pipelineStateType = sourceShaderAsset->m_pipelineStateType;

            // copy Srg assets
            AZ_Assert(srgAssets.size() == sourceShaderAsset->m_shaderResourceGroupAssets.size(), "incorrect number of Srg assets passed to Clone()");
            for (const auto& srgAsset : srgAssets)
            {
                m_asset->m_shaderResourceGroupAssets.push_back(srgAsset);
            }

            m_asset->m_shaderOptionGroupLayout = sourceShaderAsset->m_shaderOptionGroupLayout;

            // copy root variant assets
            for (auto& perAPIShaderData : sourceShaderAsset->m_perAPIShaderData)
            {
                // find the matching ShaderVariantAsset
                AZ::Data::Asset<ShaderVariantAsset> foundVariantAsset;
                for (const auto& variantAsset : rootVariantAssets)
                {
                    if (variantAsset.first == perAPIShaderData.m_APIType)
                    {
                        foundVariantAsset = variantAsset.second;
                        break;
                    }
                }

                if (!foundVariantAsset)
                {
                    ReportWarning("Failed to find variant asset for API [%d]", perAPIShaderData.m_APIType);
                }

                m_asset->m_perAPIShaderData.push_back(perAPIShaderData);
                m_asset->m_perAPIShaderData.back().m_rootShaderVariantAsset = foundVariantAsset;
            }

            m_asset->m_drawListName = sourceShaderAsset->m_drawListName;
        }
    } // namespace RPI
} // namespace AZ
