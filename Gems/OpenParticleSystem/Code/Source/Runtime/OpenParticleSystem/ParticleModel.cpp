/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ParticleModel.h"

namespace OpenParticle
{
    void ParticleModel::SetupModel(AZ::Data::Asset<AZ::RPI::ModelAsset>& model, size_t lodIndex)
    {
        m_lastContractHash = AZ::HashValue64{ 0 };
        m_modelLod = nullptr;
        m_modelInstance = AZ::RPI::Model::FindOrCreate(model);
        if (!m_modelInstance)
        {
            AZ_Error(
                "ParticleModel", false,
                "Failed to create model instance for mesh particle. "
                "Ensure the model asset is processed and its gem is enabled. Asset hint: '%s'",
                model.GetHint().c_str());
            return;
        }
        if (lodIndex < m_modelInstance->GetLodCount())
        {
            m_modelLod = m_modelInstance->GetLods()[lodIndex];
            m_lodIndex = lodIndex;
        }
        else
        {
            m_modelLod = m_modelInstance->GetLods()[0];
            m_lodIndex = 0;
        }
    }

    size_t ParticleModel::GetMeshCount() const
    {
        return m_modelLod ? m_modelLod->GetMeshes().size() : 0;
    }

    AZ::Data::Instance<AZ::RPI::ModelLod> ParticleModel::GetModelLod() const
    {
        return m_modelLod;
    }

    bool ParticleModel::BuildInputStreamLayouts(const AZ::RPI::ShaderInputContract& contract)
    {
        if (!m_modelLod)
        {
            AZ_WarningOnce("ParticleModel", false,
                "BuildInputStreamLayouts called with null ModelLod. The mesh particle's model asset failed to load or instantiate.");
            return false;
        }

        const AZ::HashValue64 contractHash = contract.GetHash();
        if (contractHash == m_lastContractHash && !m_inputStreamLayouts.empty())
        {
            return true;
        }

        m_inputStreamLayouts.clear();
        m_uvStreamTangentBitmasks.clear();

        auto meshSize = GetMeshCount();
        m_inputStreamLayouts.resize(meshSize);
        m_uvStreamTangentBitmasks.resize(meshSize);
        m_streamBufferViews.resize(meshSize);

        for (size_t it = 0; it < meshSize; ++it)
        {
            AZ::RHI::InputStreamLayoutBuilder layoutBuilder;
            AZ::RPI::UvStreamTangentBitmask uvStreamTangentBitmaskOut;
            layoutBuilder.Begin();

            AZ::RPI::MaterialUvNameMap uvNameMap;
            const auto& meshMaterial = m_modelLod->GetMeshes()[it].m_material;
            if (meshMaterial)
            {
                auto matAsset = meshMaterial->GetAsset();
                if (matAsset && matAsset.IsReady() && matAsset->GetMaterialTypeAsset())
                {
                    uvNameMap = matAsset->GetMaterialTypeAsset()->GetUvNameMap();
                }
            }

            if (!GetStreamsForMesh(layoutBuilder, &uvStreamTangentBitmaskOut, contract, it, {}, uvNameMap))
            {
                AZ_WarningOnce("ParticleModel", false,
                    "GetStreamsForMesh failed for mesh index %zu. The shader's input contract is not satisfied by the mesh's vertex streams.",
                    it);
                return false;
            }
            m_inputStreamLayouts[it] = layoutBuilder.End();
            m_uvStreamTangentBitmasks[it] = uvStreamTangentBitmaskOut;
        }

        m_lastContractHash = contractHash;
        return true;
    }

    bool ParticleModel::IsInputStreamLayoutsValid(size_t meshIndex) const
    {
        return meshIndex < GetMeshCount() &&
            meshIndex < m_inputStreamLayouts.size() &&
            meshIndex < m_streamBufferViews.size() &&
            AZ::RHI::ValidateStreamBufferViews(m_inputStreamLayouts[meshIndex], m_streamBufferViews[meshIndex]);
    }

    const AZ::RHI::InputStreamLayout& ParticleModel::GetInputStreamLayout(size_t meshIndex) const
    {
        if (meshIndex >= m_inputStreamLayouts.size())
        {
            static AZ::RHI::InputStreamLayout s_emptyLayout;
            AZ_Error("ParticleModel", false, "meshIndex %zu is invalid for m_inputStreamLayouts (size: %zu).", meshIndex, m_inputStreamLayouts.size());
            return s_emptyLayout;
        }
        return m_inputStreamLayouts[meshIndex];
    }

    const AZ::RPI::UvStreamTangentBitmask& ParticleModel::GetUvStreamTangentBitmask(size_t meshIndex) const
    {
        AZ_Assert(meshIndex < m_uvStreamTangentBitmasks.size(), "meshIndex is invalid.");
        return m_uvStreamTangentBitmasks[meshIndex];
    }

