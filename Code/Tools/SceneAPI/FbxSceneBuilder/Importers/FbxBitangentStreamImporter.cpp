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
#include <SceneAPI/FbxSceneBuilder/Importers/FbxBitangentStreamImporter.h>
#include <SceneAPI/FbxSceneBuilder/Importers/FbxImporterUtilities.h>
#include <SceneAPI/FbxSceneBuilder/Importers/Utilities/RenamedNodesMap.h>
#include <SceneAPI/FbxSDKWrapper/FbxNodeWrapper.h>
#include <SceneAPI/FbxSDKWrapper/FbxVertexBitangentWrapper.h>
#include <SceneAPI/SceneData/GraphData/MeshData.h>
#include <SceneAPI/SceneData/GraphData/SkinMeshData.h>
#include <SceneAPI/SceneData/GraphData/MeshVertexBitangentData.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>
#include <SceneAPI/SceneCore/DataTypes/DataTypeUtilities.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace FbxSceneBuilder
        {
            FbxBitangentStreamImporter::FbxBitangentStreamImporter()
            {
                BindToCall(&FbxBitangentStreamImporter::ImportBitangents);
            }


            void FbxBitangentStreamImporter::Reflect(ReflectContext* context)
            {
                SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context);
                if (serializeContext)
                {
                    serializeContext->Class<FbxBitangentStreamImporter, SceneCore::LoadingComponent>()->Version(1);
                }
            }


            Events::ProcessingResult FbxBitangentStreamImporter::ImportBitangents(SceneNodeAppendedContext& context)
            {
                AZ_TraceContext("Importer", "Bitangents");
                std::shared_ptr<FbxSDKWrapper::FbxMeshWrapper> fbxMesh = context.m_sourceNode.GetMesh();
                if (!fbxMesh)
                {
                    return Events::ProcessingResult::Ignored;
                }

                Events::ProcessingResultCombiner combinedStreamResults;               
                const int numBitangentSets = context.m_sourceNode.GetMesh()->GetElementBitangentCount();
                for (int elementIndex = 0; elementIndex < numBitangentSets; ++elementIndex)
                {
                    AZ_TraceContext("Bitangent set index", elementIndex);

                    FbxSDKWrapper::FbxVertexBitangentWrapper fbxVertexBitangents = fbxMesh->GetElementBitangent(elementIndex);
                    if (!fbxVertexBitangents.IsValid())
                    {
                        AZ_TracePrintf(Utilities::WarningWindow, "Invalid bitangent set found, ignoring");
                        continue;
                    }

                    const AZStd::string originalNodeName = AZStd::string::format("BitangentSet_Fbx_%d", elementIndex);
                    const AZStd::string nodeName = AZ::SceneAPI::DataTypes::Utilities::CreateUniqueName<SceneData::GraphData::MeshVertexBitangentData>(originalNodeName, context.m_scene.GetManifest());
                    AZ_TraceContext("Bitangent Set Name", nodeName);
                    if (originalNodeName != nodeName)
                    {
                        AZ_TracePrintf(Utilities::WarningWindow, "Bitangent set '%s' has been renamed to '%s' because the name was already in use.", originalNodeName.c_str(), nodeName.c_str());
                    }
                    
                    AZStd::shared_ptr<DataTypes::IGraphObject> parentData = context.m_scene.GetGraph().GetNodeContent(context.m_currentGraphPosition);
                    AZ_Assert(parentData && parentData->RTTI_IsTypeOf(SceneData::GraphData::MeshData::TYPEINFO_Uuid()), "Tried to construct bitangent set attribute for invalid or non-mesh parent data");
                    if (!parentData || !parentData->RTTI_IsTypeOf(SceneData::GraphData::MeshData::TYPEINFO_Uuid()))
                    {
                        combinedStreamResults += Events::ProcessingResult::Failure;
                        continue;
                    }

                    const SceneData::GraphData::MeshData* const parentMeshData = azrtti_cast<SceneData::GraphData::MeshData*>(parentData.get());
                    const size_t vertexCount = parentMeshData->GetVertexCount();
                    AZStd::shared_ptr<SceneData::GraphData::MeshVertexBitangentData> bitangentStream = BuildVertexBitangentData(fbxVertexBitangents, vertexCount, fbxMesh);
                    
                    AZ_Assert(bitangentStream, "Failed to allocate bitangent data for scene graph.");
                    if (!bitangentStream)
                    {
                        combinedStreamResults += Events::ProcessingResult::Failure;
                        continue;
                    }

                    bitangentStream->SetBitangentSetIndex(elementIndex);
                    bitangentStream->SetTangentSpace(AZ::SceneAPI::DataTypes::TangentSpace::FromFbx);

                    Containers::SceneGraph::NodeIndex newIndex = context.m_scene.GetGraph().AddChild(context.m_currentGraphPosition, nodeName.c_str());
                    AZ_Assert(newIndex.IsValid(), "Failed to create SceneGraph node for attribute.");
                    if (!newIndex.IsValid())
                    {
                        combinedStreamResults += Events::ProcessingResult::Failure;
                        continue;
                    }

                    Events::ProcessingResult streamResults;
                    SceneAttributeDataPopulatedContext dataPopulated(context, bitangentStream, newIndex, nodeName);
                    streamResults = Events::Process(dataPopulated);
                    if (streamResults != Events::ProcessingResult::Failure)
                    {
                        streamResults = AddAttributeDataNodeWithContexts(dataPopulated);
                    }

                    combinedStreamResults += streamResults;
                }

                return combinedStreamResults.GetResult();
            }


            AZStd::shared_ptr<SceneData::GraphData::MeshVertexBitangentData> FbxBitangentStreamImporter::BuildVertexBitangentData(const FbxSDKWrapper::FbxVertexBitangentWrapper& bitangents, size_t vertexCount, const std::shared_ptr<FbxSDKWrapper::FbxMeshWrapper>& fbxMesh)
            {
                AZStd::shared_ptr<SceneData::GraphData::MeshVertexBitangentData> bitangentData = AZStd::make_shared<AZ::SceneData::GraphData::MeshVertexBitangentData>();
                bitangentData->ReserveContainerSpace(vertexCount);

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
                        
                        const Vector3 bitangent = bitangents.GetElementAt(fbxPolygonIndex, fbxPolygonVertexIndex, fbxControlPointIndex);
                        bitangentData->AppendBitangent(bitangent);
                    }
                }

                if (bitangentData->GetCount() != vertexCount)
                {
                    AZ_TracePrintf(Utilities::ErrorWindow, "Vertex count (%i) doesn't match the number of entries for the bitangent stream %s (%i)", vertexCount, bitangents.GetName(), bitangentData->GetCount());
                    return nullptr;
                }

                return bitangentData;
            }
        } // namespace FbxSceneBuilder
    } // namespace SceneAPI
} // namespace AZ
