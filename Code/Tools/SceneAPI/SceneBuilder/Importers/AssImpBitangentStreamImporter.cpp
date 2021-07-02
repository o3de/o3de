/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzToolsFramework/Debug/TraceContext.h>
#include <SceneAPI/SceneBuilder/Importers/AssImpBitangentStreamImporter.h>
#include <SceneAPI/SceneBuilder/Importers/ImporterUtilities.h>
#include <SceneAPI/SceneBuilder/Importers/Utilities/AssImpMeshImporterUtilities.h>
#include <SceneAPI/SceneData/GraphData/MeshVertexBitangentData.h>
#include <SceneAPI/SDKWrapper/AssImpNodeWrapper.h>
#include <SceneAPI/SDKWrapper/AssImpSceneWrapper.h>
#include <SceneAPI/SDKWrapper/AssImpTypeConverter.h>
#include <SceneAPI/SceneData/GraphData/MeshData.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>

#include <assimp/scene.h>
#include <assimp/mesh.h>
namespace AZ
{
    namespace SceneAPI
    {
        namespace SceneBuilder
        {
            const char* AssImpBitangentStreamImporter::m_defaultNodeName = "Bitangent";

            AssImpBitangentStreamImporter::AssImpBitangentStreamImporter()
            {
                BindToCall(&AssImpBitangentStreamImporter::ImportBitangentStreams);
            }

            void AssImpBitangentStreamImporter::Reflect(ReflectContext* context)
            {
                SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context);
                if (serializeContext)
                {
                    serializeContext->Class<AssImpBitangentStreamImporter, SceneCore::LoadingComponent>()->Version(3); // LYN-3250
                }
            }

            Events::ProcessingResult AssImpBitangentStreamImporter::ImportBitangentStreams(AssImpSceneNodeAppendedContext& context)
            {
                AZ_TraceContext("Importer", m_defaultNodeName);
                if (!context.m_sourceNode.ContainsMesh())
                {
                    return Events::ProcessingResult::Ignored;
                }
                const aiNode* currentNode = context.m_sourceNode.GetAssImpNode();
                const aiScene* scene = context.m_sourceScene.GetAssImpScene();

                const auto meshHasTangentsAndBitangents = [&scene](const unsigned int meshIndex)
                {
                    return scene->mMeshes[meshIndex]->HasTangentsAndBitangents();
                };

                // If there are no bitangents on any meshes, there's nothing to import in this function.
                const bool anyMeshHasTangentsAndBitangents = AZStd::any_of(currentNode->mMeshes, currentNode->mMeshes + currentNode->mNumMeshes, meshHasTangentsAndBitangents);
                if (!anyMeshHasTangentsAndBitangents)
                {
                    return Events::ProcessingResult::Ignored;
                }

                // AssImp nodes with multiple meshes on them occur when AssImp split a mesh on material.
                // This logic recombines those meshes to minimize the changes needed to replace FBX SDK with AssImp, FBX SDK did not separate meshes,
                // and the engine has code to do this later.
                const bool allMeshesHaveTangentsAndBitangents = AZStd::all_of(currentNode->mMeshes, currentNode->mMeshes + currentNode->mNumMeshes, meshHasTangentsAndBitangents);
                if (!allMeshesHaveTangentsAndBitangents)
                {
                    AZ_Error(
                        Utilities::ErrorWindow, false,
                        "Node with name %s has meshes with and without bitangents. "
                            "Placeholder incorrect bitangents will be generated to allow the data to process, "
                            "but the source art needs to be fixed to correct this. Either apply bitangents to all meshes on this node, "
                            "or remove all bitangents from all meshes on this node.",
                        currentNode->mName.C_Str());
                }

                const uint64_t vertexCount = GetVertexCountForAllMeshesOnNode(*currentNode, *scene);

                AZStd::shared_ptr<SceneData::GraphData::MeshVertexBitangentData> bitangentStream =
                    AZStd::make_shared<AZ::SceneData::GraphData::MeshVertexBitangentData>();
                // AssImp only has one bitangentStream per mesh.
                bitangentStream->SetBitangentSetIndex(0);

                bitangentStream->SetTangentSpace(AZ::SceneAPI::DataTypes::TangentSpace::FromSourceScene);
                bitangentStream->ReserveContainerSpace(vertexCount);
                for (int sdkMeshIndex = 0; sdkMeshIndex < currentNode->mNumMeshes; ++sdkMeshIndex)
                {
                    const aiMesh* mesh = scene->mMeshes[currentNode->mMeshes[sdkMeshIndex]];

                    for (int v = 0; v < mesh->mNumVertices; ++v)
                    {
                        if (!mesh->HasTangentsAndBitangents())
                        {
                            // This node has mixed meshes with and without bitangents.
                            // An error was already thrown above. Output stub bitangents so
                            // the mesh can still be output in some form, even if the data isn't correct.
                            // The bitangent count needs to match the vertex count on the associated mesh node.
                            bitangentStream->AppendBitangent(Vector3::CreateAxisY());
                        }
                        else
                        {
                            const Vector3 bitangent(
                                AssImpSDKWrapper::AssImpTypeConverter::ToVector3(mesh->mBitangents[v]));
                            bitangentStream->AppendBitangent(bitangent);
                        }
                    }
                }

                Containers::SceneGraph::NodeIndex newIndex =
                    context.m_scene.GetGraph().AddChild(context.m_currentGraphPosition, m_defaultNodeName);

                Events::ProcessingResult bitangentResults;
                AssImpSceneAttributeDataPopulatedContext dataPopulated(context, bitangentStream, newIndex, m_defaultNodeName);
                bitangentResults = Events::Process(dataPopulated);

                if (bitangentResults != Events::ProcessingResult::Failure)
                {
                    bitangentResults = AddAttributeDataNodeWithContexts(dataPopulated);
                }
                return bitangentResults;
            }

        } // namespace SceneBuilder
    } // namespace SceneAPI
} // namespace AZ
