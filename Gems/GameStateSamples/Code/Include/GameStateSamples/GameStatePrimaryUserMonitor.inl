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

#include <GameState/GameStateRequestBus.h>
#include <GameStateSamples/GameStateLevelLoading.h>
#include <GameStateSamples/GameStateLevelPaused.h>
#include <GameStateSamples/GameStateLevelRunning.h>
#include <GameStateSamples/GameStatePrimaryControllerDisconnected.h>
#include <GameStateSamples/GameStatePrimaryUserMonitor.h>
#include <GameStateSamples/GameStatePrimaryUserSignedOut.h>

#include <LocalUser/LocalUserRequestBus.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace GameStateSamples
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline void GameStatePrimaryUserMonitor::OnPushed()
    {
        m_primaryControllerDisconnectedWhileLevelLoading = false;
        m_primaryUserSignedOutWhileLevelLoading = false;

        GameState::GameStateNotificationBus::Handler::BusConnect();
        AzFramework::InputDeviceNotificationBus::Handler::BusConnect();
        AzFramework::ApplicationLifecycleEvents::Bus::Handler::BusConnect();
        LocalUser::LocalUserNotificationBus::Handler::BusConnect();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline void GameStatePrimaryUserMonitor::OnPopped()
    {
        LocalUser::LocalUserNotificationBus::Handler::BusDisconnect();
        AzFramework::ApplicationLifecycleEvents::Bus::Handler::BusDisconnect();
        AzFramework::InputDeviceNotificationBus::Handler::BusDisconnect();
        GameState::GameStateNotificationBus::Handler::BusDisconnect();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline void GameStatePrimaryUserMonitor::OnActiveGameStateChanged(AZStd::shared_ptr<IGameState> oldGameState,
                                                                      AZStd::shared_ptr<IGameState> newGameState)
    {
        if (m_primaryUserSignedOutWhileLevelLoading &&
            azrtti_istypeof<GameStateLevelLoading>(oldGameState.get()))
        {
            // The primary user signed out while a level was loading,
            // we had to wait until the level finished loading before
            // transitioning to the primary user signed out game state.
            m_primaryUserSignedOutWhileLevelLoading = false;
            m_primaryControllerDisconnectedWhileLevelLoading = false;
            PushPrimaryUserSignedOutGameState();
            return;
        }

        if (m_primaryControllerDisconnectedWhileLevelLoading &&
            azrtti_istypeof<GameStateLevelLoading>(oldGameState.get()))
        {
            // The controller disconnected while a level was loading,
            // we had to wait until the level finished loading before
            // transitioning to the controller disconnected game state.
            m_primaryControllerDisconnectedWhileLevelLoading = false;
            PushPrimaryControllerDisconnectedGameState();
            return;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline void GameStatePrimaryUserMonitor::OnInputDeviceConnectedEvent(const AzFramework::InputDevice& inputDevice)
    {
        const AzFramework::LocalUserId primaryLocalUserId = LocalUser::LocalUserRequests::GetPrimaryLocalUserId();
        if (m_primaryControllerDisconnectedWhileLevelLoading &&
            primaryLocalUserId == inputDevice.GetAssignedLocalUserId())
        {
            // The controller disconnected while a level was loading,
            // but was reconnected before the level finished loading.
            m_primaryControllerDisconnectedWhileLevelLoading = false;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline void GameStatePrimaryUserMonitor::OnInputDeviceDisconnectedEvent(const AzFramework::InputDevice& inputDevice)
    {
        const AzFramework::LocalUserId primaryLocalUserId = LocalUser::LocalUserRequests::GetPrimaryLocalUserId();
        if (primaryLocalUserId != inputDevice.GetAssignedLocalUserId() ||
            primaryLocalUserId == AzFramework::LocalUserIdNone)
        {
            // The disconnected controller does not belong to the primary user,
            // or the primary user has not yet been set.
            return;
        }

        if (GameState::GameStateRequests::IsActiveGameStateOfType<GameStateLevelLoading>())
        {
            // The controller disconnected while a level is loading;
            // we have to wait until the level has finished loading.
            m_primaryControllerDisconnectedWhileLevelLoading = true;
            return;
        }

        PushPrimaryControllerDisconnectedGameState();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline void GameStatePrimaryUserMonitor::OnApplicationUnconstrained(AzFramework::ApplicationLifecycleEvents::Event /*lastEvent*/)
    {
        const AzFramework::LocalUserId primaryLocalUserId = LocalUser::LocalUserRequests::GetPrimaryLocalUserId();
        if (primaryLocalUserId == AzFramework::LocalUserIdNone)
        {
            // The primary user has yet to be set
            m_primaryUserSignedOutWhileLevelLoading = false;
            return;
        }

        bool isPrimaryLocalUserSignedIn = false;
        LocalUser::LocalUserRequestBus::BroadcastResult(isPrimaryLocalUserSignedIn,
                                                        &LocalUser::LocalUserRequests::IsLocalUserSignedIn,
                                                        primaryLocalUserId);
        if (isPrimaryLocalUserSignedIn)
        {
            // The primary user is still signed in
            m_primaryUserSignedOutWhileLevelLoading = false;
            return;
        }

        if (GameState::GameStateRequests::IsActiveGameStateOfType<GameStateLevelLoading>())
        {
            // The primary user signed out while a level is loading;
            // we have to wait until the level has finished loading.
            m_primaryUserSignedOutWhileLevelLoading = true;
            return;
        }

        PushPrimaryUserSignedOutGameState();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline void GameStatePrimaryUserMonitor::OnLocalUserSignedIn(AzFramework::LocalUserId localUserId)
    {
        const AzFramework::LocalUserId primaryLocalUserId = LocalUser::LocalUserRequests::GetPrimaryLocalUserId();
        if (m_primaryUserSignedOutWhileLevelLoading &&
            primaryLocalUserId == localUserId)
        {
            // The primary user signed out while a level was loading,
            // but signed in again before the level finished loading.
            m_primaryUserSignedOutWhileLevelLoading = false;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline void GameStatePrimaryUserMonitor::OnLocalUserSignedOut(AzFramework::LocalUserId localUserId)
    {
        const AzFramework::LocalUserId primaryLocalUserId = LocalUser::LocalUserRequests::GetPrimaryLocalUserId();
        if (primaryLocalUserId != localUserId ||
            primaryLocalUserId == AzFramework::LocalUserIdNone)
        {
            // The user that signed out is not the primary user,
            // or the primary user has not yet been set.
            return;
        }

        if (GameState::GameStateRequests::IsActiveGameStateOfType<GameStateLevelLoading>())
        {
            // The primary user signed out while a level is loading;
            // we have to wait until the level has finished loading.
            m_primaryUserSignedOutWhileLevelLoading = true;
            return;
        }

        PushPrimaryUserSignedOutGameState();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline void GameStatePrimaryUserMonitor::PushPrimaryControllerDisconnectedGameState()
    {
        if (GameState::GameStateRequests::DoesStackContainGameStateOfType<GameStatePrimaryControllerDisconnected>() ||
            GameState::GameStateRequests::DoesStackContainGameStateOfType<GameStatePrimaryUserSignedOut>())
        {
            // The controller disconnection has already been detected,
            // or the primary user signed out (which takes precedence).
            return;
        }

        // Ensure the game is paused if needed before pushing the controller disconnected game state
        TryPushLevelPausedGameState();
        GameState::GameStateRequests::CreateAndPushNewOverridableGameStateOfType<GameStatePrimaryControllerDisconnected>();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline void GameStatePrimaryUserMonitor::PushPrimaryUserSignedOutGameState()
    {
        if (GameState::GameStateRequests::DoesStackContainGameStateOfType<GameStatePrimaryUserSignedOut>())
        {
            // The primary user sign out has already been detected
            return;
        }

        if (GameState::GameStateRequests::IsActiveGameStateOfType<GameStatePrimaryControllerDisconnected>())
        {
            // The primary user signing out takes precedence over their controller being disconnected
            GameState::GameStateRequestBus::Broadcast(&GameState::GameStateRequests::PopActiveGameState);
        }

        // Ensure the game is paused if needed before pushing the primary user signed out game state
        TryPushLevelPausedGameState();
        GameState::GameStateRequests::CreateAndPushNewOverridableGameStateOfType<GameStatePrimaryUserSignedOut>();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline void GameStatePrimaryUserMonitor::TryPushLevelPausedGameState()
    {
        if (GameState::GameStateRequests::DoesStackContainGameStateOfType<GameStateLevelPaused>() ||
            !GameState::GameStateRequests::DoesStackContainGameStateOfType<GameStateLevelRunning>())
        {
            // The game has already been paused or is not actively running yet
            return;
        }

        GameState::GameStateRequests::CreateAndPushNewOverridableGameStateOfType<GameStateLevelPaused>();
    }
}
