/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/EBus/Event.h>
#include <AzCore/EBus/OrderedEvent.h>
#include <AzFramework/Physics/Collision/CollisionEvents.h>
#include <AzFramework/Physics/Common/PhysicsTypes.h>

namespace AZ
{
    class Vector3;
}

namespace AzPhysics
{
    struct SystemConfiguration;
    struct SceneConfiguration;

    namespace SystemEvents
    {
        //! Event that triggers when the physics system configuration has been changed.
        //! When triggered the event will send the newly applied SystemConfiguration object.
        using OnConfigurationChangedEvent = AZ::Event<const SystemConfiguration*>;

        //! Event triggers when the physics system has completed initialization.
        //! When triggered the event will send the SystemConfiguration used to initialize the system.
        using OnInitializedEvent = AZ::Event<const SystemConfiguration*>;

        //! Event triggers when the physics system has completed reinitialization.
        using OnReinitializedEvent = AZ::Event<>;

        //! Event triggers when the physics system has completed its shutdown. 
        using OnShutdownEvent = AZ::Event<>;

        //! Event triggers at the beginning of the SystemInterface::Simulate call.
        //! Parameter is the total time that the physics system will run for during the Simulate call.
        using OnPresimulateEvent = AZ::Event<float>;

        //! Event triggers at the end of the SystemInterface::Simulate call.
        //! Parameter is the total time that the physics system will run for during the Simulate call.
        using OnPostsimulateEvent = AZ::Event<float>;

        //! Event trigger when a Scene is added to the simulation.
        //! When triggered will send the handle to the new Scene.
        using OnSceneAddedEvent = AZ::Event<AzPhysics::SceneHandle>;

        //! Event trigger when a Scene is removed from the simulation.
        //! When triggered will send the handle to the old Scene (after this call, the Handle will be invalid).
        using OnSceneRemovedEvent = AZ::Event<AzPhysics::SceneHandle>;

        //! Event that triggers when the default scene configuration changes.
        //! When triggered the event will send the new default scene configuration.
        using OnDefaultSceneConfigurationChangedEvent = AZ::Event<const SceneConfiguration*>;
    }

    namespace SceneEvents
    {
        //! Event that triggers when a new config is set on a scene.
        //! When triggered the event will send a handle to the Scene that triggered the event and the new configuration.
        using OnSceneConfigurationChanged = AZ::Event<AzPhysics::SceneHandle, const AzPhysics::SceneConfiguration&>;

        //! Event that triggers when a Simulated body has been added to a scene.
        //! When triggered the event will send a handle to the Scene that triggered the event and a handle to the new simulated body.
        using OnSimulationBodyAdded = AZ::Event<AzPhysics::SceneHandle, AzPhysics::SimulatedBodyHandle>;

        //! Event that triggers when a Simulated body has been removed from a scene.
        //! When triggered the event will send a handle to the Scene that triggered the event and a handle to the removed simulated body (after this call, the Handle will be invalid).
        using OnSimulationBodyRemoved = AZ::Event<AzPhysics::SceneHandle, AzPhysics::SimulatedBodyHandle>;

        //! Event that triggers when a Simulated body has its simulation enabled.
        //! When triggered the event will send a handle to the Scene that triggered the event and a handle to the affected simulated body.
        using OnSimulationBodySimulationEnabled = AZ::Event<AzPhysics::SceneHandle, AzPhysics::SimulatedBodyHandle>;

        //! Event that triggers when a Simulated body has its simulation disabled.
        //! When triggered the event will send a handle to the Scene that triggered the event and a handle to the affected simulated body.
        using OnSimulationBodySimulationDisabled = AZ::Event<AzPhysics::SceneHandle, AzPhysics::SimulatedBodyHandle>;

