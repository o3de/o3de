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

#include <GameState/GameStateRequestBus.h>
#include <GameStateSamples/GameStatePrimaryControllerDisconnected.h>
#include <GameStateSamples/GameStateSamples_Traits_Platform.h>

#include <MessagePopup/MessagePopupBus.h>

#include <LocalUser/LocalUserRequestBus.h>

#include <AzFramework/Input/Utils/IsAnyKeyOrButton.h>

#include <ILocalizationManager.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace GameStateSamples
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline void GameStatePrimaryControllerDisconnected::OnEnter()
    {
        ShowPrimaryControllerDisconnectedPopup();
        AzFramework::InputChannelEventListener::Connect();
        AzFramework::InputDeviceNotificationBus::Handler::BusConnect();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline void GameStatePrimaryControllerDisconnected::OnExit()
    {
        AzFramework::InputDeviceNotificationBus::Handler::BusDisconnect();
        AzFramework::InputChannelEventListener::Disconnect();
        HidePrimaryControllerDisconnectedPopup();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline AZ::s32 GameStatePrimaryControllerDisconnected::GetPriority() const
    {
        // Re-connecting the primary user's controller takes precedence over everything else
        return AzFramework::InputChannelEventListener::GetPriorityFirst();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline bool GameStatePrimaryControllerDisconnected::OnInputChannelEventFiltered(const AzFramework::InputChannel & inputChannel)
    {
        if (inputChannel.IsStateEnded() &&
            AzFramework::IsAnyKeyOrButton(inputChannel))
        {
            const AzFramework::LocalUserId primaryLocalUserId = LocalUser::LocalUserRequests::GetPrimaryLocalUserId();
            if (primaryLocalUserId == inputChannel.GetInputDevice().GetAssignedLocalUserId())
            {
                AZ_Assert(GameState::GameStateRequests::IsActiveGameStateOfType<GameStatePrimaryControllerDisconnected>(),
                          "The active game state is not an instance of GameStatePrimaryControllerDisconnected");
                GameState::GameStateRequestBus::Broadcast(&GameState::GameStateRequests::PopActiveGameState);
            }
        }

        return true; // Consume all input while this game state is active
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline void GameStatePrimaryControllerDisconnected::OnInputDeviceConnectedEvent(const AzFramework::InputDevice& inputDevice)
    {
        const AzFramework::LocalUserId primaryLocalUserId = LocalUser::LocalUserRequests::GetPrimaryLocalUserId();
        if (primaryLocalUserId == inputDevice.GetAssignedLocalUserId())
        {
            AZ_Assert(GameState::GameStateRequests::IsActiveGameStateOfType<GameStatePrimaryControllerDisconnected>(),
                      "The active game state is not an instance of GameStatePrimaryControllerDisconnected");
            GameState::GameStateRequestBus::Broadcast(&GameState::GameStateRequests::PopActiveGameState);
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline void GameStatePrimaryControllerDisconnected::ShowPrimaryControllerDisconnectedPopup()
    {
        if (m_primaryControllerDisconnectedPopupId != 0)
        {
            // We're already displaying the message popup
            return;
        }

        string localizedMessage;
        const char* localizationKey = AZ_TRAIT_GAMESTATESAMPLES_PRIMARY_CONTROLLER_DISCONNECTED_LOC_KEY;
        bool wasLocalized = false;
        LocalizationManagerRequestBus::BroadcastResult(wasLocalized,
                                                       &LocalizationManagerRequestBus::Events::LocalizeString_ch,
                                                       localizationKey,
                                                       localizedMessage,
                                                       false);
        const char* popupMessage = wasLocalized && localizedMessage != localizationKey ?
            localizedMessage.c_str() :
            AZ_TRAIT_GAMESTATESAMPLES_PRIMARY_CONTROLLER_DISCONNECTED_DEFAULT_MESSAGE;

        MessagePopup::MessagePopupRequestBus::BroadcastResult(m_primaryControllerDisconnectedPopupId,
                                                              &MessagePopup::MessagePopupRequests::ShowPopup,
                                                              popupMessage,
                                                              MessagePopup::EPopupButtons_NoButtons);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline void GameStatePrimaryControllerDisconnected::HidePrimaryControllerDisconnectedPopup()
    {
        if (m_primaryControllerDisconnectedPopupId != 0)
        {
            MessagePopup::MessagePopupRequestBus::Broadcast(&MessagePopup::MessagePopupRequests::HidePopup,
                                                            m_primaryControllerDisconnectedPopupId, 0);
            m_primaryControllerDisconnectedPopupId = 0;
        }
    }
}
