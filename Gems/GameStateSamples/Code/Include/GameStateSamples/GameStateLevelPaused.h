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

#include <AzFramework/Input/Events/InputChannelEventListener.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace GameStateSamples
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Game state that is active while gameplay is paused.
    class GameStateLevelPaused : public GameState::IGameState
                               , public AzFramework::InputChannelEventListener
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        // Allocator
        AZ_CLASS_ALLOCATOR(GameStateLevelPaused, AZ::SystemAllocator, 0);

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Type Info
        AZ_RTTI(GameStateLevelPaused, "{6CAA4810-AA67-4A96-BB23-3EFA4BCCBF12}", IGameState);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Default constructor
        GameStateLevelPaused() = default;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Default destructor
        ~GameStateLevelPaused() override = default;

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
        //! \ref AzFramework::InputChannelEventListener::GetPriority
        AZ::s32 GetPriority() const override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputChannelEventListener::OnInputChannelEventFiltered
        bool OnInputChannelEventFiltered(const AzFramework::InputChannel & inputChannel) override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Convenience functions to load and unload the pause menu UI canvas.
        ///@{
        virtual void LoadPauseMenuCanvas();
        virtual void UnloadPauseMenuCanvas();
        virtual void SetPauseMenuCanvasDrawOrder();
        virtual const char* GetPauseMenuCanvasAssetPath();
        ///@}

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Convenience functions to pause and unpause the game.
        ///@{
        virtual void PauseGame();
        virtual void UnpauseGame();
        ///@}

    protected:
        ////////////////////////////////////////////////////////////////////////////////////////////
        // Variables
        AZ::EntityId m_pauseMenuCanvasEntityId; //!< Id of the UI canvas being displayed
    };
} // namespace GameStateSamples

// Include the implementation inline so the class can be instantiated outside of the gem.
#include <GameStateSamples/GameStateLevelPaused.inl>
