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
#include <SceneAPI/FbxSceneBuilder/Importers/AssImpTangentStreamImporter.h>
#include <SceneAPI/FbxSceneBuilder/Importers/FbxImporterUtilities.h>
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
                    serializeContext->Class<AssImpTangentStreamImporter, SceneCore::LoadingComponent>()->Version(1);
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

                AZStd::shared_ptr<DataTypes::IGraphObject> parentData =
                    context.m_scene.GetGraph().GetNodeContent(context.m_currentGraphPosition);
                if (!parentData || !parentData->RTTI_IsTypeOf(SceneData::GraphData::MeshData::TYPEINFO_Uuid()))
                {
                    AZ_Error(Utilities::ErrorWindow, false,
                        "Tried to construct tangent stream attribute for invalid or non-mesh parent data");
                    return Events::ProcessingResult::Failure;
                }
                const SceneData::GraphData::MeshData* const parentMeshData = azrtti_cast<SceneData::GraphData::MeshData*>(parentData.get());
                size_t vertexCount = parentMeshData->GetVertexCount();

                int sdkMeshIndex = parentMeshData->GetSdkMeshIndex();
                if (sdkMeshIndex < 0 || sdkMeshIndex >= currentNode->mNumMeshes)
                {
                    AZ_Error(Utilities::ErrorWindow, false,
                        "Tried to construct tangent stream attribute for invalid or non-mesh parent data, mesh index is invalid");
                    return Events::ProcessingResult::Failure;
                }

                aiMesh* mesh = scene->mMeshes[currentNode->mMeshes[sdkMeshIndex]];

                if (!mesh->HasTangentsAndBitangents())
                {
                    return Events::ProcessingResult::Ignored;
                }

                AZStd::shared_ptr<SceneData::GraphData::MeshVertexTangentData> tangentStream =
                    AZStd::make_shared<AZ::SceneData::GraphData::MeshVertexTangentData>();

                // AssImp only has one tangentStream per mesh.
                tangentStream->SetTangentSetIndex(0);

                tangentStream->SetTangentSpace(AZ::SceneAPI::DataTypes::TangentSpace::FromFbx);
                tangentStream->ReserveContainerSpace(vertexCount);

                for (int v = 0; v < mesh->mNumVertices; ++v)
                {
                    // Vector4's constructor that takes in a vector3 sets w to 1.0f automatically.
                    const Vector4 tangent(AssImpSDKWrapper::AssImpTypeConverter::ToVector3(mesh->mTangents[v]));
                    tangentStream->AppendTangent(tangent);
                }

                AZStd::string nodeName(AZStd::string::format("%s", m_defaultNodeName));
                Containers::SceneGraph::NodeIndex newIndex =
                    context.m_scene.GetGraph().AddChild(context.m_currentGraphPosition, nodeName.c_str());

                Events::ProcessingResult tangentResults;
                AssImpSceneAttributeDataPopulatedContext dataPopulated(context, tangentStream, newIndex, nodeName.c_str());
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
