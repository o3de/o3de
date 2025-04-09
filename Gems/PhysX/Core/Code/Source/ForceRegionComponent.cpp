/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Source/ForceRegionComponent.h>
#include <Source/ForceRegionForces.h>

#include <AzCore/Component/Entity.h>
#include <AzCore/Debug/Profiler.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/Physics/PhysicsScene.h>
#include <AzFramework/Physics/PhysicsSystem.h>
#include <AzFramework/Physics/RigidBodyBus.h>
#include <AzFramework/Physics/Collision/CollisionEvents.h>

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

    ForceRegionComponent::ForceRegionComponent()
    {
        InitPhysicsTickHandler();
    }

    ForceRegionComponent::ForceRegionComponent(ForceRegion&& forceRegion, bool debug)
        : m_forceRegion(std::move(forceRegion))
        , m_debugForces(debug)
    {
        InitPhysicsTickHandler();
    }

    void ForceRegionComponent::Activate()
    {
        if (auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get())
        {
            AzPhysics::SceneHandle sceneHandle = sceneInterface->GetSceneHandle(AzPhysics::DefaultPhysicsSceneName);
            sceneInterface->RegisterSceneSimulationFinishHandler(sceneHandle, m_sceneFinishSimHandler);
        }

        // During entity activation the simulated bodies are not created yet.
        // Connect to RigidBodyNotificationBus to listen when they get enabled to register the trigger handlers.
        Physics::RigidBodyNotificationBus::Handler::BusConnect(GetEntityId());

        if (m_debugForces)
        {
            AzFramework::EntityDebugDisplayEventBus::Handler::BusConnect(GetEntityId());
        }
        m_forceRegion.Activate(GetEntityId());
    }

    void ForceRegionComponent::Deactivate()
    {
        Physics::RigidBodyNotificationBus::Handler::BusDisconnect();

        m_forceRegion.Deactivate();
        if (m_debugForces)
        {
            AzFramework::EntityDebugDisplayEventBus::Handler::BusDisconnect();
        }
        m_onTriggerEnterHandler.Disconnect();
        m_onTriggerExitHandler.Disconnect();
        m_sceneFinishSimHandler.Disconnect();

        m_entities.clear(); // On re-activation, each entity in this force region triggers OnTriggerEnter again.
    }

    void ForceRegionComponent::OnPhysicsEnabled(const AZ::EntityId& entityId)
    {
        if (auto* physicsSystem = AZ::Interface<AzPhysics::SystemInterface>::Get())
        {
            AZStd::pair<AzPhysics::SceneHandle, AzPhysics::SimulatedBodyHandle> foundBody =
                physicsSystem->FindAttachedBodyHandleFromEntityId(entityId);
            if (foundBody.first != AzPhysics::InvalidSceneHandle)
            {
                AzPhysics::SimulatedBodyEvents::RegisterOnTriggerEnterHandler(
                    foundBody.first, foundBody.second, m_onTriggerEnterHandler);
                AzPhysics::SimulatedBodyEvents::RegisterOnTriggerExitHandler(
                    foundBody.first, foundBody.second, m_onTriggerExitHandler);
            }
        }
    }

    void ForceRegionComponent::OnPhysicsDisabled([[maybe_unused]] const AZ::EntityId& entityId)
    {
        m_onTriggerEnterHandler.Disconnect();
        m_onTriggerExitHandler.Disconnect();
    }

    void ForceRegionComponent::InitPhysicsTickHandler()
    {
        m_sceneFinishSimHandler = AzPhysics::SceneEvents::OnSceneSimulationFinishHandler([this](
            [[maybe_unused]] AzPhysics::SceneHandle sceneHandle,
            float fixedDeltatime)
            {
                this->PostPhysicsSubTick(fixedDeltatime);
            }, aznumeric_cast<int32_t>(AzPhysics::SceneEvents::PhysicsStartFinishSimulationPriority::Components));

        m_onTriggerEnterHandler = AzPhysics::SimulatedBodyEvents::OnTriggerEnter::Handler([this](
            [[maybe_unused]] AzPhysics::SimulatedBodyHandle bodyHandle,
            const  AzPhysics::TriggerEvent& triggerEvent)
            {
                OnTriggerEnter(triggerEvent);
            });

        m_onTriggerExitHandler = AzPhysics::SimulatedBodyEvents::OnTriggerExit::Handler([this](
            [[maybe_unused]] AzPhysics::SimulatedBodyHandle bodyHandle,
            const  AzPhysics::TriggerEvent& triggerEvent)
            {
                OnTriggerExit(triggerEvent);
            });
    }

    void ForceRegionComponent::PostPhysicsSubTick(float fixedDeltaTime)
    {
        AZ_PROFILE_FUNCTION(Physics);

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

    void ForceRegionComponent::OnTriggerEnter(const AzPhysics::TriggerEvent& triggerEvent)
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

    void ForceRegionComponent::OnTriggerExit(const AzPhysics::TriggerEvent& triggerEvent)
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
