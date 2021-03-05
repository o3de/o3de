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

#include <Cry_Geo.h>
#include <IIndexedMesh.h>
#include <AzCore/Math/MathUtils.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzToolsFramework/Debug/TraceContext.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneCore/Containers/SceneGraph.h>
#include <SceneAPI/SceneCore/Containers/Utilities/Filters.h>
#include <SceneAPI/SceneCore/Containers/Views/SceneGraphChildIterator.h>
#include <SceneAPI/SceneCore/Containers/Views/SceneGraphDownwardsIterator.h>
#include <SceneAPI/SceneCore/Containers/Views/SceneGraphUpwardsIterator.h>
#include <SceneAPI/SceneCore/Containers/Views/PairIterator.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IBoneData.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/ISkinWeightData.h>
#include <SceneAPI/SceneCore/DataTypes/Groups/ISkinGroup.h>
#include <SceneAPI/SceneData/GraphData/MeshData.h>
#include <RC/ResourceCompilerScene/Common/CommonExportContexts.h>
#include <RC/ResourceCompilerScene/Common/SkinWeightExporter.h>

namespace AZ
{
    namespace RC
    {
        namespace SceneEvents = AZ::SceneAPI::Events;
        namespace SceneDataTypes = AZ::SceneAPI::DataTypes;
        namespace SceneContainers = AZ::SceneAPI::Containers;
        namespace SceneUtils = AZ::SceneAPI::Utilities;
        namespace SceneViews = SceneContainers::Views;

        SkinWeightExporter::SkinWeightExporter()
        {
            BindToCall(&SkinWeightExporter::ResolveRootBoneFromNode);
            BindToCall(&SkinWeightExporter::ProcessSkinWeights);
            BindToCall(&SkinWeightExporter::ProcessTouchBendableSkinWeights);
        }

        void SkinWeightExporter::Reflect(ReflectContext* context)
        {
            SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context);
            if (serializeContext)
            {
                serializeContext->Class<SkinWeightExporter, SceneAPI::SceneCore::RCExportingComponent>()->Version(1);
            }
        }

        SceneEvents::ProcessingResult SkinWeightExporter::ResolveRootBoneFromNode(ResolveRootBoneFromNodeContext& context)
        {
            using namespace SceneContainers;
            using namespace SceneContainers::Views;
            using namespace SceneDataTypes;
            using namespace SceneEvents;
            
            const SceneContainers::SceneGraph& graph = context.m_scene.GetGraph();
            auto attributeView = MakeSceneGraphChildView<AcceptEndPointsOnly>(graph, context.m_nodeIndex, graph.GetContentStorage().begin(), true);
            auto weights = AZStd::find_if(attributeView.begin(), attributeView.end(), DerivedTypeFilter<ISkinWeightData>());
            if (weights == attributeView.end() || azrtti_cast<const SceneDataTypes::ISkinWeightData*>(*weights)->GetBoneCount() == 0)
            {
                AZ_TracePrintf(SceneUtils::WarningWindow, "No skin weight data, skin weight data ignored.");
                return SceneEvents::ProcessingResult::Ignored;
            }

            const AZStd::string& boneName = azrtti_cast<const SceneDataTypes::ISkinWeightData*>(*weights)->GetBoneName(0);
            AZ_TraceContext("Bone name", boneName);

            ProcessingResult result = SceneEvents::Process<ResolveRootBoneFromBoneContext>(context.m_rootBoneName, context.m_scene, boneName);
            if (result == ProcessingResult::Ignored)
            {
                AZ_TracePrintf(SceneUtils::WarningWindow, "No system registered that can resolve bone names.");
            }
            else if (result == ProcessingResult::Failure)
            {
                AZ_TracePrintf(SceneUtils::ErrorWindow, "Failed to resolve skeleton from bone.");
            }
            return result;
        }

        SceneEvents::ProcessingResult SkinWeightExporter::ProcessSkinWeights(MeshNodeExportContext& context)
        {
            if (context.m_phase != Phase::Filling || !context.m_group.RTTI_IsTypeOf(AZ::SceneAPI::DataTypes::ISkinGroup::TYPEINFO_Uuid()))
            {
                return SceneEvents::ProcessingResult::Ignored;
            }

            AZ_TraceContext("Root bone", context.m_rootBoneName);

            BoneNameIdMap boneNameIdMap;
            SceneEvents::ProcessingResult result = SceneAPI::Events::Process<BuildBoneMapContext>(context.m_scene, context.m_rootBoneName, boneNameIdMap);
            if (result == SceneEvents::ProcessingResult::Ignored)
            {
                AZ_TracePrintf(SceneUtils::WarningWindow, "No system registered that can handle skeletons for skins.");
                return SceneEvents::ProcessingResult::Ignored;
            }
            else if (result == SceneEvents::ProcessingResult::Failure)
            {
                AZ_TracePrintf(SceneUtils::ErrorWindow, "Failed to load bone mapping for skin.");
                return SceneEvents::ProcessingResult::Failure;
            }

            SetSkinWeights(context, boneNameIdMap);
            return SceneEvents::ProcessingResult::Success;
        }

