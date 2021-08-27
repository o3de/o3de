/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
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

        void ShaderAssetCreator::BeginSupervariant(const Name& name)
        {
            if (!ValidateIsReady())
            {
                return;
            }

            if (m_currentSupervariant)
            {
                ReportError("Call EndSupervariant() before calling BeginSupervariant again.");
                return;
            }

            if (m_asset->m_currentAPITypeIndex == ShaderAsset::InvalidAPITypeIndex)
            {
                ReportError("Can not begin supervariant with name [%s] because this function must be called between BeginAPI()/EndAPI()", name.GetCStr());
                return;
            }

            if (m_asset->m_perAPIShaderData.empty())
            {
                ReportError("Can not add supervariant with name [%s] because there's no per API shader data", name.GetCStr());
                return;
            }

            ShaderAsset::ShaderApiDataContainer& perAPIShaderData = m_asset->m_perAPIShaderData[m_asset->m_perAPIShaderData.size() - 1];
            if (perAPIShaderData.m_supervariants.empty())
            {
                if (!name.IsEmpty())
                {
                    ReportError("The first supervariant must be nameless. Name [%s] is invalid", name.GetCStr());
                    return;
                }
            }
            else
            {
                if (name.IsEmpty())
                {
                    ReportError(
                        "Only the first supervariant can be nameless. So far there are %zu supervariants",
                        perAPIShaderData.m_supervariants.size());
                    return;
                }
            }

            perAPIShaderData.m_supervariants.push_back({});
            m_currentSupervariant = &perAPIShaderData.m_supervariants[perAPIShaderData.m_supervariants.size() - 1];
            m_currentSupervariant->m_name = name;
        }

        void ShaderAssetCreator::SetSrgLayoutList(const ShaderResourceGroupLayoutList& srgLayoutList)
        {
            if (!ValidateIsReady())
            {
                return;
            }

            if (!m_currentSupervariant)
            {
                ReportError("BeginSupervariant() should be called first before calling %s", __FUNCTION__);
                return;
            }

            m_currentSupervariant->m_srgLayoutList = srgLayoutList;
            for (auto srgLayout : m_currentSupervariant->m_srgLayoutList)
            {
                if (!srgLayout->Finalize())
                {
                    ReportError(
                        "The current supervariant [%s], failed to finalize SRG Layout [%s]", m_currentSupervariant->m_name.GetCStr(),
                        srgLayout->GetName().GetCStr());
                    return;
                }
            }
        }

        //! [Required] Assigns the pipeline layout descriptor shared by all variants in the shader. Shader variants
        //! embedded in a single shader asset are required to use the same pipeline layout. It is not necessary to call
        //! Finalize() on the pipeline layout prior to assignment, but still permitted.
        void ShaderAssetCreator::SetPipelineLayout(RHI::Ptr<RHI::PipelineLayoutDescriptor> pipelineLayoutDescriptor)
        {
            if (!ValidateIsReady())
            {
                return;
            }
            if (!m_currentSupervariant)
            {
                ReportError("BeginSupervariant() should be called first before calling %s", __FUNCTION__);
                return;
            }
            m_currentSupervariant->m_pipelineLayoutDescriptor = pipelineLayoutDescriptor;
        }

        //! Assigns the contract for inputs required by the shader.
        void ShaderAssetCreator::SetInputContract(const ShaderInputContract& contract)
        {
            if (!ValidateIsReady())
            {
                return;
            }
            if (!m_currentSupervariant)
            {
                ReportError("BeginSupervariant() should be called first before calling %s", __FUNCTION__);
                return;
            }
            m_currentSupervariant->m_inputContract = contract;
        }

        //! Assigns the contract for outputs required by the shader.
        void ShaderAssetCreator::SetOutputContract(const ShaderOutputContract& contract)
        {
            if (!ValidateIsReady())
            {
                return;
            }
            if (!m_currentSupervariant)
            {
                ReportError("BeginSupervariant() should be called first before calling %s", __FUNCTION__);
                return;
            }
            m_currentSupervariant->m_outputContract = contract;
        }

        //! Assigns the render states for the draw pipeline. Ignored for non-draw pipelines.
        void ShaderAssetCreator::SetRenderStates(const RHI::RenderStates& renderStates)
        {
            if (!ValidateIsReady())
            {
                return;
            }
            if (!m_currentSupervariant)
            {
                ReportError("BeginSupervariant() should be called first before calling %s", __FUNCTION__);
                return;
            }
            m_currentSupervariant->m_renderStates = renderStates;
        }

        //! [Optional] Not all shaders have attributes before functions. Some attributes do not exist for all RHI::APIType either.
        void ShaderAssetCreator::SetShaderStageAttributeMapList(const RHI::ShaderStageAttributeMapList& shaderStageAttributeMapList)
        {
            if (!ValidateIsReady())
            {
                return;
            }
            if (!m_currentSupervariant)
            {
                ReportError("BeginSupervariant() should be called first before calling %s", __FUNCTION__);
                return;
            }
            m_currentSupervariant->m_attributeMaps = shaderStageAttributeMapList;
        }

        //! [Required] There's always a root variant for each supervariant.
        void ShaderAssetCreator::SetRootShaderVariantAsset(Data::Asset<ShaderVariantAsset> shaderVariantAsset)
        {
            if (!ValidateIsReady())
            {
                return;
            }
            if (!m_currentSupervariant)
            {
                ReportError("BeginSupervariant() should be called first before calling %s", __FUNCTION__);
                return;
            }
            if (!shaderVariantAsset)
            {
                ReportError("Invalid root variant");
                return;
            }
            m_currentSupervariant->m_rootShaderVariantAsset = shaderVariantAsset;
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

            return RHI::PipelineStateType::Count;
        }

        bool ShaderAssetCreator::EndSupervariant()
        {
            if (!ValidateIsReady())
            {
                return false;
            }

            if (!m_currentSupervariant)
            {
                ReportError("Can not end a supervariant that has not started");
                return false;
            }

            if (!m_currentSupervariant->m_rootShaderVariantAsset.IsReady())
            {
                ReportError(
                    "The current supervariant [%s], is missing the root ShaderVariantAsset", m_currentSupervariant->m_name.GetCStr());
                return false;
            }

            // Supervariant specific resources
            if (m_currentSupervariant->m_pipelineLayoutDescriptor)
            {
                if (!m_currentSupervariant->m_pipelineLayoutDescriptor->IsFinalized())
                {
                    if (m_currentSupervariant->m_pipelineLayoutDescriptor->Finalize() != RHI::ResultCode::Success)
                    {
                        ReportError("Failed to finalize pipeline layout descriptor.");
                        return false;
                    }
                }
            }
            else
            {
                ReportError("PipelineLayoutDescriptor not specified.");
                return false;
            }

            const ShaderInputContract& shaderInputContract = m_currentSupervariant->m_inputContract;
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
                    ReportError(
                        "Input stream channel [%s] appears multiple times. For supervariant with name [%s]",
                        channel.m_semantic.ToString().c_str(), m_currentSupervariant->m_name.GetCStr());
                    return false;
                }
            }

            auto pipelineStateType = GetPipelineStateType(m_currentSupervariant->m_rootShaderVariantAsset);
            if (pipelineStateType == RHI::PipelineStateType::Count)
            {
                ReportError("Invalid pipelineStateType for supervariant [%s]", m_currentSupervariant->m_name.GetCStr());
                return false;
            }


            if (m_currentSupervariant->m_name.IsEmpty())
            {
                m_asset->m_pipelineStateType = pipelineStateType;
            }
            else
            {
                if (m_asset->m_pipelineStateType != pipelineStateType)
                {
                    ReportError("All supervariants must be of the same pipelineStateType. Current pipelineStateType is [%d], but for supervariant [%s] the pipelineStateType is [%d]",
                        m_asset->m_pipelineStateType, m_currentSupervariant->m_name.GetCStr(), pipelineStateType);
                    return false;
                }
            }

            m_currentSupervariant = nullptr;
            return true;
        }

        bool ShaderAssetCreator::EndAPI()
        {
            if (!ValidateIsReady())
            {
                return false;
            }
            if (m_currentSupervariant)
            {
                ReportError("EndSupervariant() must be called before calling EndAPI()");
                return false;
            }
            
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

        void ShaderAssetCreator::Clone(const Data::AssetId& assetId, const ShaderAsset& sourceShaderAsset, [[maybe_unused]] const ShaderRootVariantAssets& rootVariantAssets)
        {
            BeginCommon(assetId);

            m_asset->m_name = sourceShaderAsset.m_name;
            m_asset->m_pipelineStateType = sourceShaderAsset.m_pipelineStateType;
            m_asset->m_drawListName = sourceShaderAsset.m_drawListName;
            m_asset->m_shaderOptionGroupLayout = sourceShaderAsset.m_shaderOptionGroupLayout;
            m_asset->m_shaderAssetBuildTimestamp = sourceShaderAsset.m_shaderAssetBuildTimestamp;

            // copy root variant assets
            for (auto& perAPIShaderData : sourceShaderAsset.m_perAPIShaderData)
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
                if (m_asset->m_perAPIShaderData.back().m_supervariants.empty())
                {
                    ReportWarning("Attempting to clone a shader asset that has no supervariants for API [%d]", perAPIShaderData.m_APIType);
                }
                else
                {
                    // currently we only support one supervariant when cloning
                    // [GFX TODO][ATOM-15740] Support multiple supervariants in ShaderAssetCreator::Clone
                    m_asset->m_perAPIShaderData.back().m_supervariants[0].m_rootShaderVariantAsset = foundVariantAsset;
                }
            }

        }
    } // namespace RPI
} // namespace AZ
