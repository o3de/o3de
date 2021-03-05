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
#include <PhysX_precompiled.h>

#include <DefaultWorldComponent.h>
#include <AzFramework/Physics/CollisionNotificationBus.h>
#include <AzFramework/Physics/TriggerBus.h>
#include <AzFramework/Physics/PhysicsSystem.h>

#include <World.h>

namespace PhysX
{
    DefaultWorldComponent::DefaultWorldComponent()
        : m_onDefaultSceneConfigChangedHandler(
            [this](const AzPhysics::SceneConfiguration* config)
            {
                if (config != nullptr)
                {
                    UpdateDefaultConfiguration(*config);
                }
            })
    {

    }

    void DefaultWorldComponent::Activate()
    {
        AzFramework::GameEntityContextEventBus::Handler::BusConnect();
        if (auto* physicsSystem = AZ::Interface<AzPhysics::SystemInterface>::Get())
        {
            physicsSystem->RegisterOnDefaultSceneConfigurationChangedEventHandler(m_onDefaultSceneConfigChangedHandler);
        }
    }

    void DefaultWorldComponent::Deactivate()
    {
        AzFramework::GameEntityContextEventBus::Handler::BusDisconnect();
        Physics::DefaultWorldBus::Handler::BusDisconnect();
        m_onDefaultSceneConfigChangedHandler.Disconnect();
    }

    void DefaultWorldComponent::OnTriggerEnter(const Physics::TriggerEvent& triggerEvent)
    {
        Physics::TriggerNotificationBus::QueueEvent(triggerEvent.m_triggerBody->GetEntityId(), &Physics::TriggerNotifications::OnTriggerEnter, triggerEvent);
    }

    void DefaultWorldComponent::OnTriggerExit(const Physics::TriggerEvent& triggerEvent)
    {
        Physics::TriggerNotificationBus::QueueEvent(triggerEvent.m_triggerBody->GetEntityId(), &Physics::TriggerNotifications::OnTriggerExit, triggerEvent);
    }

    void DefaultWorldComponent::OnCollisionBegin(const Physics::CollisionEvent& event)
    {
        Physics::CollisionNotificationBus::QueueEvent(event.m_body1->GetEntityId(), &Physics::CollisionNotifications::OnCollisionBegin, event);
    }

    void DefaultWorldComponent::OnCollisionPersist(const Physics::CollisionEvent& event)
    {
        Physics::CollisionNotificationBus::QueueEvent(event.m_body1->GetEntityId(), &Physics::CollisionNotifications::OnCollisionPersist, event);
    }

    void DefaultWorldComponent::OnCollisionEnd(const Physics::CollisionEvent& event)
    {
        Physics::CollisionNotificationBus::QueueEvent(event.m_body1->GetEntityId(), &Physics::CollisionNotifications::OnCollisionEnd, event);
    }

    AZStd::shared_ptr<Physics::World> DefaultWorldComponent::GetDefaultWorld()
    {
        if (auto* physicsSystem = AZ::Interface<AzPhysics::SystemInterface>::Get())
        {
            if (auto scene = physicsSystem->GetScene(m_sceneHandle))
            {
                return scene->GetLegacyWorld();
            }
        }
        return nullptr;
    }

    void DefaultWorldComponent::OnPreGameEntitiesStarted()
    {
        if (auto* physicsSystem = AZ::Interface<AzPhysics::SystemInterface>::Get())
        {
            AzPhysics::SceneConfiguration sceneConfig = physicsSystem->GetDefaultSceneConfiguration();
            sceneConfig.m_legacyId = Physics::DefaultPhysicsWorldId;
            m_sceneHandle = physicsSystem->AddScene(sceneConfig);
            if (m_sceneHandle != AzPhysics::InvalidSceneHandle)
            {
                // Temporary until LYN-438 work is complete
                if (auto* scene = physicsSystem->GetScene(m_sceneHandle))
                {
                    if (AZStd::shared_ptr<Physics::World> legacyWorld = scene->GetLegacyWorld())
                    {
                        legacyWorld->SetEventHandler(this);
                    }
                }
                Physics::DefaultWorldBus::Handler::BusConnect();
            }
        }
    }

    void DefaultWorldComponent::OnGameEntitiesReset()
    {
        Physics::DefaultWorldBus::Handler::BusDisconnect();
        if (auto* physicsSystem = AZ::Interface<AzPhysics::SystemInterface>::Get())
        {
            physicsSystem->RemoveScene(m_sceneHandle);
            m_onDefaultSceneConfigChangedHandler.Disconnect();
        }
        m_sceneHandle = AzPhysics::InvalidSceneHandle;
    }

    void DefaultWorldComponent::UpdateDefaultConfiguration(const AzPhysics::SceneConfiguration& config)
    {
        auto* physicsSystem = AZ::Interface<AzPhysics::SystemInterface>::Get();
        if (physicsSystem == nullptr)
        {
            return;
        }

        auto* scene = physicsSystem->GetScene(m_sceneHandle);
        if (scene == nullptr)
        {
            return;
        }

        const AzPhysics::SceneConfiguration& currentConfig = scene->GetConfiguration();

        //this will be removed/improved with LYN-438
        constexpr const float tolerance = 0.0001f;
        const bool gravityChanged = !currentConfig.m_legacyConfiguration.m_gravity.IsClose(config.m_legacyConfiguration.m_gravity);
        const bool maxTimeStepChanged = !AZ::IsClose(currentConfig.m_legacyConfiguration.m_maxTimeStep, config.m_legacyConfiguration.m_maxTimeStep, tolerance);
        const bool fixedTimeStepChanged = !AZ::IsClose(currentConfig.m_legacyConfiguration.m_fixedTimeStep, config.m_legacyConfiguration.m_fixedTimeStep, tolerance);
        if (gravityChanged)
        {
            Physics::WorldRequestBus::Broadcast(
                &Physics::WorldRequests::SetGravity, config.m_legacyConfiguration.m_gravity);
        }
        if (maxTimeStepChanged)
        {
            Physics::WorldRequestBus::Broadcast(
                &Physics::WorldRequests::SetMaxDeltaTime, config.m_legacyConfiguration.m_maxTimeStep);
        }
        if (fixedTimeStepChanged)
        {
            Physics::WorldRequestBus::Broadcast(
                &Physics::WorldRequests::SetFixedDeltaTime, config.m_legacyConfiguration.m_fixedTimeStep);
        }

        if (currentConfig != config)
        {
            scene->UpdateConfiguration(config);
        }
    }
} // namespace PhysX
