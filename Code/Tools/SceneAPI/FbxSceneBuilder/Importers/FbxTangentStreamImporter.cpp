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
#include <SceneAPI/FbxSceneBuilder/Importers/FbxTangentStreamImporter.h>
#include <SceneAPI/FbxSceneBuilder/Importers/FbxImporterUtilities.h>
#include <SceneAPI/FbxSceneBuilder/Importers/Utilities/RenamedNodesMap.h>
#include <SceneAPI/FbxSDKWrapper/FbxNodeWrapper.h>
#include <SceneAPI/FbxSDKWrapper/FbxVertexTangentWrapper.h>
#include <SceneAPI/SceneData/GraphData/MeshData.h>
#include <SceneAPI/SceneData/GraphData/SkinMeshData.h>
#include <SceneAPI/SceneData/GraphData/MeshVertexTangentData.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>
#include <SceneAPI/SceneCore/DataTypes/DataTypeUtilities.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace FbxSceneBuilder
        {
            FbxTangentStreamImporter::FbxTangentStreamImporter()
            {
                BindToCall(&FbxTangentStreamImporter::ImportTangents);
            }


            void FbxTangentStreamImporter::Reflect(ReflectContext* context)
            {
                SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context);
                if (serializeContext)
                {
                    serializeContext->Class<FbxTangentStreamImporter, SceneCore::LoadingComponent>()->Version(1);
                }
            }


            Events::ProcessingResult FbxTangentStreamImporter::ImportTangents(SceneNodeAppendedContext& context)
            {
                AZ_TraceContext("Importer", "Tangents");
                std::shared_ptr<FbxSDKWrapper::FbxMeshWrapper> fbxMesh = context.m_sourceNode.GetMesh();
                if (!fbxMesh)
                {
                    return Events::ProcessingResult::Ignored;
                }

                Events::ProcessingResultCombiner combinedStreamResults;
                const int numTangentSets = context.m_sourceNode.GetMesh()->GetElementTangentCount();
                for (int elementIndex = 0; elementIndex < numTangentSets; ++elementIndex)
                {
                    AZ_TraceContext("Tangent set index", elementIndex);

                    FbxSDKWrapper::FbxVertexTangentWrapper fbxVertexTangents = fbxMesh->GetElementTangent(elementIndex);
                    if (!fbxVertexTangents.IsValid())
                    {
                        AZ_TracePrintf(Utilities::WarningWindow, "Invalid tangent set found, ignoring");
                        continue;
                    }

                    const AZStd::string originalNodeName = AZStd::string::format("TangentSet_Fbx_%d", elementIndex);
                    const AZStd::string nodeName = AZ::SceneAPI::DataTypes::Utilities::CreateUniqueName<SceneData::GraphData::MeshVertexTangentData>(originalNodeName, context.m_scene.GetManifest());
                    AZ_TraceContext("Tangent Set Name", nodeName);
                    if (originalNodeName != nodeName)
                    {
                        AZ_TracePrintf(Utilities::WarningWindow, "Tangent set '%s' has been renamed to '%s' because the name was already in use.", originalNodeName.c_str(), nodeName.c_str());
                    }
                    
                    AZStd::shared_ptr<DataTypes::IGraphObject> parentData = context.m_scene.GetGraph().GetNodeContent(context.m_currentGraphPosition);
                    AZ_Assert(parentData && parentData->RTTI_IsTypeOf(SceneData::GraphData::MeshData::TYPEINFO_Uuid()), "Tried to construct tangent set attribute for invalid or non-mesh parent data");
                    if (!parentData || !parentData->RTTI_IsTypeOf(SceneData::GraphData::MeshData::TYPEINFO_Uuid()))
                    {
                        combinedStreamResults += Events::ProcessingResult::Failure;
                        continue;
                    }

                    const SceneData::GraphData::MeshData* const parentMeshData = azrtti_cast<SceneData::GraphData::MeshData*>(parentData.get());
                    const size_t vertexCount = parentMeshData->GetVertexCount();
                    AZStd::shared_ptr<SceneData::GraphData::MeshVertexTangentData> tangentStream = BuildVertexTangentData(fbxVertexTangents, vertexCount, fbxMesh);
                    
                    AZ_Assert(tangentStream, "Failed to allocate tangent data for scene graph.");
                    if (!tangentStream)
                    {
                        combinedStreamResults += Events::ProcessingResult::Failure;
                        continue;
                    }

                    tangentStream->SetTangentSetIndex(elementIndex);
                    tangentStream->SetTangentSpace(AZ::SceneAPI::DataTypes::TangentSpace::FromFbx);

                    const Containers::SceneGraph::NodeIndex newIndex = context.m_scene.GetGraph().AddChild(context.m_currentGraphPosition, nodeName.c_str());
                    AZ_Assert(newIndex.IsValid(), "Failed to create SceneGraph node for attribute.");
                    if (!newIndex.IsValid())
                    {
                        combinedStreamResults += Events::ProcessingResult::Failure;
                        continue;
                    }

                    Events::ProcessingResult streamResults;
                    SceneAttributeDataPopulatedContext dataPopulated(context, tangentStream, newIndex, nodeName);
                    streamResults = Events::Process(dataPopulated);

                    if (streamResults != Events::ProcessingResult::Failure)
                    {
                        streamResults = AddAttributeDataNodeWithContexts(dataPopulated);
                    }

                    combinedStreamResults += streamResults;
                }

                return combinedStreamResults.GetResult();
            }


            AZStd::shared_ptr<SceneData::GraphData::MeshVertexTangentData> FbxTangentStreamImporter::BuildVertexTangentData(const FbxSDKWrapper::FbxVertexTangentWrapper& tangents, size_t vertexCount, const std::shared_ptr<FbxSDKWrapper::FbxMeshWrapper>& fbxMesh)
            {
                AZStd::shared_ptr<SceneData::GraphData::MeshVertexTangentData> tangentData = AZStd::make_shared<AZ::SceneData::GraphData::MeshVertexTangentData>();
                tangentData->ReserveContainerSpace(vertexCount);

                const int fbxPolygonCount = fbxMesh->GetPolygonCount();
                const int* const fbxPolygonVertices = fbxMesh->GetPolygonVertices();
                for (int fbxPolygonIndex = 0; fbxPolygonIndex < fbxPolygonCount; ++fbxPolygonIndex)
                {
                    const int fbxPolygonVertexCount = fbxMesh->GetPolygonSize(fbxPolygonIndex);
                    if (fbxPolygonVertexCount <= 2)
                    {
                        continue;
                    }

                    const int fbxVertexStartIndex = fbxMesh->GetPolygonVertexIndex(fbxPolygonIndex);
                    for (int index = 0; index < fbxPolygonVertexCount; ++index)
                    {
                        const int fbxPolygonVertexIndex = fbxVertexStartIndex + index;
                        const int fbxControlPointIndex = fbxPolygonVertices[fbxPolygonVertexIndex];
                        
                        const Vector3 tangent = tangents.GetElementAt(fbxPolygonIndex, fbxPolygonVertexIndex, fbxControlPointIndex);
                        tangentData->AppendTangent(AZ::Vector4(tangent.GetX(), tangent.GetY(), tangent.GetZ(), 1.0f));
                    }
                }

                if (tangentData->GetCount() != vertexCount)
                {
                    AZ_TracePrintf(Utilities::ErrorWindow, "Vertex count (%i) doesn't match the number of entries for the tangent stream %s (%i)", vertexCount, tangents.GetName(), tangentData->GetCount());
                    return nullptr;
                }

                return tangentData;
            }

        } // namespace FbxSceneBuilder
    } // namespace SceneAPI
} // namespace AZ
