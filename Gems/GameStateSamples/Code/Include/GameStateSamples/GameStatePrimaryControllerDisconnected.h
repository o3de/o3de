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
#include <AzFramework/Input/Buses/Notifications/InputDeviceNotificationBus.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace GameStateSamples
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Game state that is active while the primary user's controller is disconnected.
    class GameStatePrimaryControllerDisconnected : public GameState::IGameState
                                                 , public AzFramework::InputChannelEventListener
                                                 , public AzFramework::InputDeviceNotificationBus::Handler
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        // Allocator
        AZ_CLASS_ALLOCATOR(GameStatePrimaryControllerDisconnected, AZ::SystemAllocator);

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Type Info
        AZ_RTTI(GameStatePrimaryControllerDisconnected, "{47FCBC7A-49CB-4FEB-842A-C730CCB19940}", IGameState);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Default constructor
        GameStatePrimaryControllerDisconnected() = default;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Default destructor
        ~GameStatePrimaryControllerDisconnected() override = default;

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
        //! \ref AzFramework::InputDeviceNotifications::OnInputDeviceConnectedEvent
        void OnInputDeviceConnectedEvent(const AzFramework::InputDevice& inputDevice) override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Convenience functions to show/hide the primary controller disconnected popup.
        ///@{
        virtual void ShowPrimaryControllerDisconnectedPopup();
        virtual void HidePrimaryControllerDisconnectedPopup();
        ///@}

    private:
        ////////////////////////////////////////////////////////////////////////////////////////////
        // Variables
        AZ::u32 m_primaryControllerDisconnectedPopupId = 0; //!< Id of the popup being displayed
    };
} // namespace GameStateSamples

// Include the implementation inline so the class can be instantiated outside of the gem.
#include <GameStateSamples/GameStatePrimaryControllerDisconnected.inl>
