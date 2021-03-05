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

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/smart_ptr/make_shared.h>

#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneCore/Containers/SceneGraph.h>
#include <SceneAPI/SceneCore/Containers/Views/SceneGraphDownwardsIterator.h>
#include <SceneAPI/SceneCore/Containers/Views/PairIterator.h>
#include <SceneAPI/SceneCore/Containers/Utilities/Filters.h>
#include <SceneAPI/SceneCore/Containers/Utilities/SceneGraphUtilities.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IMeshData.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IBoneData.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/ISkinWeightData.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IAnimationData.h>
#include <SceneAPI/SceneCore/DataTypes/DataTypeUtilities.h>
#include <SceneAPI/SceneCore/Utilities/SceneGraphSelector.h>
#include <SceneAPI/SceneCore/Events/GraphMetaInfoBus.h>
#include <SceneAPI/SceneData/Rules/MaterialRule.h>
#include <SceneAPI/SceneData/Rules/TangentsRule.h>

#include <SceneAPIExt/Groups/ActorGroup.h>
#include <SceneAPIExt/Behaviors/ActorGroupBehavior.h>
#include <SceneAPIExt/Behaviors/MeshRuleBehavior.h>
#include <SceneAPIExt/Behaviors/SkinRuleBehavior.h>
#include <SceneAPIExt/Behaviors/LodRuleBehavior.h>
#include <SceneAPIExt/Rules/ActorPhysicsSetupRule.h>
#include <SceneAPIExt/Rules/SimulatedObjectSetupRule.h>
#include <SceneAPIExt/Rules/ActorScaleRule.h>
#include <SceneAPIExt/Rules/MetaDataRule.h>
#include <SceneAPIExt/Rules/MeshRule.h>
#include <SceneAPIExt/Rules/SkinRule.h>
#include <SceneAPIExt/Rules/CoordinateSystemRule.h>
#include <SceneAPIExt/Rules/MorphTargetRule.h>
#include <SceneAPIExt/Rules/LodRule.h>
#include <SceneAPIExt/Rules/SkeletonOptimizationRule.h>

namespace EMotionFX
{
    namespace Pipeline
    {
        namespace Behavior
        {
            const int ActorGroupBehavior::s_animationsPreferredTabOrder = 3;

            void ActorGroupBehavior::Reflect(AZ::ReflectContext* context)
            {
                Group::ActorGroup::Reflect(context);
                Rule::ActorPhysicsSetupRule::Reflect(context);
                Rule::SimulatedObjectSetupRule::Reflect(context);
                Rule::ActorScaleRule::Reflect(context);
                Rule::MetaDataRule::Reflect(context);
                Rule::CoordinateSystemRule::Reflect(context);
                Rule::MorphTargetRule::Reflect(context);
                Rule::LodRule::Reflect(context);
                Rule::SkeletonOptimizationRule::Reflect(context);

                AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
                if (serializeContext)
                {
                    serializeContext->Class<ActorGroupBehavior, AZ::SceneAPI::SceneCore::BehaviorComponent>()->Version(1);
                }
            }

            void ActorGroupBehavior::Activate()
            {
                AZ::SceneAPI::Events::ManifestMetaInfoBus::Handler::BusConnect();
                AZ::SceneAPI::Events::AssetImportRequestBus::Handler::BusConnect();
            }

            void ActorGroupBehavior::Deactivate()
            {
                AZ::SceneAPI::Events::AssetImportRequestBus::Handler::BusDisconnect();
                AZ::SceneAPI::Events::ManifestMetaInfoBus::Handler::BusDisconnect();
            }

            void ActorGroupBehavior::GetCategoryAssignments(CategoryRegistrationList& categories, const AZ::SceneAPI::Containers::Scene& scene)
            {
                const bool hasRequiredData = AZ::SceneAPI::Utilities::DoesSceneGraphContainDataLike<AZ::SceneAPI::DataTypes::IBoneData>(scene, false);
                if (SceneHasActorGroup(scene) || hasRequiredData)
                {
                    categories.emplace_back("Actors", Group::ActorGroup::TYPEINFO_Uuid(), s_animationsPreferredTabOrder);
                }
            }

