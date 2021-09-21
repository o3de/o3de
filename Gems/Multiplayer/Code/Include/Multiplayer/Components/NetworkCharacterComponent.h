/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Source/AutoGen/NetworkCharacterComponent.AutoComponent.h>
#include <PhysX/CharacterGameplayBus.h>
#include <Multiplayer/Components/NetBindComponent.h>

namespace Physics
{
    class Character;
}

namespace Multiplayer
{
    //! NetworkCharacterComponent
    //! Provides multiplayer support for game-play player characters.
    class NetworkCharacterComponent
        : public NetworkCharacterComponentBase
        , private PhysX::CharacterGameplayRequestBus::Handler
    {
        friend class NetworkCharacterComponentController;

    public:
        AZ_MULTIPLAYER_COMPONENT(Multiplayer::NetworkCharacterComponent, s_networkCharacterComponentConcreteUuid, Multiplayer::NetworkCharacterComponentBase)

        static void Reflect(AZ::ReflectContext* context);

        NetworkCharacterComponent();

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC_CE("NetworkRigidBodyService"));
        }

        // AZ::Component
        void OnInit() override {}
        void OnActivate(Multiplayer::EntityIsMigrating entityIsMigrating) override;
        void OnDeactivate(Multiplayer::EntityIsMigrating entityIsMigrating) override;

    private:
        void OnTranslationChangedEvent(const AZ::Vector3& translation);
        void OnSyncRewind();

        // CharacterGameplayRequestBus
        bool IsOnGround() const override;
        float GetGravityMultiplier() const override { return {}; }
        void SetGravityMultiplier([[maybe_unused]] float gravityMultiplier) override {}
        AZ::Vector3 GetFallingVelocity() const override { return {}; }
        void SetFallingVelocity([[maybe_unused]] const AZ::Vector3& fallingVelocity) override {}

        Physics::Character* m_physicsCharacter = nullptr;
        Multiplayer::EntitySyncRewindEvent::Handler m_syncRewindHandler = Multiplayer::EntitySyncRewindEvent::Handler([this]() { OnSyncRewind(); });
        AZ::Event<AZ::Vector3>::Handler m_translationEventHandler;
    };

    //! NetworkCharacterComponentController
    //! This is the network controller for NetworkCharacterComponent.
    //! Class provides the ability to move characters in physical space while keeping the network in-sync.
    class NetworkCharacterComponentController
        : public NetworkCharacterComponentControllerBase
    {
    public:
        NetworkCharacterComponentController(NetworkCharacterComponent& parent);

        // NetworkCharacterComponentControllerBase
        void OnActivate(Multiplayer::EntityIsMigrating entityIsMigrating) override;
        void OnDeactivate(Multiplayer::EntityIsMigrating entityIsMigrating) override;

        //! TryMoveWithVelocity
        //! Will move this character entity kinematically through physical world while also ensuring the network stays in-sync.
        //! Velocity will be applied over delta-time to determine the movement amount.
        //! Returns this entity's world-space position after the move.
        AZ::Vector3 TryMoveWithVelocity(const AZ::Vector3& velocity, float deltaTime);
    };
}
