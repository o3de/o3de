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
#include "PhysX_precompiled.h"

#include <Source/ForceRegionComponent.h>
#include <Source/ForceRegionForces.h>
#include <Source/World.h>

#include <AzCore/Component/Entity.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/Physics/World.h>
#include <AzFramework/Physics/WorldBody.h>
#include <AzFramework/Physics/WorldEventhandler.h>
#include <AzFramework/Physics/RigidBodyBus.h>

namespace PhysX
{
    void ForceRegionComponent::Reflect(AZ::ReflectContext* context)
    {
        ForceRegion::Reflect(context);
        ForceWorldSpace::Reflect(context);
        ForceLocalSpace::Reflect(context);
        ForceSplineFollow::Reflect(context);
        ForceSimpleDrag::Reflect(context);
        ForceLinearDamping::Reflect(context);
        ForcePoint::Reflect(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<ForceRegionComponent, AZ::Component>()
                ->Version(1)
                ->Field("ForceRegion", &ForceRegionComponent::m_forceRegion)
                ->Field("DebugForces", &ForceRegionComponent::m_debugForces)
                ;
        }
    }

    ForceRegionComponent::ForceRegionComponent(ForceRegion&& forceRegion, bool debug)
        : m_forceRegion(std::move(forceRegion))
        , m_debugForces(debug)
    {
    }

    void ForceRegionComponent::Activate()
    {
        Physics::WorldNotificationBus::Handler::BusConnect(Physics::DefaultPhysicsWorldId);
        Physics::TriggerNotificationBus::Handler::BusConnect(m_entity->GetId());
        if (m_debugForces)
        {
            AzFramework::EntityDebugDisplayEventBus::Handler::BusConnect(m_entity->GetId());
        }
        m_forceRegion.Activate(GetEntityId());
    }

    void ForceRegionComponent::Deactivate()
    {
        m_forceRegion.Deactivate();
        if (m_debugForces)
        {
            AzFramework::EntityDebugDisplayEventBus::Handler::BusDisconnect();
        }
        Physics::TriggerNotificationBus::Handler::BusDisconnect();
        Physics::WorldNotificationBus::Handler::BusDisconnect();

        m_entities.clear(); // On re-activation, each entity in this force region triggers OnTriggerEnter again.
    }

    void ForceRegionComponent::OnPostPhysicsSubtick(float fixedDeltaTime)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Physics);

        for (auto entityId : m_entities)
        {
            EntityParams entity = ForceRegionUtil::CreateEntityParams(entityId);

            AZ::Vector3 netForce = m_forceRegion.CalculateNetForce(entity);

            if (!netForce.IsZero())
            {
                netForce *= fixedDeltaTime;
                Physics::RigidBodyRequestBus::Event(entityId, &Physics::RigidBodyRequestBus::Events::ApplyLinearImpulse, netForce);
            }
        }
    }

    int ForceRegionComponent::GetPhysicsTickOrder()
    {
        return WorldNotifications::Components;
    }

    void ForceRegionComponent::OnTriggerEnter(const Physics::TriggerEvent& triggerEvent)
    {
        if (!triggerEvent.m_otherBody)
        {
            return;
        }

        // Ignore self
        AZ::EntityId entityId = triggerEvent.m_otherBody->GetEntityId();
        if (entityId == GetEntityId())
        {
            return;
        }

        auto newEntityHasPhysics = Physics::RigidBodyRequestBus::FindFirstHandler(entityId) != nullptr;
        if (newEntityHasPhysics)
        {
            m_entities.insert(entityId);
        }
    }

    void ForceRegionComponent::OnTriggerExit(const Physics::TriggerEvent& triggerEvent)
    {
        if (triggerEvent.m_otherBody == nullptr)
        {
            return;
        }
        m_entities.erase(triggerEvent.m_otherBody->GetEntityId());
    }

    void ForceRegionComponent::DisplayEntityViewport([[maybe_unused]] const AzFramework::ViewportInfo& viewportInfo
        , AzFramework::DebugDisplayRequests& debugDisplayRequests)
    {
        for (auto entityId : m_entities)
        {
            EntityParams entityParams = ForceRegionUtil::CreateEntityParams(entityId);

            AZ::Vector3 netForce = m_forceRegion.CalculateNetForce(entityParams);
            if (!netForce.IsZero())
            {
                netForce = netForce.GetNormalizedEstimate() * entityParams.m_aabb.GetExtents().GetMaxElement() * 2.0f; // Ensures arrow length is longer than entity AABB so that it can be seen.
                AZ::Vector3 entityPosition = entityParams.m_aabb.GetCenter();
                debugDisplayRequests.DrawArrow(entityPosition, entityPosition + netForce, 1.5f);
            }
        }
    }

}
