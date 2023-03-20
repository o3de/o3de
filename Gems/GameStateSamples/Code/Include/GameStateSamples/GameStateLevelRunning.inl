/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <GameState/GameStateRequestBus.h>
#include <GameStateSamples/GameStateLevelRunning.h>
#include <GameStateSamples/GameStateLevelLoading.h>
#include <GameStateSamples/GameStateLevelPaused.h>
#include <GameStateSamples/GameStateSamples_Traits_Platform.h>

#include <AzFramework/Input/Buses/Requests/InputTextEntryRequestBus.h>
#include <AzFramework/Input/Devices/Gamepad/InputDeviceGamepad.h>
#include <AzFramework/Input/Devices/Keyboard/InputDeviceKeyboard.h>
#include <AzFramework/Input/Devices/Touch/InputDeviceTouch.h>
#include <AzFramework/Input/Devices/VirtualKeyboard/InputDeviceVirtualKeyboard.h>

#include <LyShine/Bus/UiButtonBus.h>
#include <LyShine/Bus/UiCanvasBus.h>
#include <LyShine/Bus/UiCanvasManagerBus.h>

#include <ILevelSystem.h>
#include <ISystem.h>
#include <IConsole.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace GameStateSamples
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline void GameStateLevelRunning::OnPushed()
    {
        // Load the pause button if there's a touch input device connected
        const AzFramework::InputDevice* inputDeviceTouch = nullptr;
        AzFramework::InputDeviceRequestBus::EventResult(inputDeviceTouch,
                                                        AzFramework::InputDeviceTouch::Id,
                                                        &AzFramework::InputDeviceRequests::GetInputDevice);
        if (inputDeviceTouch && inputDeviceTouch->IsConnected())
        {
            LoadPauseButtonCanvas();
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline void GameStateLevelRunning::OnPopped()
    {
        UnloadPauseButtonCanvas();

        ISystem* iSystem = GetISystem();
        ILevelSystem* levelSystem = iSystem ? iSystem->GetILevelSystem() : nullptr;
        if (levelSystem && !iSystem->GetGlobalEnvironment()->IsEditor())
        {
            // Unload the currently loaded level
            levelSystem->UnloadLevel();
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline void GameStateLevelRunning::OnEnter()
    {
        InputChannelEventListener::Connect();
        AzFramework::ApplicationLifecycleEvents::Bus::Handler::BusConnect();

        if (ISystem* iSystem = GetISystem())
        {
            iSystem->GetISystemEventDispatcher()->RegisterListener(this);
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline void GameStateLevelRunning::OnExit()
    {
        if (ISystem* iSystem = GetISystem())
        {
            iSystem->GetISystemEventDispatcher()->RemoveListener(this);
        }

        AzFramework::ApplicationLifecycleEvents::Bus::Handler::BusDisconnect();
        InputChannelEventListener::Disconnect();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline AZ::s32 GameStateLevelRunning::GetPriority() const
    {
        // Make pausing the game precedence over any UI that might be showing
        return AzFramework::InputChannelEventListener::GetPriorityUI() + 1;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline bool GameStateLevelRunning::OnInputChannelEventFiltered(const AzFramework::InputChannel & inputChannel)
    {
        if (inputChannel.IsStateEnded() &&
            (inputChannel.GetInputChannelId() == AzFramework::InputDeviceGamepad::Button::Start ||
             inputChannel.GetInputChannelId() == AzFramework::InputDeviceKeyboard::Key::Escape))
        {
            PushLevelPausedGameState();
            return true; // Consume this input
        }

        return false; // Don't consume other input
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline void GameStateLevelRunning::OnApplicationConstrained(AzFramework::ApplicationLifecycleEvents::Event /*lastEvent*/)
    {
        bool pauseOnApplicationConstrained = false;
    #if AZ_TRAIT_GAMESTATESAMPLES_PAUSE_ON_APPLICATION_CONSTRAINED
        pauseOnApplicationConstrained = true;
    #else
        pauseOnApplicationConstrained = false;
    #endif // AZ_TRAIT_GAMESTATESAMPLES_PAUSE_ON_APPLICATION_CONSTRAINED
        ISystem* iSystem = GetISystem();
        IConsole* iConsole = iSystem ? iSystem->GetIConsole() : nullptr;
        if (iConsole && iConsole->GetCVar("sys_pauseOnApplicationConstrained"))
        {
            switch (iConsole->GetCVar("sys_pauseOnApplicationConstrained")->GetIVal())
            {
                case 0: { pauseOnApplicationConstrained = false; } break;
                case 1: { pauseOnApplicationConstrained = true; } break;
                default: break; // Use the default value that was set above
            }
        }

        // Do not pause if the application was constrained because the virtual keyboard was shown
        bool hasTextEntryStarted = false;
        AzFramework::InputTextEntryRequestBus::EventResult(hasTextEntryStarted,
                                                           AzFramework::InputDeviceVirtualKeyboard::Id,
                                                           &AzFramework::InputTextEntryRequests::HasTextEntryStarted);
        if (pauseOnApplicationConstrained && !hasTextEntryStarted)
        {
            PushLevelPausedGameState();
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline void GameStateLevelRunning::OnSystemEvent(ESystemEvent event, UINT_PTR, UINT_PTR)
    {
        // If the user happens to initiate a level load outside the context of these game states,
        // for example via executing the 'map' command from the debug console or in autoexec.cfg,
        // this will also be detected by checking for the ESYSTEM_EVENT_LEVEL_LOAD_PREPARE event.
        if (event == ESYSTEM_EVENT_LEVEL_LOAD_PREPARE)
        {
            // Replace the level running game state with the level loading game state
            AZ_Assert(GameState::GameStateRequests::IsActiveGameStateOfType<GameStateLevelRunning>(),
                      "The active game state is not of type GameStateLevelRunning");
            AZ_Assert(!GameState::GameStateRequests::DoesStackContainGameStateOfType<GameStateLevelLoading>(),
                      "The game state stack already contains an instance of GameStateLevelLoading");
            AZStd::shared_ptr<GameState::IGameState> gameStateLevelLoading = GameState::GameStateRequests::CreateNewOverridableGameStateOfType<GameStateLevelLoading>();
            GameState::GameStateRequestBus::Broadcast(&GameState::GameStateRequests::ReplaceActiveGameState, gameStateLevelLoading);
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline void GameStateLevelRunning::PushLevelPausedGameState()
    {
        AZ_Assert(!GameState::GameStateRequests::DoesStackContainGameStateOfType<GameStateLevelPaused>(),
                  "The game state stack already contains an instance of GameStateLevelPaused");
        GameState::GameStateRequests::CreateAndPushNewOverridableGameStateOfType<GameStateLevelPaused>();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline void GameStateLevelRunning::LoadPauseButtonCanvas()
    {
        // Load the UI canvas
        const char* uiCanvasAssetPath = GetPauseButtonCanvasAssetPath();
        UiCanvasManagerBus::BroadcastResult(m_pauseButtonCanvasEntityId,
                                            &UiCanvasManagerInterface::LoadCanvas,
                                            uiCanvasAssetPath);
        if (!m_pauseButtonCanvasEntityId.IsValid())
        {
            AZ_Warning("GameStateLevelRunning", false, "Could not load %s", uiCanvasAssetPath);
            return;
        }

        // Display the pause HUD
        UiCanvasBus::Event(m_pauseButtonCanvasEntityId, &UiCanvasInterface::SetEnabled, true);

        // Setup the 'Pause' button to push the level paused state
        AZ::EntityId pauseButtonElementId;
        UiCanvasBus::EventResult(pauseButtonElementId,
                                 m_pauseButtonCanvasEntityId,
                                 &UiCanvasInterface::FindElementEntityIdByName,
                                 "PauseButton");

        auto OnPauseButtonClicked = [this]([[maybe_unused]] AZ::EntityId clickedEntityId, [[maybe_unused]] AZ::Vector2 point)
        {
            PushLevelPausedGameState();
        };
        UiButtonBus::Event(pauseButtonElementId, &UiButtonInterface::SetOnClickCallback, OnPauseButtonClicked);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline void GameStateLevelRunning::UnloadPauseButtonCanvas()
    {
        if (m_pauseButtonCanvasEntityId.IsValid())
        {
            // Unload the pause menu
            UiCanvasManagerBus::Broadcast(&UiCanvasManagerInterface::UnloadCanvas,
                                          m_pauseButtonCanvasEntityId);
            m_pauseButtonCanvasEntityId.SetInvalid();
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline const char* GameStateLevelRunning::GetPauseButtonCanvasAssetPath()
    {
        return "@products@/ui/canvases/defaultpausebuttonfortouchscreens.uicanvas";
    }
}
