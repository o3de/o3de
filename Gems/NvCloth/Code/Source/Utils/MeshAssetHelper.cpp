/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomLyIntegration/CommonFeatures/Mesh/MeshComponentBus.h>
#include <Atom/RPI.Reflect/Model/ModelAsset.h>

#include <NvCloth/ITangentSpaceHelper.h>

#include <Utils/MeshAssetHelper.h>

namespace NvCloth
{
    MeshAssetHelper::MeshAssetHelper(AZ::EntityId entityId)
        : AssetHelper(entityId)
    {
    }

    void MeshAssetHelper::GatherClothMeshNodes(MeshNodeList& meshNodes)
    {
        AZ::Data::Asset<AZ::RPI::ModelAsset> modelDataAsset;
        AZ::Render::MeshComponentRequestBus::EventResult(
            modelDataAsset, m_entityId, &AZ::Render::MeshComponentRequestBus::Events::GetModelAsset);
        if (!modelDataAsset.IsReady())
        {
            return;
        }

        const AZ::RPI::ModelAsset* modelAsset = modelDataAsset.Get();
        if (!modelAsset)
        {
            return;
        }

        const auto lodAssets = modelAsset->GetLodAssets();

        AZStd::set<AZStd::string> meshNodeNames;

        if (!lodAssets.empty())
        {
            const int lodLevel = 0;

            if (const AZ::RPI::ModelLodAsset* lodAsset = lodAssets[lodLevel].Get())
            {
                const auto meshes = lodAsset->GetMeshes();

                for (const auto& mesh : meshes)
                {
                    const bool hasClothData = (mesh.GetSemanticBufferAssetView(AZ::Name("CLOTH_DATA")) != nullptr);
                    if (hasClothData)
                    {
                        meshNodeNames.insert(mesh.GetName().GetStringView());
                    }
                }
            }
        }

        meshNodes.assign(meshNodeNames.begin(), meshNodeNames.end());
    }

    bool MeshAssetHelper::ObtainClothMeshNodeInfo(
        const AZStd::string& meshNode, 
        MeshNodeInfo& meshNodeInfo,
        MeshClothInfo& meshClothInfo)
    {
        AZ_PROFILE_FUNCTION(Cloth);

        AZ::Data::Asset<AZ::RPI::ModelAsset> modelDataAsset;
        AZ::Render::MeshComponentRequestBus::EventResult(
            modelDataAsset, m_entityId, &AZ::Render::MeshComponentRequestBus::Events::GetModelAsset);
        if (!modelDataAsset.IsReady())
        {
            return false;
        }

        const AZ::RPI::ModelAsset* modelAsset = modelDataAsset.Get();
        if (!modelAsset)
        {
            return false;
        }

        const auto lodAssets = modelAsset->GetLodAssets();

        AZStd::vector<const AZ::RPI::ModelLodAsset::Mesh*> meshNodes;
        AZStd::vector<size_t> meshPrimitiveIndices;

        if(!lodAssets.empty())
        {
            const int lodLevel = 0;

            if (const AZ::RPI::ModelLodAsset* lodAsset = lodAssets[lodLevel].Get())
            {
                const auto meshes = lodAsset->GetMeshes();

                for (size_t meshIndex = 0; meshIndex < meshes.size(); ++meshIndex)
                {
                    if (meshNode == meshes[meshIndex].GetName().GetStringView())
                    {
                        meshNodes.push_back(&meshes[meshIndex]);
                        meshPrimitiveIndices.push_back(meshIndex);
                        meshNodeInfo.m_lodLevel = lodLevel;
                    }
                }
            }
        }

        bool infoObtained = false;

        if (!meshNodes.empty())
        {
            bool dataCopied = CopyDataFromMeshes(meshNodes, meshClothInfo);

            if (dataCopied)
            {
                const size_t numSubMeshes = meshNodes.size();

                meshNodeInfo.m_subMeshes.reserve(meshNodes.size());

                int firstVertex = 0;
                int firstIndex = 0;
                for (size_t subMeshIndex = 0; subMeshIndex < numSubMeshes; ++subMeshIndex)
                {
                    MeshNodeInfo::SubMesh subMesh;
                    subMesh.m_primitiveIndex = static_cast<int>(meshPrimitiveIndices[subMeshIndex]);
                    subMesh.m_verticesFirstIndex = firstVertex;
                    subMesh.m_numVertices = static_cast<int>(meshNodes[subMeshIndex]->GetVertexCount());
                    subMesh.m_indicesFirstIndex = firstIndex;
                    subMesh.m_numIndices = static_cast<int>(meshNodes[subMeshIndex]->GetIndexCount());

                    firstVertex += subMesh.m_numVertices;
                    firstIndex += subMesh.m_numIndices;

                    meshNodeInfo.m_subMeshes.push_back(AZStd::move(subMesh));
                }

                infoObtained = true;
            }
            else
            {
                AZ_Error("MeshAssetHelper", false, "Failed to extract data from node %s in model %s",
                    meshNode.c_str(), modelDataAsset.GetHint().c_str());
            }
        }

        return infoObtained;
    }

