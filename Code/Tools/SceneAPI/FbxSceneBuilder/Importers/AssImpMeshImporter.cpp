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
#include <SceneAPI/FbxSceneBuilder/Importers/AssImpMeshImporter.h>
#include <SceneAPI/SDKWrapper/AssImpNodeWrapper.h>
#include <SceneAPI/SDKWrapper/AssImpSceneWrapper.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzToolsFramework/Debug/TraceContext.h>
#include <SceneAPI/FbxSceneBuilder/FbxSceneSystem.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>
#include <SceneAPI/SceneData/GraphData/MeshData.h>

#include <assimp/scene.h>
#include <assimp/mesh.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace FbxSceneBuilder
        {
            AssImpMeshImporter::AssImpMeshImporter()
            {
                BindToCall(&AssImpMeshImporter::ImportMesh);
            }

            void AssImpMeshImporter::Reflect(ReflectContext* context)
            {
                SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context);
                if (serializeContext)
                {
                    serializeContext->Class<AssImpMeshImporter, SceneCore::LoadingComponent>()->Version(1);
                }
            }

            Events::ProcessingResult AssImpMeshImporter::ImportMesh(AssImpNodeEncounteredContext& context)
            {
                AZ_TraceContext("Importer", "Mesh");
                if (!context.m_sourceNode.ContainsMesh())
                {
                    return Events::ProcessingResult::Ignored;
                }

                AZStd::unordered_map<int, int> assImpMatIndexToLYIndex;
                int lyMeshIndex = 0;

                aiNode* currentNode = context.m_sourceNode.GetAssImpNode();
                const aiScene* scene = context.m_sourceScene.GetAssImpScene();

                for (int m = 0; m < currentNode->mNumMeshes; ++m)
                {
                    AZStd::shared_ptr<SceneData::GraphData::MeshData> newMesh =
                        AZStd::make_shared<SceneData::GraphData::MeshData>();

                    newMesh->SetUnitSizeInMeters(context.m_sourceSceneSystem.GetUnitSizeInMeters());
                    newMesh->SetOriginalUnitSizeInMeters(context.m_sourceSceneSystem.GetOriginalUnitSizeInMeters());

                    newMesh->SetSdkMeshIndex(m);

                    aiMesh* mesh = scene->mMeshes[currentNode->mMeshes[m]];

                    // Lumberyard materials are created in order based on mesh references in the scene
                    if (assImpMatIndexToLYIndex.find(mesh->mMaterialIndex) == assImpMatIndexToLYIndex.end())
                    {
                        assImpMatIndexToLYIndex.insert(AZStd::pair<int, int>(mesh->mMaterialIndex, lyMeshIndex++));
                    }

                    for (int vertIdx = 0; vertIdx < mesh->mNumVertices; ++vertIdx)
                    {
                        AZ::Vector3 vertex(
                            mesh->mVertices[vertIdx].x,
                            mesh->mVertices[vertIdx].y,
                            mesh->mVertices[vertIdx].z);

                        context.m_sourceSceneSystem.SwapVec3ForUpAxis(vertex);
                        context.m_sourceSceneSystem.ConvertUnit(vertex);
                        newMesh->AddPosition(vertex);

                        if (mesh->HasNormals())
                        {
                            AZ::Vector3 normal(
                                mesh->mNormals[vertIdx].x,
                                mesh->mNormals[vertIdx].y,
                                mesh->mNormals[vertIdx].z);
                            context.m_sourceSceneSystem.SwapVec3ForUpAxis(normal);
                            normal.NormalizeSafe();
                            newMesh->AddNormal(normal);
                        }
                    }

                    for (int faceIdx = 0; faceIdx < mesh->mNumFaces; ++faceIdx)
                    {
                        aiFace face = mesh->mFaces[faceIdx];
                        AZ::SceneAPI::DataTypes::IMeshData::Face meshFace;
                        if (face.mNumIndices != 3)
                        {
                            // AssImp should have triangulated everything, so if this happens then someone has
                            // probably changed AssImp's import settings. The engine only supports triangles.
                            AZ_Error(Utilities::ErrorWindow, false, "Mesh has a face with %d vertices, only 3 vertices are supported per face.");
                            continue;
                        }
                        for (int idx = 0; idx < face.mNumIndices; ++idx)
                        {
                            meshFace.vertexIndex[idx] = face.mIndices[idx];
                        }

                        newMesh->AddFace(meshFace, assImpMatIndexToLYIndex[mesh->mMaterialIndex]);
                    }

                    context.m_createdData.push_back(std::move(newMesh));
                }
                return Events::ProcessingResult::Success;
            }
        } // namespace FbxSceneBuilder
    } // namespace SceneAPI
} // namespace AZ
