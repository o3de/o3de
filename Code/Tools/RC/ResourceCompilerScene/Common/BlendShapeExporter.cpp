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
#include <CGFContent.h>
#include <IIndexedMesh.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Math/MathUtils.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/algorithm.h>
#include <AzToolsFramework/Debug/TraceContext.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>
#include <SceneAPI/SceneCore/Utilities/SceneGraphSelector.h>
#include <SceneAPI/SceneCore/DataTypes/DataTypeUtilities.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneCore/Containers/SceneGraph.h>
#include <SceneAPI/SceneCore/Containers/Utilities/Filters.h>
#include <SceneAPI/SceneCore/Containers/Views/SceneGraphDownwardsIterator.h>
#include <SceneAPI/SceneCore/Containers/Views/SceneGraphUpwardsIterator.h>
#include <SceneAPI/SceneCore/Containers/Views/SceneGraphChildIterator.h>
#include <SceneAPI/SceneCore/Containers/Views/PairIterator.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IBlendShapeData.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/ITransform.h>
#include <SceneAPI/SceneCore/DataTypes/Groups/ISkinGroup.h>
#include <SceneAPI/SceneCore/DataTypes/Rules/IBlendShapeRule.h>
#include <RC/ResourceCompilerScene/Common/CommonExportContexts.h>
#include <RC/ResourceCompilerScene/Common/BlendShapeExporter.h>

namespace AZ
{
    namespace RC
    {
        BlendShapeExporter::BlendShapeExporter()
        {
            BindToCall(&BlendShapeExporter::ProcessBlendShapes);
        }

        void BlendShapeExporter::Reflect(ReflectContext* context)
        {
            SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context);
            if (serializeContext)
            {
                serializeContext->Class<BlendShapeExporter, SceneAPI::SceneCore::RCExportingComponent>()->Version(1);
            }
        }

        SceneAPI::Events::ProcessingResult BlendShapeExporter::ProcessBlendShapes(MeshNodeExportContext& context)
        {
            if (context.m_phase != Phase::Filling)
            {
                return SceneAPI::Events::ProcessingResult::Ignored;
            }

            if (!context.m_group.RTTI_IsTypeOf(AZ::SceneAPI::DataTypes::ISkinGroup::TYPEINFO_Uuid()))
            {
                return SceneAPI::Events::ProcessingResult::Ignored;
            }

            AZStd::shared_ptr<const AZ::SceneAPI::DataTypes::IBlendShapeRule> blendShapeRule = context.m_group.GetRuleContainerConst().FindFirstByType<AZ::SceneAPI::DataTypes::IBlendShapeRule>();
            if (!blendShapeRule)
            {
                return SceneAPI::Events::ProcessingResult::Ignored;
            }

            const SceneAPI::Containers::SceneGraph& graph = context.m_scene.GetGraph();

            CSkinningInfo* skinInfo = context.m_container.GetSkinningInfo();

            for (size_t index = 0; index < blendShapeRule->GetSceneNodeSelectionList().GetSelectedNodeCount(); ++index)
            {
                SceneAPI::Containers::SceneGraph::NodeIndex nodeIndex = graph.Find(blendShapeRule->GetSceneNodeSelectionList().GetSelectedNode(index));

                if (!nodeIndex.IsValid())
                {
                    AZ_TracePrintf(SceneAPI::Utilities::ErrorWindow, "Invalid name %s for blend shape.", blendShapeRule->GetSceneNodeSelectionList().GetSelectedNode(index).c_str());
                    return SceneAPI::Events::ProcessingResult::Failure;
                }

                AZStd::shared_ptr<const SceneAPI::DataTypes::IBlendShapeData> blendShape =
                    azrtti_cast<const SceneAPI::DataTypes::IBlendShapeData*>(graph.GetNodeContent(nodeIndex));


                if (!blendShape)
                {
                    AZ_TracePrintf(SceneAPI::Utilities::ErrorWindow, "Unable to find blend shape.");
                    return SceneAPI::Events::ProcessingResult::Failure;
                }

                SceneAPI::DataTypes::MatrixType skinTransform = SceneAPI::DataTypes::MatrixType::Identity();
                //Check to see if the blend shape parent skin has a transform an propagate that transform onto the vertices. 
                auto view = MakeSceneGraphChildView(graph, graph.GetNodeParent(nodeIndex), graph.GetContentStorage().begin(), true);
                auto transform = AZStd::find_if(view.begin(), view.end(), SceneAPI::Containers::DerivedTypeFilter<SceneAPI::DataTypes::ITransform>());

                if (transform != view.end())
                {
                    skinTransform = azrtti_cast<const SceneAPI::DataTypes::ITransform*>(*transform)->GetMatrix();
                }

                MorphTargets* target = new MorphTargets();
                target->MeshID = -1; //Based on the collada importer there's not a great way to set this.
                target->m_strName = string(graph.GetNodeName(nodeIndex).GetName());

                const size_t controlPointCount = blendShape->GetUsedControlPointCount();

                for (size_t controlPointIndex = 0; controlPointIndex < controlPointCount; ++controlPointIndex)
                {
                    SMeshMorphTargetVertex vert;
                    vert.nVertexId = controlPointIndex;
                    
                    AZ::Vector3 vtx = blendShape->GetPosition(blendShape->GetUsedPointIndexForControlPoint(controlPointIndex));

                    //Apply base skin transform if one exists.
                    vtx = skinTransform * vtx;

                    vert.ptVertex = Vec3(vtx.GetX(), vtx.GetY(), vtx.GetZ());
                    target->m_arrIntMorph.push_back(vert);
                }

                skinInfo->m_arrMorphTargets.push_back(target);
            }

            return SceneAPI::Events::ProcessingResult::Success;

        }
    } // namespace RC
} // namespace AZ
