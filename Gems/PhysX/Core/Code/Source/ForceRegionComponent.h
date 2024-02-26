/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Source/ForceRegionForces.h>
#include <Source/ForceRegion.h>

#include <PhysX/ComponentTypeIds.h>

#include <AzCore/Component/Component.h>

#include <AzFramework/Physics/RigidBodyBus.h>
#include <AzFramework/Physics/Common/PhysicsEvents.h>
#include <AzFramework/Physics/Common/PhysicsSimulatedBodyEvents.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>

namespace AzPhysics
{
    struct TriggerEvent;
}

namespace PhysX
{
    /// ForceRegionComponent
    /// Applies a forces to objects within a region.
    /// Uses a PhysX trigger collider to receive notifications about entities entering and exiting the region. 
    /// A net force will be calculated per entity by summing all the attached forces on each tick.
    class ForceRegionComponent
        : public AZ::Component
        , protected Physics::RigidBodyNotificationBus::Handler
        , private AzFramework::EntityDebugDisplayEventBus::Handler
    {
    public:
        AZ_COMPONENT(ForceRegionComponent, ForceRegionComponentTypeId);
        static void Reflect(AZ::ReflectContext* context);

        ForceRegionComponent();
        explicit ForceRegionComponent(ForceRegion&& forceRegion, bool debug);
        ~ForceRegionComponent() = default;

    protected:
        
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC_CE("TransformService"));
            required.push_back(AZ_CRC_CE("PhysicsTriggerService"));
        }

        // Component
        void Activate() override;
        void Deactivate() override;

        // Physics::RigidBodyNotifications overrides...
        void OnPhysicsEnabled(const AZ::EntityId& entityId) override;
        void OnPhysicsDisabled(const AZ::EntityId& entityId) override;

        // EntityDebugDisplayEventBus
        void DisplayEntityViewport(const AzFramework::ViewportInfo& viewportInfo
            , AzFramework::DebugDisplayRequests& debugDisplayRequests) override;

    private:
        void InitPhysicsTickHandler();
        void PostPhysicsSubTick(float fixedDeltaTime);
        void OnTriggerEnter(const AzPhysics::TriggerEvent& triggerEvent);
        void OnTriggerExit(const AzPhysics::TriggerEvent& triggerEvent);

        AZStd::unordered_set<AZ::EntityId> m_entities; ///< Collection of entity IDs contained within the region.
        ForceRegion m_forceRegion; ///< Calculates the net force.
        bool m_debugForces = false; ///< Draws debug lines for entities in the region

        AzPhysics::SceneEvents::OnSceneSimulationFinishHandler m_sceneFinishSimHandler;
        AzPhysics::SimulatedBodyEvents::OnTriggerEnter::Handler m_onTriggerEnterHandler;
        AzPhysics::SimulatedBodyEvents::OnTriggerExit::Handler m_onTriggerExitHandler;
    };
}