            void ActorGroupBehavior::GetAvailableModifiers(ModifiersList & modifiers, const AZ::SceneAPI::Containers::Scene & scene, const AZ::SceneAPI::DataTypes::IManifestObject & target)
            {
                AZ_TraceContext("Object Type", target.RTTI_GetTypeName());
                if (target.RTTI_IsTypeOf(Group::IActorGroup::TYPEINFO_Uuid()))
                {
                    const Group::IActorGroup* group = azrtti_cast<const Group::IActorGroup*>(&target);
                    const AZ::SceneAPI::Containers::RuleContainer& rules = group->GetRuleContainerConst();

                    AZStd::unordered_set<AZ::Uuid> existingRules;
                    const size_t ruleCount = rules.GetRuleCount();
                    for (size_t i = 0; i < ruleCount; ++i)
                    {
                        existingRules.insert(rules.GetRule(i)->RTTI_GetType());
                    }
                    if (existingRules.find(Rule::MeshRule::TYPEINFO_Uuid()) == existingRules.end())
                    {
                        modifiers.push_back(Rule::MeshRule::TYPEINFO_Uuid());
                    }
                    if (existingRules.find(AZ::SceneAPI::SceneData::MaterialRule::TYPEINFO_Uuid()) == existingRules.end())
                    {
                        modifiers.push_back(AZ::SceneAPI::SceneData::MaterialRule::TYPEINFO_Uuid());
                    }
                    if (existingRules.find(Rule::SkinRule::TYPEINFO_Uuid()) == existingRules.end())
                    {
                        modifiers.push_back(Rule::SkinRule::TYPEINFO_Uuid());
                    }
                    if (existingRules.find(Rule::ActorScaleRule::TYPEINFO_Uuid()) == existingRules.end())
                    {
                        modifiers.push_back(Rule::ActorScaleRule::TYPEINFO_Uuid());
                    }
                    if (existingRules.find(Rule::CoordinateSystemRule::TYPEINFO_Uuid()) == existingRules.end())
                    {
                        modifiers.push_back(Rule::CoordinateSystemRule::TYPEINFO_Uuid());
                    }
                    if (existingRules.find(Rule::SkeletonOptimizationRule::TYPEINFO_Uuid()) == existingRules.end())
                    {
                        modifiers.push_back(Rule::SkeletonOptimizationRule::TYPEINFO_Uuid());
                    }
                    if (existingRules.find(Rule::MorphTargetRule::TYPEINFO_Uuid()) == existingRules.end())
                    {
                        AZ::SceneAPI::SceneData::SceneNodeSelectionList selection;
                        const size_t morphTargetShapeCount = Rule::MorphTargetRule::SelectMorphTargets(scene, selection);
                        if (morphTargetShapeCount > 0)
                        {
                            modifiers.push_back(Rule::MorphTargetRule::TYPEINFO_Uuid());
                        }
                    }
                    if (existingRules.find(AZ::SceneAPI::SceneData::TangentsRule::TYPEINFO_Uuid()) == existingRules.end())
                    {
                        modifiers.push_back(AZ::SceneAPI::SceneData::TangentsRule::TYPEINFO_Uuid());
                    }
                }

            }

