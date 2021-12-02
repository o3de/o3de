/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <GameState/GameStateRequestBus.h>
#include <GameStateSamples/GameStateMainMenu.h>
#include <GameStateSamples/GameStatePrimaryUserSelection.h>
#include <GameStateSamples/GameStatePrimaryUserMonitor.h>

#include <LyShine/Bus/UiCanvasBus.h>
#include <LyShine/Bus/UiCanvasManagerBus.h>

#include <LocalUser/LocalUserRequestBus.h>

#include <AzFramework/Input/Utils/IsAnyKeyOrButton.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace GameStateSamples
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline void GameStatePrimaryUserSelection::OnPushed()
    {
        // We could load the UI canvas here and keep it cached until OnPopped is called in order to
        // speed up re-entering this game state, but doing so would consume memory for the lifetime
        // of the process that is only needed while this state is active (which is not very often).
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline void GameStatePrimaryUserSelection::OnPopped()
    {
        // See comment above in OnPushed
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline void GameStatePrimaryUserSelection::OnEnter()
    {
        // In case we are returning to this game state from another (rather than for the first time)
        LocalUser::LocalUserRequestBus::Broadcast(&LocalUser::LocalUserRequests::ClearAllLocalUserIdToLocalPlayerSlotAssignments);
        UiCanvasManagerBus::Broadcast(&UiCanvasManagerInterface::SetLocalUserIdInputFilterForAllCanvases, AzFramework::LocalUserIdAny);

        // Load and display the UI canvas
        LoadPrimaryUserSelectionCanvas();

        // Start listening for input in order to determine the primary user
        InputChannelEventListener::Connect();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline void GameStatePrimaryUserSelection::OnExit()
    {
        // Stop listening for input
        InputChannelEventListener::Disconnect();

        // Hide and unload the UI canvas
        UnloadPrimaryUserSelectionCanvas();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline AZ::s32 GameStatePrimaryUserSelection::GetPriority() const
    {
        // Take precedence over all other input in order to detect the press of a button or key that
        // will identify the primary user. If you override this class and wish to detect the primary
        // user through another means or want the UI displayed to process input, you should override
        // GameStatePrimaryUserSelection::OnInputChannelEventFiltered to do nothing.
        return AzFramework::InputChannelEventListener::GetPriorityFirst();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline bool GameStatePrimaryUserSelection::OnInputChannelEventFiltered(const AzFramework::InputChannel & inputChannel)
    {
        if (inputChannel.IsStateEnded() && AzFramework::IsAnyKeyOrButton(inputChannel))
        {
            const AzFramework::LocalUserId localUserId = inputChannel.GetInputDevice().GetAssignedLocalUserId();
            if (localUserId == AzFramework::LocalUserIdAny ||
                localUserId == AzFramework::LocalUserIdNone)
            {
                // No local user is associated with this input device yet, so prompt for user sign-in
                inputChannel.GetInputDevice().PromptLocalUserSignIn();
            }
            else
            {
                SetPrimaryLocalUser(localUserId);
                PushPrimaryUserMonitorGameState();
                PushMainMenuGameState();
            }
        }

        // Consume the input regardless because nothing else should be able to
        // process it while we're waiting to determine who the primary user is.
        return true;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline void GameStatePrimaryUserSelection::SetPrimaryLocalUser(AzFramework::LocalUserId localUserId)
    {
        AZ::u32 assignedSlot = LocalUser::LocalPlayerSlotNone;
        LocalUser::LocalUserRequestBus::BroadcastResult(assignedSlot,
                                                        &LocalUser::LocalUserRequests::AssignLocalUserIdToLocalPlayerSlot,
                                                        localUserId,
                                                        LocalUser::LocalPlayerSlotPrimary);
        AZ_Assert(assignedSlot == LocalUser::LocalPlayerSlotPrimary,
                  "Could not assign local user id %u to the primary local player slot", localUserId);

        // Make it so only the primary user can interact with the UI
        UiCanvasManagerBus::Broadcast(&UiCanvasManagerInterface::SetLocalUserIdInputFilterForAllCanvases, localUserId);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline void GameStatePrimaryUserSelection::PushPrimaryUserMonitorGameState()
    {
        // Push the game state that monitors for events related to the primary user we must respond to
        if (GameState::GameStateRequests::DoesStackContainGameStateOfType<GameStatePrimaryUserMonitor>())
        {
            AZ_Assert(false, "The game state stack already contains an instance of GameStatePrimaryUserMonitor");
            return;
        }

        GameState::GameStateRequests::CreateAndPushNewOverridableGameStateOfType<GameStatePrimaryUserMonitor>();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline void GameStatePrimaryUserSelection::PushMainMenuGameState()
    {
        // Push the game menu game state
        if (GameState::GameStateRequests::DoesStackContainGameStateOfType<GameStateMainMenu>())
        {
            AZ_Assert(false, "The game state stack already contains an instance of GameStateMainMenu");
            return;
        }

        GameState::GameStateRequests::CreateAndPushNewOverridableGameStateOfType<GameStateMainMenu>();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline void GameStatePrimaryUserSelection::LoadPrimaryUserSelectionCanvas()
    {
        // Load the UI canvas
        const char* uiCanvasAssetPath = GetPrimaryUserSelectionCanvasAssetPath();
        UiCanvasManagerBus::BroadcastResult(m_primaryUserSelectionCanvasEntityId,
                                            &UiCanvasManagerInterface::LoadCanvas,
                                            uiCanvasAssetPath);
        if (!m_primaryUserSelectionCanvasEntityId.IsValid())
        {
            AZ_Warning("GameStatePrimaryUserSelection", false, "Could not load %s", uiCanvasAssetPath);
            return;
        }

        // Display the canvas and set it to stay loaded when a level unloads
        UiCanvasBus::Event(m_primaryUserSelectionCanvasEntityId, &UiCanvasInterface::SetEnabled, true);
        UiCanvasBus::Event(m_primaryUserSelectionCanvasEntityId, &UiCanvasInterface::SetKeepLoadedOnLevelUnload, true);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline void GameStatePrimaryUserSelection::UnloadPrimaryUserSelectionCanvas()
    {
        if (m_primaryUserSelectionCanvasEntityId.IsValid())
        {
            // Unload the main menu
            UiCanvasManagerBus::Broadcast(&UiCanvasManagerInterface::UnloadCanvas,
                                          m_primaryUserSelectionCanvasEntityId);
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline const char* GameStatePrimaryUserSelection::GetPrimaryUserSelectionCanvasAssetPath()
    {
        return "@products@/ui/canvases/defaultprimaryuserselectionscreen.uicanvas";
    }
}
