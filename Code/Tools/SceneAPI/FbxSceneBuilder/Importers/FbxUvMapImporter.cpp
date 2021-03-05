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
#include <SceneAPI/FbxSceneBuilder/Importers/FbxUvMapImporter.h>
#include <SceneAPI/FbxSceneBuilder/Importers/FbxImporterUtilities.h>
#include <SceneAPI/FbxSceneBuilder/Importers/Utilities/RenamedNodesMap.h>
#include <SceneAPI/FbxSDKWrapper/FbxNodeWrapper.h>
#include <SceneAPI/FbxSDKWrapper/FbxUVWrapper.h>
#include <SceneAPI/SceneData/GraphData/MeshData.h>
#include <SceneAPI/SceneData/GraphData/SkinMeshData.h>
#include <SceneAPI/SceneData/GraphData/MeshVertexUVData.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace FbxSceneBuilder
        {
            FbxUvMapImporter::FbxUvMapImporter()
            {
                BindToCall(&FbxUvMapImporter::ImportUvMaps);
            }

            void FbxUvMapImporter::Reflect(ReflectContext* context)
            {
                SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context);
                if (serializeContext)
                {
                    serializeContext->Class<FbxUvMapImporter, SceneCore::LoadingComponent>()->Version(1);
                }
            }

            Events::ProcessingResult FbxUvMapImporter::ImportUvMaps(SceneNodeAppendedContext& context)
            {
                AZ_TraceContext("Importer", "UV Map");
                std::shared_ptr<FbxSDKWrapper::FbxMeshWrapper> fbxMesh =
                    context.m_sourceNode.GetMesh();
                if (!fbxMesh)
                {
                    return Events::ProcessingResult::Ignored;
                }

                Events::ProcessingResultCombiner combinedUvMapResults;

                for (int uvElementIndex = 0; uvElementIndex < context.m_sourceNode.GetMesh()->GetElementUVCount(); ++uvElementIndex)
                {
                    AZ_TraceContext("UV Map index", uvElementIndex);

                    FbxSDKWrapper::FbxUVWrapper fbxVertexUVs = fbxMesh->GetElementUV(uvElementIndex);

                    if (!fbxVertexUVs.IsValid())
                    {
                        AZ_TracePrintf(Utilities::WarningWindow, "Invalid UV Map found, ignoring");
                        continue;
                    }

                    AZStd::string nodeName = fbxVertexUVs.GetName();
                    RenamedNodesMap::SanitizeNodeName(nodeName, context.m_scene.GetGraph(), context.m_currentGraphPosition, "UV");
                    AZ_TraceContext("UV Map Name", nodeName);
                    
                    AZStd::shared_ptr<DataTypes::IGraphObject> parentData =
                        context.m_scene.GetGraph().GetNodeContent(context.m_currentGraphPosition);
                    AZ_Assert(parentData && parentData->RTTI_IsTypeOf(SceneData::GraphData::MeshData::TYPEINFO_Uuid()),
                        "Tried to construct uv stream attribute for invalid or non-mesh parent data");
                    if (!parentData || !parentData->RTTI_IsTypeOf(SceneData::GraphData::MeshData::TYPEINFO_Uuid()))
                    {
                        combinedUvMapResults += Events::ProcessingResult::Failure;
                        continue;
                    }

                    const SceneData::GraphData::MeshData* const parentMeshData =
                        azrtti_cast<SceneData::GraphData::MeshData*>(parentData.get());
                    size_t vertexCount = parentMeshData->GetVertexCount();

                    AZStd::shared_ptr<SceneData::GraphData::MeshVertexUVData> uvMap = BuildVertexUVData(fbxVertexUVs, vertexCount, fbxMesh);
                    
                    AZ_Assert(uvMap, "Failed to allocate UV map data for scene graph.");
                    if (!uvMap)
                    {
                        combinedUvMapResults += Events::ProcessingResult::Failure;
                        continue;
                    }

                    Containers::SceneGraph::NodeIndex newIndex =
                        context.m_scene.GetGraph().AddChild(context.m_currentGraphPosition, nodeName.c_str());

                    AZ_Assert(newIndex.IsValid(), "Failed to create SceneGraph node for attribute.");
                    if (!newIndex.IsValid())
                    {
                        combinedUvMapResults += Events::ProcessingResult::Failure;
                        continue;
                    }

                    Events::ProcessingResult uvMapResults;
                    SceneAttributeDataPopulatedContext dataPopulated(context, uvMap, newIndex, nodeName);
                    uvMapResults = Events::Process(dataPopulated);

                    if (uvMapResults != Events::ProcessingResult::Failure)
                    {
                        uvMapResults = AddAttributeDataNodeWithContexts(dataPopulated);
                    }

                    combinedUvMapResults += uvMapResults;
                }

                return combinedUvMapResults.GetResult();
            }

            AZStd::shared_ptr<SceneData::GraphData::MeshVertexUVData> FbxUvMapImporter::BuildVertexUVData(const FbxSDKWrapper::FbxUVWrapper& uvs,
                size_t vertexCount, const std::shared_ptr<FbxSDKWrapper::FbxMeshWrapper>& fbxMesh)
            {
                AZStd::shared_ptr<SceneData::GraphData::MeshVertexUVData> uvData = AZStd::make_shared<AZ::SceneData::GraphData::MeshVertexUVData>();
                uvData->ReserveContainerSpace(vertexCount);
                uvData->SetCustomName(uvs.GetName());

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

                    for (int uvIndex = 0; uvIndex < fbxPolygonVertexCount; ++uvIndex)
                    {
                        const int fbxPolygonVertexIndex = fbxVertexStartIndex + uvIndex;
                        const int fbxControlPointIndex = fbxPolygonVertices[fbxPolygonVertexIndex];

                        Vector2 uv = uvs.GetElementAt(fbxPolygonIndex, fbxPolygonVertexIndex, fbxControlPointIndex);
                        uv.SetY(1.0f - uv.GetY());
                        uvData->AppendUV(uv);
                    }
                }

                if (uvData->GetCount() != vertexCount)
                {
                    AZ_TracePrintf(Utilities::ErrorWindow, "Vertex count (%i) doesn't match the number of entries for the uv set %s (%i)",
                        vertexCount, uvs.GetName(), uvData->GetCount());
                    return nullptr;
                }

                return uvData;

            }
        } // namespace FbxSceneBuilder
    } // namespace SceneAPI
} // namespace AZ
