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

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzToolsFramework/Debug/TraceContext.h>
#include <SceneAPI/FbxSceneBuilder/Importers/AssImpColorStreamImporter.h>
#include <SceneAPI/FbxSceneBuilder/Importers/FbxImporterUtilities.h>
#include <SceneAPI/SDKWrapper/AssImpNodeWrapper.h>
#include <SceneAPI/SDKWrapper/AssImpSceneWrapper.h>
#include <SceneAPI/SDKWrapper/AssImpTypeConverter.h>
#include <SceneAPI/SceneData/GraphData/MeshData.h>
#include <SceneAPI/SceneData/GraphData/MeshVertexColorData.h>

#include <assimp/scene.h>
#include <assimp/mesh.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace FbxSceneBuilder
        {
            const char* AssImpColorStreamImporter::m_defaultNodeName = "Col";

            AssImpColorStreamImporter::AssImpColorStreamImporter()
            {
                BindToCall(&AssImpColorStreamImporter::ImportColorStreams);
            }

            void AssImpColorStreamImporter::Reflect(ReflectContext* context)
            {
                SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context);
                if (serializeContext)
                {
                    serializeContext->Class<AssImpColorStreamImporter, SceneCore::LoadingComponent>()->Version(1);
                }
            }

            Events::ProcessingResult AssImpColorStreamImporter::ImportColorStreams(AssImpSceneNodeAppendedContext& context)
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
                if (!parentData || !parentData->RTTI_IsTypeOf(SceneData::GraphData::MeshData::TYPEINFO_Uuid()))
                {
                    AZ_Error(Utilities::ErrorWindow, false,
                        "Tried to construct color stream attribute for invalid or non-mesh parent data");
                    return Events::ProcessingResult::Failure;
                }
                const SceneData::GraphData::MeshData* const parentMeshData = azrtti_cast<SceneData::GraphData::MeshData*>(parentData.get());
                size_t vertexCount = parentMeshData->GetVertexCount();

                int sdkMeshIndex = parentMeshData->GetSdkMeshIndex();
                if (sdkMeshIndex < 0)
                {
                    AZ_Error(Utilities::ErrorWindow, false,
                        "Tried to construct color stream attribute for invalid or non-mesh parent data, mesh index is missing");
                    return Events::ProcessingResult::Failure;
                }

                aiMesh* mesh = scene->mMeshes[currentNode->mMeshes[sdkMeshIndex]];

                Events::ProcessingResultCombiner combinedVertexColorResults;
                for (int colorSetIndex = 0; colorSetIndex < mesh->GetNumColorChannels(); ++colorSetIndex)
                {
                    AZStd::shared_ptr<SceneData::GraphData::MeshVertexColorData> vertexColors =
                        AZStd::make_shared<AZ::SceneData::GraphData::MeshVertexColorData>();
                    vertexColors->ReserveContainerSpace(vertexCount);

                    for (int v = 0; v < mesh->mNumVertices; ++v)
                    {
                        AZ::SceneAPI::DataTypes::Color vertexColor(
                            AssImpSDKWrapper::AssImpTypeConverter::ToColor(mesh->mColors[colorSetIndex][v]));
                        vertexColors->AppendColor(vertexColor);
                    }

                    AZStd::string nodeName(AZStd::string::format("%s%d",m_defaultNodeName,colorSetIndex));
                    Containers::SceneGraph::NodeIndex newIndex =
                        context.m_scene.GetGraph().AddChild(context.m_currentGraphPosition, nodeName.c_str());

                    Events::ProcessingResult colorMapResults;
                    AssImpSceneAttributeDataPopulatedContext dataPopulated(context, vertexColors, newIndex, nodeName.c_str());
                    colorMapResults = Events::Process(dataPopulated);

                    if (colorMapResults != Events::ProcessingResult::Failure)
                    {
                        colorMapResults = AddAttributeDataNodeWithContexts(dataPopulated);
                    }

                    combinedVertexColorResults += colorMapResults;
                }

                return combinedVertexColorResults.GetResult();

            }

        } // namespace FbxSceneBuilder
    } // namespace SceneAPI
} // namespace AZ
