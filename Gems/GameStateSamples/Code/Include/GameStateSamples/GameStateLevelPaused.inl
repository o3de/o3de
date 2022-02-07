/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <GameState/GameStateRequestBus.h>
#include <GameStateSamples/GameStateLevelPaused.h>
#include <GameStateSamples/GameStateMainMenu.h>
#include <GameStateSamples/GameStateOptionsMenu.h>

#include <AzFramework/Input/Devices/Gamepad/InputDeviceGamepad.h>
#include <AzFramework/Input/Devices/Keyboard/InputDeviceKeyboard.h>

#include <LyShine/Bus/UiButtonBus.h>
#include <LyShine/Bus/UiCanvasBus.h>
#include <LyShine/Bus/UiCanvasManagerBus.h>
#include <LyShine/Bus/UiCursorBus.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace GameStateSamples
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline void GameStateLevelPaused::OnPushed()
    {
        LoadPauseMenuCanvas();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline void GameStateLevelPaused::OnPopped()
    {
        UnloadPauseMenuCanvas();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline void GameStateLevelPaused::OnEnter()
    {
        InputChannelEventListener::Connect();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline void GameStateLevelPaused::OnExit()
    {
        InputChannelEventListener::Disconnect();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline AZ::s32 GameStateLevelPaused::GetPriority() const
    {
        // Make unpausing the game precedence over any UI that might be showing
        return AzFramework::InputChannelEventListener::GetPriorityUI() + 1;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline bool GameStateLevelPaused::OnInputChannelEventFiltered(const AzFramework::InputChannel & inputChannel)
    {
        if (inputChannel.IsStateEnded() &&
            (inputChannel.GetInputChannelId() == AzFramework::InputDeviceGamepad::Button::Start ||
             inputChannel.GetInputChannelId() == AzFramework::InputDeviceKeyboard::Key::Escape))
        {
            AZ_Assert(GameState::GameStateRequests::IsActiveGameStateOfType<GameStateLevelPaused>(),
                      "The active game state is not an instance of GameStateLevelPaused");
            GameState::GameStateRequestBus::Broadcast(&GameState::GameStateRequests::PopActiveGameState);
            return true; // Consume this input
        }

        return false; // Don't consume other input
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline void GameStateLevelPaused::LoadPauseMenuCanvas()
    {
        // Load the UI canvas
        const char* uiCanvasAssetPath = GetPauseMenuCanvasAssetPath();
        UiCanvasManagerBus::BroadcastResult(m_pauseMenuCanvasEntityId,
                                            &UiCanvasManagerInterface::LoadCanvas,
                                            uiCanvasAssetPath);
        if (!m_pauseMenuCanvasEntityId.IsValid())
        {
            AZ_Warning("GameStateLevelPaused", false, "Could not load %s", uiCanvasAssetPath);
            return;
        }

        // Display the pause menu
        UiCanvasBus::Event(m_pauseMenuCanvasEntityId, &UiCanvasInterface::SetEnabled, true);
        SetPauseMenuCanvasDrawOrder();

        // Display the UI cursor
        UiCursorBus::Broadcast(&UiCursorInterface::IncrementVisibleCounter);

        // Setup the 'Resume' button to return to the level running state
        AZ::EntityId resumeButtonElementId;
        UiCanvasBus::EventResult(resumeButtonElementId,
                                 m_pauseMenuCanvasEntityId,
                                 &UiCanvasInterface::FindElementEntityIdByName,
                                 "ResumeButton");
        UiButtonBus::Event(resumeButtonElementId,
                           &UiButtonInterface::SetOnClickCallback,
                           []([[maybe_unused]] AZ::EntityId clickedEntityId, [[maybe_unused]] AZ::Vector2 point)
        {
            AZ_Assert(GameState::GameStateRequests::IsActiveGameStateOfType<GameStateLevelPaused>(),
                      "The active game state is not an instance of GameStateLevelPaused");
            GameState::GameStateRequestBus::Broadcast(&GameState::GameStateRequests::PopActiveGameState);
        });

        // Setup the 'Options' button to open the options menu
        AZ::EntityId optionsButtonElementId;
        UiCanvasBus::EventResult(optionsButtonElementId,
                                 m_pauseMenuCanvasEntityId,
                                 &UiCanvasInterface::FindElementEntityIdByName,
                                 "OptionsButton");
        UiButtonBus::Event(optionsButtonElementId,
                           &UiButtonInterface::SetOnClickCallback,
                           []([[maybe_unused]] AZ::EntityId clickedEntityId, [[maybe_unused]] AZ::Vector2 point)
        {
            AZ_Assert(GameState::GameStateRequests::IsActiveGameStateOfType<GameStateLevelPaused>(),
                      "The active game state is not an instance of GameStateLevelPaused");
            GameState::GameStateRequests::CreateAndPushNewOverridableGameStateOfType<GameStateOptionsMenu>();
        });

        // Setup the 'Return to Main Menu' button to return to the main menu state
        AZ::EntityId returnToMainMenuButtonElementId;
        UiCanvasBus::EventResult(returnToMainMenuButtonElementId,
                                 m_pauseMenuCanvasEntityId,
                                 &UiCanvasInterface::FindElementEntityIdByName,
                                 "ReturnToMainMenuButton");
        const bool enableReturnToMainMenuButton = GameState::GameStateRequests::DoesStackContainGameStateOfType<GameStateMainMenu>();
        UiElementBus::Event(returnToMainMenuButtonElementId, &UiElementInterface::SetIsEnabled, enableReturnToMainMenuButton);
        if (enableReturnToMainMenuButton)
        {
            UiButtonBus::Event(returnToMainMenuButtonElementId,
                               &UiButtonInterface::SetOnClickCallback,
                               []([[maybe_unused]] AZ::EntityId clickedEntityId, [[maybe_unused]] AZ::Vector2 point)
            {
                AZ_Assert(GameState::GameStateRequests::IsActiveGameStateOfType<GameStateLevelPaused>(),
                          "The active game state is not an instance of GameStateLevelPaused");
                GameState::GameStateRequests::PopActiveGameStateUntilOfType<GameStateMainMenu>();
            });
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline void GameStateLevelPaused::UnloadPauseMenuCanvas()
    {
        if (m_pauseMenuCanvasEntityId.IsValid())
        {
            // Hide the UI cursor
            UiCursorBus::Broadcast(&UiCursorInterface::DecrementVisibleCounter);

            // Unload the pause menu
            UiCanvasManagerBus::Broadcast(&UiCanvasManagerInterface::UnloadCanvas,
                                          m_pauseMenuCanvasEntityId);
            m_pauseMenuCanvasEntityId.SetInvalid();
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline void GameStateLevelPaused::SetPauseMenuCanvasDrawOrder()
    {
        // Loaded canvases are already stored sorted by draw order...
        UiCanvasManagerInterface::CanvasEntityList canvases;
        UiCanvasManagerBus::BroadcastResult(canvases, &UiCanvasManagerInterface::GetLoadedCanvases);

        // ...so get the draw order of the top-most displayed UI canvas...
        int highestDrawOrder = 0;
        UiCanvasBus::EventResult(highestDrawOrder, canvases.back(), &UiCanvasInterface::GetDrawOrder);

        // ...and increment it by one unless it's already set to int max...
        if (highestDrawOrder != std::numeric_limits<int>::max())
        {
            ++highestDrawOrder;
        }

        // ...ensuring the pause menu gets displayed on top of all other loaded canvases,
        // with the exception of 'special' ones like message popups or the loading screen
        // that use a draw order of int max.
        UiCanvasBus::Event(m_pauseMenuCanvasEntityId, &UiCanvasInterface::SetDrawOrder, highestDrawOrder);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline const char* GameStateLevelPaused::GetPauseMenuCanvasAssetPath()
    {
        return "@products@/ui/canvases/defaultpausemenuscreen.uicanvas";
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline void GameStateLevelPaused::PauseGame()
    {
        // We need a way to pause the game.
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline void GameStateLevelPaused::UnpauseGame()
    {
        // We need a way to un-pause the game.
    }
}