    const AZ::RPI::ModelLod::StreamBufferViewList& ParticleModel::GetStreamBufferViewList(size_t meshIndex) const
    {
        if (meshIndex >= m_streamBufferViews.size())
        {
            static AZ::RPI::ModelLod::StreamBufferViewList s_emptyList;
            AZ_Error("ParticleModel", false, "meshIndex %zu is invalid for m_streamBufferViews (size: %zu).", meshIndex, m_streamBufferViews.size());
            return s_emptyList;
        }
        return m_streamBufferViews[meshIndex];
    }

    bool ParticleModel::GetStreamsForMesh(
        AZ::RHI::InputStreamLayoutBuilder& layoutBuilder,
        AZ::RPI::UvStreamTangentBitmask* uvStreamTangentBitmaskOut,
        const AZ::RPI::ShaderInputContract& contract,
        size_t meshIndex,
        const AZ::RPI::MaterialModelUvOverrideMap& materialModelUvMap,
        const AZ::RPI::MaterialUvNameMap& materialUvNameMap)
    {
        const auto& mesh = m_modelLod->GetMeshes()[meshIndex];
        m_streamBufferViews[meshIndex].clear();
        auto firstUv = FindFirstUvStreamFromMesh(meshIndex);
        auto defaultUv = FindDefaultUvStream(meshIndex, materialUvNameMap);
        bool result = true;

        if (uvStreamTangentBitmaskOut)
        {
            uvStreamTangentBitmaskOut->Reset();
        }

        for (auto& contractStreamChannel : contract.m_streamChannels)
        {
            auto iter = FindMatchingStream(
                meshIndex, materialModelUvMap, materialUvNameMap, contractStreamChannel, defaultUv, firstUv, uvStreamTangentBitmaskOut);

            if (iter == mesh.m_streamInfo.end())
            {
                AZ::RHI::Format dummyStreamFormat = AZ::RHI::Format::R8G8B8A8_UINT;
                layoutBuilder.AddBuffer()->Channel(contractStreamChannel.m_semantic, dummyStreamFormat);

                AZ::RHI::StreamBufferView dummyBuffer{ *mesh.GetIndexBufferView().GetBuffer(), 0, 0, 4 };
                m_streamBufferViews[meshIndex].push_back(dummyBuffer);
            }
            else
            {
                if (AZ::RHI::GetFormatComponentCount(iter->m_format) < contractStreamChannel.m_componentCount)
                {
                    if (contractStreamChannel.m_isOptional)
                    {
                        AZ::RHI::Format dummyStreamFormat = AZ::RHI::Format::R32G32B32A32_FLOAT;
                        layoutBuilder.AddBuffer()->Channel(contractStreamChannel.m_semantic, dummyStreamFormat);
                        AZ::RHI::StreamBufferView dummyBuffer{ *mesh.GetIndexBufferView().GetBuffer(), 0, 0, 4 };
                        m_streamBufferViews[meshIndex].push_back(dummyBuffer);
                    }
                    else
                    {
                        AZ_Error(
                            "Mesh", false, "Mesh format (%s) for stream '%s' provides %d components but the shader requires %d.",
                            AZ::RHI::ToString(iter->m_format), contractStreamChannel.m_semantic.ToString().c_str(),
                            AZ::RHI::GetFormatComponentCount(iter->m_format), contractStreamChannel.m_componentCount);
                        result = false;
                    }
                }
                else
                {
                    const size_t iterIndex = static_cast<size_t>(iter - mesh.m_streamInfo.begin());
                    const auto& meshStreamViews = mesh.GetStreamBufferViews();
                    if (iterIndex >= meshStreamViews.size())
                    {
                        result = false;
                        continue;
                    }
                    layoutBuilder.AddBuffer()->Channel(contractStreamChannel.m_semantic, iter->m_format);
                    m_streamBufferViews[meshIndex].push_back(meshStreamViews[iterIndex]);
                }
            }
        }
        return result;
    }


