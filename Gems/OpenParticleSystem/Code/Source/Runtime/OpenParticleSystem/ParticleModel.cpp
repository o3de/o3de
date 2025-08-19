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
        m_modelInstance = AZ::RPI::Model::FindOrCreate(model);
        AZ_Assert(m_modelInstance, "model instance is nullptr");
        if (lodIndex < m_modelInstance->GetLodCount())
        {
            m_modelLod = m_modelInstance->GetLods()[lodIndex];
            m_lodIndex = lodIndex;
        }
        else
        {
            m_modelLod = m_modelInstance->GetLods()[0];
        }
    }

    size_t ParticleModel::GetMeshCount() const
    {
        return m_modelLod->GetMeshes().size();
    }

    AZ::Data::Instance<AZ::RPI::ModelLod> ParticleModel::GetModelLod() const
    {
        return m_modelLod;
    }

    bool ParticleModel::BuildInputStreamLayouts(const AZ::RPI::ShaderInputContract& contract)
    {
        m_inputStreamLayouts.clear();
        m_buffers.clear();

        auto meshSize = GetMeshCount();
        m_streamBufferViews.resize(meshSize);
        if (!SetMeshStreamBuffers())
        {
            return false;
        }

        for (auto it = 0; it < meshSize; ++it)
        {
            AZ::RHI::InputStreamLayoutBuilder layoutBuilder;
            AZ::RPI::UvStreamTangentBitmask uvStreamTangentBitmaskOut;
            layoutBuilder.Begin();
            if (!GetStreamsForMesh(
                    layoutBuilder, &uvStreamTangentBitmaskOut, contract, it, {},
                    m_modelLod->GetMeshes()[it].m_material->GetAsset()->GetMaterialTypeAsset()->GetUvNameMap()))
            {
                continue;
            }
            layoutBuilder.AddBuffer(AZ::RHI::StreamStepFunction::PerInstance)
                ->Channel("OFFSET", AZ::RHI::Format::R32G32B32A32_FLOAT)
                ->Channel("COLOR", AZ::RHI::Format::R32G32B32A32_FLOAT)
                ->Channel("INITROTATION", AZ::RHI::Format::R32G32B32A32_FLOAT)
                ->Channel("ROTATEVECTOR", AZ::RHI::Format::R32G32B32A32_FLOAT)
                ->Channel("SCALE", AZ::RHI::Format::R32G32B32A32_FLOAT);
            m_inputStreamLayouts.emplace_back(layoutBuilder.End());
        }

        return true;
    }

    void ParticleModel::SetParticleStreamBufferView(const AZ::RHI::StreamBufferView& streamBufferView, size_t meshIndex)
    {
        if (meshIndex < GetMeshCount())
        {
            m_streamBufferViews[meshIndex].emplace_back(streamBufferView);
        }
    }

    bool ParticleModel::IsInputStreamLayoutsValid(size_t meshIndex) const
    {
        return meshIndex < GetMeshCount() &&
            AZ::RHI::ValidateStreamBufferViews(m_inputStreamLayouts[meshIndex], m_streamBufferViews[meshIndex]);
    }

    const AZ::RHI::InputStreamLayout& ParticleModel::GetInputStreamLayout(size_t meshIndex) const
    {
        AZ_Assert(meshIndex < m_inputStreamLayouts.size(), "meshIndex is invalid.");
        return m_inputStreamLayouts[meshIndex];
    }

    const AZ::RPI::ModelLod::StreamBufferViewList& ParticleModel::GetStreamBufferViewList(size_t meshIndex) const
    {
        AZ_Assert(meshIndex < m_streamBufferViews.size(), "meshIndex is invalid.");
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
        if (!SetMeshStreamBuffers())
        {
            return false;
        }
        const AZ::RPI::ShaderInputContract meshContract = CheckMeshContract(contract);
        auto firstUv = FindFirstUvStreamFromMesh(meshIndex);
        auto defaultUv = FindDefaultUvStream(meshIndex, materialUvNameMap);
        bool result = true;

        if (uvStreamTangentBitmaskOut)
        {
            uvStreamTangentBitmaskOut->Reset();
        }

        for (auto& contractStreamChannel : meshContract.m_streamChannels)
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
                    AZ_Error(
                        "Mesh", false, "Mesh format (%s) for stream '%s' provides %d components but the shader requires %d.",
                        AZ::RHI::ToString(iter->m_format), contractStreamChannel.m_semantic.ToString().c_str(),
                        AZ::RHI::GetFormatComponentCount(iter->m_format), contractStreamChannel.m_componentCount);
                    result = false;
                }
                else
                {
                    if (iter->m_bufferIndex >= m_buffers.size())
                    {
                        result = false;
                        continue;
                    }
                    auto descriptor = m_buffers[iter->m_bufferIndex]->GetBufferViewDescriptor();
                    AZ::u32 byteOffset = descriptor.m_elementOffset * descriptor.m_elementSize;
                    AZ::u32 byteCount = descriptor.m_elementCount * descriptor.m_elementSize;
                    AZ::u32 byteStride = descriptor.m_elementSize;
                    layoutBuilder.AddBuffer()->Channel(contractStreamChannel.m_semantic, iter->m_format);
                    AZ::RHI::StreamBufferView bufferView(
                        *m_buffers[iter->m_bufferIndex]->GetRHIBuffer(), byteOffset, byteCount, byteStride);

                    m_streamBufferViews[meshIndex].push_back(bufferView);
                }
            }
        }
        return result;
    }

    AZ::RPI::ShaderInputContract ParticleModel::CheckMeshContract(const AZ::RPI::ShaderInputContract& contract)
    {
        const AZStd::set<AZStd::string> ParticleSemantic = { "OFFSET", "COLOR", "INITROTATION", "ROTATEVECTOR", "SCALE" };

        AZ::RPI::ShaderInputContract meshContract(contract);

        for (const auto& semantic : ParticleSemantic)
        {
            auto it = AZStd::find_if(
                meshContract.m_streamChannels.cbegin(), meshContract.m_streamChannels.cend(),
                [&semantic](const AZ::RPI::ShaderInputContract::StreamChannelInfo& channel)
                {
                    return channel.m_semantic.m_name == AZ::Name(semantic);
                });
            if (it != meshContract.m_streamChannels.cend())
            {
                meshContract.m_streamChannels.erase(it);
            }
        }

        return meshContract;
    }

    bool ParticleModel::SetMeshStreamBuffers()
    {
        for (const auto& mesh : m_modelInstance->GetModelAsset()->GetLodAssets()[m_lodIndex]->GetMeshes())
        {
            const AZ::RPI::BufferAssetView& indexBufferAssetView = mesh.GetIndexBufferAssetView();
            const AZ::Data::Asset<AZ::RPI::BufferAsset>& indexBufferAsset = indexBufferAssetView.GetBufferAsset();
            if (indexBufferAsset)
            {
                AZ::Data::Instance<AZ::RPI::Buffer> indexBuffer = AZ::RPI::Buffer::FindOrCreate(indexBufferAsset);
                if (!indexBuffer)
                {
                    return false;
                }
                m_buffers.emplace_back(indexBuffer);
            }
            for (const auto& streamBufferInfo : mesh.GetStreamBufferInfoList())
            {
                const AZ::Data::Asset<AZ::RPI::BufferAsset>& streamBufferAsset = streamBufferInfo.m_bufferAssetView.GetBufferAsset();
                const AZ::Data::Instance<AZ::RPI::Buffer>& streamBuffer = AZ::RPI::Buffer::FindOrCreate(streamBufferAsset);
                if (streamBuffer == nullptr)
                {
                    return false;
                }
                m_buffers.emplace_back(streamBuffer);
            }
        }
        return true;
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
