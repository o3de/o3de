/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
        void OnPreRender(float deltaTime, float blendFactor);

        void OnRotationChangedEvent(const AZ::Quaternion& rotation);
        void OnTranslationChangedEvent(const AZ::Vector3& translation);
        void OnScaleChangedEvent(float scale);
        void OnResetCountChangedEvent();

        AZ::Transform m_previousTransform = AZ::Transform::CreateIdentity();
        AZ::Transform m_targetTransform = AZ::Transform::CreateIdentity();

        AZ::Event<AZ::Quaternion>::Handler m_rotationEventHandler;
        AZ::Event<AZ::Vector3>::Handler m_translationEventHandler;
        AZ::Event<float>::Handler m_scaleEventHandler;
        AZ::Event<uint8_t>::Handler m_resetCountEventHandler;

        EntityPreRenderEvent::Handler m_entityPreRenderEventHandler;
    };

    class NetworkTransformComponentController
        : public NetworkTransformComponentControllerBase
    {
    public:
        NetworkTransformComponentController(NetworkTransformComponent& parent);

        void OnActivate(Multiplayer::EntityIsMigrating entityIsMigrating) override;
        void OnDeactivate(Multiplayer::EntityIsMigrating entityIsMigrating) override;

    private:
        void OnTransformChangedEvent(const AZ::Transform& worldTm);

        AZ::TransformChangedEvent::Handler m_transformChangedHandler;
    };
}
