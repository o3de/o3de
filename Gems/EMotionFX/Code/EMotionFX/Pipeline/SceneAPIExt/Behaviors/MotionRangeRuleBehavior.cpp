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
#include <AzCore/std/smart_ptr/make_shared.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneCore/Containers/SceneGraph.h>
#include <SceneAPI/SceneCore/Containers/Utilities/Filters.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IAnimationData.h>
#include <SceneAPIExt/Groups/MotionGroup.h>
#include <SceneAPIExt/Rules/MotionRangeRule.h>
#include <SceneAPIExt/Behaviors/MotionRangeRuleBehavior.h>

namespace EMotionFX
{
    namespace Pipeline
    {
        namespace Behavior
        {
            void MotionRangeRuleBehavior::Reflect(AZ::ReflectContext* context)
            {
                Rule::MotionRangeRule::Reflect(context);

                AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
                if (serializeContext)
                {
                    serializeContext->Class<MotionRangeRuleBehavior, AZ::SceneAPI::SceneCore::BehaviorComponent>()->Version(1);
                }
            }

            void MotionRangeRuleBehavior::Activate()
            {
                AZ::SceneAPI::Events::ManifestMetaInfoBus::Handler::BusConnect();
                AZ::SceneAPI::Events::AssetImportRequestBus::Handler::BusConnect();
            }

            void MotionRangeRuleBehavior::Deactivate()
            {
                AZ::SceneAPI::Events::AssetImportRequestBus::Handler::BusDisconnect();
                AZ::SceneAPI::Events::ManifestMetaInfoBus::Handler::BusDisconnect();
            }

            void MotionRangeRuleBehavior::InitializeObject(const AZ::SceneAPI::Containers::Scene& scene, AZ::SceneAPI::DataTypes::IManifestObject& target)
            {
                if (target.RTTI_IsTypeOf(Rule::MotionRangeRule::TYPEINFO_Uuid()))
                {
                    Rule::MotionRangeRule* motionRangeRule = azrtti_cast<Rule::MotionRangeRule*>(&target);

                    auto contentStorage = scene.GetGraph().GetContentStorage();
                    auto animationData = AZStd::find_if(contentStorage.begin(), contentStorage.end(), AZ::SceneAPI::Containers::DerivedTypeFilter<AZ::SceneAPI::DataTypes::IAnimationData>());
                    if (animationData == contentStorage.end())
                    {
                        return;
                    }

                    const AZ::SceneAPI::DataTypes::IAnimationData* animation = azrtti_cast<const AZ::SceneAPI::DataTypes::IAnimationData*>(animationData->get());
                    const AZ::u32 frameCount = aznumeric_caster(animation->GetKeyFrameCount());
                    motionRangeRule->SetStartFrame(0);
                    motionRangeRule->SetEndFrame(frameCount - 1);
                }
            }

            AZ::SceneAPI::Events::ProcessingResult MotionRangeRuleBehavior::UpdateManifest(AZ::SceneAPI::Containers::Scene& scene, ManifestAction action,
                RequestingApplication /*requester*/)
            {
                if (action != ManifestAction::Update)
                {
                    return AZ::SceneAPI::Events::ProcessingResult::Ignored;
                }

                bool updated = false;
                AZ::SceneAPI::Containers::SceneManifest& manifest = scene.GetManifest();
                auto valueStorage = manifest.GetValueStorage();
                auto view = AZ::SceneAPI::Containers::MakeDerivedFilterView<Group::MotionGroup>(valueStorage);
                for (Group::MotionGroup& group : view)
                {
                    AZ::SceneAPI::Containers::RuleContainer& rules = group.GetRuleContainer();
                    if (rules.ContainsRuleOfType<Rule::MotionRangeRule>())
                    {
                        AZStd::shared_ptr<Rule::MotionRangeRule> motionRangeRule = rules.FindFirstByType<Rule::MotionRangeRule>();
                        if (motionRangeRule->GetProcessRangeRuleConversion())
                        {
                            // If the motionRangeRule is converted from an older data version, we check if we should keep it.
                            motionRangeRule->SetProcessRangeRuleConversion(false);
                            auto contentStorage = scene.GetGraph().GetContentStorage();
                            auto animationData = AZStd::find_if(contentStorage.begin(), contentStorage.end(), AZ::SceneAPI::Containers::DerivedTypeFilter<AZ::SceneAPI::DataTypes::IAnimationData>());
                            if (animationData == contentStorage.end())
                            {
                                continue;
                            }
                            const AZ::SceneAPI::DataTypes::IAnimationData* animation = azrtti_cast<const AZ::SceneAPI::DataTypes::IAnimationData*>(animationData->get());
                            const AZ::u32 frameCount = aznumeric_caster(animation->GetKeyFrameCount());
                            if (motionRangeRule->GetStartFrame() == 0 && motionRangeRule->GetEndFrame() == frameCount - 1)
                            {
                                // if the startframe/endframe matches the fbx animation length, remove it.
                                rules.RemoveRule(motionRangeRule);
                                updated = true;
                            }
                        }
                    }
                }
                return updated ? AZ::SceneAPI::Events::ProcessingResult::Success : AZ::SceneAPI::Events::ProcessingResult::Ignored;
            }
        } // Behavior
    } // Pipeline
} // EMotionFX
