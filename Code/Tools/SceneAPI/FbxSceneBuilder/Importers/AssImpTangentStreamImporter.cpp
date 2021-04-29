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
#include <SceneAPI/FbxSceneBuilder/Importers/AssImpTangentStreamImporter.h>
#include <SceneAPI/FbxSceneBuilder/Importers/FbxImporterUtilities.h>
#include <SceneAPI/FbxSceneBuilder/Importers/Utilities/AssImpMeshImporterUtilities.h>
#include <SceneAPI/SceneData/GraphData/MeshVertexTangentData.h>
#include <SceneAPI/SDKWrapper/AssImpNodeWrapper.h>
#include <SceneAPI/SDKWrapper/AssImpSceneWrapper.h>
#include <SceneAPI/SDKWrapper/AssImpTypeConverter.h>
#include <SceneAPI/SceneData/GraphData/MeshData.h>

#include <assimp/scene.h>
#include <assimp/mesh.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace FbxSceneBuilder
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
                    serializeContext->Class<AssImpTangentStreamImporter, SceneCore::LoadingComponent>()->Version(2); // LYN-2576
                }
            }

            Events::ProcessingResult AssImpTangentStreamImporter::ImportTangentStreams(AssImpSceneNodeAppendedContext& context)
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
                bool hasAnyTangents = false;
                // Used to assist error reporting and check that all meshes have tangents or no meshes should have tangents.
                int localMeshIndex = -1;
                const uint64_t vertexCount = AZStd::accumulate(currentNode->mMeshes, currentNode->mMeshes + currentNode->mNumMeshes, uint64_t{ 0u },
                    [scene, currentNode, &hasAnyTangents, &localMeshIndex](auto runningTotal, unsigned int meshIndex)
                    {
                        ++localMeshIndex;
                        const char* mixedTangentsError =
                            "Node with name %s has meshes with and without tangents. "
                            "Placeholder incorrect tangents will be generated to allow the data to process, "
                            "but the source art needs to be fixed to correct this. Either apply tangents to all meshes on this node, "
                            "or remove all tangents from all meshes on this node.";
                        aiMesh* mesh = scene->mMeshes[meshIndex];
                        if (!mesh->HasTangentsAndBitangents())
                        {
                            AZ_Error(
                                Utilities::ErrorWindow, !hasAnyTangents, mixedTangentsError, currentNode->mName.C_Str());
                            return runningTotal + mesh->mNumVertices;
                        }
                        AZ_Error(
                            Utilities::ErrorWindow, localMeshIndex == 0 || hasAnyTangents, mixedTangentsError, currentNode->mName.C_Str());
                        hasAnyTangents = true;
                        return runningTotal + mesh->mNumVertices;
                    });
                if (!hasAnyTangents)
                {
                    return Events::ProcessingResult::Ignored;
                }

                AZStd::shared_ptr<SceneData::GraphData::MeshVertexTangentData> tangentStream =
                    AZStd::make_shared<AZ::SceneData::GraphData::MeshVertexTangentData>();
                // AssImp only has one tangentStream per mesh.
                tangentStream->SetTangentSetIndex(0);

                tangentStream->SetTangentSpace(AZ::SceneAPI::DataTypes::TangentSpace::FromFbx);
                tangentStream->ReserveContainerSpace(vertexCount);
                for (int sdkMeshIndex = 0; sdkMeshIndex < currentNode->mNumMeshes; ++sdkMeshIndex)
                {
                    aiMesh* mesh = scene->mMeshes[currentNode->mMeshes[sdkMeshIndex]];

                    for (int v = 0; v < mesh->mNumVertices; ++v)
                    {
                        if (!mesh->HasTangentsAndBitangents())
                        {
                            // This node has mixed meshes with and without tangents.
                            // An error was already thrown above. Output stub tangents so
                            // the mesh can still be output in some form, even if the data isn't correct.
                            // The tangent count needs to match the vertex count on the associated mesh node.
                            const Vector4 tangent(1, 0, 0, 0);
                            tangentStream->AppendTangent(tangent);
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

        } // namespace FbxSceneBuilder
    } // namespace SceneAPI
} // namespace AZ
