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
#include <AzCore/std/numeric.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzToolsFramework/Debug/TraceContext.h>
#include <SceneAPI/FbxSceneBuilder/Importers/AssImpColorStreamImporter.h>
#include <SceneAPI/FbxSceneBuilder/Importers/ImporterUtilities.h>
#include <SceneAPI/FbxSceneBuilder/Importers/Utilities/AssImpMeshImporterUtilities.h>
#include <SceneAPI/SDKWrapper/AssImpNodeWrapper.h>
#include <SceneAPI/SDKWrapper/AssImpSceneWrapper.h>
#include <SceneAPI/SDKWrapper/AssImpTypeConverter.h>
#include <SceneAPI/SceneData/GraphData/MeshData.h>
#include <SceneAPI/SceneData/GraphData/MeshVertexColorData.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>

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
                    serializeContext->Class<AssImpColorStreamImporter, SceneCore::LoadingComponent>()->Version(3); // LYN-3250
                }
            }

            Events::ProcessingResult AssImpColorStreamImporter::ImportColorStreams(AssImpSceneNodeAppendedContext& context)
            {
                AZ_TraceContext("Importer", m_defaultNodeName);
                if (!context.m_sourceNode.ContainsMesh())
                {
                    return Events::ProcessingResult::Ignored;
                }
                const aiNode* currentNode = context.m_sourceNode.GetAssImpNode();
                const aiScene* scene = context.m_sourceScene.GetAssImpScene();

                // This node has at least one mesh, verify that the color channel counts are the same for all meshes.
                int expectedColorChannels = scene->mMeshes[currentNode->mMeshes[0]]->GetNumColorChannels();
                for (int localMeshIndex = 1; localMeshIndex < currentNode->mNumMeshes; ++localMeshIndex)
                {
                    const aiMesh* mesh = scene->mMeshes[currentNode->mMeshes[localMeshIndex]];
                    if (expectedColorChannels != mesh->GetNumColorChannels())
                    {
                        AZ_Error(
                            Utilities::ErrorWindow,
                            false,
                            "Color channel count %d for node %s, for mesh %s at index %d does not match expected count %d. "
                            "Placeholder incorrect color values will be generated to allow the data to process, but the source art "
                            "needs to be fixed to correct this. All meshes on this node should have the same number of color channels.",
                            mesh->GetNumColorChannels(),
                            currentNode->mName.C_Str(),
                            mesh->mName.C_Str(),
                            localMeshIndex,
                            expectedColorChannels);
                        if (mesh->GetNumColorChannels() > expectedColorChannels)
                        {
                            expectedColorChannels = mesh->GetNumColorChannels();
                        }
                    }
                }

                if (expectedColorChannels == 0)
                {
                    return Events::ProcessingResult::Ignored;
                }

                const uint64_t vertexCount = GetVertexCountForAllMeshesOnNode(*currentNode, *scene);

                Events::ProcessingResultCombiner combinedVertexColorResults;
                for (int colorSetIndex = 0; colorSetIndex < expectedColorChannels; ++colorSetIndex)
                {

                    AZStd::shared_ptr<SceneData::GraphData::MeshVertexColorData> vertexColors =
                        AZStd::make_shared<AZ::SceneData::GraphData::MeshVertexColorData>();
                    vertexColors->ReserveContainerSpace(vertexCount);

                    for (int sdkMeshIndex = 0; sdkMeshIndex < currentNode->mNumMeshes; ++sdkMeshIndex)
                    {
                        const aiMesh* mesh = scene->mMeshes[currentNode->mMeshes[sdkMeshIndex]];
                        for (int v = 0; v < mesh->mNumVertices; ++v)
                        {
                            if (colorSetIndex < mesh->GetNumColorChannels())
                            {
                                AZ::SceneAPI::DataTypes::Color vertexColor(
                                    AssImpSDKWrapper::AssImpTypeConverter::ToColor(mesh->mColors[colorSetIndex][v]));
                                vertexColors->AppendColor(vertexColor);
                            }
                            else
                            {
                                // An error was already emitted if this mesh has less color channels
                                // than other meshes on the parent node. Append an arbitrary color value
                                // so the mesh can still be processed.
                                // It's better to let the engine load a partially valid mesh than to completely fail.
                                AZ::SceneAPI::DataTypes::Color vertexColor(0,0,0,1);
                                vertexColors->AppendColor(vertexColor);
                            }
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
