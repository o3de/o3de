/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <GameState/GameStateRequestBus.h>
#include <GameStateSamples/GameStateLevelLoading.h>
#include <GameStateSamples/GameStateLevelRunning.h>

#include <IConsole.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace GameStateSamples
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline void GameStateLevelLoading::OnEnter()
    {
        ISystem* iSystem = GetISystem();
        if (iSystem)
        {
            iSystem->GetISystemEventDispatcher()->RegisterListener(this);
        }

        IConsole* iConsole = iSystem ? iSystem->GetIConsole() : nullptr;
        if (iConsole)
        {
            iConsole->GetCVar("level_load_screen_uicanvas_path")->Set("@products@/ui/canvases/defaultlevelloadingscreen.uicanvas");
            iConsole->GetCVar("level_load_screen_sequence_to_auto_play")->Set("DefaultLevelLoadingAnimatedSequence");
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline void GameStateLevelLoading::OnExit()
    {
        if (ISystem* iSystem = GetISystem())
        {
            iSystem->GetISystemEventDispatcher()->RemoveListener(this);
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline void GameStateLevelLoading::OnSystemEvent(ESystemEvent event, UINT_PTR, UINT_PTR)
    {
        if (event == ESYSTEM_EVENT_LEVEL_LOAD_END)
        {
            // Replace the level loading game state with the level running game state
            AZ_Assert(GameState::GameStateRequests::IsActiveGameStateOfType<GameStateLevelLoading>(),
                      "The active game state is not of type GameStateLevelLoading");
            AZ_Assert(!GameState::GameStateRequests::DoesStackContainGameStateOfType<GameStateLevelRunning>(),
                      "The game state stack already contains an instance of GameStateLevelRunning");
            AZStd::shared_ptr<GameState::IGameState> gameStateLevelRunning = GameState::GameStateRequests::CreateNewOverridableGameStateOfType<GameStateLevelRunning>();
            GameState::GameStateRequestBus::Broadcast(&GameState::GameStateRequests::ReplaceActiveGameState, gameStateLevelRunning);
        }
    }
}
