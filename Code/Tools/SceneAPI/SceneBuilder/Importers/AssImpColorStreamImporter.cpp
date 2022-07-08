/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/numeric.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzToolsFramework/Debug/TraceContext.h>
#include <SceneAPI/SceneBuilder/Importers/AssImpColorStreamImporter.h>
#include <SceneAPI/SceneBuilder/Importers/ImporterUtilities.h>
#include <SceneAPI/SceneBuilder/Importers/Utilities/AssImpMeshImporterUtilities.h>
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
        namespace SceneBuilder
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
                const unsigned int expectedColorChannels = scene->mMeshes[currentNode->mMeshes[0]]->GetNumColorChannels();
                [[maybe_unused]] const bool allMeshesHaveSameNumberOfColorChannels =
                    AZStd::all_of(currentNode->mMeshes + 1, currentNode->mMeshes + currentNode->mNumMeshes, [scene, expectedColorChannels](const unsigned int meshIndex)
                        {
                            return scene->mMeshes[meshIndex]->GetNumColorChannels() == expectedColorChannels;
                        });

                AZ_Error(
                    Utilities::ErrorWindow,
                    allMeshesHaveSameNumberOfColorChannels,
                    "Color channel counts for node %s has meshes with different color channel counts. "
                    "The color channel count for the first mesh will be used, and placeholder incorrect color values "
                    "will be generated to allow the data to process, but the source art needs to be fixed to correct this. "
                    "All meshes on this node should have the same number of color channels.",
                    currentNode->mName.C_Str());

                if (expectedColorChannels == 0)
                {
                    return Events::ProcessingResult::Ignored;
                }

                const uint64_t vertexCount = GetVertexCountForAllMeshesOnNode(*currentNode, *scene);

                Events::ProcessingResultCombiner combinedVertexColorResults;
                for (unsigned int colorSetIndex = 0; colorSetIndex < expectedColorChannels; ++colorSetIndex)
                {
                    AZStd::shared_ptr<SceneData::GraphData::MeshVertexColorData> vertexColors =
                        AZStd::make_shared<AZ::SceneData::GraphData::MeshVertexColorData>();
                    vertexColors->ReserveContainerSpace(vertexCount);

                    for (unsigned int sdkMeshIndex = 0; sdkMeshIndex < currentNode->mNumMeshes; ++sdkMeshIndex)
                    {
                        const aiMesh* mesh = scene->mMeshes[currentNode->mMeshes[sdkMeshIndex]];
                        for (unsigned int v = 0; v < mesh->mNumVertices; ++v)
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
                                // than other meshes on the parent node. Append an arbitrary color value, fully opaque black,
                                // so the mesh can still be processed.
                                // It's better to let the engine load a partially valid mesh than to completely fail.
                                vertexColors->AppendColor(AZ::SceneAPI::DataTypes::Color(0.0f,0.0f,0.0f,1.0f));
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

        } // namespace SceneBuilder
    } // namespace SceneAPI
} // namespace AZ
