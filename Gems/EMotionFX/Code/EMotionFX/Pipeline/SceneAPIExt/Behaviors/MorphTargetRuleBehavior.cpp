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
#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneCore/Containers/SceneGraph.h>
#include <SceneAPI/SceneCore/Containers/SceneManifest.h>
#include <SceneAPI/SceneCore/Containers/Utilities/Filters.h>
#include <SceneAPI/SceneCore/Events/GraphMetaInfoBus.h>
#include <SceneAPI/SceneCore/Utilities/SceneGraphSelector.h>
#include <SceneAPI/SceneData/ManifestBase/SceneNodeSelectionList.h>

#include <SceneAPIExt/Groups/IActorGroup.h>
#include <SceneAPIExt/Groups/IMotionGroup.h>
#include <SceneAPIExt/Rules/MorphTargetRule.h>
#include <SceneAPIExt/Behaviors/MorphTargetRuleBehavior.h>

namespace EMotionFX
{
    namespace Pipeline
    {
        namespace Behavior
        {
            void MorphTargetRuleBehavior::Activate()
            {
                AZ::SceneAPI::Events::ManifestMetaInfoBus::Handler::BusConnect();
                AZ::SceneAPI::Events::AssetImportRequestBus::Handler::BusConnect();
            }

            void MorphTargetRuleBehavior::Deactivate()
            {
                AZ::SceneAPI::Events::AssetImportRequestBus::Handler::BusDisconnect();
                AZ::SceneAPI::Events::ManifestMetaInfoBus::Handler::BusDisconnect();
            }

            void MorphTargetRuleBehavior::Reflect(AZ::ReflectContext* context)
            {
                AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
                if (serializeContext)
                {
                    serializeContext->Class<MorphTargetRuleBehavior, BehaviorComponent>()->Version(1);
                }
            }

            void MorphTargetRuleBehavior::InitializeObject(const AZ::SceneAPI::Containers::Scene& scene, AZ::SceneAPI::DataTypes::IManifestObject& target)
            {
                if (target.RTTI_IsTypeOf(Group::IActorGroup::TYPEINFO_Uuid()))
                {
                    AZ::SceneAPI::SceneData::SceneNodeSelectionList selection;
                    const size_t morphTargetShapeCount = Rule::MorphTargetRule::SelectMorphTargets(scene, selection);

                    if (morphTargetShapeCount > 0)
                    {
                        AZStd::shared_ptr<Rule::MorphTargetRule> morphTargetRule = AZStd::make_shared<Rule::MorphTargetRule>();
                        selection.CopyTo(morphTargetRule->GetSceneNodeSelectionList());
                        
                        Group::IActorGroup* actorGroup = azrtti_cast<Group::IActorGroup*>(&target);
                        actorGroup->GetRuleContainer().AddRule(AZStd::move(morphTargetRule));
                    }
                }
                else if (target.RTTI_IsTypeOf(Group::IMotionGroup::TYPEINFO_Uuid()))
                {
                    const size_t morphCount = Rule::MorphTargetRuleReadOnly::DetectMorphTargetAnimations(scene);
                    if (morphCount)
                    {
                        Group::IMotionGroup* motionGroup = azrtti_cast<Group::IMotionGroup*>(&target);
                        motionGroup->GetRuleContainer().AddRule(AZStd::make_shared<Rule::MorphTargetRuleReadOnly>(morphCount));
                    }
                }
                else if (target.RTTI_IsTypeOf(Rule::MorphTargetRule::TYPEINFO_Uuid()))
                {
                    Rule::MorphTargetRule* rule = azrtti_cast<Rule::MorphTargetRule*>(&target);
                    Rule::MorphTargetRule::SelectMorphTargets(scene, rule->GetSceneNodeSelectionList());
                }
            }

            AZ::SceneAPI::Events::ProcessingResult MorphTargetRuleBehavior::UpdateManifest(AZ::SceneAPI::Containers::Scene& scene, ManifestAction action,
                RequestingApplication /*requester*/)
            {
                if (action == ManifestAction::Update)
                {
                    UpdateMorphTargetRules(scene);
                    return AZ::SceneAPI::Events::ProcessingResult::Success;
                }
                else
                {
                    return AZ::SceneAPI::Events::ProcessingResult::Ignored;
                }
            }

            void MorphTargetRuleBehavior::UpdateMorphTargetRules(const AZ::SceneAPI::Containers::Scene& scene) const
            {
                const AZ::SceneAPI::Containers::SceneManifest& manifest = scene.GetManifest();
                auto valueStorage = manifest.GetValueStorage();

                auto view = AZ::SceneAPI::Containers::MakeDerivedFilterView<AZ::SceneAPI::DataTypes::ISceneNodeGroup>(valueStorage);
                for (const AZ::SceneAPI::DataTypes::ISceneNodeGroup& group : view)
                {
                    const AZ::SceneAPI::Containers::RuleContainer& rules = group.GetRuleContainerConst();
                    const size_t ruleCount = rules.GetRuleCount();
                    for (size_t index = 0; index < ruleCount; ++index)
                    {
                        Rule::MorphTargetRule* rule = azrtti_cast<Rule::MorphTargetRule*>(rules.GetRule(index).get());
                        if (rule)
                        {
                            AZ::SceneAPI::Utilities::SceneGraphSelector::UpdateNodeSelection(scene.GetGraph(), rule->GetSceneNodeSelectionList());
                        }
                    }
                }
            }
        } // namespace Behavior
    } // namespace Pipeline
} // namespace EMotionFX
