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
#include <SceneAPI/SDKWrapper/AssImpNodeWrapper.h>
#include <SceneAPI/SDKWrapper/AssImpSceneWrapper.h>
#include <SceneAPI/SceneData/GraphData/MeshData.h>
#include <SceneAPI/SceneData/GraphData/MeshVertexUVData.h>

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
                    serializeContext->Class<AssImpUvMapImporter, SceneCore::LoadingComponent>()->Version(2); // LYN-2576
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

                GetMeshDataFromParentResult meshDataResult(GetMeshDataFromParent(context));
                if (!meshDataResult.IsSuccess())
                {
                    return meshDataResult.GetError();
                }
                const SceneData::GraphData::MeshData* const parentMeshData(meshDataResult.GetValue());

                size_t vertexCount = parentMeshData->GetVertexCount();

                int sdkMeshIndex = parentMeshData->GetSdkMeshIndex();
                AZ_Assert(sdkMeshIndex >= 0,
                    "Tried to construct uv stream attribute for invalid or non-mesh parent data, mesh index is missing");

                aiMesh* mesh = scene->mMeshes[currentNode->mMeshes[sdkMeshIndex]];

                Events::ProcessingResultCombiner combinedUvMapResults;
                for (int texCoordIndex = 0; texCoordIndex < AI_MAX_NUMBER_OF_TEXTURECOORDS; ++texCoordIndex)
                {
                    if (!mesh->mTextureCoords[texCoordIndex])
                    {
                        continue;
                    }

                    AZStd::shared_ptr<SceneData::GraphData::MeshVertexUVData> uvMap =
                        AZStd::make_shared<AZ::SceneData::GraphData::MeshVertexUVData>();
                    uvMap->ReserveContainerSpace(vertexCount);

                    AZStd::string name(AZStd::string::format("%s%d", m_defaultNodeName, texCoordIndex));
                    if (mesh->mTextureCoordsNames[texCoordIndex].C_Str())
                    {
                        name = AZStd::string::format("%s", mesh->mTextureCoordsNames[texCoordIndex].C_Str());
                    }

                    uvMap->SetCustomName(name.c_str());

                    for (int v = 0; v < mesh->mNumVertices; ++v)
                    {
                        AZ::Vector2 vertexUV(
                            mesh->mTextureCoords[texCoordIndex][v].x,
                            // The engine's V coordinate is reverse of how it's stored in the FBX file.
                            1.0f - mesh->mTextureCoords[texCoordIndex][v].y);
                        uvMap->AppendUV(vertexUV);
                    }

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
