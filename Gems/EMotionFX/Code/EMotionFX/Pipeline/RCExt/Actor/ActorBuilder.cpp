
/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <SceneAPI/SceneCore/Containers/Views/PairIterator.h>
#include <SceneAPI/SceneCore/Containers/Views/SceneGraphDownwardsIterator.h>
#include <SceneAPI/SceneCore/Containers/Views/SceneGraphChildIterator.h>
#include <SceneAPI/SceneCore/Containers/Utilities/Filters.h>
#include <SceneAPI/SceneCore/Containers/Utilities/SceneGraphUtilities.h>
#include <SceneAPI/SceneCore/Utilities/FileUtilities.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IBoneData.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IMeshData.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/ITransform.h>
#include <SceneAPI/SceneCore/DataTypes/DataTypeUtilities.h>
#include <SceneAPI/SceneData/Rules/CoordinateSystemRule.h>

#include <SceneAPIExt/Rules/LodRule.h>
#include <SceneAPIExt/Rules/SkeletonOptimizationRule.h>
#include <SceneAPIExt/Groups/ActorGroup.h>
#include <RCExt/Actor/ActorBuilder.h>
#include <RCExt/ExportContexts.h>

#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/Node.h>
#include <EMotionFX/Exporters/ExporterLib/Exporter/Exporter.h>
#include <MCore/Source/AzCoreConversions.h>

#include <AzCore/Math/MathUtils.h>
#include <AzCore/Math/Matrix3x4.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/Interface/Interface.h>
#include <AzFramework/Application/Application.h>
#include <AzToolsFramework/Debug/TraceContext.h>

namespace EMotionFX
{
    namespace Pipeline
    {
        namespace SceneEvents = AZ::SceneAPI::Events;
        namespace SceneUtil = AZ::SceneAPI::Utilities;
        namespace SceneContainers = AZ::SceneAPI::Containers;
        namespace SceneViews = AZ::SceneAPI::Containers::Views;
        namespace SceneDataTypes = AZ::SceneAPI::DataTypes;

        EMotionFX::Transform SceneDataMatrixToEmfxTransformConverted(
            const SceneDataTypes::MatrixType& azTransform, const AZ::SceneAPI::CoordinateSystemConverter& coordSysConverter)
        {
            EMotionFX::Transform transform;
            transform.InitFromAZTransform(AZ::Transform::CreateFromMatrix3x4(coordSysConverter.ConvertMatrix3x4(azTransform)));
            return transform;
        }


        ActorBuilder::ActorBuilder()
        {
            BindToCall(&ActorBuilder::BuildActor);
        }


        void ActorBuilder::Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (serializeContext)
            {
                serializeContext->Class<ActorBuilder, AZ::SceneAPI::SceneCore::ExportingComponent>()->Version(1);
            }
        }

