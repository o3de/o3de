/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Source/AutoGen/NetworkHitVolumesComponent.AutoComponent.h>
#include <Multiplayer/Components/NetBindComponent.h>
#include <Integration/ActorComponentBus.h>
#include <AzCore/Component/TransformBus.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzFramework/Physics/CharacterBus.h>

namespace Physics
{
    class CharacterRequests;
    class CharacterHitDetectionConfiguration;
}

namespace Multiplayer
{
    class NetworkHitVolumesComponent
        : public NetworkHitVolumesComponentBase
        , private EMotionFX::Integration::ActorComponentNotificationBus::Handler
        , private Physics::CharacterNotificationBus::Handler
    {
    public:
        struct AnimatedHitVolume final
        {
            AnimatedHitVolume
            (
                AzNetworking::ConnectionId connectionId,
                Physics::CharacterRequests* character,
                const char* hitVolumeName,
                const Physics::ColliderConfiguration* colliderConfig,
                const Physics::ShapeConfiguration* shapeConfig,
                const uint32_t jointIndex
            );

            ~AnimatedHitVolume() = default;

            void UpdateTransform(const AZ::Transform& transform);
            void SyncToCurrentTransform();

            Multiplayer::RewindableObject<AZ::Transform, Multiplayer::RewindHistorySize> m_transform;
            AZStd::shared_ptr<Physics::Shape> m_physicsShape;

            // Cached so we don't have to do subsequent lookups by name
            const Physics::ColliderConfiguration* m_colliderConfig = nullptr;
            const Physics::ShapeConfiguration* m_shapeConfig = nullptr;
            AZ::Transform m_colliderOffSetTransform;
            const AZ::u32 m_jointIndex = 0;
        };

        AZ_MULTIPLAYER_COMPONENT(Multiplayer::NetworkHitVolumesComponent, s_networkHitVolumesComponentConcreteUuid, Multiplayer::NetworkHitVolumesComponentBase);

        static void Reflect(AZ::ReflectContext* context);

        NetworkHitVolumesComponent();

        void OnInit() override;
        void OnActivate(Multiplayer::EntityIsMigrating entityIsMigrating) override;
        void OnDeactivate(Multiplayer::EntityIsMigrating entityIsMigrating) override;

    private:
        void OnPreRender(float deltaTime);
        void OnTransformUpdate(const AZ::Transform& transform);
        void OnSyncRewind();

        void CreateHitVolumes();
        void DestroyHitVolumes();

        //! ActorComponentNotificationBus::Handler
        //! @{
        void OnActorInstanceCreated(EMotionFX::ActorInstance* actorInstance) override;
        void OnActorInstanceDestroyed(EMotionFX::ActorInstance* actorInstance) override;
        //! @}
        
        //! Physics::CharacterNotificationBus overrides ...
        //! @{
        void OnCharacterActivated(const AZ::EntityId& entityId) override;
        void OnCharacterDeactivated(const AZ::EntityId& entityId) override;
        //! @}

        void DrawDebugHitVolumes();

        Physics::CharacterRequests* m_physicsCharacter = nullptr;
        EMotionFX::Integration::ActorComponentRequests* m_actorComponent = nullptr;
        const Physics::CharacterColliderConfiguration* m_hitDetectionConfig = nullptr;

        AZStd::vector<AnimatedHitVolume> m_animatedHitVolumes;

        Multiplayer::EntitySyncRewindEvent::Handler m_syncRewindHandler;
        Multiplayer::EntityPreRenderEvent::Handler m_preRenderHandler;
        AZ::TransformChangedEvent::Handler m_transformChangedHandler;

        AzFramework::DebugDisplayRequests* m_debugDisplay = nullptr;
    };
}
