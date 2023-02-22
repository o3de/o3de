/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <GameState/GameState.h>

#include <AzFramework/Input/Events/InputChannelEventListener.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace GameStateSamples
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Game state that is active while waiting to determine who the primary user is.
    class GameStatePrimaryUserSelection : public GameState::IGameState
                                        , public AzFramework::InputChannelEventListener
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        // Allocator
        AZ_CLASS_ALLOCATOR(GameStatePrimaryUserSelection, AZ::SystemAllocator);

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Type Info
        AZ_RTTI(GameStatePrimaryUserSelection, "{953A3CBD-92BD-4B9A-9FD2-C1DC6E9A8BF8}", IGameState);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Default constructor
        GameStatePrimaryUserSelection() = default;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Default destructor
        ~GameStatePrimaryUserSelection() override = default;

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
        //! Used to detect the press of a button / key that will identify the primary user. If you
        //! override this class and wish to detect the primary user through another means, or want
        //! the UI displayed to process input, you should override this function to do nothing and
        //! handle setting the primary user yourself, before transitioning to the next game state.
        bool OnInputChannelEventFiltered(const AzFramework::InputChannel & inputChannel) override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Convenience function to set the primary local user and transition to the next game state
        //! \param[in] localUserId The local user id to set as the primary user
        virtual void SetPrimaryLocalUser(AzFramework::LocalUserId localUserId);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Convenience function to push the default primary user monitor game state.
        //! Override if you wish to push a different primary user monitor game state.
        virtual void PushPrimaryUserMonitorGameState();

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Convenience function to push the default main menu game state.
        //! Override if you wish to push a different main menu game state.
        virtual void PushMainMenuGameState();

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Convenience functions to load and unload the primary user selection UI canvas.
        ///@{
        virtual void LoadPrimaryUserSelectionCanvas();
        virtual void UnloadPrimaryUserSelectionCanvas();
        virtual const char* GetPrimaryUserSelectionCanvasAssetPath();

    protected:
        ////////////////////////////////////////////////////////////////////////////////////////////
        // Variables
        AZ::EntityId m_primaryUserSelectionCanvasEntityId; //!< Id of the UI canvas being displayed
    };
} // namespace GameStateSamples

// Include the implementation inline so the class can be instantiated outside of the gem.
#include <GameStateSamples/GameStatePrimaryUserSelection.inl>
