
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
#include <RC/ResourceCompilerScene/Common/CommonExportContexts.h>

#include <SceneAPI/SceneCore/Containers/Views/PairIterator.h>
#include <SceneAPI/SceneCore/Containers/Views/SceneGraphDownwardsIterator.h>
#include <SceneAPI/SceneCore/Containers/Views/SceneGraphChildIterator.h>
#include <SceneAPI/SceneCore/Containers/Utilities/Filters.h>
#include <SceneAPI/SceneCore/Containers/Utilities/SceneGraphUtilities.h>
#include <SceneAPI/SceneCore/Utilities/FileUtilities.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IBoneData.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IMeshData.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IMaterialData.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/ITransform.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IMeshVertexUVData.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IMeshVertexColorData.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/ISkinWeightData.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IMeshVertexTangentData.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IMeshVertexBitangentData.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IMeshVertexColorData.h>
#include <SceneAPI/SceneCore/DataTypes/DataTypeUtilities.h>
#include <SceneAPI/SceneCore/DataTypes/Rules/IBlendShapeRule.h>
#include <SceneAPI/SceneCore/DataTypes/Rules/IMaterialRule.h>
#include <SceneAPI/SceneCore/DataTypes/Rules/IClothRule.h>
#include <SceneAPI/SceneCore/Events/GraphMetaInfoBus.h>
#include <SceneAPI/SceneData/Rules/TangentsRule.h>

#include <SceneAPIExt/Rules/MorphTargetRule.h>
#include <SceneAPIExt/Rules/IMeshRule.h>
#include <SceneAPIExt/Rules/MeshRule.h>
#include <SceneAPIExt/Rules/ISkinRule.h>
#include <SceneAPIExt/Rules/LodRule.h>
#include <SceneAPIExt/Rules/IActorScaleRule.h>
#include <SceneAPIExt/Rules/CoordinateSystemRule.h>
#include <SceneAPIExt/Rules/SkeletonOptimizationRule.h>
#include <SceneAPIExt/Groups/ActorGroup.h>
#include <RCExt/Actor/ActorBuilder.h>
#include <RCExt/ExportContexts.h>
#include <RCExt/CoordinateSystemConverter.h>

#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/Node.h>
#include <EMotionFX/Source/Mesh.h>
#include <EMotionFX/Source/MeshBuilder.h>
#include <EMotionFX/Source/MeshBuilderSkinningInfo.h>
#include <EMotionFX/Source/StandardMaterial.h>
#include <EMotionFX/Source/SoftSkinDeformer.h>
#include <EMotionFX/Source/DualQuatSkinDeformer.h>
#include <EMotionFX/Source/MeshDeformerStack.h>
#include <EMotionFX/Source/SoftSkinManager.h>
#include <EMotionFX/Exporters/ExporterLib/Exporter/Exporter.h>
#include <MCore/Source/AzCoreConversions.h>

