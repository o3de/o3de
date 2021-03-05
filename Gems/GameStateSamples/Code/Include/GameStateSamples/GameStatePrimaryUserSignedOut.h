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

#include <LocalUser/LocalUserNotificationBus.h>

#include <AzFramework/Input/Events/InputChannelEventListener.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace GameStateSamples
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Game state that is active while the primary user is signed out.
    class GameStatePrimaryUserSignedOut : public GameState::IGameState
                                        , public AzFramework::InputChannelEventListener
                                        , public LocalUser::LocalUserNotificationBus::Handler
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        // Allocator
        AZ_CLASS_ALLOCATOR(GameStatePrimaryUserSignedOut, AZ::SystemAllocator, 0);

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Type Info
        AZ_RTTI(GameStatePrimaryUserSignedOut, "{5750DA57-349F-4401-B133-977C68ED70A3}", IGameState);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Default constructor
        GameStatePrimaryUserSignedOut() = default;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Default destructor
        ~GameStatePrimaryUserSignedOut() override = default;

    protected:
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
        //! \ref LocalUser::LocalUserNotifications::OnLocalUserSignedIn
        void OnLocalUserSignedIn(AzFramework::LocalUserId localUserId) override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Convenience functions to show/hide the primary user signed out popup.
        ///@{
        virtual void ShowPrimaryUserSignedOutPopup();
        virtual void HidePrimaryUserSignedOutPopup();
        ///@}

    private:
        ////////////////////////////////////////////////////////////////////////////////////////////
        // Variables
        AZ::u32 m_primaryUserSignedOutPopupId = 0; //!< Id of the popup being displayed
    };
} // namespace GameStateSamples

// Include the implementation inline so the class can be instantiated outside of the gem.
#include <GameStateSamples/GameStatePrimaryUserSignedOut.inl>
