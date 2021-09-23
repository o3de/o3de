/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Source/AutoGen/NetworkTransformComponent.AutoComponent.h>
#include <Multiplayer/Components/NetBindComponent.h>
#include <AzCore/Component/TransformBus.h>

namespace Multiplayer
{
    class NetworkTransformComponent
        : public NetworkTransformComponentBase
    {
    public:
        AZ_MULTIPLAYER_COMPONENT(Multiplayer::NetworkTransformComponent, s_networkTransformComponentConcreteUuid, Multiplayer::NetworkTransformComponentBase);

        static void Reflect(AZ::ReflectContext* context);

        NetworkTransformComponent();

        void OnInit() override;
        void OnActivate(Multiplayer::EntityIsMigrating entityIsMigrating) override;
        void OnDeactivate(Multiplayer::EntityIsMigrating entityIsMigrating) override;

    private:
        void OnPreRender(float deltaTime);
        void OnCorrection();
        void OnParentChanged(NetEntityId parentId);
        
        EntityPreRenderEvent::Handler m_entityPreRenderEventHandler;
        EntityCorrectionEvent::Handler m_entityCorrectionEventHandler;
        AZ::Event<NetEntityId>::Handler m_parentChangedEventHandler;

        Multiplayer::HostFrameId m_targetHostFrameId = HostFrameId(0);
    };

    class NetworkTransformComponentController
        : public NetworkTransformComponentControllerBase
    {
    public:
        NetworkTransformComponentController(NetworkTransformComponent& parent);

        void OnActivate(Multiplayer::EntityIsMigrating entityIsMigrating) override;
        void OnDeactivate(Multiplayer::EntityIsMigrating entityIsMigrating) override;

    private:
        void OnTransformChangedEvent(const AZ::Transform& localTm, const AZ::Transform& worldTm);
        void OnParentIdChangedEvent(AZ::EntityId oldParent, AZ::EntityId newParent);

        AZ::TransformChangedEvent::Handler m_transformChangedHandler;
        AZ::ParentChangedEvent::Handler m_parentIdChangedHandler;
    };
}
