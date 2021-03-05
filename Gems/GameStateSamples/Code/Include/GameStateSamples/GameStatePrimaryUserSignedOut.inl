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
#include <GameStateSamples/GameStatePrimaryUserSignedOut.h>
#include <GameStateSamples/GameStatePrimaryUserSelection.h>

#include <MessagePopup/MessagePopupBus.h>

#include <LocalUser/LocalUserRequestBus.h>

#include <AzFramework/Input/Utils/IsAnyKeyOrButton.h>

#include <ILocalizationManager.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace GameStateSamples
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline void GameStatePrimaryUserSignedOut::OnEnter()
    {
        ShowPrimaryUserSignedOutPopup();
        AzFramework::InputChannelEventListener::Connect();
        LocalUser::LocalUserNotificationBus::Handler::BusConnect();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline void GameStatePrimaryUserSignedOut::OnExit()
    {
        LocalUser::LocalUserNotificationBus::Handler::BusDisconnect();
        AzFramework::InputChannelEventListener::Disconnect();
        HidePrimaryUserSignedOutPopup();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline AZ::s32 GameStatePrimaryUserSignedOut::GetPriority() const
    {
        // Re-establishing a primary user takes precedence over everything else
        return AzFramework::InputChannelEventListener::GetPriorityFirst();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline bool GameStatePrimaryUserSignedOut::OnInputChannelEventFiltered(const AzFramework::InputChannel & inputChannel)
    {
        if (inputChannel.IsStateEnded() &&
            AzFramework::IsAnyKeyOrButton(inputChannel))
        {
            const AzFramework::LocalUserId assignedLocalUserId = inputChannel.GetInputDevice().GetAssignedLocalUserId();
            const AzFramework::LocalUserId primaryLocalUserId = LocalUser::LocalUserRequests::GetPrimaryLocalUserId();
            if (assignedLocalUserId == primaryLocalUserId)
            {
                // We received input from the primary user, so just pop this state
                AZ_Assert(GameState::GameStateRequests::IsActiveGameStateOfType<GameStatePrimaryUserSignedOut>(),
                          "The active game state is not an instance of GameStatePrimaryUserSignedOut");
                GameState::GameStateRequestBus::Broadcast(&GameState::GameStateRequests::PopActiveGameState);
            }
            else if (assignedLocalUserId == AzFramework::LocalUserIdAny ||
                     assignedLocalUserId == AzFramework::LocalUserIdNone)
            {
                // We received input from a device that is not associated with a user, so prompt for user sign-in
                inputChannel.GetInputDevice().PromptLocalUserSignIn();
            }
            else
            {
                // We received input from a different user confirming we want to select a new one
                AZ_Assert(GameState::GameStateRequests::IsActiveGameStateOfType<GameStatePrimaryUserSignedOut>(),
                          "The active game state is not an instance of GameStatePrimaryUserSignedOut");
                GameState::GameStateRequests::PopActiveGameStateUntilOfType<GameStatePrimaryUserSelection>();
            }
        }

        return true; // Consume all input while this game state is active
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline void GameStatePrimaryUserSignedOut::OnLocalUserSignedIn(AzFramework::LocalUserId localUserId)
    {
        const AzFramework::LocalUserId primaryLocalUserId = LocalUser::LocalUserRequests::GetPrimaryLocalUserId();
        if (primaryLocalUserId == localUserId)
        {
            // The primary user signed back in
            AZ_Assert(GameState::GameStateRequests::IsActiveGameStateOfType<GameStatePrimaryUserSignedOut>(),
                        "The active game state is not an instance of GameStatePrimaryUserSignedOut");
            GameState::GameStateRequestBus::Broadcast(&GameState::GameStateRequests::PopActiveGameState);
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline void GameStatePrimaryUserSignedOut::ShowPrimaryUserSignedOutPopup()
    {
        if (m_primaryUserSignedOutPopupId != 0)
        {
            // We're already displaying the message popup
            return;
        }

        string localizedMessage;
        const char* localizationKey = "@PRIMARY_CONTROLLER_DISCONNECTED_LOC_KEY";
        bool wasLocalized = false;
        LocalizationManagerRequestBus::BroadcastResult(wasLocalized,
                                                       &LocalizationManagerRequestBus::Events::LocalizeString_ch,
                                                       localizationKey,
                                                       localizedMessage,
                                                       false);
        const char* popupMessage = wasLocalized && localizedMessage != localizationKey ?
                                   localizedMessage.c_str() :
                                   "Primary profile signed out.\n\nEither sign in again with the same profile, or press any button while signed into a different profile to return to the main menu.\n\n(any unsaved progress will be lost)";
        MessagePopup::MessagePopupRequestBus::BroadcastResult(m_primaryUserSignedOutPopupId,
                                                              &MessagePopup::MessagePopupRequests::ShowPopup,
                                                              popupMessage,
                                                              MessagePopup::EPopupButtons_NoButtons);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline void GameStatePrimaryUserSignedOut::HidePrimaryUserSignedOutPopup()
    {
        if (m_primaryUserSignedOutPopupId != 0)
        {
            MessagePopup::MessagePopupRequestBus::Broadcast(&MessagePopup::MessagePopupRequests::HidePopup,
                                                            m_primaryUserSignedOutPopupId, 0);
            m_primaryUserSignedOutPopupId = 0;
        }
    }
}