    AZ::RPI::ModelLod::StreamInfoList::const_iterator ParticleModel::FindMatchingStream(
        size_t meshIndex,
        const AZ::RPI::MaterialModelUvOverrideMap& materialModelUvMap,
        const AZ::RPI::MaterialUvNameMap& materialUvNameMap,
        const AZ::RPI::ShaderInputContract::StreamChannelInfo& contractStreamChannel,
        AZ::RPI::ModelLod::StreamInfoList::const_iterator defaultUv,
        AZ::RPI::ModelLod::StreamInfoList::const_iterator firstUv,
        AZ::RPI::UvStreamTangentBitmask* uvStreamTangentBitmaskOut)
    {
        const auto& mesh = m_modelLod->GetMeshes()[meshIndex];
        auto iter = mesh.m_streamInfo.end();
        auto materialUvIter = AZStd::find_if(
            materialUvNameMap.begin(), materialUvNameMap.end(),
            [&contractStreamChannel](const AZ::RPI::UvNamePair& uvNamePair)
            {
                return uvNamePair.m_shaderInput == contractStreamChannel.m_semantic;
            });
        const bool isUv = materialUvIter != materialUvNameMap.end();
        if (isUv)
        {
            const AZ::Name& materialUvName = materialUvIter->m_uvName;
            auto modelUvMapIter = materialModelUvMap.find(materialUvIter->m_shaderInput);
            if (modelUvMapIter != materialModelUvMap.end())
            {
                const AZ::Name& modelUvName = modelUvMapIter->second;
                if (!modelUvName.IsEmpty())
                {
                    iter = AZStd::find_if(
                        mesh.m_streamInfo.begin(), mesh.m_streamInfo.end(),
                        [&modelUvName](const AZ::RPI::ModelLod::StreamBufferInfo& info)
                        {
                            return info.m_customName == modelUvName ||
                                info.m_semantic.ToString() == modelUvName.GetStringView(); // For unnamed UVs, use the semantic instead.
                        });
                }
            }

            if (iter == mesh.m_streamInfo.end())
            {
                if (!materialUvName.IsEmpty())
                {
                    iter = AZStd::find_if(
                        mesh.m_streamInfo.begin(), mesh.m_streamInfo.end(),
                        [&materialUvName](const AZ::RPI::ModelLod::StreamBufferInfo& info)
                        {
                            return info.m_customName == materialUvName;
                        });
                }
            }
        }

        if (iter == mesh.m_streamInfo.end())
        {
            iter = AZStd::find_if(
                mesh.m_streamInfo.begin(), mesh.m_streamInfo.end(),
                [&contractStreamChannel](const AZ::RPI::ModelLod::StreamBufferInfo& info)
                {
                    return info.m_semantic == contractStreamChannel.m_semantic;
                });
        }

        if (iter == mesh.m_streamInfo.end() && isUv)
        {
            iter = defaultUv;
        }

        if (isUv && uvStreamTangentBitmaskOut)
        {
            uvStreamTangentBitmaskOut->ApplyTangent(iter == firstUv ? 0 : AZ::RPI::UvStreamTangentBitmask::UnassignedTangent);
        }

        return iter;
    }

    AZ::RPI::ModelLod::StreamInfoList::const_iterator ParticleModel::FindFirstUvStreamFromMesh(size_t meshIndex)
    {
        const auto& mesh = m_modelLod->GetMeshes()[meshIndex];

        auto firstUv = AZStd::find_if(
            mesh.m_streamInfo.begin(), mesh.m_streamInfo.end(),
            [](const AZ::RPI::ModelLod::StreamBufferInfo& info)
            {
                return info.m_semantic.m_name.GetStringView().starts_with(AZ::RHI::ShaderSemantic::UvStreamSemantic);
            });

        return firstUv;
    }

    AZ::RPI::ModelLod::StreamInfoList::const_iterator ParticleModel::FindDefaultUvStream(
        size_t meshIndex, const AZ::RPI::MaterialUvNameMap& materialUvNameMap)
    {
        const auto& mesh = m_modelLod->GetMeshes()[meshIndex];
        auto defaultUv = mesh.m_streamInfo.end();
        for (const auto& materialUvNamePair : materialUvNameMap)
        {
            const AZ::Name& uvCustomName = materialUvNamePair.m_uvName;
            const AZ::RHI::ShaderSemantic& shaderInput = materialUvNamePair.m_shaderInput;
            if (!uvCustomName.IsEmpty())
            {
                defaultUv = AZStd::find_if(
                    mesh.m_streamInfo.begin(), mesh.m_streamInfo.end(),
                    [&uvCustomName](const AZ::RPI::ModelLod::StreamBufferInfo& info)
                    {
                        return info.m_customName == uvCustomName;
                    });
            }

            if (defaultUv == mesh.m_streamInfo.end())
            {
                defaultUv = AZStd::find_if(
                    mesh.m_streamInfo.begin(), mesh.m_streamInfo.end(),
                    [&shaderInput](const AZ::RPI::ModelLod::StreamBufferInfo& info)
                    {
                        return info.m_semantic == shaderInput;
                    });
            }

            if (defaultUv != mesh.m_streamInfo.end())
            {
                break;
            }
        }

        return defaultUv;
    }
} // namespace OpenParticle
