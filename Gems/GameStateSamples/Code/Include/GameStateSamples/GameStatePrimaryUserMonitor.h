/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <GameState/GameState.h>
#include <GameState/GameStateNotificationBus.h>

#include <LocalUser/LocalUserNotificationBus.h>

#include <AzFramework/API/ApplicationAPI.h>
#include <AzFramework/Input/Buses/Notifications/InputDeviceNotificationBus.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace GameStateSamples
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Game state that is pushed after determining the primary user (GameStatePrimaryUserSelection)
    //! that monitors for events related to the primary user we must respond to (eg. user sign-out).
    //! This state will almost never be active, so it won't receive updates, but will rather sit in
    //! the stack monitoring for events and respond them by pushing (or popping) other game states.
    class GameStatePrimaryUserMonitor : public GameState::IGameState
                                      , public GameState::GameStateNotificationBus::Handler
                                      , public AzFramework::InputDeviceNotificationBus::Handler
                                      , public AzFramework::ApplicationLifecycleEvents::Bus::Handler
                                      , public LocalUser::LocalUserNotificationBus::Handler
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        // Allocator
        AZ_CLASS_ALLOCATOR(GameStatePrimaryUserMonitor, AZ::SystemAllocator);

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Type Info
        AZ_RTTI(GameStatePrimaryUserMonitor, "{2B7DB914-DEEC-4A2F-B178-9AD953D70FE0}", IGameState);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Default constructor
        GameStatePrimaryUserMonitor() = default;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Default destructor
        ~GameStatePrimaryUserMonitor() override = default;

    protected:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref GameState::GameState::OnPushed
        void OnPushed() override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref GameState::GameState::OnPopped
        void OnPopped() override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref GameState::GameStateNotifications::OnActiveGameStateChanged
        void OnActiveGameStateChanged(AZStd::shared_ptr<IGameState> oldGameState,
                                      AZStd::shared_ptr<IGameState> newGameState) override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputDeviceNotifications::OnInputDeviceConnectedEvent
        void OnInputDeviceConnectedEvent(const AzFramework::InputDevice& inputDevice) override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputDeviceNotifications::OnInputDeviceDisconnectedEvent
        void OnInputDeviceDisconnectedEvent(const AzFramework::InputDevice& inputDevice) override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::ApplicationLifecycleEvents::OnApplicationUnconstrained
        void OnApplicationUnconstrained(AzFramework::ApplicationLifecycleEvents::Event /*lastEvent*/) override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref LocalUser::LocalUserNotifications::OnLocalUserSignedIn
        void OnLocalUserSignedIn(AzFramework::LocalUserId localUserId) override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref LocalUser::LocalUserNotifications::OnLocalUserSignedOut
        void OnLocalUserSignedOut(AzFramework::LocalUserId localUserId) override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Convenience function to push the default primary controller disconnected game state.
        //! Override if you wish to push a different primary controller disconnected game state.
        virtual void PushPrimaryControllerDisconnectedGameState();

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Convenience function to push the default primary user signed out game state.
        //! Override if you wish to push a different primary user signed out game state.
        virtual void PushPrimaryUserSignedOutGameState();

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Convenience function to push the default level paused game state (if it is needed).
        //! Override if you wish to push a different level paused game state.
        virtual void TryPushLevelPausedGameState();

    private:
        ////////////////////////////////////////////////////////////////////////////////////////////
        // Variables
        bool m_primaryControllerDisconnectedWhileLevelLoading = false;
        bool m_primaryUserSignedOutWhileLevelLoading = false;
    };
} // namespace GameStateSamples

// Include the implementation inline so the class can be instantiated outside of the gem.
#include <GameStateSamples/GameStatePrimaryUserMonitor.inl>
