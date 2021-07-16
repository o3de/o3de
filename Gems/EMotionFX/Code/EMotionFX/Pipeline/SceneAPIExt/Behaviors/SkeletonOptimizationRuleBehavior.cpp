/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneCore/Containers/SceneManifest.h>
#include <SceneAPI/SceneCore/Containers/Utilities/Filters.h>
#include <SceneAPI/SceneCore/Containers/Views/SceneGraphDownwardsIterator.h>
#include <SceneAPI/SceneCore/Utilities/SceneGraphSelector.h>

#include <SceneAPIExt/Rules/SkeletonOptimizationRule.h>
#include <SceneAPIExt/Behaviors/SkeletonOptimizationRuleBehavior.h>
#include <SceneAPIExt/Groups/IActorGroup.h>

namespace EMotionFX
{
    namespace Pipeline
    {
        namespace Behavior
        {
            void SkeletonOptimizationRuleBehavior::Activate()
            {
                SceneEvents::ManifestMetaInfoBus::Handler::BusConnect();
                SceneEvents::AssetImportRequestBus::Handler::BusConnect();
            }

            void SkeletonOptimizationRuleBehavior::Deactivate()
            {
                SceneEvents::AssetImportRequestBus::Handler::BusDisconnect();
                SceneEvents::ManifestMetaInfoBus::Handler::BusDisconnect();
            }

            void SkeletonOptimizationRuleBehavior::Reflect(AZ::ReflectContext* context)
            {
                AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
                if (serializeContext)
                {
                    serializeContext->Class<SkeletonOptimizationRuleBehavior, BehaviorComponent>()->Version(1);
                }
            }

            void SkeletonOptimizationRuleBehavior::InitializeObject(const SceneContainers::Scene& scene, SceneDataTypes::IManifestObject& target)
            {
                if (!azrtti_istypeof<Rule::SkeletonOptimizationRule>(&target))
                {
                    return;
                }

                // By default the critical bones list should be empty.
                Rule::SkeletonOptimizationRule* rule = azrtti_cast<Rule::SkeletonOptimizationRule*>(&target);
                AZ::SceneAPI::Utilities::SceneGraphSelector::UnselectAll(scene.GetGraph(), rule->GetCriticalBonesList());
            }

            SceneEvents::ProcessingResult SkeletonOptimizationRuleBehavior::UpdateManifest(SceneContainers::Scene& scene, ManifestAction action,
                RequestingApplication requester)
            {
                AZ_UNUSED(requester);
                if (action == ManifestAction::Update)
                {
                    return UpdateSelection(scene);
                }
                else
                {
                    return SceneEvents::ProcessingResult::Ignored;
                }
            }

            SceneEvents::ProcessingResult SkeletonOptimizationRuleBehavior::UpdateSelection(SceneContainers::Scene& scene) const
            {
                SceneContainers::SceneManifest& manifest = scene.GetManifest();
                auto valueStorage = manifest.GetValueStorage();
                auto view = SceneContainers::MakeDerivedFilterView<Group::IActorGroup>(valueStorage);
                for (Group::IActorGroup& group : view)
                {
                    const SceneContainers::RuleContainer& rules = group.GetRuleContainer();
                    const AZStd::shared_ptr<Rule::SkeletonOptimizationRule> rule = rules.FindFirstByType<Rule::SkeletonOptimizationRule>();
                    if (!rule)
                    {
                        continue;
                    }

                    AZ::SceneAPI::Utilities::SceneGraphSelector::UpdateNodeSelection(scene.GetGraph(), rule->GetCriticalBonesList());
                }

                return SceneEvents::ProcessingResult::Success;
            }
        }
    }
}