        SceneEvents::ProcessingResult ActorBuilder::BuildActor(ActorBuilderContext& context)
        {
            if (context.m_phase != AZ::RC::Phase::Filling)
            {
                return SceneEvents::ProcessingResult::Ignored;
            }

            const SceneContainers::SceneGraph& graph = context.m_scene.GetGraph();

            const Group::IActorGroup& actorGroup = context.m_group;
            const char* rootBoneName = actorGroup.GetSelectedRootBone().c_str();
            AZ_TraceContext("Root bone", rootBoneName);

            SceneContainers::SceneGraph::NodeIndex rootBoneNodeIndex = graph.Find(rootBoneName);
            if (!rootBoneNodeIndex.IsValid())
            {
                AZ_Trace(SceneUtil::ErrorWindow, "Root bone cannot be found.\n");
                return SceneEvents::ProcessingResult::Failure;
            }

            // Get the coordinate system conversion rule.
            AZ::SceneAPI::CoordinateSystemConverter coordSysConverter;
            AZStd::shared_ptr<AZ::SceneAPI::SceneData::CoordinateSystemRule> coordinateSystemRule = actorGroup.GetRuleContainerConst().FindFirstByType<AZ::SceneAPI::SceneData::CoordinateSystemRule>();
            if (coordinateSystemRule)
            {
                coordinateSystemRule->UpdateCoordinateSystemConverter();
                coordSysConverter = coordinateSystemRule->GetCoordinateSystemConverter();
            }

            // Collect the node indices that emfx cares about and construct the boneNameEmfxIndex map for quick search.
            AZStd::vector<SceneContainers::SceneGraph::NodeIndex> nodeIndices; // Include both bone and mesh nodes.
            AZStd::vector<SceneContainers::SceneGraph::NodeIndex> meshIndices;
            BoneNameEmfxIndexMap boneNameEmfxIndexMap;
            BuildPreExportStructure(context, rootBoneNodeIndex, nodeIndices, meshIndices, boneNameEmfxIndexMap);

            EMotionFX::Actor* actor = context.m_actor;
            EMotionFX::Skeleton* actorSkeleton = actor->GetSkeleton();
            const AZ::u32 exfxNodeCount = aznumeric_cast<AZ::u32>(nodeIndices.size());
            actor->SetNumNodes(aznumeric_cast<AZ::u32>(exfxNodeCount));
            actor->ResizeTransformData();

            EMotionFX::Pose* bindPose = actor->GetBindPose();
            AZ_Assert(bindPose, "BindPose not available for actor");
            AZStd::unordered_map<const SceneContainers::SceneGraph::NodeIndex::IndexType, EMotionFX::Node*> actorSkeletonLookup;
            for (AZ::u32 emfxNodeIndex = 0; emfxNodeIndex < exfxNodeCount; ++emfxNodeIndex)
            {
                const SceneContainers::SceneGraph::NodeIndex& nodeIndex = nodeIndices[emfxNodeIndex];
                const SceneContainers::SceneGraph::Name& graphNodeName = graph.GetNodeName(nodeIndex);
                const char* nodeName = graphNodeName.GetName();
                EMotionFX::Node* emfxNode = EMotionFX::Node::Create(nodeName, actorSkeleton);

                emfxNode->SetNodeIndex(emfxNodeIndex);

                // Add the emfx node to the actor
                actorSkeleton->SetNode(emfxNodeIndex, emfxNode);
                actorSkeletonLookup.insert(AZStd::make_pair(nodeIndex.AsNumber(), emfxNode));

                // Set the parent, and add this node as child inside the parent
                // Only if this node has a parent and the parent node is valid
                EMotionFX::Node* emfxParentNode = nullptr;
                if (graph.HasNodeParent(nodeIndex) && graph.GetNodeParent(nodeIndex) != graph.GetRoot())
                {
                    const SceneContainers::SceneGraph::NodeIndex nodeParent = graph.GetNodeParent(nodeIndex);
                    if (actorSkeletonLookup.find(nodeParent.AsNumber()) != actorSkeletonLookup.end())
                    {
                        emfxParentNode = actorSkeletonLookup[nodeParent.AsNumber()];
                    }
                }

                if (emfxParentNode)
                {
                    emfxNode->SetParentIndex(emfxParentNode->GetNodeIndex());
                    emfxParentNode->AddChild(emfxNodeIndex);
                }
                else
                {
                    // If this node has no parent, then it should be added as root node
                    actorSkeleton->AddRootNode(emfxNodeIndex);
                }

                // Set the decomposed bind pose local transformation
                EMotionFX::Transform outTransform = EMotionFX::Transform::CreateIdentity();
                auto view = SceneViews::MakeSceneGraphChildView<SceneViews::AcceptEndPointsOnly>(graph, nodeIndex, graph.GetContentStorage().begin(), true);
                auto result = AZStd::find_if(view.begin(), view.end(), SceneContainers::DerivedTypeFilter<SceneDataTypes::ITransform>());
                if (result != view.end())
                {
                    // Check if the node contain any transform node.
                    const SceneDataTypes::MatrixType azTransform = azrtti_cast<const SceneDataTypes::ITransform*>(result->get())->GetMatrix();
                    outTransform = SceneDataMatrixToEmfxTransformConverted(azTransform, coordSysConverter);
                }
                else
                {
                    // This node itself could be a transform node.
                    AZStd::shared_ptr<const SceneDataTypes::ITransform> transformData = azrtti_cast<const SceneDataTypes::ITransform*>(graph.GetNodeContent(nodeIndex));
                    if (transformData)
                    {
                        outTransform = SceneDataMatrixToEmfxTransformConverted(transformData->GetMatrix(), coordSysConverter);
                    }
                }
                bindPose->SetLocalSpaceTransform(emfxNodeIndex, outTransform);
            }

            // Add LOD Level to actor.
            const AZ::SceneAPI::Containers::RuleContainer& rules = actorGroup.GetRuleContainerConst();
            AZStd::shared_ptr<Rule::LodRule> lodRule = rules.FindFirstByType<Rule::LodRule>();
            if (lodRule)
            {
                // LOD rules contain rules that starts from 1. The number of lod level in actor start from 0.
                const AZ::u32 lodRuleCount = static_cast<AZ::u32>(lodRule->GetLodRuleCount());
                const AZ::u32 lodLevelCount = lodRuleCount + 1;
                while (actor->GetNumLODLevels() < lodLevelCount)
                {
                    actor->AddLODLevel();
                }

                // Set up the LOD mask for bones.
                for (AZ::u32 emfxNodeIndex = 0; emfxNodeIndex < exfxNodeCount; ++emfxNodeIndex)
                {
                    EMotionFX::Node* emfxNode = actorSkeleton->GetNode(emfxNodeIndex);
                    const char* emfxNodeName = emfxNode->GetName();
                    for (AZ::u32 ruleIndex = 0; ruleIndex < lodRuleCount; ++ruleIndex)
                    {
                        const bool containSkeleton = lodRule->ContainsNodeByRuleIndex(emfxNodeName, ruleIndex);
                        // lod Rule 0 contains information about LOD 1, so we pass in index + 1 to emfx node.
                        emfxNode->SetSkeletalLODStatus(ruleIndex + 1, containSkeleton);
                    }
                }
            }

            // Get the critical bones list, mark the bones that is critical to prevent it to be optimized out.
            AZStd::shared_ptr<Rule::SkeletonOptimizationRule> skeletonOptimizationRule = actorGroup.GetRuleContainerConst().FindFirstByType<Rule::SkeletonOptimizationRule>();
            if (skeletonOptimizationRule)
            {
                const SceneData::SceneNodeSelectionList& criticalBonesList = skeletonOptimizationRule->GetCriticalBonesList();

                criticalBonesList.EnumerateSelectedNodes(
                    [&](const AZStd::string& bonePath)
                    {
                        // The bone name stores the node path in the scene. We need to convert it to EMotionFX node name.
                        const SceneContainers::SceneGraph::NodeIndex nodeIndex = graph.Find(bonePath);
                        if (!nodeIndex.IsValid())
                        {
                            AZ_Trace(
                                SceneUtil::WarningWindow, "Critical bone %s is not stored in the scene. Skipping it.", bonePath.c_str());
                            return true;
                        }

                        AZStd::shared_ptr<const SceneDataTypes::IBoneData> boneData =
                            azrtti_cast<const SceneDataTypes::IBoneData*>(graph.GetNodeContent(nodeIndex));
                        if (!boneData)
                        {
                            // Make sure we are dealing with bone here.
                            return true;
                        }

                        const SceneContainers::SceneGraph::Name& nodeName = graph.GetNodeName(nodeIndex);
                        EMotionFX::Node* emfxNode = actorSkeleton->FindNodeByName(nodeName.GetName());
                        if (!emfxNode)
                        {
                            AZ_Trace(
                                SceneUtil::WarningWindow,
                                "Critical bone %s is not in the actor skeleton hierarchy. Skipping it.",
                                nodeName.GetName());
                            return true;
                        }
                        // Critical node cannot be optimized out.
                        emfxNode->SetIsCritical(true);

                        return true;
                    });

                actor->SetOptimizeSkeleton(skeletonOptimizationRule->GetServerSkeletonOptimization());
            }

            SceneEvents::ProcessingResultCombiner result;
            // Process Morph Targets
            {
                AZStd::vector<AZ::u32> meshIndicesAsNumbers;
                for (auto& meshIndex : meshIndices)
                {
                    meshIndicesAsNumbers.push_back(meshIndex.AsNumber());
                }
                ActorMorphBuilderContext actorMorphBuilderContext(context.m_scene, &meshIndicesAsNumbers, context.m_group, context.m_actor, coordSysConverter, AZ::RC::Phase::Construction);
                result += SceneEvents::Process(actorMorphBuilderContext);
                result += SceneEvents::Process<ActorMorphBuilderContext>(actorMorphBuilderContext, AZ::RC::Phase::Filling);
                result += SceneEvents::Process<ActorMorphBuilderContext>(actorMorphBuilderContext, AZ::RC::Phase::Finalizing);
            }

            // Post create actor
            actor->SetUnitType(MCore::Distance::UNITTYPE_METERS);
            actor->SetFileUnitType(MCore::Distance::UNITTYPE_METERS);
            actor->PostCreateInit(/*makeGeomLodsCompatibleWithSkeletalLODs=*/false, /*convertUnitType=*/false);

            // Only enable joints that are used for skinning (and their parents).
            // On top of that, enable all joints marked as critical joints.
            const AZStd::shared_ptr<Rule::SkeletonOptimizationRule> skeletonOptimizeRule = actorGroup.GetRuleContainerConst().FindFirstByType<Rule::SkeletonOptimizationRule>();
            if (skeletonOptimizeRule && skeletonOptimizeRule->GetAutoSkeletonLOD())
            {
                AZStd::vector<AZStd::string> criticalJoints;
                const auto& criticalBonesList = skeletonOptimizeRule->GetCriticalBonesList();
                criticalBonesList.EnumerateSelectedNodes(
                    [&](const AZStd::string& criticalBonePath)
                    {
                        SceneContainers::SceneGraph::NodeIndex nodeIndex = graph.Find(criticalBonePath);
                        if (!nodeIndex.IsValid())
                        {
                            AZ_Trace(
                                SceneUtil::WarningWindow,
                                "Critical bone '%s' is not stored in the scene. Skipping it.",
                                criticalBonePath.c_str());
                            return true;
                        }

                        const AZStd::shared_ptr<const SceneDataTypes::IBoneData> boneData =
                            azrtti_cast<const SceneDataTypes::IBoneData*>(graph.GetNodeContent(nodeIndex));
                        if (!boneData)
                        {
                            return true;
                        }

                        const SceneContainers::SceneGraph::Name& nodeName = graph.GetNodeName(nodeIndex);
                        if (nodeName.GetNameLength() > 0)
                        {
                            criticalJoints.emplace_back(nodeName.GetName());
                        }

                        return true;
                    });

                // Mark all skeletal joints for each LOD to be enabled or disabled, based on the skinning data and critical list.
                // Also print the results to the log.
                actor->AutoSetupSkeletalLODsBasedOnSkinningData(criticalJoints);
            }

            // Scale the actor
            if (coordinateSystemRule)
            {
                const float scaleFactor = coordinateSystemRule->GetScale();
                if (!AZ::IsClose(scaleFactor, 1.0f, FLT_EPSILON)) // If the scale factor is 1, no need to call Scale
                {
                    actor->Scale(scaleFactor);
                }
            }

            if (result.GetResult() != AZ::SceneAPI::Events::ProcessingResult::Failure)
            {
                return SceneEvents::ProcessingResult::Success;
            }
            return AZ::SceneAPI::Events::ProcessingResult::Failure;
        }