            void ActorGroupBehavior::InitializeObject(const AZ::SceneAPI::Containers::Scene& scene, AZ::SceneAPI::DataTypes::IManifestObject& target)
            {
                if (!target.RTTI_IsTypeOf(Group::ActorGroup::TYPEINFO_Uuid()))
                {
                    return;
                }

                Group::ActorGroup* group = azrtti_cast<Group::ActorGroup*>(&target);
                group->SetName(AZ::SceneAPI::DataTypes::Utilities::CreateUniqueName<Group::IActorGroup>(scene.GetName(), scene.GetManifest()));

                // LOD Rule need to be built first in the actor, so we know which mesh and bone belongs to LOD.
                // After this call, LOD rule will be populated with all the LOD nodes (bones and meshes).
                Behavior::LodRuleBehavior::BuildLODRuleForActor(scene, *group);

                const AZ::SceneAPI::Containers::SceneGraph& graph = scene.GetGraph();

                // For LOD mesh that belongs to LOD rule, we don't add it.
                AZ::SceneAPI::Containers::RuleContainer& rules = group->GetRuleContainer();
                AZStd::shared_ptr<Rule::LodRule> lodRule = rules.FindFirstByType<Rule::LodRule>();

                // Gather all the mesh nodes that should be selected by default
                AZ::SceneAPI::DataTypes::ISceneNodeSelectionList& allMeshSelectionList = group->GetSceneNodeSelectionList();
                AZ::SceneAPI::Utilities::SceneGraphSelector::UnselectAll(graph, allMeshSelectionList);

                AZ::SceneAPI::DataTypes::ISceneNodeSelectionList& baseMeshSelectionList = group->GetBaseNodeSelectionList();
                AZ::SceneAPI::Utilities::SceneGraphSelector::UnselectAll(graph, baseMeshSelectionList);

                AZ::SceneAPI::Containers::SceneGraph::ContentStorageConstData graphContent = graph.GetContentStorage();
                auto view = AZ::SceneAPI::Containers::Views::MakeFilterView(graphContent, AZ::SceneAPI::Containers::DerivedTypeFilter<AZ::SceneAPI::DataTypes::IMeshData>());
                for (auto iter = view.begin(), iterEnd = view.end(); iter != iterEnd; ++iter)
                {
                    AZ::SceneAPI::Containers::SceneGraph::NodeIndex nodeIndex = graph.ConvertToNodeIndex(iter.GetBaseIterator());
                    const char* nodePath = graph.GetNodeName(nodeIndex).GetPath();
                    if (!lodRule || !lodRule->ContainsNodeByPath(nodePath))
                    {
                        // We will only add the base mesh to the base mesh list.
                        baseMeshSelectionList.AddSelectedNode(nodePath);
                    }
                    // We will add all the mesh to the all mesh selection list.
                    allMeshSelectionList.AddSelectedNode(nodePath);
                }
                // Since the selection list contain both selected and unselected list, we need to call this function to make sure they matched up.
                AZ::SceneAPI::Utilities::SceneGraphSelector::UpdateNodeSelection(graph, allMeshSelectionList);
                AZ::SceneAPI::Utilities::SceneGraphSelector::UpdateNodeSelection(graph, baseMeshSelectionList);

                if (!rules.ContainsRuleOfType<Rule::CoordinateSystemRule>())
                {
                    rules.AddRule(AZStd::make_shared<Rule::CoordinateSystemRule>());
                }

                if (!rules.ContainsRuleOfType<AZ::SceneAPI::SceneData::TangentsRule>())
                {
                    rules.AddRule(AZStd::make_shared<AZ::SceneAPI::SceneData::TangentsRule>());
                }
            }

            AZ::SceneAPI::Events::ProcessingResult ActorGroupBehavior::UpdateManifest(AZ::SceneAPI::Containers::Scene& scene, ManifestAction action,
                RequestingApplication /*requester*/)
            {
                if (action == ManifestAction::ConstructDefault)
                {
                    return BuildDefault(scene);
                }
                else if (action == ManifestAction::Update)
                {
                    return UpdateActorGroups(scene);
                }
                else
                {
                    return AZ::SceneAPI::Events::ProcessingResult::Ignored;
                }
            }

            AZ::SceneAPI::Events::ProcessingResult ActorGroupBehavior::BuildDefault(AZ::SceneAPI::Containers::Scene& scene) const
            {
                const bool hasBoneOrSkinData = AZ::SceneAPI::Utilities::DoesSceneGraphContainDataLike<AZ::SceneAPI::DataTypes::IBoneData>(scene, true) ||
                                                AZ::SceneAPI::Utilities::DoesSceneGraphContainDataLike<AZ::SceneAPI::DataTypes::ISkinWeightData>(scene, true);
                const bool hasAnimationData = AZ::SceneAPI::Utilities::DoesSceneGraphContainDataLike<AZ::SceneAPI::DataTypes::IAnimationData>(scene, true);
                if (SceneHasActorGroup(scene) || !hasBoneOrSkinData || hasAnimationData)
                {
                    return AZ::SceneAPI::Events::ProcessingResult::Ignored;
                }

                // Add a default actor group to the manifest.
                AZStd::shared_ptr<Group::ActorGroup> group = AZStd::make_shared<Group::ActorGroup>();

                // This is a group that's generated automatically so may not be saved to disk but would need to be recreated
                //      in the same way again. To guarantee the same uuid, generate a stable one instead.
                group->OverrideId(AZ::SceneAPI::DataTypes::Utilities::CreateStableUuid(scene, Group::ActorGroup::TYPEINFO_Uuid()));

                EBUS_EVENT(AZ::SceneAPI::Events::ManifestMetaInfoBus, InitializeObject, scene, *group);
                scene.GetManifest().AddEntry(AZStd::move(group));

                return AZ::SceneAPI::Events::ProcessingResult::Success;
            }

