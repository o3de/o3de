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
#include <SceneAPI/FbxSceneBuilder/Importers/FbxColorStreamImporter.h>
#include <SceneAPI/FbxSceneBuilder/Importers/FbxImporterUtilities.h>
#include <SceneAPI/FbxSceneBuilder/Importers/Utilities/RenamedNodesMap.h>
#include <SceneAPI/FbxSDKWrapper/FbxNodeWrapper.h>
#include <SceneAPI/FbxSDKWrapper/FbxVertexColorWrapper.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>
#include <SceneAPI/SceneData/GraphData/MeshData.h>
#include <SceneAPI/SceneData/GraphData/SkinMeshData.h>
#include <SceneAPI/SceneData/GraphData/MeshVertexColorData.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace FbxSceneBuilder
        {
            FbxColorStreamImporter::FbxColorStreamImporter()
            {
                BindToCall(&FbxColorStreamImporter::ImportColorStreams);
            }

            void FbxColorStreamImporter::Reflect(ReflectContext* context)
            {
                SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context);
                if (serializeContext)
                {
                    serializeContext->Class<FbxColorStreamImporter, SceneCore::LoadingComponent>()->Version(1);
                }
            }

            Events::ProcessingResult FbxColorStreamImporter::ImportColorStreams(SceneNodeAppendedContext& context)
            {
                AZ_TraceContext("Importer", "Color Stream");
                std::shared_ptr<FbxSDKWrapper::FbxMeshWrapper> fbxMesh =
                        context.m_sourceNode.GetMesh();
                if (!fbxMesh)
                {
                    return Events::ProcessingResult::Ignored;
                }

                Events::ProcessingResultCombiner combinedVertexColorResults;

                for (int i = 0; i < context.m_sourceNode.GetMesh()->GetElementVertexColorCount(); ++i)
                {
                    AZ_TraceContext("Vertex color index", i);

                    FbxSDKWrapper::FbxVertexColorWrapper fbxVertexColors = 
                        fbxMesh->GetElementVertexColor(i);

                    if (!fbxVertexColors.IsValid())
                    {
                        AZ_TracePrintf(Utilities::WarningWindow, "Invalid vertex color channel found, ignoring");
                        continue;
                    }

                    AZStd::string nodeName = fbxVertexColors.GetName();
                    RenamedNodesMap::SanitizeNodeName(nodeName, context.m_scene.GetGraph(), context.m_currentGraphPosition, "ColorStream");
                    AZ_TraceContext("Color Stream Name", nodeName);

                    AZStd::shared_ptr<DataTypes::IGraphObject> parentData = 
                        context.m_scene.GetGraph().GetNodeContent(context.m_currentGraphPosition);
                    AZ_Assert(parentData && parentData->RTTI_IsTypeOf(SceneData::GraphData::MeshData::TYPEINFO_Uuid()),
                        "Tried to construct color stream attribute for invalid or non-mesh parent data");
                    if (!parentData || !parentData->RTTI_IsTypeOf(SceneData::GraphData::MeshData::TYPEINFO_Uuid()))
                    {
                        combinedVertexColorResults += Events::ProcessingResult::Failure;
                        continue;
                    }

                    SceneData::GraphData::MeshData* parentMeshData = 
                        azrtti_cast<SceneData::GraphData::MeshData*>(parentData.get());
                    size_t vertexCount = parentMeshData->GetVertexCount();
                    AZStd::shared_ptr<SceneData::GraphData::MeshVertexColorData> vertexColors =
                        BuildVertexColorData(fbxVertexColors, vertexCount, fbxMesh);
                    AZ_Assert(vertexColors, "Failed to allocate vertex color data for scene graph.");
                    if (!vertexColors)
                    {
                        combinedVertexColorResults += Events::ProcessingResult::Failure;
                        continue;
                    }

                    Containers::SceneGraph::NodeIndex newIndex =
                        context.m_scene.GetGraph().AddChild(context.m_currentGraphPosition, nodeName.c_str());

                    AZ_Assert(newIndex.IsValid(), "Failed to create SceneGraph node for attribute.");
                    if (!newIndex.IsValid())
                    {
                        combinedVertexColorResults += Events::ProcessingResult::Failure;
                        continue;
                    }

                    Events::ProcessingResult vertexColorResult;
                    SceneAttributeDataPopulatedContext dataPopulated(context, vertexColors, newIndex, nodeName);
                    vertexColorResult = AddAttributeDataNodeWithContexts(dataPopulated);

                    combinedVertexColorResults += vertexColorResult;
                }

                return combinedVertexColorResults.GetResult();
            }

            AZStd::shared_ptr<SceneData::GraphData::MeshVertexColorData> FbxColorStreamImporter::BuildVertexColorData(const FbxSDKWrapper::FbxVertexColorWrapper& fbxVertexColors, size_t vertexCount, const std::shared_ptr<FbxSDKWrapper::FbxMeshWrapper>& fbxMesh)
            {
                AZ_Assert(fbxVertexColors.IsValid(), "BuildVertexColorData was called for invalid color stream data.");
                if (!fbxVertexColors.IsValid())
                {
                    return nullptr;
                }

                AZStd::shared_ptr<SceneData::GraphData::MeshVertexColorData> colorData = AZStd::make_shared<AZ::SceneData::GraphData::MeshVertexColorData>();
                colorData->ReserveContainerSpace(vertexCount);
                colorData->SetCustomName(fbxVertexColors.GetName());

                const int fbxPolygonCount = fbxMesh->GetPolygonCount();
                const int* const fbxPolygonVertices = fbxMesh->GetPolygonVertices();
                for (int fbxPolygonIndex = 0; fbxPolygonIndex < fbxPolygonCount; ++fbxPolygonIndex)
                {
                    const int fbxPolygonVertexCount = fbxMesh->GetPolygonSize(fbxPolygonIndex);
                    if (fbxPolygonVertexCount < 3)
                    {
                        continue;
                    }

                    const int fbxVertexStartIndex = fbxMesh->GetPolygonVertexIndex(fbxPolygonIndex);

                    for (int polygonVertexIndex = 0; polygonVertexIndex < fbxPolygonVertexCount; ++polygonVertexIndex)
                    {
                        const int fbxPolygonVertexIndex = fbxVertexStartIndex + polygonVertexIndex;
                        const int fbxControlPointIndex = fbxPolygonVertices[fbxPolygonVertexIndex];

                        FbxSDKWrapper::FbxColorWrapper color = fbxVertexColors.GetElementAt(fbxPolygonIndex, fbxPolygonVertexIndex, fbxControlPointIndex);

                        colorData->AppendColor({color.GetR(), color.GetG(), color.GetB(), color.GetAlpha()});
                    }
                }

                if (colorData->GetCount() != vertexCount)
                {
                    AZ_TracePrintf(Utilities::ErrorWindow, "Vertex count (%i) doesn't match the number of entries for the vertex color stream %s (%i)",
                        vertexCount, fbxVertexColors.GetName(), colorData->GetCount());
                    return nullptr;
                }

                return colorData;
            }
        } // namespace FbxSceneBuilder
    } // namespace SceneAPI
} // namespace AZ
