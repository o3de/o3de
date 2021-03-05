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

#include <SceneAPIExt/Groups/IActorGroup.h>
#include <SceneAPIExt/Rules/SkinRule.h>
#include <SceneAPIExt/Behaviors/SkinRuleBehavior.h>

namespace EMotionFX
{
    namespace Pipeline
    {
        namespace Behavior
        {
            void SkinRuleBehavior::Reflect(AZ::ReflectContext* context)
            {
                Rule::SkinRule::Reflect(context);

                AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
                if (serializeContext)
                {
                    serializeContext->Class<SkinRuleBehavior, AZ::SceneAPI::SceneCore::BehaviorComponent>()->Version(1);
                }
            }

            void SkinRuleBehavior::Activate()
            {
                AZ::SceneAPI::Events::ManifestMetaInfoBus::Handler::BusConnect();
            }

            void SkinRuleBehavior::Deactivate()
            {
                AZ::SceneAPI::Events::ManifestMetaInfoBus::Handler::BusDisconnect();
            }

            void SkinRuleBehavior::InitializeObject([[maybe_unused]] const AZ::SceneAPI::Containers::Scene& scene, AZ::SceneAPI::DataTypes::IManifestObject& target)
            {
                if (target.RTTI_IsTypeOf(Group::IActorGroup::TYPEINFO_Uuid()))
                {
                    Group::IActorGroup* actorGroup = azrtti_cast<Group::IActorGroup*>(&target);

                    // TODO: Check if the sceneGraph contain skin data.
                    AZ::SceneAPI::Containers::RuleContainer& rules = actorGroup->GetRuleContainer();
                    if (!rules.ContainsRuleOfType<Rule::ISkinRule>())
                    {
                        rules.AddRule(AZStd::make_shared<Rule::SkinRule>());
                    }
                }
            }
        } // Behavior
    } // Pipeline
} // EMotionFX