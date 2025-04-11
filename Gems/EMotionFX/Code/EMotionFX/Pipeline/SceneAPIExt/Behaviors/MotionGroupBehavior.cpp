/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <SceneAPI/SceneData/GraphData/RootBoneData.h>
#include <SceneAPI/SceneCore/Containers/Utilities/SceneGraphUtilities.h>
#include <SceneAPI/SceneCore/Containers/Views/SceneGraphDownwardsIterator.h>
#include <SceneAPI/SceneCore/Containers/Views/PairIterator.h>
#include <SceneAPI/SceneCore/DataTypes/DataTypeUtilities.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IAnimationData.h>
#include <SceneAPI/SceneData/Rules/CoordinateSystemRule.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/smart_ptr/make_shared.h>

#include <SceneAPIExt/Behaviors/MotionGroupBehavior.h>
#include <SceneAPIExt/Groups/MotionGroup.h>
#include <SceneAPIExt/Rules/MotionAdditiveRule.h>
#include <SceneAPIExt/Rules/MotionCompressionSettingsRule.h>
#include <SceneAPIExt/Rules/MotionMetaDataRule.h>
#include <SceneAPIExt/Rules/MotionSamplingRule.h>
#include <SceneAPIExt/Rules/MotionRangeRule.h>
#include <SceneAPIExt/Rules/MorphTargetRule.h>
#include <SceneAPIExt/Rules/RootMotionExtractionRule.h>

namespace EMotionFX
{
    namespace Pipeline
    {
        namespace Behavior
        {
            void MotionGroupBehavior::Reflect(AZ::ReflectContext* context)
            {
                Group::MotionGroup::Reflect(context);
                Rule::MotionAdditiveRule::Reflect(context);
                Rule::MotionCompressionSettingsRule::Reflect(context);
                Rule::MotionMetaData::Reflect(context);
                Rule::MotionMetaDataRule::Reflect(context);
                Rule::MotionSamplingRule::Reflect(context);
                Rule::MorphTargetRuleReadOnly::Reflect(context);
                
                AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
                if (serializeContext)
                {
                    serializeContext->Class<MotionGroupBehavior, AZ::SceneAPI::SceneCore::BehaviorComponent>()->Version(1);
                }
            }

            void MotionGroupBehavior::Activate()
            {
                AZ::SceneAPI::Events::ManifestMetaInfoBus::Handler::BusConnect();
                AZ::SceneAPI::Events::AssetImportRequestBus::Handler::BusConnect();
            }

            void MotionGroupBehavior::Deactivate()
            {
                AZ::SceneAPI::Events::AssetImportRequestBus::Handler::BusDisconnect();
                AZ::SceneAPI::Events::ManifestMetaInfoBus::Handler::BusDisconnect();
            }

            void MotionGroupBehavior::GetCategoryAssignments(CategoryRegistrationList& categories, const AZ::SceneAPI::Containers::Scene& scene)
            {
                if (SceneHasMotionGroup(scene) || AZ::SceneAPI::Utilities::DoesSceneGraphContainDataLike<AZ::SceneAPI::DataTypes::IAnimationData>(scene, false))
                {
                    categories.emplace_back("Motions", Group::MotionGroup::TYPEINFO_Uuid(), s_motionGroupPreferredTabOrder);
                }
            }