#include <GFxFramework/MaterialIO/Material.h>
#include <AzCore/Math/MathUtils.h>
#include <AzCore/Math/Matrix3x4.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/Interface/Interface.h>
#include <AzFramework/Application/Application.h>
#include <AzFramework/API/AtomActiveInterface.h>
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
            const SceneDataTypes::MatrixType& azTransform, const CoordinateSystemConverter& coordSysConverter)
        {
            return EMotionFX::Transform(
                coordSysConverter.ConvertVector3(azTransform.GetTranslation()),
                coordSysConverter.ConvertQuaternion(AZ::Quaternion::CreateFromMatrix3x4(azTransform)),
                coordSysConverter.ConvertScale(azTransform.RetrieveScale()));
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


        bool ActorBuilder::GetIsMorphed(const AZ::SceneAPI::Containers::SceneGraph& graph, const AZ::SceneAPI::Containers::SceneGraph::NodeIndex& nodeIndex, const AZ::SceneAPI::DataTypes::IBlendShapeRule* morphTargetRule) const
        {
            if (!morphTargetRule)
            {
                return false;
            }

            const AZ::SceneAPI::Containers::SceneGraph::NodeIndex parentIndex = graph.GetNodeParent(nodeIndex);
            const AZ::SceneAPI::DataTypes::ISceneNodeSelectionList& morphTargetNodes = morphTargetRule->GetSceneNodeSelectionList();

            auto nameStorage = graph.GetNameStorage();
            auto contentStorage = graph.GetContentStorage();
            auto nameContentView = SceneViews::MakePairView(nameStorage, contentStorage);

            const size_t selectionNodeCount = morphTargetRule->GetSceneNodeSelectionList().GetSelectedNodeCount();
            for (size_t morphIndex = 0; morphIndex < selectionNodeCount; ++morphIndex)
            {
                AZ::SceneAPI::Containers::SceneGraph::NodeIndex morphNodeIndex = graph.Find(morphTargetRule->GetSceneNodeSelectionList().GetSelectedNode(morphIndex));

                // Check if the morph target node is one of the child nodes down the hierarchy.
                auto graphDownwardsView = SceneViews::MakeSceneGraphDownwardsView<SceneViews::BreadthFirst>(graph, nodeIndex, nameContentView.begin(), true);
                for (auto it = graphDownwardsView.begin(); it != graphDownwardsView.end(); ++it)
                {
                    const SceneContainers::SceneGraph::NodeIndex& graphNodeIndex = graph.ConvertToNodeIndex(it.GetHierarchyIterator());
                    if (!it->second)
                    {
                        continue;
                    }

                    if (graphNodeIndex == morphNodeIndex)
                    {
                        return true;
                    }
                }
            }

            return false;
        }


        SceneEvents::ProcessingResult ActorBuilder::BuildActor(ActorBuilderContext& context)
        {
            if (context.m_phase != AZ::RC::Phase::Filling)
            {
                return SceneEvents::ProcessingResult::Ignored;
            }

            ActorSettings actorSettings;
            ExtractActorSettings(context.m_group, actorSettings);

            NodeIndexSet selectedBaseMeshNodeIndices;
            GetNodeIndicesOfSelectedBaseMeshes(context, selectedBaseMeshNodeIndices);

            const SceneContainers::SceneGraph& graph = context.m_scene.GetGraph();

            const Group::IActorGroup& actorGroup = context.m_group;
            const char* rootBoneName = actorGroup.GetSelectedRootBone().c_str();
            AZ_TraceContext("Root bone", rootBoneName);

            SceneContainers::SceneGraph::NodeIndex rootBoneNodeIndex = graph.Find(rootBoneName);
            if (!rootBoneNodeIndex.IsValid())
            {
                AZ_TracePrintf(SceneUtil::ErrorWindow, "Root bone cannot be found.\n");
                return SceneEvents::ProcessingResult::Failure;
            }

            // Get the coordinate system conversion rule.
            CoordinateSystemConverter ruleCoordSysConverter;
            AZStd::shared_ptr<Rule::CoordinateSystemRule> coordinateSystemRule = actorGroup.GetRuleContainerConst().FindFirstByType<Rule::CoordinateSystemRule>();
            if (coordinateSystemRule)
            {
                coordinateSystemRule->UpdateCoordinateSystemConverter();
                ruleCoordSysConverter = coordinateSystemRule->GetCoordinateSystemConverter();
            }

            CoordinateSystemConverter coordSysConverter = ruleCoordSysConverter;
            if (context.m_scene.GetOriginalSceneOrientation() != SceneContainers::Scene::SceneOrientation::ZUp)
            {
                AZ::Transform orientedTarget = ruleCoordSysConverter.GetTargetTransform();
                AZ::Transform rotationZ = AZ::Transform::CreateRotationZ(-AZ::Constants::Pi);
                orientedTarget = orientedTarget * rotationZ;

                //same as rule
                // X, Y and Z are all at the same indices inside the target coordinate system, compared to the source coordinate system.
                const AZ::u32 targetBasisIndices[3] = { 0, 1, 2 };
                coordSysConverter = CoordinateSystemConverter::CreateFromTransforms(ruleCoordSysConverter.GetSourceTransform(), orientedTarget, targetBasisIndices);
            }

            // Collect the node indices that emfx cares about and construct the boneNameEmfxIndex map for quick search.
            AZStd::vector<SceneContainers::SceneGraph::NodeIndex> nodeIndices;
            BoneNameEmfxIndexMap boneNameEmfxIndexMap;
            BuildPreExportStructure(context, rootBoneNodeIndex, selectedBaseMeshNodeIndices, nodeIndices, boneNameEmfxIndexMap);

            EMotionFX::Actor* actor = context.m_actor;
            EMotionFX::Skeleton* actorSkeleton = actor->GetSkeleton();
            const AZ::u32 exfxNodeCount = aznumeric_cast<AZ::u32>(nodeIndices.size());
            actor->SetNumNodes(aznumeric_cast<AZ::u32>(exfxNodeCount));
            actor->ResizeTransformData();

            // Add a standard material
            // This material is used within the existing EMotionFX GL window. The engine will use a native engine material at runtime. The GL window will also be replaced by a native engine viewport
            EMotionFX::StandardMaterial* defaultMat = EMotionFX::StandardMaterial::Create("Default");
            defaultMat->SetAmbient(MCore::RGBAColor(0.0f, 0.0f, 0.0f));
            defaultMat->SetDiffuse(MCore::RGBAColor(1.0f, 1.0f, 1.0f));
            defaultMat->SetSpecular(MCore::RGBAColor(1.0f, 1.0f, 1.0f));
            defaultMat->SetShine(100.0f);
            actor->AddMaterial(0, defaultMat);

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
                const size_t criticalBoneCount = criticalBonesList.GetSelectedNodeCount();
                
                for (size_t boneIndex = 0; boneIndex < criticalBoneCount; ++boneIndex)
                {
                    const AZStd::string& bonePath = criticalBonesList.GetSelectedNode(boneIndex);

                    // The bone name stores the node path in the scene. We need to convert it to EMotionFX node name.
                    const SceneContainers::SceneGraph::NodeIndex nodeIndex = graph.Find(bonePath);
                    if (!nodeIndex.IsValid())
                    {
                        AZ_TracePrintf(SceneUtil::WarningWindow, "Critical bone %s is not stored in the scene. Skipping it.", bonePath.c_str());
                        continue;
                    }

                    AZStd::shared_ptr<const SceneDataTypes::IBoneData> boneData = azrtti_cast<const SceneDataTypes::IBoneData*>(graph.GetNodeContent(nodeIndex));
                    if (!boneData)
                    {
                        // Make sure we are dealing with bone here.
                        continue;
                    }

                    const SceneContainers::SceneGraph::Name& nodeName = graph.GetNodeName(nodeIndex);
                    EMotionFX::Node* emfxNode = actorSkeleton->FindNodeByName(nodeName.GetName());
                    if (!emfxNode)
                    {
                        AZ_TracePrintf(SceneUtil::WarningWindow, "Critical bone %s is not in the actor skeleton hierarchy. Skipping it.", nodeName.GetName());
                        continue;
                    }
                    // Critical node cannot be optimized out.
                    emfxNode->SetIsCritical(true);
                }

                actor->SetOptimizeSkeleton(skeletonOptimizationRule->GetServerSkeletonOptimization());
            }

            // Process meshs
            SceneEvents::ProcessingResultCombiner result;
            if (actorSettings.m_loadMeshes)
            {
                GetMaterialInfoForActorGroup(context);
                if (m_materialGroup)
                {
                    size_t materialCount = m_materialGroup->GetMaterialCount();
                    // So far, we have only added only the default material. For the meshes, we may need to set material indices greater than 0.
                    // To avoid feeding the EMFX mesh builder with invalid material indices, here we push the default material itself a few times
                    // so that the number of total materials added to mesh builder equals the number of materials in m_materialGroup. The actual material
                    // pushed doesn't matter since it is not used for rendering.
                    for (size_t materialIdx = 1; materialIdx < materialCount; ++materialIdx)
                    {
                        actor->AddMaterial(0, defaultMat->Clone());
                    }
                    AZ_Assert(materialCount == actor->GetNumMaterials(0), "Didn't add the desired number of materials to the actor");
                }

                // Process meshes
                for (auto& nodeIndex : selectedBaseMeshNodeIndices)
                {
                    AZStd::shared_ptr<const SceneDataTypes::IMeshData> nodeMesh = azrtti_cast<const SceneDataTypes::IMeshData*>(graph.GetNodeContent(nodeIndex));
                    AZ_Assert(nodeMesh, "Node is expected to be a mesh, but isn't.");
                    if (nodeMesh)
                    {
                        // Rename the mesh node in emfx without the postfix _lodx.
                        if (actorSkeletonLookup.find(nodeIndex.AsNumber()) == actorSkeletonLookup.end())
                        {
                            AZ_Warning(SceneUtil::WarningWindow, false, "Mesh %s does not belong under the root bone %s, skip building this mesh", graph.GetNodeName(nodeIndex).GetName(), rootBoneName);
                            continue;
                        }
                        EMotionFX::Node* emfxNode = actorSkeletonLookup[nodeIndex.AsNumber()];
                        const AZStd::string_view nodeNameView = RemoveLODSuffix(emfxNode->GetName());
                        emfxNode->SetName(AZStd::string(nodeNameView).c_str());
                        BuildMesh(context, emfxNode, nodeMesh, nodeIndex, boneNameEmfxIndexMap, actorSettings, coordSysConverter, 0);
                    }
                }

                // Process meshes for each lod.
                for (size_t ruleIndex = 0; lodRule && ruleIndex < lodRule->GetLodRuleCount(); ++ruleIndex)
                {
                    SceneData::SceneNodeSelectionList& lodNodeList = lodRule->GetSceneNodeSelectionList(ruleIndex);
                    for (size_t i = 0, count = lodNodeList.GetSelectedNodeCount(); i < count; ++i)
                    {
                        const AZStd::string& nodePath = lodNodeList.GetSelectedNode(i);
                        SceneContainers::SceneGraph::NodeIndex nodeIndex = graph.Find(nodePath);
                        AZ_Assert(nodeIndex.IsValid(), "Invalid scene graph node index");
                        if (nodeIndex.IsValid())
                        {
                            AZStd::shared_ptr<const SceneDataTypes::IMeshData> meshData =
                                azrtti_cast<const SceneDataTypes::IMeshData*>(graph.GetNodeContent(nodeIndex));
                            if (meshData)
                            {
                                // Find the Lod mesh node in emfx
                                const AZStd::string_view nodeName = RemoveLODSuffix(graph.GetNodeName(nodeIndex).GetName());
                                EMotionFX::Node* emfxNode = actorSkeleton->FindNodeByName(nodeName);
                                if (!emfxNode)
                                {
                                    AZ_Assert(false, "Tried to find the lod mesh %.*s in the actor hierarchy but didn't find any match.", static_cast<int>(nodeName.size()), nodeName.data());
                                    continue;
                                }
                                BuildMesh(context, emfxNode, meshData, nodeIndex, boneNameEmfxIndexMap, actorSettings, coordSysConverter, ruleIndex + 1);
                            }
                        }
                    }
                }

                // Process Morph Targets
                {
                    AZStd::vector<AZ::u32> meshIndices;
                    for (auto& nodeIndex : selectedBaseMeshNodeIndices)
                    {
                        meshIndices.push_back(nodeIndex.AsNumber());
                    }
                    ActorMorphBuilderContext actorMorphBuilderContext(context.m_scene, &meshIndices, context.m_group, context.m_actor, coordSysConverter, AZ::RC::Phase::Construction);
                    result += SceneEvents::Process(actorMorphBuilderContext);
                    result += SceneEvents::Process<ActorMorphBuilderContext>(actorMorphBuilderContext, AZ::RC::Phase::Filling);
                    result += SceneEvents::Process<ActorMorphBuilderContext>(actorMorphBuilderContext, AZ::RC::Phase::Finalizing);
                }
            }

            // Post create actor
            actor->SetUnitType(MCore::Distance::UNITTYPE_METERS);
            actor->SetFileUnitType(MCore::Distance::UNITTYPE_METERS);
            actor->PostCreateInit(/*makeGeomLodsCompatibleWithSkeletalLODs=*/false, /*generateOBBs=*/false, /*convertUnitType=*/false);

            // Only enable joints that are used for skinning (and their parents).
            // On top of that, enable all joints marked as critical joints.
            const AZStd::shared_ptr<Rule::SkeletonOptimizationRule> skeletonOptimizeRule = actorGroup.GetRuleContainerConst().FindFirstByType<Rule::SkeletonOptimizationRule>();
            if (skeletonOptimizeRule && skeletonOptimizeRule->GetAutoSkeletonLOD())
            {
                AZStd::vector<AZStd::string> criticalJoints;
                const auto& criticalBonesList = skeletonOptimizeRule->GetCriticalBonesList();
                const size_t numSelectedBones = criticalBonesList.GetSelectedNodeCount();
                for (size_t i = 0; i < numSelectedBones; ++i)
                {
                    const AZStd::string criticalBonePath = criticalBonesList.GetSelectedNode(i);
                    SceneContainers::SceneGraph::NodeIndex nodeIndex = graph.Find(criticalBonePath);
                    if (!nodeIndex.IsValid())
                    {
                        AZ_TracePrintf(SceneUtil::WarningWindow, "Critical bone '%s' is not stored in the scene. Skipping it.", criticalBonePath.c_str());
                        continue;
                    }

                    const AZStd::shared_ptr<const SceneDataTypes::IBoneData> boneData = azrtti_cast<const SceneDataTypes::IBoneData*>(graph.GetNodeContent(nodeIndex));
                    if (!boneData)
                    {
                        continue;
                    }

                    const SceneContainers::SceneGraph::Name& nodeName = graph.GetNodeName(nodeIndex);
                    if (nodeName.GetNameLength() > 0)
                    {
                        criticalJoints.emplace_back(nodeName.GetName());
                    }
                }

                // Mark all skeletal joints for each LOD to be enabled or disabled, based on the skinning data and critical list.
                // Also print the results to the log.
                actor->AutoSetupSkeletalLODsBasedOnSkinningData(criticalJoints);
                actor->PrintSkeletonLODs();
            }

            // Scale the actor
            AZStd::shared_ptr<Rule::IActorScaleRule> scaleRule = actorGroup.GetRuleContainerConst().FindFirstByType<Rule::IActorScaleRule>();
            if (scaleRule)
            {
                const float scaleFactor = scaleRule->GetScaleFactor();
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

        void ActorBuilder::BuildPreExportStructure(ActorBuilderContext& context, const SceneContainers::SceneGraph::NodeIndex& rootBoneNodeIndex, const NodeIndexSet& selectedBaseMeshNodeIndices,
            AZStd::vector<SceneContainers::SceneGraph::NodeIndex>& outNodeIndices, BoneNameEmfxIndexMap& outBoneNameEmfxIndexMap)
        {
            const SceneContainers::SceneGraph& graph = context.m_scene.GetGraph();
            const Group::IActorGroup& group = context.m_group;

            auto nameStorage = graph.GetNameStorage();
            auto contentStorage = graph.GetContentStorage();
            auto nameContentView = SceneViews::MakePairView(nameStorage, contentStorage);

            // The search begin from the rootBoneNodeIndex.
            auto graphDownwardsRootBoneView = SceneViews::MakeSceneGraphDownwardsView<SceneViews::BreadthFirst>(graph, rootBoneNodeIndex, nameContentView.begin(), true);
            auto it = graphDownwardsRootBoneView.begin();
            if (!it->second)
            {
                // We always skip the first node because it's introduced by scenegraph
                ++it;
                if (!it->second && it != graphDownwardsRootBoneView.end())
                {
                    // In maya / max, we skip 1 root node when it have no content (emotionfx always does this)
                    // However, fbx doesn't restrain itself from having multiple root nodes. We might want to revisit here if it ever become a problem.
                    ++it;
                }
            }

            for (; it != graphDownwardsRootBoneView.end(); ++it)
            {
                const SceneContainers::SceneGraph::NodeIndex& nodeIndex = graph.ConvertToNodeIndex(it.GetHierarchyIterator());

                // The end point in ly scene graph should not be added to the emfx actor.
                // Note: For example, the end point could be a transform node. We will process that later on its parent node.
                if (graph.IsNodeEndPoint(nodeIndex))
                {
                    continue;
                }

                // If the node is a mesh, we skip it in the first search.
                auto mesh = azrtti_cast<const SceneDataTypes::IMeshData*>(it->second);
                if (mesh)
                {
                    continue;
                }

                // If it's a bone, add it to boneNodeIndices.
                auto bone = azrtti_cast<const SceneDataTypes::IBoneData*>(it->second);
                if (bone)
                {
                    outBoneNameEmfxIndexMap[it->first.GetName()] = aznumeric_cast<AZ::u32>(outNodeIndices.size());
                }

                outNodeIndices.push_back(nodeIndex);
            }


            // We then search from the graph root to find all the meshes that we selected.
            auto meshView = SceneContainers::Views::MakeFilterView(graph.GetContentStorage().begin(), graph.GetContentStorage().end(), AZ::SceneAPI::Containers::DerivedTypeFilter<AZ::SceneAPI::DataTypes::IMeshData>());
            for (auto it2 = meshView.begin(); it2 != meshView.end(); ++it2)
            {
                // If the node is a mesh and it is one of the selected ones, add it.
                auto mesh = azrtti_cast<const SceneDataTypes::IMeshData*>(*it2);
                const SceneContainers::SceneGraph::NodeIndex& nodeIndex = graph.ConvertToNodeIndex(it2.GetBaseIterator());
                if (mesh && (selectedBaseMeshNodeIndices.find(nodeIndex) != selectedBaseMeshNodeIndices.end()))
                {
                    outNodeIndices.push_back(nodeIndex);
                }
            }
        }

        // Find vertex color data inside the mesh of a given node using a node name.
        AZ::SceneAPI::DataTypes::IMeshVertexColorData* ActorBuilder::FindVertexColorData(AZ::SceneAPI::Containers::SceneGraph& graph, const AZ::SceneAPI::Containers::SceneGraph::NodeIndex& nodeIndex, const AZStd::string& colorNodeName)
        {
            auto vertexColorNode = graph.Find(nodeIndex, colorNodeName);
            if (vertexColorNode.IsValid())
            {
                return azrtti_cast<AZ::SceneAPI::DataTypes::IMeshVertexColorData*>(graph.GetNodeContent(vertexColorNode).get());
            }
            else
            {
                return nullptr;
            }
        }

        // This method uses EMFX MeshBuilder class. This MeshBuilder class expects to be fed "Control points" in fbx parlance. However, as of the current (April 2017)
        // implementation of MeshData class and FbxMeshImporterUtilities.cpp, IMeshData does not provide a  way to get all of the original control points obtained
        // from the fbx resource. Specifically, IMeshData has information about only those control points which it uses, i.e., those of the original control points which
        // are part of polygons. So, any unconnected stray vertices or vertices which have just lines between them have all been discarded and we don't have a way
        // to get their positions/normals etc.
        // Given that IMeshData doesn't provide access to all of the control points, we have two choices.
        // Choice 1: Update the fbx pipeline code to provide access to the original control points via IMeshData or some other class.
        // Choice 2: View the subset of the control points that MeshData has as the control points for EMFX MeshBuilder.
        // The code below is based on the second choice. Reasons for this choice are: (a) Less updates to the core fbx pipeline code and hence less risk of breaking
        // existing export paths. (b) Since we ultimately are rendering only polygons anyway, we don't care about such stray vertices which are not part of any
        // polygons.
        // The newly added method IMeshData::GetUsedPointIndexForControlPoint(...) provides unique 0 based contiguous indices to the control points actually used in MeshData.
        //
        void ActorBuilder::BuildMesh(const ActorBuilderContext& context, EMotionFX::Node* emfxNode, AZStd::shared_ptr<const SceneDataTypes::IMeshData> meshData,
            const SceneContainers::SceneGraph::NodeIndex& meshNodeIndex, const BoneNameEmfxIndexMap& boneNameEmfxIndexMap, const ActorSettings& settings,
            const CoordinateSystemConverter& coordSysConverter, AZ::u8 lodLevel)
        {
            SetupMaterialDataForMesh(context, meshNodeIndex);

            SceneContainers::SceneGraph& graph = const_cast<SceneContainers::SceneGraph&>(context.m_scene.GetGraph());
            EMotionFX::Actor* actor = context.m_actor;

            // Check if this mesh is morphed.
            AZStd::shared_ptr<const EMotionFX::Pipeline::Rule::MorphTargetRule> morphTargetRule = context.m_group.GetRuleContainerConst().FindFirstByType<EMotionFX::Pipeline::Rule::MorphTargetRule>();
            const bool hasMorphTargets = GetIsMorphed(graph, meshNodeIndex, morphTargetRule.get());

            // Get the tangent space setup in the tangents rule, or import from Fbx if there isn't any rule.
            AZStd::shared_ptr<AZ::SceneAPI::SceneData::TangentsRule> tangentsRule = context.m_group.GetRuleContainerConst().FindFirstByType<AZ::SceneAPI::SceneData::TangentsRule>();
            AZ::SceneAPI::DataTypes::TangentSpace tangentSpace = AZ::SceneAPI::DataTypes::TangentSpace::MikkT;
            bool storeBitangents = false;
            bool normalizeTangents = true;
            size_t tangentUVSetIndex = 0;
            if (tangentsRule)
            {
                tangentSpace = tangentsRule->GetTangentSpace();
                storeBitangents = (tangentsRule->GetBitangentMethod() != AZ::SceneAPI::DataTypes::BitangentMethod::Orthogonal);
                tangentUVSetIndex = tangentsRule->GetUVSetIndex();
                normalizeTangents = tangentsRule->GetNormalizeVectors();
            }

            // Get the number of orgVerts (control point).
            const AZ::u32 numOrgVerts = aznumeric_cast<AZ::u32>(meshData->GetUsedControlPointCount());
            AZStd::unique_ptr<EMotionFX::MeshBuilder> meshBuilder(aznew MeshBuilder(emfxNode->GetNodeIndex(), numOrgVerts, !hasMorphTargets /* Disable duplication optimization for morph targets. */));

            // Import the skinning info if there is any. Otherwise set it to a nullptr.
            EMotionFX::MeshBuilderSkinningInfo* skinningInfo = ExtractSkinningInfo(meshData, graph, meshNodeIndex, boneNameEmfxIndexMap, settings);
            meshBuilder->SetSkinningInfo(skinningInfo);

            // Original vertex numbers.
            EMotionFX::MeshBuilderVertexAttributeLayerUInt32* orgVtxLayer = EMotionFX::MeshBuilderVertexAttributeLayerUInt32::Create(numOrgVerts, EMotionFX::Mesh::ATTRIB_ORGVTXNUMBERS, false, false);
            meshBuilder->AddLayer(orgVtxLayer);

            // The positions layer.
            EMotionFX::MeshBuilderVertexAttributeLayerVector3* posLayer = EMotionFX::MeshBuilderVertexAttributeLayerVector3::Create(numOrgVerts, EMotionFX::Mesh::ATTRIB_POSITIONS, false, true);
            meshBuilder->AddLayer(posLayer);

            // The normals layer.
            EMotionFX::MeshBuilderVertexAttributeLayerVector3* normalsLayer = EMotionFX::MeshBuilderVertexAttributeLayerVector3::Create(numOrgVerts, EMotionFX::Mesh::ATTRIB_NORMALS, false, true);
            meshBuilder->AddLayer(normalsLayer);

            // A Mesh can have multiple children that contain UV, tangent or bitangent data.
            SceneDataTypes::IMeshVertexUVData*                  meshUVDatas[2]      = { nullptr, nullptr };
            SceneDataTypes::IMeshVertexColorData*               meshColorData       = nullptr;
            SceneDataTypes::IMeshVertexTangentData*             meshTangentData     = nullptr;
            SceneDataTypes::IMeshVertexBitangentData*           meshBitangentData   = nullptr;
            EMotionFX::MeshBuilderVertexAttributeLayerVector2*  uvLayers[2]         = { nullptr, nullptr };
            EMotionFX::MeshBuilderVertexAttributeLayerVector4*  tangentLayer        = nullptr;
            EMotionFX::MeshBuilderVertexAttributeLayerVector3*  bitangentLayer      = nullptr;
            EMotionFX::MeshBuilderVertexAttributeLayerVector4*  colorLayer128       = nullptr;
            EMotionFX::MeshBuilderVertexAttributeLayerUInt32*   colorLayer32        = nullptr;

            // Get the UV sets.
            meshUVDatas[0] = AZ::SceneAPI::SceneData::TangentsRule::FindUVData(graph, meshNodeIndex, 0);
            meshUVDatas[1] = AZ::SceneAPI::SceneData::TangentsRule::FindUVData(graph, meshNodeIndex, 1);

            // Get the vertex color mode.
            AZStd::shared_ptr<EMotionFX::Pipeline::Rule::MeshRule> meshRule = context.m_group.GetRuleContainerConst().FindFirstByType<EMotionFX::Pipeline::Rule::MeshRule>();
            Rule::IMeshRule::VertexColorMode vertexColorMode = Rule::IMeshRule::VertexColorMode::Precision_32;
            AZStd::string vertexColorStreamName;
            bool vertexColorsEnabled = false;
            if (meshRule)
            {
                vertexColorMode = meshRule->GetVertexColorMode();
                vertexColorStreamName = meshRule->GetVertexColorStreamName();
                vertexColorsEnabled = !meshRule->IsVertexColorsDisabled();
            }

            // Get the color sets.
            if (vertexColorsEnabled)
            {
                meshColorData = FindVertexColorData(graph, meshNodeIndex, vertexColorStreamName);
            }

            // If we selected a UV set other than the first one (so the second), but it doesn't exist, then let's give a warning
            if (!meshUVDatas[tangentUVSetIndex] && tangentUVSetIndex > 0 && tangentSpace != AZ::SceneAPI::DataTypes::TangentSpace::EMotionFX)
            {
                AZ_TracePrintf(SceneUtil::WarningWindow, "The tangent UV set index '%d' is used, while there is no uv data for this set. There will be no tangent data!\n", tangentUVSetIndex);
            }

            // Get the tangents and bitangents for the first UV set, in the space the rule has setup.
            meshTangentData = meshUVDatas[tangentUVSetIndex] ? AZ::SceneAPI::SceneData::TangentsRule::FindTangentData(graph, meshNodeIndex, tangentUVSetIndex, tangentSpace) : nullptr;
            if (!meshTangentData)
            {
                // Try to grab the MikkT tangent space if the space we request isn't there (Fbx normals choosen, while they are not in the Fbx file).
                meshTangentData = meshUVDatas[tangentUVSetIndex] ? AZ::SceneAPI::SceneData::TangentsRule::FindTangentData(graph, meshNodeIndex, tangentUVSetIndex, AZ::SceneAPI::DataTypes::TangentSpace::MikkT) : nullptr;
                if (meshTangentData)
                {
                    AZ_TracePrintf(SceneUtil::WarningWindow, "No Fbx tangent data found, falling back to MikkT tangent space.\n");
                    tangentSpace = AZ::SceneAPI::DataTypes::TangentSpace::MikkT;
                }
            }

            // Get the right bitangent data if we want to.
            meshBitangentData = (storeBitangents && meshTangentData) ? AZ::SceneAPI::SceneData::TangentsRule::FindBitangentData(graph, meshNodeIndex, tangentUVSetIndex, tangentSpace) : nullptr;

            // Create the EMotion FX mesh data layers.
            if (meshTangentData)
            {
                tangentLayer = EMotionFX::MeshBuilderVertexAttributeLayerVector4::Create(numOrgVerts, EMotionFX::Mesh::ATTRIB_TANGENTS, false, true);
                meshBuilder->AddLayer(tangentLayer);
            }

            if (meshBitangentData)
            {
                bitangentLayer = EMotionFX::MeshBuilderVertexAttributeLayerVector3::Create(numOrgVerts, EMotionFX::Mesh::ATTRIB_BITANGENTS, false, true);
                meshBuilder->AddLayer(bitangentLayer);
            }

            // Create the UV layers.
            for (size_t i = 0; i < 2; ++i)
            {
                if (meshUVDatas[i])
                {
                    uvLayers[i] = EMotionFX::MeshBuilderVertexAttributeLayerVector2::Create(numOrgVerts, EMotionFX::Mesh::ATTRIB_UVCOORDS, false, false);
                    meshBuilder->AddLayer(uvLayers[i]);
                }
            }

            // Get the cloth data (only for full mesh LOD 0).
            const AZStd::vector<AZ::Color> clothData = (lodLevel == 0)
                ? SceneDataTypes::IClothRule::FindClothData(
                    graph, meshNodeIndex, meshData->GetVertexCount(), context.m_group.GetRuleContainerConst())
                : AZStd::vector<AZ::Color>{};

            // Create the cloth layer.
            EMotionFX::MeshBuilderVertexAttributeLayerUInt32* clothLayer = nullptr;
            if (!clothData.empty())
            {
                clothLayer = EMotionFX::MeshBuilderVertexAttributeLayerUInt32::Create(numOrgVerts, EMotionFX::Mesh::ATTRIB_CLOTH_DATA, false, false);
                meshBuilder->AddLayer(clothLayer);
            }

            // Create the color layers.
            if (meshColorData && vertexColorsEnabled)
            {
                if (vertexColorMode == Rule::IMeshRule::VertexColorMode::Precision_128)
                {
                    colorLayer128 = EMotionFX::MeshBuilderVertexAttributeLayerVector4::Create(numOrgVerts, EMotionFX::Mesh::ATTRIB_COLORS128, false, false);
                    meshBuilder->AddLayer(colorLayer128);
                }
                else if (vertexColorMode == Rule::IMeshRule::VertexColorMode::Precision_32)
                {
                    colorLayer32 = EMotionFX::MeshBuilderVertexAttributeLayerUInt32::Create(numOrgVerts, EMotionFX::Mesh::ATTRIB_COLORS32, false, false);
                    meshBuilder->AddLayer(colorLayer32);
                }
            }

            // Inverse transpose for normal, tangent and bitangent.
            const SceneDataTypes::MatrixType globalTransform = SceneUtil::BuildWorldTransform(graph, meshNodeIndex);
            SceneDataTypes::MatrixType globalTransformN = globalTransform.GetInverseFull().GetTranspose();
            globalTransformN.SetTranslation(AZ::Vector3::CreateZero());

            // Data for each vertex.
            AZ::Vector2 uv;
            AZ::Vector3 pos;
            AZ::Vector3 normal;
            AZ::Vector4 tangent;
            AZ::Vector3 bitangent;
            AZ::Vector3 bitangentVec;
            AZ::Vector4 color;

            const AZ::u32 numTriangles = meshData->GetFaceCount();
            for (AZ::u32 i = 0; i < numTriangles; ++i)
            {
                AZ::u32 materialID = 0;
                if (m_materialGroup)
                {
                    const AZ::u32 meshLocalMaterialID = meshData->GetFaceMaterialId(i);
                    if (meshLocalMaterialID < m_materialIndexMapForMesh.size())
                    {
                        materialID = m_materialIndexMapForMesh[meshLocalMaterialID];
                    }
                    else
                    {
                        AZ_TracePrintf(SceneUtil::WarningWindow, "Invalid value for the material index of the face.\n");
                    }
                }

                // Start the triangle.
                meshBuilder->BeginPolygon(materialID);

                // Determine winding.
                AZ::u32 order[3];
                if (coordSysConverter.IsSourceRightHanded() == coordSysConverter.IsTargetRightHanded())
                {
                    order[0] = 0;
                    order[1] = 1;
                    order[2] = 2;
                }
                else
                {
                    order[0] = 2;
                    order[1] = 1;
                    order[2] = 0;
                }

                // Add all triangle points (We are not supporting non-triangle polygons)
                for (AZ::u32 j = 0; j < 3; ++j)
                {
                    const AZ::u32 vertexIndex = meshData->GetVertexIndex(i, order[j]);
                    const AZ::u32 controlPointIndex = meshData->GetControlPointIndex(vertexIndex);

                    int orgVertexNumber = meshData->GetUsedPointIndexForControlPoint(controlPointIndex);
                    AZ_Assert(orgVertexNumber >= 0, "Invalid vertex number");
                    orgVtxLayer->SetCurrentVertexValue(&orgVertexNumber);

                    pos = meshData->GetPosition(vertexIndex);
                    normal = meshData->GetNormal(vertexIndex);
                    if (skinningInfo)
                    {
                        pos = globalTransform * pos;
                        normal = globalTransformN.TransformVector(normal);
                    }

                    pos = coordSysConverter.ConvertVector3(pos);
                    posLayer->SetCurrentVertexValue(&pos);

                    normal = coordSysConverter.ConvertVector3(normal);
                    normal.NormalizeSafe();
                    normalsLayer->SetCurrentVertexValue(&normal);

                    for (AZ::u32 e = 0; e < 2; ++e)
                    {
                        if (!meshUVDatas[e])
                        {
                            continue;
                        }

                        uv = meshUVDatas[e]->GetUV(vertexIndex);
                        uvLayers[e]->SetCurrentVertexValue(&uv);
                    }

                    if (meshColorData && vertexColorsEnabled)
                    {
                        if (colorLayer128)
                        {
                            const AZ::SceneAPI::DataTypes::Color vertexColor = meshColorData->GetColor(vertexIndex);
                            color.Set(vertexColor.red, vertexColor.green, vertexColor.blue, vertexColor.alpha);
                            colorLayer128->SetCurrentVertexValue(&color);
                        }
                        else if (colorLayer32)
                        {
                            const AZ::SceneAPI::DataTypes::Color sceneApiColor = meshColorData->GetColor(vertexIndex);
                            AZ::Color vertexColor;
                            vertexColor.Set(
                                AZ::GetClamp<float>(sceneApiColor.red, 0.0f, 1.0f), 
                                AZ::GetClamp<float>(sceneApiColor.green, 0.0f, 1.0f),
                                AZ::GetClamp<float>(sceneApiColor.blue, 0.0f, 1.0f),
                                AZ::GetClamp<float>(sceneApiColor.alpha, 0.0f, 1.0f));
                            AZ::u32 color32 = vertexColor.ToU32();
                            colorLayer32->SetCurrentVertexValue(&color32);
                        }
                    }

                    // Feed the tangent.
                    if (meshTangentData)
                    {
                        const AZ::Vector4& dataTangent = meshTangentData->GetTangent(vertexIndex);
                        AZ::Vector3 tangentVec = dataTangent.GetAsVector3();
                        if (skinningInfo)
                        {
                            tangentVec = globalTransformN.TransformVector(tangentVec);
                        }
                        tangentVec = coordSysConverter.ConvertVector3(tangentVec);
                        if (normalizeTangents)
                        {
                            tangentVec.NormalizeSafe();
                        }
                        tangent.Set(tangentVec.GetX(), tangentVec.GetY(), tangentVec.GetZ(), dataTangent.GetW());
                        tangentLayer->SetCurrentVertexValue(&tangent);
                    }

                    // Feed the bitangent.
                    if (meshBitangentData)
                    {
                        bitangent = meshBitangentData->GetBitangent(vertexIndex);
                        if (skinningInfo)
                        {
                            bitangent = globalTransformN.TransformVector(bitangent);
                        }
                        bitangent = coordSysConverter.ConvertVector3(bitangent);
                        if (normalizeTangents)
                        {
                            bitangent.NormalizeSafe();
                        }
                        bitangentVec.Set(bitangent.GetX(), bitangent.GetY(), bitangent.GetZ());
                        bitangentLayer->SetCurrentVertexValue(&bitangentVec);
                    }

                    // Feed the cloth data
                    if (!clothData.empty())
                    {
                        const AZ::u32 clothVertexDataU32 = clothData[vertexIndex].ToU32();
                        clothLayer->SetCurrentVertexValue(&clothVertexDataU32);
                    }

                    meshBuilder->AddPolygonVertex(orgVertexNumber);
                }

                // End the triangle.
                meshBuilder->EndPolygon();
            } // For all triangles.

            // Convert the mesh builder mesh to an EMFX mesh.
            EMotionFX::Mesh* emfxMesh = Mesh::CreateFromMeshBuilder(meshBuilder.get());

            // Try to generate tangents post vertex duplication (the old EMFX method).
            if (tangentSpace == AZ::SceneAPI::DataTypes::TangentSpace::EMotionFX)
            {
                if (!emfxMesh->CalcTangents(tangentUVSetIndex, false))
                {
                    AZ_Error("EMotionFX", false, "Failed to generate tangents for node '%s'.", emfxNode->GetName());
                }
            }
            actor->SetMesh(lodLevel, emfxNode->GetNodeIndex(), emfxMesh);

            if (!skinningInfo && settings.m_loadSkinningInfo)
            {
                CreateSkinningMeshDeformer(actor, emfxNode, emfxMesh, skinningInfo);
            }
        }


        EMotionFX::MeshBuilderSkinningInfo* ActorBuilder::ExtractSkinningInfo(AZStd::shared_ptr<const SceneDataTypes::IMeshData> meshData,
            const SceneContainers::SceneGraph& graph, const SceneContainers::SceneGraph::NodeIndex& meshNodeIndex,
            const BoneNameEmfxIndexMap& boneNameEmfxIndexMap, const ActorSettings& settings)
        {
            if (!settings.m_loadSkinningInfo)
            {
                return nullptr;
            }

            // Create the new skinning info
            EMotionFX::MeshBuilderSkinningInfo* skinningInfo = nullptr;

            auto meshChildView = SceneViews::MakeSceneGraphChildView<SceneContainers::Views::AcceptEndPointsOnly>(graph, meshNodeIndex,
                    graph.GetContentStorage().begin(), true);
            for (auto meshIt = meshChildView.begin(); meshIt != meshChildView.end(); ++meshIt)
            {
                const SceneDataTypes::ISkinWeightData* skinData = azrtti_cast<const SceneDataTypes::ISkinWeightData*>(meshIt->get());
                if (skinData)
                {
                    const size_t numUsedControlPoints = meshData->GetUsedControlPointCount();
                    if (!skinningInfo)
                    {
                        skinningInfo = EMotionFX::MeshBuilderSkinningInfo::Create(aznumeric_cast<AZ::u32>(numUsedControlPoints));
                    }

                    const size_t controlPointCount = skinData->GetVertexCount();
                    for (size_t controlPointIndex = 0; controlPointIndex < controlPointCount; ++controlPointIndex)
                    {
                        const int usedPointIndex = meshData->GetUsedPointIndexForControlPoint(controlPointIndex);
                        if (usedPointIndex < 0)
                        {
                            continue; // This control point is not used in the mesh
                        }
                        const size_t linkCount = skinData->GetLinkCount(controlPointIndex);
                        if (linkCount == 0)
                        {
                            continue;
                        }

                        EMotionFX::MeshBuilderSkinningInfo::Influence influence;
                        for (size_t linkIndex = 0; linkIndex < linkCount; ++linkIndex)
                        {
                            const SceneDataTypes::ISkinWeightData::Link& link = skinData->GetLink(controlPointIndex, linkIndex);
                            influence.mWeight = link.weight;

                            const AZStd::string& boneName = skinData->GetBoneName(link.boneId);
                            auto boneIt = boneNameEmfxIndexMap.find(boneName);
                            if (boneIt == boneNameEmfxIndexMap.end())
                            {
                                AZ_TraceContext("Missing bone in actor skinning info", boneName.c_str());
                                continue;
                            }
                            influence.mNodeNr = boneIt->second;
                            skinningInfo->AddInfluence(usedPointIndex, influence);
                        }
                    }
                }
            }

            if (skinningInfo)
            {
                skinningInfo->Optimize(settings.m_maxWeightsPerVertex, settings.m_weightThreshold);
            }

            return skinningInfo;
        }


        void ActorBuilder::CreateSkinningMeshDeformer(EMotionFX::Actor* actor, EMotionFX::Node* node, EMotionFX::Mesh* mesh, EMotionFX::MeshBuilderSkinningInfo* skinningInfo)
        {
            if (!skinningInfo)
            {
                return;
            }

            // Check if we already have a stack
            EMotionFX::MeshDeformerStack* deformerStack = actor->GetMeshDeformerStack(0, node->GetNodeIndex());
            if (!deformerStack)
            {
                // Create the stack
                EMotionFX::MeshDeformerStack* newStack = EMotionFX::MeshDeformerStack::Create(mesh);
                actor->SetMeshDeformerStack(0, node->GetNodeIndex(), newStack);
                deformerStack = actor->GetMeshDeformerStack(0, node->GetNodeIndex());
            }

            // Add a skinning deformer (it will later on get reinitialized)
            // For now we always use Linear skinning.
            EMotionFX::SoftSkinDeformer* deformer = EMotionFX::GetSoftSkinManager().CreateDeformer(mesh);
            deformerStack->AddDeformer(deformer);
        }

        void ActorBuilder::ExtractActorSettings(const Group::IActorGroup& actorGroup, ActorSettings& outSettings)
        {
            const AZ::SceneAPI::Containers::RuleContainer& rules = actorGroup.GetRuleContainerConst();
            AZStd::shared_ptr<const Rule::ISkinRule> skinRule = rules.FindFirstByType<Rule::ISkinRule>();
            if (skinRule)
            {
                outSettings.m_maxWeightsPerVertex = skinRule->GetMaxWeightsPerVertex();
                outSettings.m_weightThreshold = skinRule->GetWeightThreshold();
            }
        }

        void ActorBuilder::InstantiateMaterialGroup()
        {
            m_materialGroup = AZStd::make_shared<AZ::GFxFramework::MaterialGroup>();
        }

        bool ActorBuilder::GetMaterialInfoForActorGroup(ActorBuilderContext& context)
        {
            // Does group contain material rule?
            AZStd::shared_ptr<SceneDataTypes::IMaterialRule> materialRule = context.m_group.GetRuleContainerConst().FindFirstByType<SceneDataTypes::IMaterialRule>();
            if (!materialRule)
            {
                m_materialGroup = nullptr;
                return false;
            }

            AZStd::string materialPath = context.m_scene.GetSourceFilename();
            AzFramework::StringFunc::Path::ReplaceExtension(materialPath, AZ::GFxFramework::MaterialExport::g_mtlExtension);

            InstantiateMaterialGroup();
            bool fileRead = m_materialGroup->ReadMtlFile(materialPath.c_str());
            if (fileRead)
            {
                // Put the absolute path to the source file in the reference list, so that the Scene Builder recognizes it 
                //  as an absolute source path dependency, so that AP can resolve it.
                context.m_materialReferences.emplace_back(materialPath);
                AZ_TracePrintf(SceneUtil::LogWindow, "Actor builder uses user material %s.\n", materialPath.c_str());
                return true;
            }

            // If user material doesn't exist, use the product material generated by the FBX.
            materialPath = SceneUtil::FileUtilities::CreateOutputFileName(context.m_group.GetName(),
                    context.m_outputDirectory, AZ::GFxFramework::MaterialExport::g_mtlExtension);
            fileRead = m_materialGroup->ReadMtlFile(materialPath.c_str());
            if (fileRead)
            {
                // Scene builder will recognize relative paths in the ExportProduct's list of path dependencies as a
                //  product dependency. Get source file's path relative to the Asset Root and create the relative path 
                //  expected for the generated material, and add that to the material reference list. Can't use the output
                //  directory here as that is likely the temp directory, which can't be registered as a path dependency.
                AZStd::string relativeProductPath = context.m_scene.GetSourceFilename();
                AzFramework::StringFunc::Path::ReplaceFullName(relativeProductPath, context.m_group.GetName().c_str(), AZ::GFxFramework::MaterialExport::g_mtlExtension);
                AzFramework::ApplicationRequests::Bus::Broadcast(&AzFramework::ApplicationRequests::Bus::Events::MakePathAssetRootRelative, relativeProductPath);
                
                context.m_materialReferences.emplace_back(relativeProductPath);
                AZ_TracePrintf(SceneUtil::LogWindow, "Actor builder uses source material %s.\n", materialPath.c_str());
                return true;
            }

            // If the FBX can't generate a material, use the cached dcc material.
            materialPath = SceneUtil::FileUtilities::CreateOutputFileName(context.m_group.GetName(),
                    context.m_outputDirectory, AZ::GFxFramework::MaterialExport::g_dccMaterialExtension);
            fileRead = m_materialGroup->ReadMtlFile(materialPath.c_str());
            if (fileRead)
            {
                // Same as above for using relative paths for path dependencies to a product file.
                AZStd::string relativeProductPath = context.m_scene.GetSourceFilename();
                AzFramework::StringFunc::Path::ReplaceFullName(relativeProductPath, context.m_group.GetName().c_str(), AZ::GFxFramework::MaterialExport::g_dccMaterialExtension);
                AzFramework::ApplicationRequests::Bus::Broadcast(&AzFramework::ApplicationRequests::Bus::Events::MakePathAssetRootRelative, relativeProductPath);
                
                context.m_materialReferences.emplace_back(relativeProductPath);
                AZ_TracePrintf(SceneUtil::LogWindow, "Actor builder uses cached dcc material %s.\n", materialPath.c_str());
                return true;
            }

            // If user material, source material and dcc material all doesn't exist, we can still build the actor, but its mtl indices probably not match the mtl file.
            AZ_TracePrintf(SceneUtil::WarningWindow, "Actor builder can't find any material to get the correct actor indices.\n", materialPath.c_str());
            m_materialGroup = nullptr;
            return false;
        }


        void ActorBuilder::SetupMaterialDataForMesh(const ActorBuilderContext& context, const SceneContainers::SceneGraph::NodeIndex& meshNodeIndex)
        {
            m_materialIndexMapForMesh.clear();
            if (!m_materialGroup)
            {
                return;
            }

            const SceneContainers::SceneGraph& graph = context.m_scene.GetGraph();

            auto view = SceneViews::MakeSceneGraphChildView<SceneViews::AcceptEndPointsOnly>(
                    graph, meshNodeIndex, graph.GetContentStorage().begin(), true);
            for (auto it = view.begin(), itEnd = view.end(); it != itEnd; ++it)
            {
                if ((*it) && (*it)->RTTI_IsTypeOf(SceneDataTypes::IMaterialData::TYPEINFO_Uuid()))
                {
                    AZStd::string nodeName = graph.GetNodeName(graph.ConvertToNodeIndex(it.GetHierarchyIterator())).GetName();
                    AZ_TraceContext("Material Node Name", nodeName);

                    size_t index = m_materialGroup->FindMaterialIndex(nodeName);
                    if (index == AZ::GFxFramework::MaterialExport::g_materialNotFound)
                    {
                        AZ_TracePrintf(SceneUtil::ErrorWindow, "Unable to find material named %s in mtl file while building material index map for actor.", nodeName.c_str());
                        index = 0;
                    }
                    if (AZ::Interface<AzFramework::AtomActiveInterface>::Get())
                    {
                        const auto& materialData = AZStd::static_pointer_cast<const SceneDataTypes::IMaterialData>(*it);
                        m_materialIndexMapForMesh.push_back(aznumeric_cast<AZ::u32>(materialData->GetUniqueId()));
                    }
                    else
                    {
                        m_materialIndexMapForMesh.push_back(aznumeric_cast<AZ::u32>(index));
                    }
                }
            }
        }


        void ActorBuilder::GetNodeIndicesOfSelectedBaseMeshes(ActorBuilderContext& context, NodeIndexSet& meshNodeIndexSet) const
        {
            meshNodeIndexSet.clear();

            // When we prebuild the selected mesh node indices, we only care about the base meshes because we will later add the LOD mesh to the base mesh.
            const SceneContainers::SceneGraph& graph = context.m_scene.GetGraph();
            const SceneDataTypes::ISceneNodeSelectionList& baseNodeSelectionList = context.m_group.GetBaseNodeSelectionList();
            const size_t count = baseNodeSelectionList.GetSelectedNodeCount();
            for (size_t i = 0; i < count; ++i)
            {
                const AZStd::string& nodePath = baseNodeSelectionList.GetSelectedNode(i);
                SceneContainers::SceneGraph::NodeIndex nodeIndex = graph.Find(nodePath);
                AZ_Assert(nodeIndex.IsValid(), "Invalid scene graph node index");
                if (nodeIndex.IsValid())
                {
                    AZStd::shared_ptr<const SceneDataTypes::IMeshData> meshData = azrtti_cast<const SceneDataTypes::IMeshData*>(graph.GetNodeContent(nodeIndex));
                    if (meshData)
                    {
                        meshNodeIndexSet.insert(nodeIndex);
                    }
                }
            }
        }

        AZStd::string_view ActorBuilder::RemoveLODSuffix(const AZStd::string_view& lodName)
        {
            const size_t pos = AzFramework::StringFunc::Find(lodName, "_lod", 0, true /*reverse*/);
            if (pos != AZStd::string_view::npos)
            {
                return lodName.substr(0, pos);
            }
            return lodName;
        }
    } // namespace Pipeline
} // namespace EMotionFX
