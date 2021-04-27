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
#include <SceneAPI/FbxSceneBuilder/Importers/Utilities/AssImpMeshImporterUtilities.h>
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
                    serializeContext->Class<AssImpColorStreamImporter, SceneCore::LoadingComponent>()->Version(2); // LYN-2576
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

                int vertexCount = 0;
                int expectedColorChannels = -1;

                // AssImp separates meshes that have multiple materials.
                // This code re-combines them to match previous FBX SDK behavior,
                // so they can be separated by engine code instead.
                for (int sdkMeshIndex = 0; sdkMeshIndex < currentNode->mNumMeshes; ++sdkMeshIndex)
                {
                    aiMesh* mesh = scene->mMeshes[currentNode->mMeshes[sdkMeshIndex]];
                    if (expectedColorChannels < 0)
                    {
                        expectedColorChannels = mesh->GetNumColorChannels();
                    }
                    else if(expectedColorChannels != mesh->GetNumColorChannels())
                    {
                        AZ_Error(
                            Utilities::ErrorWindow,
                            false,
                            "Color channel count %d for node %s, for mesh %s at index %d does not match expected count %d",
                            mesh->GetNumColorChannels(),
                            currentNode->mName.C_Str(),
                            mesh->mName.C_Str(),
                            sdkMeshIndex,
                            expectedColorChannels);
                        return Events::ProcessingResult::Failure;
                    }
                    vertexCount += mesh->mNumVertices * mesh->GetNumColorChannels();
                }
                if (vertexCount == 0)
                {
                    return Events::ProcessingResult::Ignored;
                }

                Events::ProcessingResultCombiner combinedVertexColorResults;
                for (int colorSetIndex = 0; colorSetIndex < expectedColorChannels; ++colorSetIndex)
                {

                    AZStd::shared_ptr<SceneData::GraphData::MeshVertexColorData> vertexColors =
                        AZStd::make_shared<AZ::SceneData::GraphData::MeshVertexColorData>();
                    vertexColors->ReserveContainerSpace(vertexCount);

                    for (int sdkMeshIndex = 0; sdkMeshIndex < currentNode->mNumMeshes; ++sdkMeshIndex)
                    {
                        aiMesh* mesh = scene->mMeshes[currentNode->mMeshes[sdkMeshIndex]];
                        for (int v = 0; v < mesh->mNumVertices; ++v)
                        {
                            AZ::SceneAPI::DataTypes::Color vertexColor(
                                AssImpSDKWrapper::AssImpTypeConverter::ToColor(mesh->mColors[colorSetIndex][v]));
                            vertexColors->AppendColor(vertexColor);
                        }
                    }

                    AZStd::string nodeName(AZStd::string::format("%s%d", m_defaultNodeName, colorSetIndex));
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