            void MotionGroupBehavior::GetAvailableModifiers(ModifiersList & modifiers, [[maybe_unused]] const AZ::SceneAPI::Containers::Scene & scene, const AZ::SceneAPI::DataTypes::IManifestObject & target)
            {
                if (target.RTTI_IsTypeOf(Group::IMotionGroup::TYPEINFO_Uuid()))
                {
                    const Group::IMotionGroup* group = azrtti_cast<const Group::IMotionGroup*>(&target);
                    const AZ::SceneAPI::Containers::RuleContainer& rules = group->GetRuleContainerConst();

                    AZStd::unordered_set<AZ::Uuid> existingRules;
                    const size_t ruleCount = rules.GetRuleCount();
                    for (size_t i = 0; i < ruleCount; ++i)
                    {
                        existingRules.insert(rules.GetRule(i)->RTTI_GetType());
                    }
                    if (existingRules.find(azrtti_typeid<AZ::SceneAPI::SceneData::CoordinateSystemRule>()) == existingRules.end())
                    {
                        modifiers.push_back(azrtti_typeid<AZ::SceneAPI::SceneData::CoordinateSystemRule>());
                    }
                    if (existingRules.find(Rule::MotionRangeRule::TYPEINFO_Uuid()) == existingRules.end())
                    {
                        modifiers.push_back(Rule::MotionRangeRule::TYPEINFO_Uuid());
                    }
                    if (existingRules.find(Rule::MotionAdditiveRule::TYPEINFO_Uuid()) == existingRules.end())
                    {
                        modifiers.push_back(Rule::MotionAdditiveRule::TYPEINFO_Uuid());
                    }
                    if (existingRules.find(Rule::MotionSamplingRule::TYPEINFO_Uuid()) == existingRules.end())
                    {
                        modifiers.push_back(Rule::MotionSamplingRule::TYPEINFO_Uuid());
                    }
                    if (existingRules.find(Rule::RootMotionExtractionRule::TYPEINFO_Uuid()) == existingRules.end())
                    {
                        modifiers.push_back(Rule::RootMotionExtractionRule::TYPEINFO_Uuid());
                    }
                }
            }

            void MotionGroupBehavior::InitializeObject(const AZ::SceneAPI::Containers::Scene& scene, AZ::SceneAPI::DataTypes::IManifestObject& target)
            {
                if (!target.RTTI_IsTypeOf(Group::MotionGroup::TYPEINFO_Uuid()))
                {
                    return;
                }

                Group::MotionGroup* group = azrtti_cast<Group::MotionGroup*>(&target);
                group->SetName(AZ::SceneAPI::DataTypes::Utilities::CreateUniqueName<Group::IMotionGroup>(scene.GetName(), scene.GetManifest()));

                AZ::SceneAPI::Containers::RuleContainer& rules = group->GetRuleContainer();

                if (!rules.ContainsRuleOfType<Rule::MotionSamplingRule>())
                {
                    rules.AddRule(AZStd::make_shared<Rule::MotionSamplingRule>());
                }

                const AZ::SceneAPI::Containers::SceneGraph& graph = scene.GetGraph();
                auto nameStorage = graph.GetNameStorage();
                auto contentStorage = graph.GetContentStorage();

                auto nameContentView = AZ::SceneAPI::Containers::Views::MakePairView(nameStorage, contentStorage);
                AZStd::string shallowestRootBoneName;
                auto graphDownwardsView = AZ::SceneAPI::Containers::Views::MakeSceneGraphDownwardsView<AZ::SceneAPI::Containers::Views::BreadthFirst>(graph, graph.GetRoot(), nameContentView.begin(), true);
                for (auto it = graphDownwardsView.begin(); it != graphDownwardsView.end(); ++it)
                {
                    if (!it->second)
                    {
                        continue;
                    }
                    if (it->second->RTTI_IsTypeOf(AZ::SceneData::GraphData::RootBoneData::TYPEINFO_Uuid()))
                    {
                        shallowestRootBoneName = it->first.GetPath();
                        break;
                    }
                }
                group->SetSelectedRootBone(shallowestRootBoneName);
            }

            AZ::SceneAPI::Events::ProcessingResult MotionGroupBehavior::UpdateManifest(AZ::SceneAPI::Containers::Scene& scene, ManifestAction action,
                RequestingApplication /*requester*/)
            {
                if (action == ManifestAction::ConstructDefault)
                {
                    return BuildDefault(scene);
                }
                else if (action == ManifestAction::Update)
                {
                    return UpdateMotionGroupBehaviors(scene);
                }
                else
                {
                    return AZ::SceneAPI::Events::ProcessingResult::Ignored;
                }
            }

