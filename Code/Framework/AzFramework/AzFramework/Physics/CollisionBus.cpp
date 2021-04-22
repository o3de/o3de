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
                ->Attribute(AZ::Script::Attributes::Category, "PhysX")
                ->Event("SetCollisionLayer", &Physics::CollisionFilteringRequestBus::Events::SetCollisionLayer)
                ->Event("GetCollisionLayerName", &Physics::CollisionFilteringRequestBus::Events::GetCollisionLayerName)
                ->Event("SetCollisionGroup", &Physics::CollisionFilteringRequestBus::Events::SetCollisionGroup)
                ->Event("GetCollisionGroupName", &Physics::CollisionFilteringRequestBus::Events::GetCollisionGroupName)
                ->Event("ToggleCollisionLayer", &Physics::CollisionFilteringRequestBus::Events::ToggleCollisionLayer)
                ;
        }
    }
}