        void ActorBuilder::BuildPreExportStructure(ActorBuilderContext& context, const SceneContainers::SceneGraph::NodeIndex& rootBoneNodeIndex,
            AZStd::vector<SceneContainers::SceneGraph::NodeIndex>& outNodeIndices, AZStd::vector<SceneContainers::SceneGraph::NodeIndex>& outMeshIndices, BoneNameEmfxIndexMap& outBoneNameEmfxIndexMap)
        {
            const SceneContainers::SceneGraph& graph = context.m_scene.GetGraph();

            auto nameStorage = graph.GetNameStorage();
            auto contentStorage = graph.GetContentStorage();
            auto nameContentView = SceneViews::MakePairView(nameStorage, contentStorage);

            // The search begin from the rootBoneNodeIndex.
            auto graphDownwardsRootBoneView = SceneViews::MakeSceneGraphDownwardsView<SceneViews::BreadthFirst>(graph, rootBoneNodeIndex, nameContentView.begin(), true);
            for (auto it = graphDownwardsRootBoneView.begin(); it != graphDownwardsRootBoneView.end(); ++it)
            {
                const SceneContainers::SceneGraph::NodeIndex& nodeIndex = graph.ConvertToNodeIndex(it.GetHierarchyIterator());

                // The end point in ly scene graph should not be added to the emfx actor.
                // Note: For example, the end point could be a transform node. We will process that later on its parent node.
                if (graph.IsNodeEndPoint(nodeIndex))
                {
                    // Skip the end point node except if it's a root node.
                    if (graph.GetRoot() != nodeIndex)
                    {
                        continue;
                    }
                }

                auto mesh = azrtti_cast<const SceneDataTypes::IMeshData*>(it->second);
                if (mesh)
                {
                    outMeshIndices.emplace_back(nodeIndex);

                    // Don't need to add a mesh node except if it is a parent of another joint / mesh node.
                    // Example:
                    // joint_1
                    //   |____transform
                    //   |____mesh_1 (keep)
                    //         |____transform
                    //         |____mesh_2 (remove)
                    //         |      |____transform
                    //         |_______joint_2
                    //                |____transform
                    // emfx doesn't need to contain the "end-point" mesh node because mesh buffers are ultimately stored in a single atom mesh.
                    // NOTE: Joint and mesh node often have a transform node as the children. To correctly detect whether a mesh node has a joint
                    // or mesh children, we need to check the type id of the children as well.
                    if (!graph.HasNodeChild(nodeIndex))
                    {
                        continue;
                    }
                    else
                    {
                        bool hasJointOrMeshChildren = false;
                        SceneContainers::SceneGraph::NodeIndex childNodeIndex = graph.GetNodeChild(nodeIndex);
                        while (childNodeIndex.IsValid())
                        {
                            auto childContent = graph.GetNodeContent(childNodeIndex);
                            if (childContent->RTTI_IsTypeOf(SceneDataTypes::IBoneData::TYPEINFO_Uuid())
                                || childContent->RTTI_IsTypeOf(SceneDataTypes::IMeshData::TYPEINFO_Uuid()))
                            {
                                hasJointOrMeshChildren = true;
                                break;
                            }
                            childNodeIndex = graph.GetNodeSibling(childNodeIndex);
                        }

                        if (!hasJointOrMeshChildren)
                        {
                            continue;
                        }
                    }
                }

                auto bone = azrtti_cast<const SceneDataTypes::IBoneData*>(it->second);
                if (bone)
                {
                    outBoneNameEmfxIndexMap[it->first.GetName()] = aznumeric_cast<AZ::u32>(outNodeIndices.size());
                }

                // Add bones, or mesh (that has a child mesh or joint) to our list of nodes we want to export.
                outNodeIndices.push_back(nodeIndex);
            }
        }
    } // namespace Pipeline
} // namespace EMotionFX
