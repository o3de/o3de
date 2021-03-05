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
#pragma once

#include <GameState/GameState.h>

#include <GameStateSamples/GameOptionRequestBus.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace GameStateSamples
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Game state that is active while displaying the game's options menu.
    class GameStateOptionsMenu : public GameState::IGameState
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        // Allocator
        AZ_CLASS_ALLOCATOR(GameStateOptionsMenu, AZ::SystemAllocator, 0);

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Type Info
        AZ_RTTI(GameStateOptionsMenu, "{2441BA71-8AD2-47A1-92BB-478ED74ACE63}", IGameState);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Default constructor
        GameStateOptionsMenu() = default;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Default destructor
        ~GameStateOptionsMenu() override = default;

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
        //! Convenience functions to load, unload, and refresh the options menu UI canvas.
        ///@{
        virtual void LoadOptionsMenuCanvas();
        virtual void UnloadOptionsMenuCanvas();
        virtual void RefreshOptionsMenuCanvas();
        virtual void SetOptionsMenuCanvasDrawOrder();
        virtual const char* GetOptionsMenuCanvasAssetPath();
        ///@}

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Save game options to persistent storage, which is done upon exiting the options menu.
        virtual void SaveGameOptionsToPersistentStorage();

    protected:
        ////////////////////////////////////////////////////////////////////////////////////////////
        // Variables
        AZStd::shared_ptr<GameOptions> m_gameOptions;   //!< The game options object
        AZ::EntityId m_optionsMenuCanvasEntityId;       //!< Id of the UI canvas being displayed
    };
} // namespace GameStateSamples

// Include the implementation inline so the class can be instantiated outside of the gem.
#include <GameStateSamples/GameStateOptionsMenu.inl>
