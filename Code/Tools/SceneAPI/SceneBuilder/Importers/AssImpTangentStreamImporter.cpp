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
#include <SceneAPI/SceneBuilder/Importers/AssImpTangentStreamImporter.h>
#include <SceneAPI/SceneBuilder/Importers/ImporterUtilities.h>
#include <SceneAPI/SceneBuilder/Importers/Utilities/AssImpMeshImporterUtilities.h>
#include <SceneAPI/SceneData/GraphData/MeshVertexTangentData.h>
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
            const char* AssImpTangentStreamImporter::m_defaultNodeName = "Tangent";

            AssImpTangentStreamImporter::AssImpTangentStreamImporter()
            {
                BindToCall(&AssImpTangentStreamImporter::ImportTangentStreams);
            }

            void AssImpTangentStreamImporter::Reflect(ReflectContext* context)
            {
                SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context);
                if (serializeContext)
                {
                    serializeContext->Class<AssImpTangentStreamImporter, SceneCore::LoadingComponent>()->Version(3); // LYN-3250
                }
            }

            Events::ProcessingResult AssImpTangentStreamImporter::ImportTangentStreams(AssImpSceneNodeAppendedContext& context)
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

                // If there are no tangents on any meshes, there's nothing to import in this function.
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
                        "Node with name %s has meshes with and without tangents. "
                            "Placeholder incorrect tangents will be generated to allow the data to process, "
                            "but the source art needs to be fixed to correct this. Either apply tangents to all meshes on this node, "
                            "or remove all tangents from all meshes on this node.",
                        currentNode->mName.C_Str());
                }

                const uint64_t vertexCount = GetVertexCountForAllMeshesOnNode(*currentNode, *scene);

                AZStd::shared_ptr<SceneData::GraphData::MeshVertexTangentData> tangentStream =
                    AZStd::make_shared<AZ::SceneData::GraphData::MeshVertexTangentData>();
                // AssImp only has one tangentStream per mesh.
                tangentStream->SetTangentSetIndex(0);

                tangentStream->SetGenerationMethod(AZ::SceneAPI::DataTypes::TangentGenerationMethod::FromSourceScene);
                tangentStream->ReserveContainerSpace(vertexCount);
                for (unsigned int sdkMeshIndex = 0; sdkMeshIndex < currentNode->mNumMeshes; ++sdkMeshIndex)
                {
                    const aiMesh* mesh = scene->mMeshes[currentNode->mMeshes[sdkMeshIndex]];

                    for (unsigned int v = 0; v < mesh->mNumVertices; ++v)
                    {
                        if (!mesh->HasTangentsAndBitangents())
                        {
                            // This node has mixed meshes with and without tangents.
                            // An error was already thrown above. Output stub tangents so
                            // the mesh can still be output in some form, even if the data isn't correct.
                            // The tangent count needs to match the vertex count on the associated mesh node.
                            tangentStream->AppendTangent(Vector4(0.f, 1.f, 0.f, 1.f));
                        }
                        else
                        {
                            const Vector4 tangent(
                                AssImpSDKWrapper::AssImpTypeConverter::ToVector3(mesh->mTangents[v]));
                            tangentStream->AppendTangent(tangent);
                        }
                    }
                }

                Containers::SceneGraph::NodeIndex newIndex =
                    context.m_scene.GetGraph().AddChild(context.m_currentGraphPosition, m_defaultNodeName);

                Events::ProcessingResult tangentResults;
                AssImpSceneAttributeDataPopulatedContext dataPopulated(context, tangentStream, newIndex, m_defaultNodeName);
                tangentResults = Events::Process(dataPopulated);

                if (tangentResults != Events::ProcessingResult::Failure)
                {
                    tangentResults = AddAttributeDataNodeWithContexts(dataPopulated);
                }
                return tangentResults;
            }

        } // namespace SceneBuilder
    } // namespace SceneAPI
} // namespace AZ
