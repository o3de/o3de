/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Source/AutoGen/NetworkRigidBodyComponent.AutoComponent.h>
#include <AzCore/Component/TransformBus.h>
#include <AzFramework/Physics/RigidBodyBus.h>
#include <Multiplayer/Components/NetBindComponent.h>

namespace Physics
{
    class RigidBodyRequests;
}

namespace Multiplayer
{
    //! Bus for requests to the network rigid body component.
    class NetworkRigidBodyRequests : public AZ::ComponentBus
    {
    };
    using NetworkRigidBodyRequestBus = AZ::EBus<NetworkRigidBodyRequests>;

    class NetworkRigidBodyComponent final
        : public NetworkRigidBodyComponentBase
        , private Physics::RigidBodyNotificationBus::Handler
        , private NetworkRigidBodyRequestBus::Handler
    {
        friend class NetworkRigidBodyComponentController;

    public:
        AZ_MULTIPLAYER_COMPONENT(
            Multiplayer::NetworkRigidBodyComponent, s_networkRigidBodyComponentConcreteUuid, Multiplayer::NetworkRigidBodyComponentBase);

        static void Reflect(AZ::ReflectContext* context);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);

        NetworkRigidBodyComponent();

        void OnInit() override;
        void OnActivate(Multiplayer::EntityIsMigrating entityIsMigrating) override;
        void OnDeactivate(Multiplayer::EntityIsMigrating entityIsMigrating) override;

    private:
        // Physics::RigidBodyNotifications overrides ...
        void OnPhysicsEnabled(const AZ::EntityId& entityId) override;

        void OnTransformUpdate(const AZ::Transform& worldTm);
        void OnSyncRewind();

        Multiplayer::EntitySyncRewindEvent::Handler m_syncRewindHandler;
        AZ::TransformChangedEvent::Handler m_transformChangedHandler;
        Physics::RigidBodyRequests* m_physicsRigidBodyComponent = nullptr;
        Multiplayer::RewindableObject<AZ::Transform, Multiplayer::RewindHistorySize> m_transform;
    };

    class NetworkRigidBodyComponentController
        : public NetworkRigidBodyComponentControllerBase
        , private Physics::RigidBodyNotificationBus::Handler
    {
    public:
        NetworkRigidBodyComponentController(NetworkRigidBodyComponent& parent);
        void OnActivate(Multiplayer::EntityIsMigrating entityIsMigrating) override;
        void OnDeactivate(Multiplayer::EntityIsMigrating entityIsMigrating) override;
#if AZ_TRAIT_SERVER
        void HandleSendApplyImpulse(AzNetworking::IConnection* invokingConnection, const AZ::Vector3& impulse, const AZ::Vector3& worldPoint) override;

    private:
        void OnTransformUpdate();
        AZ::TransformChangedEvent::Handler m_transformChangedHandler;
#endif

    private:
        // Physics::RigidBodyNotifications overrides ...
        void OnPhysicsEnabled(const AZ::EntityId& entityId) override;

        Physics::RigidBodyRequests* m_physicsRigidBodyComponent = nullptr;
    };
} // namespace Multiplayer
