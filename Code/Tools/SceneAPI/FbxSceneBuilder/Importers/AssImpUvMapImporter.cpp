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
            const char* AssImpUvMapImporter::m_defaultNodeName = "UVMap";

            AssImpUvMapImporter::AssImpUvMapImporter()
            {
                BindToCall(&AssImpUvMapImporter::ImportUvMaps);
            }

            void AssImpUvMapImporter::Reflect(ReflectContext* context)
            {
                SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context);
                if (serializeContext)
                {
                    serializeContext->Class<AssImpUvMapImporter, SceneCore::LoadingComponent>()->Version(1);
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

                AZStd::shared_ptr<DataTypes::IGraphObject> parentData =
                    context.m_scene.GetGraph().GetNodeContent(context.m_currentGraphPosition);
                AZ_Assert(parentData && parentData->RTTI_IsTypeOf(SceneData::GraphData::MeshData::TYPEINFO_Uuid()),
                    "Tried to construct uv stream attribute for invalid or non-mesh parent data");
                if (!parentData || !parentData->RTTI_IsTypeOf(SceneData::GraphData::MeshData::TYPEINFO_Uuid()))
                {
                    return Events::ProcessingResult::Failure;
                }
                const SceneData::GraphData::MeshData* const parentMeshData =
                    azrtti_cast<SceneData::GraphData::MeshData*>(parentData.get());
                size_t vertexCount = parentMeshData->GetVertexCount();

                int sdkMeshIndex = parentMeshData->GetSdkMeshIndex();
                AZ_Assert(sdkMeshIndex >= 0,
                    "Tried to construct uv stream attribute for invalid or non-mesh parent data, mesh index is missing");

                aiMesh* mesh = scene->mMeshes[currentNode->mMeshes[sdkMeshIndex]];

                if (!mesh->mTextureCoords[0])
                {
                    return Events::ProcessingResult::Ignored;
                }

                Events::ProcessingResultCombiner combinedUvMapResults;

                AZStd::shared_ptr<SceneData::GraphData::MeshVertexUVData> uvMap =
                    AZStd::make_shared<AZ::SceneData::GraphData::MeshVertexUVData>();
                uvMap->ReserveContainerSpace(vertexCount);
                uvMap->SetCustomName(m_defaultNodeName);

                for (int v = 0; v < mesh->mNumVertices; ++v)
                {
                    AZ::Vector2 vertexUV(
                        mesh->mTextureCoords[0][v].x,
                        mesh->mTextureCoords[0][v].y);
                    uvMap->AppendUV(vertexUV);
                }

                Containers::SceneGraph::NodeIndex newIndex =
                    context.m_scene.GetGraph().AddChild(context.m_currentGraphPosition, m_defaultNodeName);

                Events::ProcessingResult uvMapResults;
                AssImpSceneAttributeDataPopulatedContext dataPopulated(context, uvMap, newIndex, m_defaultNodeName);
                uvMapResults = Events::Process(dataPopulated);

                if (uvMapResults != Events::ProcessingResult::Failure)
                {
                    uvMapResults = AddAttributeDataNodeWithContexts(dataPopulated);
                }

                combinedUvMapResults += uvMapResults;

                return combinedUvMapResults.GetResult();
            }

        } // namespace FbxSceneBuilder
    } // namespace SceneAPI
} // namespace AZ