            AZ::SceneAPI::Events::ProcessingResult ActorGroupBehavior::UpdateActorGroups(AZ::SceneAPI::Containers::Scene& scene) const
            {
                bool updated = false;
                SceneContainers::SceneManifest& manifest = scene.GetManifest();
                const SceneContainers::SceneGraph& sceneGraph = scene.GetGraph();
                auto valueStorage = manifest.GetValueStorage();
                auto view = AZ::SceneAPI::Containers::MakeDerivedFilterView<Group::ActorGroup>(valueStorage);
                for (Group::ActorGroup& group : view)
                {
                    if (group.GetName().empty())
                    {
                        group.SetName(AZ::SceneAPI::DataTypes::Utilities::CreateUniqueName<Group::IActorGroup>(scene.GetName(), scene.GetManifest()));
                        updated = true;
                    }
                    if (group.GetId().IsNull())
                    {
                        // When the uuid it's null is likely because the manifest has been updated from an older version. Include the 
                        // name of the group as there could be multiple groups.
                        group.OverrideId(AZ::SceneAPI::DataTypes::Utilities::CreateStableUuid(scene, Group::ActorGroup::TYPEINFO_Uuid(), group.GetName()));
                        updated = true;
                    }
                    SceneDataTypes::ISceneNodeSelectionList& baseNodeList = group.GetBaseNodeSelectionList();
                    SceneDataTypes::ISceneNodeSelectionList& allNodeList = group.GetSceneNodeSelectionList();

                    AZ::SceneAPI::Utilities::SceneGraphSelector::UpdateNodeSelection(sceneGraph, baseNodeList);
                    AZ::SceneAPI::Utilities::SceneGraphSelector::UnselectAll(sceneGraph, allNodeList);
                    // Populate the all node selection list based on the base node list and lod node list.
                    const size_t baseMeshCount = baseNodeList.GetSelectedNodeCount();
                    for (size_t i = 0; i < baseMeshCount; ++i)
                    {
                        allNodeList.AddSelectedNode(baseNodeList.GetSelectedNode(i));
                    }

                    const AZ::SceneAPI::Containers::RuleContainer& rules = group.GetRuleContainer();

                    // Update LOD rule
                    const AZStd::shared_ptr<Rule::LodRule> lodRule = rules.FindFirstByType<Rule::LodRule>();
                    if (lodRule)
                    {
                        const size_t lodCount = lodRule->GetLodRuleCount();
                        for (size_t lodIndex = 0; lodIndex < lodCount; ++lodIndex)
                        {
                            const SceneDataTypes::ISceneNodeSelectionList& lodMeshList = lodRule->GetSceneNodeSelectionList(lodIndex);
                            const size_t lodMeshCount = lodMeshList.GetSelectedNodeCount();
                            for (size_t meshIndex = 0; meshIndex < lodMeshCount; ++meshIndex)
                            {
                                allNodeList.AddSelectedNode(lodMeshList.GetSelectedNode(meshIndex));
                            }
                        }
                    }
                    AZ::SceneAPI::Utilities::SceneGraphSelector::UpdateNodeSelection(sceneGraph, allNodeList);
                }

                return updated ? AZ::SceneAPI::Events::ProcessingResult::Success : AZ::SceneAPI::Events::ProcessingResult::Ignored;
            }

            bool ActorGroupBehavior::SceneHasActorGroup(const AZ::SceneAPI::Containers::Scene& scene) const
            {
                const AZ::SceneAPI::Containers::SceneManifest& manifest = scene.GetManifest();
                AZ::SceneAPI::Containers::SceneManifest::ValueStorageConstData manifestData = manifest.GetValueStorage();
                auto actorGroup = AZStd::find_if(manifestData.begin(), manifestData.end(), AZ::SceneAPI::Containers::DerivedTypeFilter<Group::IActorGroup>());
                return actorGroup != manifestData.end();
            }
        } // Behavior
    } // Pipeline
} // EMotionFX
