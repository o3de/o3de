/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <GameState/GameState.h>

#include <AzFramework/API/ApplicationAPI.h>
#include <AzFramework/Input/Events/InputChannelEventListener.h>

#include <ISystem.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace GameStateSamples
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Game state that is active while the game is running.
    class GameStateLevelRunning : public GameState::IGameState
                                , public AzFramework::InputChannelEventListener
                                , public AzFramework::ApplicationLifecycleEvents::Bus::Handler
                                , public ISystemEventListener
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        // Allocator
        AZ_CLASS_ALLOCATOR(GameStateLevelRunning, AZ::SystemAllocator);

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Type Info
        AZ_RTTI(GameStateLevelRunning, "{93501205-D39D-4E91-B93C-1E16EFAEEB43}", IGameState);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Default constructor
        GameStateLevelRunning() = default;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Default destructor
        ~GameStateLevelRunning() override = default;

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
        //! \ref AzFramework::ApplicationLifecycleEvents::OnApplicationConstrained
        void OnApplicationConstrained(AzFramework::ApplicationLifecycleEvents::Event /*lastEvent*/) override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref ISystemEventListener::OnSystemEvent
        void OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam) override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Convenience function to push the default level paused game state.
        //! Override if you wish to push a different level paused game state.
        virtual void PushLevelPausedGameState();

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Convenience functions to load and unload the pause button UI canvas.
        ///@{
        virtual void LoadPauseButtonCanvas();
        virtual void UnloadPauseButtonCanvas();
        virtual const char* GetPauseButtonCanvasAssetPath();
        ///@}

    protected:
        ////////////////////////////////////////////////////////////////////////////////////////////
        // Variables
        AZ::EntityId m_pauseButtonCanvasEntityId; //!< Id of the UI canvas being displayed
    };
} // namespace GameStateSamples

// Include the implementation inline so the class can be instantiated outside of the gem.
#include <GameStateSamples/GameStateLevelRunning.inl>
