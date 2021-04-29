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
#include <SceneAPI/FbxSceneBuilder/Importers/AssImpBitangentStreamImporter.h>
#include <SceneAPI/FbxSceneBuilder/Importers/FbxImporterUtilities.h>
#include <SceneAPI/FbxSceneBuilder/Importers/Utilities/AssImpMeshImporterUtilities.h>
#include <SceneAPI/SceneData/GraphData/MeshVertexBitangentData.h>
#include <SceneAPI/SDKWrapper/AssImpNodeWrapper.h>
#include <SceneAPI/SDKWrapper/AssImpSceneWrapper.h>
#include <SceneAPI/SDKWrapper/AssImpTypeConverter.h>
#include <SceneAPI/SceneData/GraphData/MeshData.h>

#include <assimp/scene.h>
#include <assimp/mesh.h>
#pragma optimize("", off)
namespace AZ
{
    namespace SceneAPI
    {
        namespace FbxSceneBuilder
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
                aiNode* currentNode = context.m_sourceNode.GetAssImpNode();
                const aiScene* scene = context.m_sourceScene.GetAssImpScene();

                // AssImp separates meshes that have multiple materials.
                // This code re-combines them to match previous FBX SDK behavior,
                // so they can be separated by engine code instead.
                bool hasAnyBitangents = false;
                // Used to assist error reporting and check that all meshes have bitangents or no meshes should have bitangents.
                int localMeshIndex = -1;
                const uint64_t vertexCount = AZStd::accumulate(currentNode->mMeshes, currentNode->mMeshes + currentNode->mNumMeshes, uint64_t{ 0u },
                    [scene, currentNode, &hasAnyBitangents, &localMeshIndex](auto runningTotal, unsigned int meshIndex)
                    {
                        ++localMeshIndex;
                        const char* mixedTangentsError =
                            "Node with name %s has meshes with and without bitangents. "
                            "Placeholder incorrect bitangents will be generated to allow the data to process, "
                            "but the source art needs to be fixed to correct this. Either apply bitangents to all meshes on this node, "
                            "or remove all bitangents from all meshes on this node.";
                        aiMesh* mesh = scene->mMeshes[meshIndex];
                        if (!mesh->HasTangentsAndBitangents())
                        {
                            AZ_Error(
                                Utilities::ErrorWindow, !hasAnyBitangents, mixedTangentsError, currentNode->mName.C_Str());
                            return runningTotal + mesh->mNumVertices;
                        }
                        AZ_Error(
                            Utilities::ErrorWindow, localMeshIndex == 0 || hasAnyBitangents, mixedTangentsError, currentNode->mName.C_Str());
                        hasAnyBitangents = true;
                        return runningTotal + mesh->mNumVertices;
                    });
                if (!hasAnyBitangents)
                {
                    return Events::ProcessingResult::Ignored;
                }

                AZStd::shared_ptr<SceneData::GraphData::MeshVertexBitangentData> bitangentStream =
                    AZStd::make_shared<AZ::SceneData::GraphData::MeshVertexBitangentData>();
                // AssImp only has one bitangentStream per mesh.
                bitangentStream->SetBitangentSetIndex(0);

                bitangentStream->SetTangentSpace(AZ::SceneAPI::DataTypes::TangentSpace::FromFbx);
                bitangentStream->ReserveContainerSpace(vertexCount);
                for (int sdkMeshIndex = 0; sdkMeshIndex < currentNode->mNumMeshes; ++sdkMeshIndex)
                {
                    aiMesh* mesh = scene->mMeshes[currentNode->mMeshes[sdkMeshIndex]];

                    for (int v = 0; v < mesh->mNumVertices; ++v)
                    {
                        if (!mesh->HasTangentsAndBitangents())
                        {
                            // This node has mixed meshes with and without bitangents.
                            // An error was already thrown above. Output stub bitangents so
                            // the mesh can still be output in some form, even if the data isn't correct.
                            // The bitangent count needs to match the vertex count on the associated mesh node.
                            const Vector3 bitangent(1,0,0);
                            bitangentStream->AppendBitangent(bitangent);
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

        } // namespace FbxSceneBuilder
    } // namespace SceneAPI
} // namespace AZ
