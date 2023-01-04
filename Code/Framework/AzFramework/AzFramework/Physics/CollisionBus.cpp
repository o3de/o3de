/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "CollisionBus.h"
#include <AzCore/RTTI/BehaviorContext.h>

namespace Physics
{
    void CollisionFilteringRequests::Reflect(AZ::ReflectContext* context)
    {
        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<Physics::CollisionFilteringRequestBus>("CollisionFilteringBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Module, "physics")
                ->Attribute(AZ::Script::Attributes::Category, "Physics")
                ->Event("SetCollisionLayer", &Physics::CollisionFilteringRequestBus::Events::SetCollisionLayer)
                ->Event("GetCollisionLayerName", &Physics::CollisionFilteringRequestBus::Events::GetCollisionLayerName)
                ->Event("SetCollisionGroup", &Physics::CollisionFilteringRequestBus::Events::SetCollisionGroup)
                ->Event("GetCollisionGroupName", &Physics::CollisionFilteringRequestBus::Events::GetCollisionGroupName)
                ->Event("ToggleCollisionLayer", &Physics::CollisionFilteringRequestBus::Events::ToggleCollisionLayer)
                ;
        }
    }
}
