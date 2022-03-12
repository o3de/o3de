/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Physics/PhysicsSystem.h>

#include <AzCore/RTTI/BehaviorContext.h>

namespace AzPhysics
{
    void SystemInterface::Reflect(AZ::ReflectContext* context)
    {
        if (auto* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            const auto getOnPresimulateEvent = []()->SystemEvents::OnPresimulateEvent*
            {
                if (auto* physicsSystem = AZ::Interface<SystemInterface>::Get())
                {
                    return &physicsSystem->m_preSimulateEvent;
                }
                return nullptr;
            };
            const AZ::BehaviorAzEventDescription presimulateEventDescription =
            {
                "Presimulate event",
                {"Tick time"} // Parameters
            };

            const auto getOnPostsimulateEvent = []() -> SystemEvents::OnPostsimulateEvent*
            {
                if (auto* physicsSystem = AZ::Interface<SystemInterface>::Get())
                {
                    return &physicsSystem->m_postSimulateEvent;
                }
                return nullptr;
            };
            const AZ::BehaviorAzEventDescription postsimulateEventDescription =
            {
                "Postsimulate event",
                {"Tick time"} // Parameters
            };

            behaviorContext->Class<SystemInterface>("PhysicsSystemInterface")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Module, "physics")
                ->Attribute(AZ::Script::Attributes::Category, "PhysX")
                ->Method("GetOnPresimulateEvent", getOnPresimulateEvent)
                    ->Attribute(AZ::Script::Attributes::AzEventDescription, presimulateEventDescription)
                ->Method("GetOnPostsimulateEvent", getOnPostsimulateEvent)
                    ->Attribute(AZ::Script::Attributes::AzEventDescription, postsimulateEventDescription)
                ->Method("GetSceneHandle", &SystemInterface::GetSceneHandle)
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Method("GetScene", &SystemInterface::GetScene)
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ;

            behaviorContext->Method(
                    "GetPhysicsSystem",
                    []()
                    {
                        return AZ::Interface<AzPhysics::SystemInterface>::Get();
                    });

            behaviorContext
                ->Constant("DefaultPhysicsSceneName", BehaviorConstant(DefaultPhysicsSceneName))
                ->Constant("DefaultPhysicsSceneId", BehaviorConstant(DefaultPhysicsSceneId))
                ->Constant("EditorPhysicsSceneName", BehaviorConstant(EditorPhysicsSceneName))
                ->Constant("EditorPhysicsSceneId", BehaviorConstant(EditorPhysicsSceneId));
        }
    }
} // namespace AzPhysics
