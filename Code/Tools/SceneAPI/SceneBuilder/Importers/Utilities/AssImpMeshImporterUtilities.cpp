/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <assimp/mesh.h>
#include <assimp/scene.h>
#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/std/numeric.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <SceneAPI/SceneBuilder/SceneSystem.h>
#include <SceneAPI/SceneBuilder/ImportContexts/AssImpImportContexts.h>
#include <SceneAPI/SceneBuilder/Importers/Utilities/AssImpMeshImporterUtilities.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>
#include <SceneAPI/SceneData/GraphData/BlendShapeData.h>
#include <SceneAPI/SceneData/GraphData/BoneData.h>
#include <SceneAPI/SceneData/GraphData/MeshData.h>

namespace AZ::SceneAPI::SceneBuilder
{
    bool BuildSceneMeshFromAssImpMesh(const aiNode* currentNode, const aiScene* scene, const SceneSystem& sceneSystem, AZStd::vector<AZStd::shared_ptr<DataTypes::IGraphObject>>& meshes,
        const AZStd::function<AZStd::shared_ptr<SceneData::GraphData::MeshData>()>& makeMeshFunc)
    {
        AZStd::unordered_map<int, int> assImpMatIndexToLYIndex;
        int lyMeshIndex = 0;

        if(!currentNode || !scene)
        {
            return false;
        }
        auto newMesh = makeMeshFunc();

        newMesh->SetUnitSizeInMeters(sceneSystem.GetUnitSizeInMeters());
        newMesh->SetOriginalUnitSizeInMeters(sceneSystem.GetOriginalUnitSizeInMeters());

        // AssImp separates meshes that have multiple materials.
        // This code re-combines them to match previous FBX SDK behavior,
        // so they can be separated by engine code instead.
        int vertOffset = 0;
        for (unsigned int m = 0; m < currentNode->mNumMeshes; ++m)
        {
            const aiMesh* mesh = scene->mMeshes[currentNode->mMeshes[m]];

            // Lumberyard materials are created in order based on mesh references in the scene
            if (assImpMatIndexToLYIndex.find(mesh->mMaterialIndex) == assImpMatIndexToLYIndex.end())
            {
                assImpMatIndexToLYIndex.insert(AZStd::pair<int, int>(mesh->mMaterialIndex, lyMeshIndex++));
            }

            for (unsigned int vertIdx = 0; vertIdx < mesh->mNumVertices; ++vertIdx)
            {
                AZ::Vector3 vertex(mesh->mVertices[vertIdx].x, mesh->mVertices[vertIdx].y, mesh->mVertices[vertIdx].z);

                sceneSystem.SwapVec3ForUpAxis(vertex);
                sceneSystem.ConvertUnit(vertex);
                newMesh->AddPosition(vertex);
                newMesh->SetVertexIndexToControlPointIndexMap(vertIdx + vertOffset, vertIdx + vertOffset);

                if (mesh->HasNormals())
                {
                    AZ::Vector3 normal(mesh->mNormals[vertIdx].x, mesh->mNormals[vertIdx].y, mesh->mNormals[vertIdx].z);
                    sceneSystem.SwapVec3ForUpAxis(normal);
                    normal.NormalizeSafe();
                    newMesh->AddNormal(normal);
                }
            }

            for (unsigned int faceIdx = 0; faceIdx < mesh->mNumFaces; ++faceIdx)
            {
                aiFace face = mesh->mFaces[faceIdx];
                AZ::SceneAPI::DataTypes::IMeshData::Face meshFace;
                if (face.mNumIndices > 3)
                {
                    // AssImp should have triangulated everything, so if this happens then someone has
                    // probably changed AssImp's import settings. The engine only supports triangles.
                    AZ_Error(Utilities::ErrorWindow, false,
                        "Mesh on node %s has a face with %d vertices, only 3 vertices are supported per face. You could "
                        "fix it by triangulating the meshes in the dcc tool.",
                        currentNode->mName.C_Str(),
                        face.mNumIndices);
                    continue;
                }
                for (unsigned int idx = 0; idx < face.mNumIndices; ++idx)
                {
                    meshFace.vertexIndex[idx] = face.mIndices[idx] + vertOffset;
                }

                newMesh->AddFace(meshFace, assImpMatIndexToLYIndex[mesh->mMaterialIndex]);
            }
            vertOffset += mesh->mNumVertices;

        }
        meshes.push_back(newMesh);

        return true;
    }

    GetMeshDataFromParentResult GetMeshDataFromParent(AssImpSceneNodeAppendedContext& context)
    {
        const DataTypes::IGraphObject* const parentData =
            context.m_scene.GetGraph().GetNodeContent(context.m_currentGraphPosition).get();

        if (!parentData)
        {
            AZ_Error(Utilities::ErrorWindow, false,
                "GetMeshDataFromParent failed because the parent was null, it should only be called with a valid parent node");
            return AZ::Failure(Events::ProcessingResult::Failure);
        }

        if (!parentData->RTTI_IsTypeOf(SceneData::GraphData::MeshData::TYPEINFO_Uuid()))
        {
            // The parent node may contain bone information and not mesh information, skip it.
            if (parentData->RTTI_IsTypeOf(SceneData::GraphData::BoneData::TYPEINFO_Uuid()))
            {
                // Return the ignore processing result in the failure.
                return AZ::Failure(Events::ProcessingResult::Ignored);
            }
            AZ_Error(Utilities::ErrorWindow, false,
                "Tried to get mesh data from parent for non-mesh parent data");
            return AZ::Failure(Events::ProcessingResult::Failure);
        }

        const SceneData::GraphData::MeshData* const parentMeshData =
            azrtti_cast<const SceneData::GraphData::MeshData* const>(parentData);
        return AZ::Success(parentMeshData);
    }

    uint64_t GetVertexCountForAllMeshesOnNode(const aiNode& node, const aiScene& scene)
    {
        return AZStd::accumulate(node.mMeshes, node.mMeshes + node.mNumMeshes, uint64_t{ 0u },
            [&scene](auto runningTotal, unsigned int meshIndex)
            {
                return runningTotal + scene.mMeshes[meshIndex]->mNumVertices;
            });
    }
}
