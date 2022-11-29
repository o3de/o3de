/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI.Edit/ShaderPlatformInterface.h>
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
                m_asset->m_buildTimestamp = shaderAssetBuildTimestamp;
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
                m_defaultShaderOptionGroup = ShaderOptionGroup{shaderOptionGroupLayout};
            }
        }

        void ShaderAssetCreator::SetShaderOptionDefaultValue(const Name& optionName, const Name& optionValue)
        {
            if (ValidateIsReady())
            {
                if (!m_defaultShaderOptionGroup.SetValue(optionName, optionValue))
                {
                    ReportError("Could not set shader option '%s'.", optionName.GetCStr());
                }
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

            m_asset->m_defaultShaderOptionValueOverrides = m_defaultShaderOptionGroup.GetShaderVariantId();

            m_asset->SetReady();

            return EndCommon(shaderAsset);
        }

        void ShaderAssetCreator::Clone(
            const Data::AssetId& assetId,
            const ShaderAsset& sourceShaderAsset,
            [[maybe_unused]] const ShaderSupervariants& supervariants,
            const AZStd::vector<RHI::ShaderPlatformInterface*>& platformInterfaces)
        {
            BeginCommon(assetId);

            m_asset->m_name = sourceShaderAsset.m_name;
            m_asset->m_pipelineStateType = sourceShaderAsset.m_pipelineStateType;
            m_asset->m_drawListName = sourceShaderAsset.m_drawListName;
            m_asset->m_shaderOptionGroupLayout = sourceShaderAsset.m_shaderOptionGroupLayout;
            m_asset->m_buildTimestamp = sourceShaderAsset.m_buildTimestamp;

            // copy the perAPIShaderData
            for (auto& perAPIShaderData : sourceShaderAsset.m_perAPIShaderData)
            {
                // find the API in the list of supported APIs on this platform
                AZStd::vector<RHI::ShaderPlatformInterface*>::const_iterator itFoundAPI = AZStd::find_if(
                    platformInterfaces.begin(),
                    platformInterfaces.end(),
                    [&perAPIShaderData](const RHI::ShaderPlatformInterface* shaderPlatformInterface)
                    {
                        return perAPIShaderData.m_APIType == shaderPlatformInterface->GetAPIType();
                    });

                if (itFoundAPI == platformInterfaces.end())
                {
                    // the API is not supported on this platform, skip this entry
                    continue;
                }

                if (perAPIShaderData.m_supervariants.empty())
                {
                    ReportWarning("Attempting to clone a shader asset that has no supervariants for API [%d]", perAPIShaderData.m_APIType);
                    continue;
                }

                if (perAPIShaderData.m_supervariants.size() != supervariants.size())
                {
                    ReportError("Incorrect number of supervariants provided to ShaderAssetCreator::Clone");
                    return;
                }

                m_asset->m_perAPIShaderData.push_back(perAPIShaderData);

                // set the supervariants for this API
                for (auto& supervariant : m_asset->m_perAPIShaderData.back().m_supervariants)
                {
                    // find the matching Supervariant by name from the incoming list
                    ShaderSupervariants::const_iterator itFoundSuperVariant = AZStd::find_if(
                        supervariants.begin(),
                        supervariants.end(),
                        [&supervariant](const ShaderSupervariant& shaderSupervariant)
                    {
                        return supervariant.m_name == shaderSupervariant.m_name;
                    });

                    if (itFoundSuperVariant == supervariants.end())
                    {
                        ReportError("Failed to find supervariant [%s]", supervariant.m_name.GetCStr());
                        return;
                    }

                    // find the matching ShaderVariantAsset for this API
                    ShaderRootVariantAssets::const_iterator itFoundRootShaderVariantAsset = AZStd::find_if(
                        itFoundSuperVariant->m_rootVariantAssets.begin(),
                        itFoundSuperVariant->m_rootVariantAssets.end(),
                        [&perAPIShaderData](const ShaderRootVariantAssetPair& rootShaderVariantAsset)
                    {
                        return perAPIShaderData.m_APIType == rootShaderVariantAsset.first;
                    });

                    if (itFoundRootShaderVariantAsset == itFoundSuperVariant->m_rootVariantAssets.end())
                    {
                        ReportWarning("Failed to find root shader variant asset for API [%d] Supervariant [%s]", perAPIShaderData.m_APIType, supervariant.m_name.GetCStr());
                    }
                    else
                    {
                        supervariant.m_rootShaderVariantAsset = itFoundRootShaderVariantAsset->second;
                    }
                }
            }
        }
    } // namespace RPI
} // namespace AZ
