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
#include <AzCore/std/string/conversions.h>
#include <AzToolsFramework/Debug/TraceContext.h>
#include <SceneAPI/FbxSceneBuilder/Importers/FbxBlendShapeImporter.h>
#include <SceneAPI/FbxSceneBuilder/Importers/FbxImporterUtilities.h>
#include <SceneAPI/FbxSceneBuilder/Importers/Utilities/RenamedNodesMap.h>
#include <SceneAPI/FbxSceneBuilder/Importers/Utilities/FbxMeshImporterUtilities.h>
#include <SceneAPI/FbxSceneBuilder/FbxSceneSystem.h>
#include <SceneAPI/FbxSDKWrapper/FbxNodeWrapper.h>
#include <SceneAPI/FbxSDKWrapper/FbxMeshWrapper.h>
#include <SceneAPI/FbxSDKWrapper/FbxBlendShapeWrapper.h>
#include <SceneAPI/FbxSDKWrapper/FbxBlendShapeChannelWrapper.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>
#include <SceneAPI/SceneData/GraphData/MeshData.h>
#include <SceneAPI/SceneData/GraphData/SkinMeshData.h>
#include <SceneAPI/SceneData/GraphData/BlendShapeData.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace FbxSceneBuilder
        {
            FbxBlendShapeImporter::FbxBlendShapeImporter()
            {
                BindToCall(&FbxBlendShapeImporter::ImportBlendShapes);
            }

            void FbxBlendShapeImporter::Reflect(ReflectContext* context)
            {
                SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context);
                if (serializeContext)
                {
                    serializeContext->Class<FbxBlendShapeImporter, SceneCore::LoadingComponent>()->Version(1);
                }
            }

            Events::ProcessingResult FbxBlendShapeImporter::ImportBlendShapes(SceneNodeAppendedContext& context)
            {
                AZ_TraceContext("Importer", "Blend Shapes");

                if (!IsSkinnedMesh(context.m_sourceNode))
                {
                    return Events::ProcessingResult::Ignored;
                }

                Events::ProcessingResultCombiner combinedBlendShapeResult;

                const std::shared_ptr<FbxSDKWrapper::FbxMeshWrapper> sourceMesh = context.m_sourceNode.GetMesh();
                int blendShapeDeformerCount = sourceMesh->GetDeformerCount(FbxDeformer::eBlendShape);
                for (int deformerIndex = 0; deformerIndex < blendShapeDeformerCount; ++deformerIndex)
                {
                    AZ_TraceContext("Deformer Index", deformerIndex);
                    AZStd::shared_ptr<const FbxSDKWrapper::FbxBlendShapeWrapper> fbxBlendShape = sourceMesh->GetBlendShape(deformerIndex);

                    if (!fbxBlendShape)
                    {
                        AZ_TracePrintf(SceneAPI::Utilities::ErrorWindow, "Unable to extract BlendShape Deformer at index %d", deformerIndex);
                        return Events::ProcessingResult::Failure;
                    }
                    int blendShapeChannelCount = fbxBlendShape->GetBlendShapeChannelCount();
                    for (int channelIndex = 0; channelIndex < blendShapeChannelCount; ++channelIndex)
                    {
                        //extract the mesh and build a blendshape data object.
                        AZStd::shared_ptr<const FbxSDKWrapper::FbxBlendShapeChannelWrapper> blendShapeChannel = fbxBlendShape->GetBlendShapeChannel(channelIndex);

                        int shapeCount = blendShapeChannel->GetTargetShapeCount();

                        //We do not support percentage blends at this time. Take only the final shape. 
                        AZStd::shared_ptr<const FbxSDKWrapper::FbxMeshWrapper> mesh = blendShapeChannel->GetTargetShape(shapeCount - 1);

                        if (mesh)
                        {
                            //Maya is creating node names of the form cone_skin_blendShapeNode.cone_squash during export. 
                            //We need the name after the period for our naming purposes. 
                            AZStd::string nodeName(blendShapeChannel->GetName());
                            size_t dotIndex = nodeName.rfind('.');
                            if (dotIndex != AZStd::string::npos)
                            {
                                nodeName.erase(0, dotIndex + 1);
                            }
                            RenamedNodesMap::SanitizeNodeName(nodeName, context.m_scene.GetGraph(), context.m_currentGraphPosition, "BlendShape");
                            AZ_TraceContext("Blend shape name", nodeName);

                            AZStd::shared_ptr<SceneData::GraphData::BlendShapeData> blendShapeData =
                                AZStd::make_shared<SceneData::GraphData::BlendShapeData>();

                            BuildSceneBlendShapeFromFbxBlendShape(blendShapeData, mesh, context.m_sourceSceneSystem);

                            Containers::SceneGraph::NodeIndex newIndex =
                                context.m_scene.GetGraph().AddChild(context.m_currentGraphPosition, nodeName.c_str());

                            Events::ProcessingResult blendShapeResult;
                            SceneAttributeDataPopulatedContext dataPopulated(context, blendShapeData, newIndex, nodeName);
                            blendShapeResult = Events::Process(dataPopulated);

                            if (blendShapeResult != Events::ProcessingResult::Failure)
                            {
                                blendShapeResult = AddAttributeDataNodeWithContexts(dataPopulated);
                            }
                            combinedBlendShapeResult += blendShapeResult;
                        }
                        else
                        {
                            AZ_TracePrintf(SceneAPI::Utilities::ErrorWindow, "Unable to extract blendshape mesh for node '%s' from BlendShapeChannel %d", sourceMesh->GetName(), channelIndex);
                            combinedBlendShapeResult += Events::ProcessingResult::Failure;
                        }
                    }
                }

                return combinedBlendShapeResult.GetResult();
            }
        } // namespace FbxSceneBuilder
    } // namespace SceneAPI
} // namespace AZ