    bool MeshAssetHelper::CopyDataFromMeshes(
        const AZStd::vector<const AZ::RPI::ModelLodAsset::Mesh*>& meshes,
        MeshClothInfo& meshClothInfo)
    {
        uint32_t numTotalVertices = 0;
        uint32_t numTotalIndices = 0;
        for (const auto* mesh : meshes)
        {
            numTotalVertices += mesh->GetVertexCount();
            numTotalIndices += mesh->GetIndexCount();
        }
        if (numTotalVertices == 0 || numTotalIndices == 0)
        {
            return false;
        }

        meshClothInfo.m_particles.reserve(numTotalVertices);
        meshClothInfo.m_uvs.reserve(numTotalVertices);
        meshClothInfo.m_motionConstraints.reserve(numTotalVertices);
        meshClothInfo.m_backstopData.reserve(numTotalVertices);
        meshClothInfo.m_normals.reserve(numTotalVertices);
        meshClothInfo.m_indices.reserve(numTotalIndices);

        struct Vec2
        {
            float x, y;
        };
        struct Vec3
        {
            float x, y, z;
        };
        struct Vec4
        {
            float x, y, z, w;
        };
        const SimUVType uvZero(0.0f, 0.0f);

        for (const auto* mesh : meshes)
        {
            const auto sourceIndices = mesh->GetIndexBufferTyped<uint32_t>();
            const auto sourcePositions = mesh->GetSemanticBufferTyped<Vec3>(AZ::Name("POSITION"));
            const auto sourceNormals = mesh->GetSemanticBufferTyped<Vec3>(AZ::Name("NORMAL"));
            const auto sourceClothData = mesh->GetSemanticBufferTyped<Vec4>(AZ::Name("CLOTH_DATA"));
            const auto sourceUVs = mesh->GetSemanticBufferTyped<Vec2>(AZ::Name("UV"));

            if (sourceIndices.empty() ||
                sourcePositions.empty() ||
                sourceNormals.empty() ||
                sourceClothData.empty() ||
                sourceUVs.empty())
            {
                return false;
            }

            const uint32_t numVertices = mesh->GetVertexCount();
            for (uint32_t index = 0; index < numVertices; ++index)
            {
                const float inverseMass = sourceClothData[index].x;
                const float motionConstraint = sourceClothData[index].y;
                const float backstopOffset = ConvertBackstopOffset(sourceClothData[index].z);
                const float backstopRadius = sourceClothData[index].w;

                meshClothInfo.m_particles.emplace_back(
                    sourcePositions[index].x,
                    sourcePositions[index].y,
                    sourcePositions[index].z,
                    inverseMass);

                meshClothInfo.m_normals.emplace_back(
                    sourceNormals[index].x,
                    sourceNormals[index].y,
                    sourceNormals[index].z);

                meshClothInfo.m_motionConstraints.emplace_back(motionConstraint);
                meshClothInfo.m_backstopData.emplace_back(backstopOffset, backstopRadius);

                meshClothInfo.m_uvs.emplace_back(
                    sourceUVs.empty()
                    ? uvZero
                    : SimUVType(sourceUVs[index].x, sourceUVs[index].y));
            }

            meshClothInfo.m_indices.insert(meshClothInfo.m_indices.end(), sourceIndices.begin(), sourceIndices.end());
        }

        // Calculate tangents and bitangents for the whole mesh
        [[maybe_unused]] bool tangentsAndBitangentsCalculated =
            AZ::Interface<ITangentSpaceHelper>::Get()->CalculateTangentsAndBitagents(
                meshClothInfo.m_particles, meshClothInfo.m_indices, meshClothInfo.m_uvs, meshClothInfo.m_normals,
                meshClothInfo.m_tangents, meshClothInfo.m_bitangents);
        AZ_Assert(tangentsAndBitangentsCalculated, "Failed to calculate tangents and bitangents.");

        return true;
    }
} // namespace NvCloth
