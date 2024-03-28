/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/TickBus.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzFramework/Spawnable/Spawnable.h>

#include <GameState/GameStateRequestBus.h>
#include <GameStateSamples/GameStateMainMenu.h>
#include <GameStateSamples/GameStateOptionsMenu.h>
#include <GameStateSamples/GameStateLevelLoading.h>
#include <GameStateSamples/GameStatePrimaryUserSelection.h>
#include <GameStateSamples/GameStateSamples_Traits_Platform.h>

#include <LocalUser/LocalUserRequestBus.h>

#include <LyShine/Bus/UiButtonBus.h>
#include <LyShine/Bus/UiCanvasBus.h>
#include <LyShine/Bus/UiCanvasManagerBus.h>
#include <LyShine/Bus/UiCursorBus.h>
#include <LyShine/Bus/UiDynamicLayoutBus.h>
#include <LyShine/Bus/UiElementBus.h>

#include <SaveData/SaveDataRequestBus.h>

#include <ILevelSystem.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace GameStateSamples
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline void GameStateMainMenu::OnPushed()
    {
        // We could load the UI canvas here and keep it cached until OnPopped is called in order to
        // speed up re-entering this game state, but doing so would consume memory for the lifetime
        // of the process that is only needed while this state is active (which is not very often).

        bool createLocalUserLobbySubState = false;
    #if AZ_TRAIT_GAMESTATESAMPLES_LOCAL_USER_LOBBY_ENABLED
        createLocalUserLobbySubState = true;
    #else
        createLocalUserLobbySubState = false;
    #endif // AZ_TRAIT_GAMESTATESAMPLES_LOCAL_USER_LOBBY_ENABLED
        ISystem* iSystem = GetISystem();
        IConsole* iConsole = iSystem ? iSystem->GetIConsole() : nullptr;
        if (iConsole && iConsole->GetCVar("sys_localUserLobbyEnabled"))
        {
            switch (iConsole->GetCVar("sys_localUserLobbyEnabled")->GetIVal())
            {
                case 0: { createLocalUserLobbySubState = false; } break;
                case 1: { createLocalUserLobbySubState = true; } break;
                default: break; // Use the default value that was set above
            }
        }

        if (createLocalUserLobbySubState)
        {
            m_localUserLobbySubState.reset(aznew GameStateLocalUserLobby());
            m_localUserLobbySubState->OnPushed();
        }

        LoadGameOptionsFromPersistentStorage();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline void GameStateMainMenu::OnPopped()
    {
        // See comment above in OnPushed

        if (m_localUserLobbySubState)
        {
            m_localUserLobbySubState->OnPopped();
            m_localUserLobbySubState.reset();
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline void GameStateMainMenu::OnEnter()
    {
        LoadMainMenuCanvas();

        if (m_localUserLobbySubState)
        {
            m_localUserLobbySubState->OnEnter();
        }

        if (ISystem* iSystem = GetISystem())
        {
            iSystem->GetISystemEventDispatcher()->RegisterListener(this);
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline void GameStateMainMenu::OnExit()
    {
        if (ISystem* iSystem = GetISystem())
        {
            iSystem->GetISystemEventDispatcher()->RemoveListener(this);
        }

        if (m_localUserLobbySubState)
        {
            m_localUserLobbySubState->OnExit();
        }

        UnloadMainMenuCanvas();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline void GameStateMainMenu::OnUpdate()
    {
        // This should be called directly from LoadMainMenuCanvas (or right after it from OnEnter),
        // but at that point in our convoluted startup sequence the level system doesn't exist yet.
        if (m_shouldRefreshLevelListDisplay)
        {
            m_shouldRefreshLevelListDisplay = false;
            RefreshLevelListDisplay();
        }

        if (m_localUserLobbySubState)
        {
            m_localUserLobbySubState->OnUpdate();
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline void OnLevelButtonClicked(AZ::EntityId entityId, AZ::Vector2)
    {
        // Load the selected level
        AZStd::string levelName;
        UiButtonBus::EventResult(levelName, entityId, &UiButtonInterface::GetOnClickActionName);
        ISystem* iSystem = GetISystem();
        ILevelSystem* levelSystem = iSystem ? iSystem->GetILevelSystem() : nullptr;

        if (levelSystem && !levelName.empty())
        {
            // This command gets delayed by one frame, so we check for the
            // actual level load start in GameStateMainMenu::OnSystemEvent
            AZ::TickBus::QueueFunction([levelSystem, levelName]() {
                levelSystem->UnloadLevel();
                levelSystem->LoadLevel(levelName.c_str());
            });
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline void OnOptionsButtonClicked([[maybe_unused]] AZ::EntityId entityId, AZ::Vector2)
    {
        AZ_Assert(GameState::GameStateRequests::IsActiveGameStateOfType<GameStateMainMenu>(),
                  "The active game state is not an instance of GameStateMainMenu");
        GameState::GameStateRequests::CreateAndPushNewOverridableGameStateOfType<GameStateOptionsMenu>();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline void OnBackButtonClicked([[maybe_unused]] AZ::EntityId entityId, AZ::Vector2)
    {
        AZ_Assert(GameState::GameStateRequests::DoesStackContainGameStateOfType<GameStatePrimaryUserSelection>(),
                    "The game state stack doesn't contain an instance of GameStatePrimaryUserSelection");
        GameState::GameStateRequests::PopActiveGameStateUntilOfType<GameStatePrimaryUserSelection>();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline void GameStateMainMenu::OnSystemEvent(ESystemEvent event, UINT_PTR, UINT_PTR)
    {
        // If the user happens to initiate a level load outside the context of these game states,
        // for example via executing the 'map' command from the debug console or in autoexec.cfg,
        // this will also be detected by checking for the ESYSTEM_EVENT_LEVEL_LOAD_PREPARE event.
        if (event == ESYSTEM_EVENT_LEVEL_LOAD_PREPARE)
        {
            // Push the level loading game state
            AZ_Assert(!GameState::GameStateRequests::DoesStackContainGameStateOfType<GameStateLevelLoading>(),
                      "The game state stack already contains an instance of GameStateLevelLoading");
            GameState::GameStateRequests::CreateAndPushNewOverridableGameStateOfType<GameStateLevelLoading>();
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline void GameStateMainMenu::LoadMainMenuCanvas()
    {
        // Load the UI canvas
        const char* uiCanvasAssetPath = GetMainMenuCanvasAssetPath();
        UiCanvasManagerBus::BroadcastResult(m_mainMenuCanvasEntityId,
                                            &UiCanvasManagerInterface::LoadCanvas,
                                            uiCanvasAssetPath);
        if (!m_mainMenuCanvasEntityId.IsValid())
        {
            AZ_Warning("GameStateMainMenu", false, "Could not load %s", uiCanvasAssetPath);
            return;
        }

        // Display the main menu and set it to stay loaded when a level unloads
        UiCanvasBus::Event(m_mainMenuCanvasEntityId, &UiCanvasInterface::SetEnabled, true);
        UiCanvasBus::Event(m_mainMenuCanvasEntityId, &UiCanvasInterface::SetKeepLoadedOnLevelUnload, true);

        // Display the UI cursor
        UiCursorBus::Broadcast(&UiCursorInterface::IncrementVisibleCounter);

        // Setup the 'Options' button to open the options menu
        AZ::EntityId optionsButtonElementId;
        UiCanvasBus::EventResult(optionsButtonElementId,
                                 m_mainMenuCanvasEntityId,
                                 &UiCanvasInterface::FindElementEntityIdByName,
                                 "OptionsButton");
        UiButtonBus::Event(optionsButtonElementId, &UiButtonInterface::SetOnClickCallback, OnOptionsButtonClicked);

        // Setup the 'Back' button to return to the primary user selection screen
        AZ::EntityId backButtonElementId;
        UiCanvasBus::EventResult(backButtonElementId,
                                 m_mainMenuCanvasEntityId,
                                 &UiCanvasInterface::FindElementEntityIdByName,
                                 "BackButton");
        const bool enableBackButton = GameState::GameStateRequests::DoesStackContainGameStateOfType<GameStatePrimaryUserSelection>();
        UiElementBus::Event(backButtonElementId, &UiElementInterface::SetIsEnabled, enableBackButton);
        if (enableBackButton)
        {
            UiButtonBus::Event(backButtonElementId, &UiButtonInterface::SetOnClickCallback, OnBackButtonClicked);
        }
        else
        {
            // Use the wide version of the 'Options' button.
            UiElementBus::Event(optionsButtonElementId, &UiElementInterface::SetIsEnabled, false);
            UiCanvasBus::EventResult(optionsButtonElementId,
                                     m_mainMenuCanvasEntityId,
                                     &UiCanvasInterface::FindElementEntityIdByName,
                                     "OptionsButtonWide");
            UiElementBus::Event(optionsButtonElementId, &UiElementInterface::SetIsEnabled, true);
            UiButtonBus::Event(optionsButtonElementId, &UiButtonInterface::SetOnClickCallback, OnOptionsButtonClicked);
        }

        // RefreshLevelListDisplay() should be called directly here (or right after it from OnEnter),
        // but at this point in our messed up startup sequence the level system doesn't exist yet.
        m_shouldRefreshLevelListDisplay = true;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline void GameStateMainMenu::UnloadMainMenuCanvas()
    {
        m_shouldRefreshLevelListDisplay = false;
        if (m_mainMenuCanvasEntityId.IsValid())
        {
            // Hide the UI cursor
            UiCursorBus::Broadcast(&UiCursorInterface::DecrementVisibleCounter);

            // Unload the main menu
            UiCanvasManagerBus::Broadcast(&UiCanvasManagerInterface::UnloadCanvas,
                                          m_mainMenuCanvasEntityId);
            m_mainMenuCanvasEntityId.SetInvalid();
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline const char* GameStateMainMenu::GetMainMenuCanvasAssetPath()
    {
        return "@products@/ui/canvases/defaultmainmenuscreen.uicanvas";
    }


    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline void GameStateMainMenu::RefreshLevelListDisplay()
    {
        // Get the dynamic layout UI element
        AZ::EntityId dynamicLayoutElementId;
        UiCanvasBus::EventResult(dynamicLayoutElementId,
                                 m_mainMenuCanvasEntityId,
                                 &UiCanvasInterface::FindElementEntityIdByName,
                                 "DynamicColumn");
        if (dynamicLayoutElementId.IsValid())
        {
            ISystem* iSystem = GetISystem();
            ILevelSystem* levelSystem = iSystem ? iSystem->GetILevelSystem() : nullptr;

            // Run through all the assets in the asset catalog and gather up the list of level assets
            AZ::Data::AssetType levelAssetType = levelSystem->GetLevelAssetType();
            AZStd::vector<AZStd::string> levelNames;
            auto enumerateCB = [levelAssetType, &levelNames](
                [[maybe_unused]] const AZ::Data::AssetId id,
                const AZ::Data::AssetInfo& assetInfo)
            {
                if (assetInfo.m_assetType == levelAssetType)
                {
                    levelNames.emplace_back(assetInfo.m_relativePath);
                }
            };

            AZ::Data::AssetCatalogRequestBus::Broadcast(
                &AZ::Data::AssetCatalogRequestBus::Events::EnumerateAssets, nullptr, enumerateCB, nullptr);

            // Add all the levels into the UI as buttons
            UiDynamicLayoutBus::Event(dynamicLayoutElementId, &UiDynamicLayoutInterface::SetNumChildElements, static_cast<int>(levelNames.size()));
            for (int i = 0; i < levelNames.size(); ++i)
            {
                AZ::IO::PathView level(levelNames[i].c_str());

                // Get the button element id
                AZ::EntityId buttonElementId;
                UiElementBus::EventResult(buttonElementId, dynamicLayoutElementId, &UiElementInterface::GetChildEntityId, i);

                // Get the text element id
                AZ::EntityId textElementId;
                UiElementBus::EventResult(textElementId, buttonElementId, &UiElementInterface::FindChildEntityIdByName, "Text");

                // Set the name, on-click callback, and on-click action name for each button
                UiTextBus::Event(textElementId, &UiTextInterface::SetText, level.Filename().Native());
                UiButtonBus::Event(buttonElementId, &UiButtonInterface::SetOnClickCallback, OnLevelButtonClicked);
                UiButtonBus::Event(buttonElementId, &UiButtonInterface::SetOnClickActionName, levelNames[i]);

                if (i == 0)
                {
                    // Force the first level to be selected
                    UiCanvasBus::Event(m_mainMenuCanvasEntityId, &UiCanvasInterface::ForceHoverInteractable, buttonElementId);
                }
            }
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline void GameStateMainMenu::LoadGameOptionsFromPersistentStorage()
    {
        SaveData::SaveDataRequests::SaveOrLoadObjectParams<GameOptions> loadObjectParams;
        GameOptionRequestBus::BroadcastResult(loadObjectParams.serializableObject,
                                              &GameOptionRequests::GetGameOptions);
        loadObjectParams.dataBufferName = GameOptions::SaveDataBufferName;
        loadObjectParams.localUserId = LocalUser::LocalUserRequests::GetPrimaryLocalUserId();
        loadObjectParams.callback = [](const SaveData::SaveDataRequests::SaveOrLoadObjectParams<GameOptions>& params,
                                       [[maybe_unused]] SaveData::SaveDataNotifications::Result result)
        {
            params.serializableObject->OnLoadedFromPersistentData();
        };
        SaveData::SaveDataRequests::LoadObject(loadObjectParams);
    }
}
