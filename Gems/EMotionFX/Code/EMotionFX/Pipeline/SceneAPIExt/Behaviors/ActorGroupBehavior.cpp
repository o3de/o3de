/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
#include <SceneAPI/SceneCore/DataTypes/GraphData/IBlendShapeData.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IBoneData.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/ISkinWeightData.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IAnimationData.h>
#include <SceneAPI/SceneCore/DataTypes/DataTypeUtilities.h>
#include <SceneAPI/SceneCore/Utilities/SceneGraphSelector.h>
#include <SceneAPI/SceneCore/Events/GraphMetaInfoBus.h>
#include <SceneAPI/SceneData/Rules/CoordinateSystemRule.h>

#include <SceneAPIExt/Groups/ActorGroup.h>
#include <SceneAPIExt/Behaviors/ActorGroupBehavior.h>
#include <SceneAPIExt/Behaviors/LodRuleBehavior.h>
#include <SceneAPIExt/Rules/ActorPhysicsSetupRule.h>
#include <SceneAPIExt/Rules/SimulatedObjectSetupRule.h>
#include <SceneAPIExt/Rules/ActorScaleRule.h>
#include <SceneAPIExt/Rules/MetaDataRule.h>
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
                        const AZ::SceneAPI::DataTypes::IRule* rule = rules.GetRule(i).get();
                        if (rule)
                        {
                            existingRules.insert(rule->RTTI_GetType());
                        }
                        else
                        {
                            AZ_WarningOnce("EMotionFX", false, "Empty rule found in the ruleContainer, ignoring it. Check the .assetinfo file for invalid data.");
                        }
                    }
                    if (existingRules.find(Rule::ActorScaleRule::TYPEINFO_Uuid()) == existingRules.end())
                    {
                        modifiers.push_back(Rule::ActorScaleRule::TYPEINFO_Uuid());
                    }
                    if (existingRules.find(azrtti_typeid<AZ::SceneAPI::SceneData::CoordinateSystemRule>()) == existingRules.end())
                    {
                        modifiers.push_back(azrtti_typeid<AZ::SceneAPI::SceneData::CoordinateSystemRule>());
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
                    if (existingRules.find(Rule::LodRule::TYPEINFO_Uuid()) == existingRules.end())
                    {
                        modifiers.push_back(Rule::LodRule::TYPEINFO_Uuid());
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
                group->SetBestMatchingRootBone(scene.GetGraph());

                // LOD Rule need to be built first in the actor, so we know which mesh and bone belongs to LOD.
                // After this call, LOD rule will be populated with all the LOD bones
                Behavior::LodRuleBehavior::BuildLODRuleForActor(scene, *group);
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
                // Skip adding the actor group if it already exists.
                if (SceneHasActorGroup(scene))
                {
                    return AZ::SceneAPI::Events::ProcessingResult::Ignored;
                }

                // Skip adding the actor group if it doesn't contain any skin and blendshape data.
                const bool hasSkinData = AZ::SceneAPI::Utilities::DoesSceneGraphContainDataLike<AZ::SceneAPI::DataTypes::ISkinWeightData>(scene, true);
                const bool hasBlendShapeData = AZ::SceneAPI::Utilities::DoesSceneGraphContainDataLike<AZ::SceneAPI::DataTypes::IBlendShapeData>(scene, true);
                if (!hasSkinData && !hasBlendShapeData)
                {
                    return AZ::SceneAPI::Events::ProcessingResult::Ignored;
                }

                // Add a default actor group to the manifest.
                AZStd::shared_ptr<Group::ActorGroup> group = AZStd::make_shared<Group::ActorGroup>();

                // This is a group that's generated automatically so may not be saved to disk but would need to be recreated
                // in the same way again. To guarantee the same uuid, generate a stable one instead.
                group->OverrideId(AZ::SceneAPI::DataTypes::Utilities::CreateStableUuid(scene, Group::ActorGroup::TYPEINFO_Uuid()));

                EBUS_EVENT(AZ::SceneAPI::Events::ManifestMetaInfoBus, InitializeObject, scene, *group);
                scene.GetManifest().AddEntry(AZStd::move(group));

                return AZ::SceneAPI::Events::ProcessingResult::Success;
            }

            AZ::SceneAPI::Events::ProcessingResult ActorGroupBehavior::UpdateActorGroups(AZ::SceneAPI::Containers::Scene& scene) const
            {
                bool updated = false;
                SceneContainers::SceneManifest& manifest = scene.GetManifest();
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

                    // Remove nullptr in the ruleContainer.
                    AZ::SceneAPI::Containers::RuleContainer& rules = group.GetRuleContainer();
                    size_t ruleIndex = 0;
                    while (ruleIndex < rules.GetRuleCount())
                    {
                        if (!rules.GetRule(ruleIndex))
                        {
                            rules.RemoveRule(ruleIndex);
                        }
                        else
                        {
                            ruleIndex++;
                        }
                    }
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
