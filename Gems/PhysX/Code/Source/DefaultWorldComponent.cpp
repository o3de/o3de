/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <DefaultWorldComponent.h>
#include <AzFramework/Physics/PhysicsSystem.h>
#include <AzFramework/Physics/Collision/CollisionEvents.h>

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

    AzPhysics::SceneHandle DefaultWorldComponent::GetDefaultSceneHandle() const
    {
        return m_sceneHandle;
    }

    void DefaultWorldComponent::OnPreGameEntitiesStarted()
    {
        if (auto* physicsSystem = AZ::Interface<AzPhysics::SystemInterface>::Get())
        {
            AzPhysics::SceneConfiguration sceneConfig = physicsSystem->GetDefaultSceneConfiguration();
            sceneConfig.m_sceneName = AzPhysics::DefaultPhysicsSceneName;
            m_sceneHandle = physicsSystem->AddScene(sceneConfig);
            if (m_sceneHandle != AzPhysics::InvalidSceneHandle)
            {
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
        if (currentConfig != config)
        {
            scene->UpdateConfiguration(config);
        }
    }
} // namespace PhysX