            AZ::SceneAPI::Events::ProcessingResult MotionGroupBehavior::BuildDefault(AZ::SceneAPI::Containers::Scene& scene) const
            {
                if (SceneHasMotionGroup(scene) || !AZ::SceneAPI::Utilities::DoesSceneGraphContainDataLike<AZ::SceneAPI::DataTypes::IAnimationData>(scene, true))
                {
                    return AZ::SceneAPI::Events::ProcessingResult::Ignored;
                }

                // There are animations but no motion group, so add a default motion group to the manifest.
                AZStd::shared_ptr<Group::MotionGroup> group = AZStd::make_shared<Group::MotionGroup>();

                // This is a group that's generated automatically so may not be saved to disk but would need to be recreated
                //      in the same way again. To guarantee the same uuid, generate a stable one instead.
                group->OverrideId(AZ::SceneAPI::DataTypes::Utilities::CreateStableUuid(scene, Group::MotionGroup::TYPEINFO_Uuid()));

                AZ::SceneAPI::Events::ManifestMetaInfoBus::Broadcast(
                    &AZ::SceneAPI::Events::ManifestMetaInfoBus::Events::InitializeObject, scene, *group);
                scene.GetManifest().AddEntry(AZStd::move(group));

                return AZ::SceneAPI::Events::ProcessingResult::Success;
            }

            AZ::SceneAPI::Events::ProcessingResult MotionGroupBehavior::UpdateMotionGroupBehaviors(AZ::SceneAPI::Containers::Scene& scene) const
            {
                bool updated = false;
                AZ::SceneAPI::Containers::SceneManifest& manifest = scene.GetManifest();
                auto valueStorage = manifest.GetValueStorage();
                auto view = AZ::SceneAPI::Containers::MakeDerivedFilterView<Group::MotionGroup>(valueStorage);
                const size_t morphAnimationCount = Rule::MorphTargetRuleReadOnly::DetectMorphTargetAnimations(scene);
                for (Group::MotionGroup& group : view)
                {
                    if (group.GetName().empty())
                    {
                        group.SetName(AZ::SceneAPI::DataTypes::Utilities::CreateUniqueName<Group::IMotionGroup>(scene.GetName(), scene.GetManifest()));
                        updated = true;
                    }
                    if (group.GetId().IsNull())
                    {
                        // When the uuid it's null is likely because the manifest has been updated from an older version. Include the 
                        // name of the group as there could be multiple groups.
                        group.OverrideId(AZ::SceneAPI::DataTypes::Utilities::CreateStableUuid(scene, Group::MotionGroup::TYPEINFO_Uuid(), group.GetName()));
                        updated = true;
                    }

                    AZ::SceneAPI::Containers::RuleContainer& rules = group.GetRuleContainer();
                    AZStd::shared_ptr<Rule::MotionSamplingRule> motionSamplingRule = rules.FindFirstByType<Rule::MotionSamplingRule>();
                    if (!motionSamplingRule)
                    {
                        group.GetRuleContainer().AddRule(AZStd::make_shared<Rule::MotionSamplingRule>());
                    }

                    if (morphAnimationCount)
                    {
                        AZStd::shared_ptr<Rule::MorphTargetRuleReadOnly> rule = rules.FindFirstByType<Rule::MorphTargetRuleReadOnly>();
                        if (rule)
                        { 
                            if (rule->GetMorphAnimationCount() != morphAnimationCount)
                            {
                                rule->SetMorphAnimationCount(morphAnimationCount);
                            }
                        }
                        else
                        {
                            rules.AddRule(AZStd::make_shared<Rule::MorphTargetRuleReadOnly>(morphAnimationCount));
                        }
                    }
                    else
                    {
                        AZStd::shared_ptr<Rule::MorphTargetRuleReadOnly> rule = rules.FindFirstByType<Rule::MorphTargetRuleReadOnly>();
                        if (rule)
                        {
                            rules.RemoveRule(rule);
                        }
                    }
                }

                return updated ? AZ::SceneAPI::Events::ProcessingResult::Success : AZ::SceneAPI::Events::ProcessingResult::Ignored;
            }

            bool MotionGroupBehavior::SceneHasMotionGroup(const AZ::SceneAPI::Containers::Scene& scene) const
            {
                const AZ::SceneAPI::Containers::SceneManifest& manifest = scene.GetManifest();
                AZ::SceneAPI::Containers::SceneManifest::ValueStorageConstData manifestData = manifest.GetValueStorage();
                auto motionGroup = AZStd::find_if(manifestData.begin(), manifestData.end(), AZ::SceneAPI::Containers::DerivedTypeFilter<Group::IMotionGroup>());
                return motionGroup != manifestData.end();
            }
        } // Behavior
    } // Pipeline
} // EMotionFX
