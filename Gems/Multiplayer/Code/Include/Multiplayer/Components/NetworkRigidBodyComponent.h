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
        , private NetworkRigidBodyRequestBus::Handler
    {
        friend class NetworkRigidBodyComponentController;

    public:
        AZ_MULTIPLAYER_COMPONENT(
            Multiplayer::NetworkRigidBodyComponent, s_networkRigidBodyComponentConcreteUuid, Multiplayer::NetworkRigidBodyComponentBase);

        static void Reflect(AZ::ReflectContext* context);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        NetworkRigidBodyComponent();

        void OnInit() override;
        void OnActivate(Multiplayer::EntityIsMigrating entityIsMigrating) override;
        void OnDeactivate(Multiplayer::EntityIsMigrating entityIsMigrating) override;

    private:
        void OnTransformUpdate(const AZ::Transform& worldTm);
        void OnSyncRewind();

        Multiplayer::EntitySyncRewindEvent::Handler m_syncRewindHandler;
        AZ::TransformChangedEvent::Handler m_transformChangedHandler;
        Physics::RigidBodyRequests* m_physicsRigidBodyComponent = nullptr;
        Multiplayer::RewindableObject<AZ::Transform, Multiplayer::RewindHistorySize> m_transform;
    };

    class NetworkRigidBodyComponentController
        : public NetworkRigidBodyComponentControllerBase
    {
    public:
        NetworkRigidBodyComponentController(NetworkRigidBodyComponent& parent);
        void OnActivate(Multiplayer::EntityIsMigrating entityIsMigrating) override;
        void OnDeactivate(Multiplayer::EntityIsMigrating entityIsMigrating) override;
        void HandleSendApplyImpulse(AzNetworking::IConnection* invokingConnection, const AZ::Vector3& impulse, const AZ::Vector3& worldPoint) override;
    private:
        void OnTransformUpdate();
        AZ::TransformChangedEvent::Handler m_transformChangedHandler;
    };
} // namespace Multiplayer
