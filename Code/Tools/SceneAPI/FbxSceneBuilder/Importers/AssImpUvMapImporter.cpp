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

#include <AzCore/Math/Vector2.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/std/numeric.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzToolsFramework/Debug/TraceContext.h>
#include <SceneAPI/FbxSceneBuilder/ImportContexts/AssImpImportContexts.h>
#include <SceneAPI/FbxSceneBuilder/Importers/AssImpUvMapImporter.h>
#include <SceneAPI/FbxSceneBuilder/Importers/ImporterUtilities.h>
#include <SceneAPI/FbxSceneBuilder/Importers/Utilities/AssImpMeshImporterUtilities.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>
#include <SceneAPI/SceneData/GraphData/MeshData.h>
#include <SceneAPI/SceneData/GraphData/MeshVertexUVData.h>
#include <SceneAPI/SDKWrapper/AssImpNodeWrapper.h>
#include <SceneAPI/SDKWrapper/AssImpSceneWrapper.h>

#include <assimp/scene.h>
#include <assimp/mesh.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace FbxSceneBuilder
        {
            const char* AssImpUvMapImporter::m_defaultNodeName = "UV";

            AssImpUvMapImporter::AssImpUvMapImporter()
            {
                BindToCall(&AssImpUvMapImporter::ImportUvMaps);
            }

            void AssImpUvMapImporter::Reflect(ReflectContext* context)
            {
                SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context);
                if (serializeContext)
                {
                    serializeContext->Class<AssImpUvMapImporter, SceneCore::LoadingComponent>()->Version(4); // LYN-3250
                }
            }

            Events::ProcessingResult AssImpUvMapImporter::ImportUvMaps(AssImpSceneNodeAppendedContext& context)
            {
                AZ_TraceContext("Importer", m_defaultNodeName);
                if (!context.m_sourceNode.ContainsMesh())
                {
                    return Events::ProcessingResult::Ignored;
                }
                const aiNode* currentNode = context.m_sourceNode.GetAssImpNode();
                const aiScene* scene = context.m_sourceScene.GetAssImpScene();

                // AssImp separates meshes that have multiple materials.
                // This code re-combines them to match previous FBX SDK behavior,
                // so they can be separated by engine code instead.
                bool foundTextureCoordinates = false;
                AZStd::array<int, AI_MAX_NUMBER_OF_TEXTURECOORDS> meshesPerTextureCoordinateIndex = {};
                for (int localMeshIndex = 0; localMeshIndex < currentNode->mNumMeshes; ++localMeshIndex)
                {
                    aiMesh* mesh = scene->mMeshes[currentNode->mMeshes[localMeshIndex]];
                    for (int texCoordIndex = 0; texCoordIndex < meshesPerTextureCoordinateIndex.size(); ++texCoordIndex)
                    {
                        if (!mesh->mTextureCoords[texCoordIndex])
                        {
                            continue;
                        }
                        ++meshesPerTextureCoordinateIndex[texCoordIndex];
                        foundTextureCoordinates = true;
                    }
                }

                if (!foundTextureCoordinates)
                {
                    return Events::ProcessingResult::Ignored;
                }

                const uint64_t vertexCount = GetVertexCountForAllMeshesOnNode(*currentNode, *scene);

                for (int texCoordIndex = 0; texCoordIndex < meshesPerTextureCoordinateIndex.size(); ++texCoordIndex)
                {
                    int meshesWithIndex = meshesPerTextureCoordinateIndex[texCoordIndex];
                    AZ_Error(
                        Utilities::ErrorWindow,
                        meshesWithIndex == 0 || meshesWithIndex == currentNode->mNumMeshes,
                        "Texture coordinate index %d for node %s is not on all meshes on this node. "
                        "Placeholder arbitrary texture values will be generated to allow the data to process, but the source art "
                        "needs to be fixed to correct this. All meshes on this node should have the same number of texture coordinate channels.",
                        texCoordIndex,
                        currentNode->mName.C_Str());
                }

                Events::ProcessingResultCombiner combinedUvMapResults;
                for (int texCoordIndex = 0; texCoordIndex < meshesPerTextureCoordinateIndex.size(); ++texCoordIndex)
                {
                    // No meshes have this texture coordinate index, skip it.
                    if (meshesPerTextureCoordinateIndex[texCoordIndex] == 0)
                    {
                        continue;
                    }

                    AZStd::shared_ptr<SceneData::GraphData::MeshVertexUVData> uvMap =
                        AZStd::make_shared<AZ::SceneData::GraphData::MeshVertexUVData>();
                    uvMap->ReserveContainerSpace(vertexCount);
                    bool customNameFound = false;
                    AZStd::string name(AZStd::string::format("%s%d", m_defaultNodeName, texCoordIndex));
                    for (int sdkMeshIndex = 0; sdkMeshIndex < currentNode->mNumMeshes; ++sdkMeshIndex)
                    {
                        const aiMesh* mesh = scene->mMeshes[currentNode->mMeshes[sdkMeshIndex]];
                        if(mesh->mTextureCoords[texCoordIndex])
                        {
                            if (mesh->mTextureCoordsNames[texCoordIndex].length > 0)
                            {
                                if (!customNameFound)
                                {
                                    name = mesh->mTextureCoordsNames[texCoordIndex].C_Str();
                                    customNameFound = true;
                                }
                                else
                                {
                                    AZ_Warning(Utilities::WarningWindow,
                                        strcmp(name.c_str(), mesh->mTextureCoordsNames[texCoordIndex].C_Str()) == 0,
                                        "Node %s has conflicting mesh coordinate names at index %d, %s and %s. Using %s.",
                                        currentNode->mName.C_Str(),
                                        texCoordIndex,
                                        name.c_str(),
                                        mesh->mTextureCoordsNames[texCoordIndex].C_Str(),
                                        name.c_str());
                                }
                            }
                        }

                        for (int v = 0; v < mesh->mNumVertices; ++v)
                        {
                            if (mesh->mTextureCoords[texCoordIndex])
                            {
                                AZ::Vector2 vertexUV(
                                    mesh->mTextureCoords[texCoordIndex][v].x,
                                    // The engine's V coordinate is reverse of how it's stored in the FBX file.
                                    1.0f - mesh->mTextureCoords[texCoordIndex][v].y);
                                uvMap->AppendUV(vertexUV);
                            }
                            else
                            {
                                // An error was already emitted if the UV channels for all meshes on this node do not match.
                                // Append an arbitrary UV value so that the mesh can still be processed.
                                // It's better to let the engine load a partially valid mesh than to completely fail.
                                uvMap->AppendUV(AZ::Vector2::CreateZero());
                            }
                        }
                    }

                    uvMap->SetCustomName(name.c_str());
                    Containers::SceneGraph::NodeIndex newIndex =
                        context.m_scene.GetGraph().AddChild(context.m_currentGraphPosition, name.c_str());

                    Events::ProcessingResult uvMapResults;
                    AssImpSceneAttributeDataPopulatedContext dataPopulated(context, uvMap, newIndex, name);
                    uvMapResults = Events::Process(dataPopulated);

                    if (uvMapResults != Events::ProcessingResult::Failure)
                    {
                        uvMapResults = AddAttributeDataNodeWithContexts(dataPopulated);
                    }

                    combinedUvMapResults += uvMapResults;

                }

                return combinedUvMapResults.GetResult();
            }

        } // namespace FbxSceneBuilder
    } // namespace SceneAPI
} // namespace AZ