        //! Enum for use with the OnSceneSimulationStartEvent and OnSceneSimulationFinishEvent AZ::OrderedEvent calls.
        //! Higher values are called before lower values.
        enum class PhysicsStartFinishSimulationPriority : int32_t
        {
            Default = 0, //!< All other systems (Game code).
            Audio = 1000, //!< Audio systems (occlusion).
            Scripting = 2000, //!< Scripting systems (script canvas).
            Components = 3000, //!< C++ components (force region).
            Animation = 4000, //!< Animation system (ragdolls).
            Physics = 5000 //!< The physics system itself
        };

        //! Event triggers at the beginning of the Scene::StartSimulation call.
        //! This will not trigger if the scene if not Enabled (Scene::IsEnabled() returns true).
        //! When triggered the event will send a handle to the Scene that triggers the event and the delta time in seconds used to step this update.
        //! @note This may fire multiple times per frame.
        using OnSceneSimulationStartEvent = AZ::OrderedEvent<AzPhysics::SceneHandle, float>;
        using OnSceneSimulationStartHandler = AZ::OrderedEventHandler<AzPhysics::SceneHandle, float>;

        //! Event triggers at the End of the Scene::FinishSimulation call.
        //! This will not trigger if the scene if not Enabled (Scene::IsEnabled() returns true).
        //! When triggered the event will send a handle to the Scene that triggers the event and the delta time in seconds used to step this update.
        //! @note This may fire multiple times per frame.
        using OnSceneSimulationFinishEvent = AZ::OrderedEvent<AzPhysics::SceneHandle, float>;
        using OnSceneSimulationFinishHandler = AZ::OrderedEventHandler<AzPhysics::SceneHandle, float>;

        //! Event triggers during the Scene::FinishSimulation call before the OnSceneSimulationFinishEvent for a scene
        //! and only if the SceneConfiguration::m_enableActiveActors is true.
        //! This will not trigger if the scene is not Enabled (Scene::IsEnabled() must return true to trigger).
        //! When triggered, the event will send a handle of the Scene that triggered the event and a list of SimulatedBodyHandles that were updated in this tick.
        using OnSceneActiveSimulatedBodiesEvent = AZ::Event<AzPhysics::SceneHandle, const AzPhysics::SimulatedBodyHandleList&, float>;

        //! Event triggers with an ordered list of all the collision Begin/Persist/End events that happened during a single sub simulation step.
        //! When triggered the event will send a handle to the Scene that triggers the event and the list of collision events that occurred.
        //! @note The event will trigger at the end of the Scene::FinishSimulation call and only if collision events were generated and will be
        //! triggered before the OnSceneSimulationFinishEvent, SimulatedBodyEvents::OnCollisionBegin, SimulatedBodyEvents::OnCollisionPersist, and SimulatedBodyEvents::OnCollisionEnd.
        //! This may fire multiple times per frame.
        //! The CollisionEventList is only valid for the duration of the callback.
        using OnSceneCollisionsEvent = AZ::Event<AzPhysics::SceneHandle, const AzPhysics::CollisionEventList&>;

        //! Event triggers with an ordered list of all the trigger Enter/Exit events that happened during single sub simulation step.
        //! When triggered the event will send a handle to the Scene that triggers the event and the list of trigger events that occurred.
        //! @note The event will trigger at the end of the Scene::FinishSimulation call and only if trigger events were generated and will be
        //! triggered before the OnSceneSimulationFinishEvent, SimulatedBodyEvents::OnTriggerEnter and SimulatedBodyEvents::OnTriggerExit.
        //! This may fire multiple times per frame.
        //! The TriggerEventList is only valid for the duration of the callback.
        using OnSceneTriggersEvent = AZ::Event<AzPhysics::SceneHandle, const AzPhysics::TriggerEventList&>;

        //! Event trigger when the gravity has been changed on the scene.
        //! When triggered the event will send a handle to the Scene that triggers the event and the new gravity vector.
        using OnSceneGravityChangedEvent = AZ::Event<AzPhysics::SceneHandle, const AZ::Vector3&>;
    }
}
