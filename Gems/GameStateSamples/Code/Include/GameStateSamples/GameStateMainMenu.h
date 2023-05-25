/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <GameState/GameState.h>
#include <GameStateSamples/GameStateLocalUserLobby.h>

#include <AzCore/std/smart_ptr/unique_ptr.h>

#include <ISystem.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace GameStateSamples
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Game state that is active while displaying the main game menu (or another front-end menu).
    class GameStateMainMenu : public GameState::IGameState
                            , public ISystemEventListener
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        // Allocator
        AZ_CLASS_ALLOCATOR(GameStateMainMenu, AZ::SystemAllocator);

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Type Info
        AZ_RTTI(GameStateMainMenu, "{53EB59EC-77F1-4C8E-AC5F-B2A94F15AF31}", IGameState);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Default constructor
        GameStateMainMenu() = default;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Default destructor
        ~GameStateMainMenu() override = default;

    protected:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref GameState::GameState::OnPushed
        void OnPushed() override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref GameState::GameState::OnPopped
        void OnPopped() override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref GameState::GameState::OnEnter
        void OnEnter() override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref GameState::GameState::OnExit
        void OnExit() override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref GameState::GameState::OnUpdate
        void OnUpdate() override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref ISystemEventListener::OnSystemEvent
        void OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam) override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Convenience functions to load and unload the main game menu UI canvas.
        ///@{
        virtual void LoadMainMenuCanvas();
        virtual void UnloadMainMenuCanvas();
        virtual const char* GetMainMenuCanvasAssetPath();
        ///@}

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Refresh the list of levels displayed in the main menu.
        virtual void RefreshLevelListDisplay();

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Load options from persistent storage, which is done upon first entry into the main menu.
        virtual void LoadGameOptionsFromPersistentStorage();

    protected:
        ////////////////////////////////////////////////////////////////////////////////////////////
        // Variables
        AZStd::unique_ptr<GameStateLocalUserLobby> m_localUserLobbySubState;
        AZ::EntityId                               m_mainMenuCanvasEntityId;
        bool                                       m_shouldRefreshLevelListDisplay = false;
    };
} // namespace GameStateSamples

// Include the implementation inline so the class can be instantiated outside of the gem.
#include <GameStateSamples/GameStateMainMenu.inl>
