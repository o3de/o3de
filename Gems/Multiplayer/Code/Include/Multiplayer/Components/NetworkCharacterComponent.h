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
    //! NetworkCharacterRequests
    //! ComponentBus handled by NetworkCharacterComponentController.
    //! Bus was created for exposing controller methods to script; C++ users should access the controller directly.
    class NetworkCharacterRequests : public AZ::ComponentBus
    {
    public:
        //! TryMoveWithVelocity
        //! Will move this character entity kinematically through physical world while also ensuring the network stays in-sync.
        //! Velocity will be applied over delta-time to determine the movement amount.
        //! Returns this entity's world-space position after the move.
        virtual AZ::Vector3 TryMoveWithVelocity(const AZ::Vector3& velocity, float deltaTime) = 0;
    };

    typedef AZ::EBus<NetworkCharacterRequests> NetworkCharacterRequestBus;
 
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
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

        NetworkCharacterComponent();

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
        , private NetworkCharacterRequestBus::Handler
    {
    public:
        AZ_RTTI(NetworkCharacterComponentController, "{C91851A2-8B95-4484-9F97-BFF9D1F528A0}")
        static void Reflect(AZ::ReflectContext* context);
        NetworkCharacterComponentController(NetworkCharacterComponent& parent);

        // NetworkCharacterComponentControllerBase
        void OnActivate(Multiplayer::EntityIsMigrating entityIsMigrating) override;
        void OnDeactivate(Multiplayer::EntityIsMigrating entityIsMigrating) override;

        // NetworkCharacterRequestBus::Handler
        //! TryMoveWithVelocity
        //! Will move this character entity kinematically through physical world while also ensuring the network stays in-sync.
        //! Velocity will be applied over delta-time to determine the movement amount.
        //! Returns this entity's world-space position after the move.
        AZ::Vector3 TryMoveWithVelocity(const AZ::Vector3& velocity, float deltaTime) override;
    };
}