        SceneEvents::ProcessingResult SkinWeightExporter::ProcessTouchBendableSkinWeights(TouchBendableMeshNodeExportContext& context)
        {
            if (context.m_phase != Phase::Filling)
            {
                return SceneEvents::ProcessingResult::Ignored;
            }

            AZ_TraceContext("Root bone", context.m_rootBoneName);

            BoneNameIdMap boneNameIdMap;
            SceneEvents::ProcessingResult result = SceneAPI::Events::Process<BuildBoneMapContext>(context.m_scene, context.m_rootBoneName, boneNameIdMap);
            if (result == SceneEvents::ProcessingResult::Ignored)
            {
                AZ_TracePrintf(SceneUtils::WarningWindow, "No system registered that can handle skeletons for skins.");
                return SceneEvents::ProcessingResult::Ignored;
            }
            else if (result == SceneEvents::ProcessingResult::Failure)
            {
                AZ_TracePrintf(SceneUtils::ErrorWindow, "Failed to load bone mapping for skin.");
                return SceneEvents::ProcessingResult::Failure;
            }

            SetSkinWeights(context, boneNameIdMap);
            return SceneEvents::ProcessingResult::Success;
        }

        void SkinWeightExporter::SetSkinWeights(MeshNodeExportContext& context, BoneNameIdMap boneNameIdMap)
        {
            AZStd::shared_ptr<const SceneDataTypes::ISkinWeightData> skinWeights = nullptr;
            AZStd::shared_ptr<const SceneData::GraphData::MeshData> meshData = nullptr;
            const SceneContainers::SceneGraph& graph = context.m_scene.GetGraph();
            SceneContainers::SceneGraph::NodeIndex index = graph.GetNodeChild(context.m_nodeIndex);
            while (index.IsValid())
            {
                skinWeights = azrtti_cast<const SceneDataTypes::ISkinWeightData*>(graph.GetNodeContent(index));
                if (skinWeights)
                {
                    // Support first set of skin weights for now
                    auto parentIndex = graph.GetNodeParent(index);
                    if (parentIndex.IsValid())
                    {
                        meshData = azrtti_cast<const SceneData::GraphData::MeshData*>(graph.GetNodeContent(parentIndex));  
                    }
                    else
                    {
                        AZ_TracePrintf(SceneUtils::WarningWindow, "Invalid mesh parent data for skin weights data");
                    }
                    break;
                }
                index = graph.GetNodeSibling(index);
            }

            if (skinWeights)
            {
                if (skinWeights->GetVertexCount() == 0)
                {
                    AZ_TracePrintf(SceneUtils::WarningWindow, "Empty skin weight data, skin weight data ignored.");
                    return;
                }                
                
                bool hasExtraWeights = false;
                for (size_t vertexIndex = 0; vertexIndex < skinWeights->GetVertexCount(); ++vertexIndex)
                {
                    if (skinWeights->GetLinkCount(vertexIndex) > 4)
                    {
                        hasExtraWeights = true;
                        break;
                    }
                }

                context.m_mesh.ReallocStream(CMesh::BONEMAPPING, 0, context.m_mesh.GetVertexCount());
                if (hasExtraWeights)
                {
                    context.m_mesh.ReallocStream(CMesh::EXTRABONEMAPPING, 0, context.m_mesh.GetVertexCount());
                }

                for (size_t vertexIndex = 0; vertexIndex < context.m_mesh.GetVertexCount(); ++vertexIndex)
                {
                    int controlPointIndex = meshData->GetControlPointIndex(vertexIndex);
                    size_t linkCount = skinWeights->GetLinkCount(controlPointIndex);
                    for (size_t linkIndex = 0; linkIndex < 4 && linkIndex < linkCount; ++linkIndex)
                    {
                        const SceneDataTypes::ISkinWeightData::Link& link = skinWeights->GetLink(controlPointIndex, linkIndex);
                        context.m_mesh.m_pBoneMapping[vertexIndex].weights[linkIndex] = aznumeric_caster(GetClamp<float>(255.0f*link.weight, 0.0f, 255.0f));
                        context.m_mesh.m_pBoneMapping[vertexIndex].boneIds[linkIndex] = 
                            aznumeric_caster(GetGlobalBoneId(skinWeights, boneNameIdMap, link.boneId));
                    }

                    if (hasExtraWeights)
                    {
                        for (size_t linkIndex = 4; linkIndex < 8 && linkIndex < linkCount; ++linkIndex)
                        {
                            const SceneDataTypes::ISkinWeightData::Link& link = skinWeights->GetLink(controlPointIndex, linkIndex);
                            context.m_mesh.m_pExtraBoneMapping[vertexIndex].weights[linkIndex - 4] = aznumeric_caster(GetClamp<float>(255.0f*link.weight, 0.0f, 255.0f));
                            context.m_mesh.m_pExtraBoneMapping[vertexIndex].boneIds[linkIndex - 4] = 
                                aznumeric_caster(GetGlobalBoneId(skinWeights, boneNameIdMap, link.boneId));
                        }
                    }
                }
            }
        }

        int SkinWeightExporter::GetGlobalBoneId(
            const AZStd::shared_ptr<const SceneDataTypes::ISkinWeightData>& skinWeights, BoneNameIdMap boneNameIdMap, int boneId)
        {
            AZ_TraceContext("Bone id", boneId);
            const AZStd::string& boneName = skinWeights->GetBoneName(boneId);
            AZ_TraceContext("Bone name", boneName);

            if (boneName.empty())
            {
                AZ_TracePrintf(SceneUtils::WarningWindow, "Invalid local bone id referenced in skin weight data");
                return -1;
            }
            
            auto it = boneNameIdMap.find(boneName);
            if (it == boneNameIdMap.end())
            {
                AZ_TracePrintf(SceneUtils::WarningWindow, "Local bone name referenced in skin weight data doesn't exist in global bone map");
                return -1;
            }
            return it->second;
        }
    } // namespace RC
} // namespace AZ
