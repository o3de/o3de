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
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzToolsFramework/Debug/TraceContext.h>
#include <SceneAPI/FbxSceneBuilder/ImportContexts/AssImpImportContexts.h>
#include <SceneAPI/FbxSceneBuilder/Importers/AssImpUvMapImporter.h>
#include <SceneAPI/FbxSceneBuilder/Importers/FbxImporterUtilities.h>
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
                    serializeContext->Class<AssImpUvMapImporter, SceneCore::LoadingComponent>()->Version(3); // LYN-2506
                }
            }

            Events::ProcessingResult AssImpUvMapImporter::ImportUvMaps(AssImpSceneNodeAppendedContext& context)
            {
                AZ_TraceContext("Importer", m_defaultNodeName);
                if (!context.m_sourceNode.ContainsMesh())
                {
                    return Events::ProcessingResult::Ignored;
                }
                aiNode* currentNode = context.m_sourceNode.GetAssImpNode();
                const aiScene* scene = context.m_sourceScene.GetAssImpScene();

                // AssImp separates meshes that have multiple materials.
                // This code re-combines them to match previous FBX SDK behavior,
                // so they can be separated by engine code instead.
                int vertexCount = 0;
                int expectedUvCount = -1;
                for (int sdkMeshIndex = 0; sdkMeshIndex < currentNode->mNumMeshes; ++sdkMeshIndex)
                {
                    aiMesh* mesh = scene->mMeshes[currentNode->mMeshes[sdkMeshIndex]];
                    int texCoordCount = 0;
                    for (int texCoordIndex = 0; texCoordIndex < AI_MAX_NUMBER_OF_TEXTURECOORDS; ++texCoordIndex)
                    {
                        if (!mesh->mTextureCoords[texCoordIndex])
                        {
                            continue;
                        }
                        ++texCoordCount;
                    }

                    if (expectedUvCount < 0)
                    {
                        expectedUvCount = texCoordCount;
                    }
                    else if (expectedUvCount != texCoordCount)
                    {
                        AZ_Error(
                            Utilities::ErrorWindow,
                            false,
                            "Texture coordinate count %d for node %s, for mesh %s at index %d does not match expected count %d",
                            texCoordCount,
                            currentNode->mName.C_Str(),
                            mesh->mName.C_Str(),
                            sdkMeshIndex,
                            expectedUvCount);
                        return Events::ProcessingResult::Failure;
                    }
                    vertexCount += mesh->mNumVertices * texCoordCount;
                }
                if (vertexCount == 0)
                {
                    return Events::ProcessingResult::Ignored;
                }
                Events::ProcessingResultCombiner combinedUvMapResults;
                for (int texCoordIndex = 0; texCoordIndex < AI_MAX_NUMBER_OF_TEXTURECOORDS; ++texCoordIndex)
                {
                    AZStd::shared_ptr<SceneData::GraphData::MeshVertexUVData> uvMap =
                        AZStd::make_shared<AZ::SceneData::GraphData::MeshVertexUVData>();
                    uvMap->ReserveContainerSpace(vertexCount);
                    bool hasTexCoords = false;
                    AZStd::string name(AZStd::string::format("%s%d", m_defaultNodeName, texCoordIndex));
                    for (int sdkMeshIndex = 0; sdkMeshIndex < currentNode->mNumMeshes; ++sdkMeshIndex)
                    {
                        aiMesh* mesh = scene->mMeshes[currentNode->mMeshes[sdkMeshIndex]];
                        if (!mesh->mTextureCoords[texCoordIndex])
                        {
                            continue;
                        }
                        if (!hasTexCoords)
                        {
                            if (mesh->mTextureCoordsNames[texCoordIndex].length)
                            {
                                name = mesh->mTextureCoordsNames[texCoordIndex].C_Str();
                            }
                            uvMap->SetCustomName(name.c_str());
                            hasTexCoords = true;
                        }
                        else if(mesh->mTextureCoordsNames[texCoordIndex].length)
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

                        for (int v = 0; v < mesh->mNumVertices; ++v)
                        {
                            AZ::Vector2 vertexUV(
                                mesh->mTextureCoords[texCoordIndex][v].x,
                                // The engine's V coordinate is reverse of how it's stored in the FBX file.
                                1.0f - mesh->mTextureCoords[texCoordIndex][v].y);
                            uvMap->AppendUV(vertexUV);
                        }
                    }
                    if (hasTexCoords)
                    {
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

                }

                return combinedUvMapResults.GetResult();
            }

        } // namespace FbxSceneBuilder
    } // namespace SceneAPI
} // namespace AZ
