/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates, or
* a third party where indicated.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <AzFramework/Physics/PhysicsSystem.h>

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

            behaviorContext->Class<SystemInterface>("System Interface")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Module, "physics")
                ->Attribute(AZ::Script::Attributes::Category, "PhysX")
                ->Method("GetOnPresimulateEvent", getOnPresimulateEvent)
                    ->Attribute(AZ::Script::Attributes::AzEventDescription, presimulateEventDescription)
                ->Method("GetOnPostsimulateEvent", getOnPostsimulateEvent)
                    ->Attribute(AZ::Script::Attributes::AzEventDescription, postsimulateEventDescription)
                ;
        }
    }
} // namespace AzPhysics
