/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <GameState/GameStateRequestBus.h>
#include <GameStateSamples/GameStateLocalUserLobby.h>

#include <LyShine/Bus/UiCanvasBus.h>
#include <LyShine/Bus/UiCanvasManagerBus.h>
#include <LyShine/Bus/UiElementBus.h>
#include <LyShine/Bus/UiTextBus.h>

#include <LocalUser/LocalUserRequestBus.h>

#include <AzFramework/Input/Buses/Requests/InputDeviceRequestBus.h>
#include <AzFramework/Input/Buses/Requests/InputLightBarRequestBus.h>
#include <AzFramework/Input/Devices/Gamepad/InputDeviceGamepad.h>
#include <AzFramework/Input/Utils/IsAnyKeyOrButton.h>

#include <AzCore/std/sort.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace GameStateSamples
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline void GameStateLocalUserLobby::OnPushed()
    {
        // We could load the UI canvas here and keep it cached until OnPopped is called in order to
        // speed up re-entering this game state, but doing so would consume memory for the lifetime
        // of the process that is only needed while this state is active (which is not very often).
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline void GameStateLocalUserLobby::OnPopped()
    {
        // See comment above in OnPushed
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline void GameStateLocalUserLobby::OnEnter()
    {
        AzFramework::ApplicationLifecycleEvents::Bus::Handler::BusConnect();
        AzFramework::InputDeviceNotificationBus::Handler::BusConnect();
        LocalUser::LocalUserNotificationBus::Handler::BusConnect();
        InputChannelEventListener::Connect();
        RefreshLocalPlayerSlotAssignments();
        LoadSignedInUserOverlayCanvas();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline void GameStateLocalUserLobby::OnExit()
    {
        UnloadSignedInUserOverlayCanvas();
        InputChannelEventListener::Disconnect();
        LocalUser::LocalUserNotificationBus::Handler::BusDisconnect();
        AzFramework::InputDeviceNotificationBus::Handler::BusDisconnect();
        AzFramework::ApplicationLifecycleEvents::Bus::Handler::BusDisconnect();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline void GameStateLocalUserLobby::OnUpdate()
    {
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline AZ::s32 GameStateLocalUserLobby::GetPriority() const
    {
        return AzFramework::InputChannelEventListener::GetPriorityFirst();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline bool GameStateLocalUserLobby::OnInputChannelEventFiltered(const AzFramework::InputChannel & inputChannel)
    {
        if (inputChannel.IsStateBegan() && AzFramework::IsAnyKeyOrButton(inputChannel))
        {
            const AzFramework::LocalUserId localUserId = inputChannel.GetInputDevice().GetAssignedLocalUserId();
            if (localUserId == AzFramework::LocalUserIdAny ||
                localUserId == AzFramework::LocalUserIdNone)
            {
                // No local user is associated with this input device yet, so prompt for user sign-in
                inputChannel.GetInputDevice().PromptLocalUserSignIn();
            }
            else
            {
                // Assign the local user to the first available slot (if they haven't already been assigned one)
                AZ::u32 assignedSlot = LocalUser::LocalPlayerSlotNone;
                LocalUser::LocalUserRequestBus::BroadcastResult(assignedSlot,
                                                                &LocalUser::LocalUserRequests::GetLocalPlayerSlotOccupiedByLocalUserId,
                                                                localUserId);
                if (assignedSlot == LocalUser::LocalPlayerSlotNone)
                {
                    // This call to AssignLocalUserIdToLocalPlayerSlot will trigger another to
                    // OnLocalUserIdAssignedToLocalPlayerSlot, which is where we update the UI.
                    LocalUser::LocalUserRequestBus::Broadcast(&LocalUser::LocalUserRequests::AssignLocalUserIdToLocalPlayerSlot,
                                                              localUserId,
                                                              LocalUser::LocalPlayerSlotAny);
                }
            }
        }

        // Don't consume the input
        return false;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline void GameStateLocalUserLobby::OnInputDeviceConnectedEvent(const AzFramework::InputDevice& inputDevice)
    {
        const AzFramework::LocalUserId localUserId = inputDevice.GetAssignedLocalUserId();
        const AzFramework::LocalUserId primaryLocalUserId = LocalUser::LocalUserRequests::GetPrimaryLocalUserId();
        if (localUserId == AzFramework::LocalUserIdNone ||
            localUserId == primaryLocalUserId)
        {
            // The connected controller does not belong to any user, or it belongs to the primary
            // user which is handled in GameStatePrimaryControllerDisconnected
            return;
        }

        // A secondary controller was connected, so assign the associated local
        // user id to a free local player slot. Note that we only do this while
        // in GameStateLocalUserLobby, not during gameplay, as we do not want to
        // assign a local user id to a local player slot mid-game. To account
        // for the case where a controller/user connects during gameplay (and
        // does not disconnect by the time we return to the main menu) we call
        // RefreshLocalPlayerSlotAssignments from GameStateLocalUserLobby::OnEnter.
        //
        // This call to AssignLocalUserIdToLocalPlayerSlot will trigger another to
        // OnLocalUserIdAssignedToLocalPlayerSlot, which is where we update the UI.
        LocalUser::LocalUserRequestBus::Broadcast(&LocalUser::LocalUserRequests::AssignLocalUserIdToLocalPlayerSlot,
                                                  localUserId,
                                                  LocalUser::LocalPlayerSlotAny);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline void GameStateLocalUserLobby::OnInputDeviceDisconnectedEvent(const AzFramework::InputDevice& inputDevice)
    {
        const AzFramework::LocalUserId localUserId = inputDevice.GetAssignedLocalUserId();
        const AzFramework::LocalUserId primaryLocalUserId = LocalUser::LocalUserRequests::GetPrimaryLocalUserId();
        if (localUserId == AzFramework::LocalUserIdNone ||
            localUserId == primaryLocalUserId)
        {
            // The disconnected controller does not belong to any user, or it belongs to the primary
            // user which is handled in GameStatePrimaryUserMonitor::OnInputDeviceDisconnectedEvent
            return;
        }

        // A secondary controller was disconnected, so remove the associated local
        // user id from their local player slot. Note that we only do this while in
        // GameStateLocalUserLobby, not during gameplay, as we don't want to remove a
        // local user id from a local player slot mid-game. To account for the case
        // where a user disconnects during gameplay (and does not re-connect by the
        // time we exit to the main menu) we call RefreshLocalPlayerSlotAssignments
        // from GameStateLocalUserLobby::OnEnter
        //
        // This call to RemoveLocalUserIdFromLocalPlayerSlot will trigger another to
        // OnLocalUserIdRemovedFromLocalPlayerSlot, which is where we update the UI.
        LocalUser::LocalUserRequestBus::Broadcast(&LocalUser::LocalUserRequests::RemoveLocalUserIdFromLocalPlayerSlot,
                                                  localUserId);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline void GameStateLocalUserLobby::OnApplicationUnconstrained(AzFramework::ApplicationLifecycleEvents::Event /*lastEvent*/)
    {
        RefreshLocalPlayerSlotAssignments();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline void GameStateLocalUserLobby::OnLocalUserIdAssignedToLocalPlayerSlot(AzFramework::LocalUserId localUserId,
                                                                                AZ::u32 newLocalPlayerSlot,
                                                                                AZ::u32 previousLocalPlayerSlot)
    {
        RefreshSignedInUserOverlay(newLocalPlayerSlot);
        RefreshSignedInUserOverlay(previousLocalPlayerSlot);
        RefreshGamepadLightBarColor(localUserId, newLocalPlayerSlot);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline void GameStateLocalUserLobby::OnLocalUserIdRemovedFromLocalPlayerSlot(AzFramework::LocalUserId localUserId,
                                                                                 AZ::u32 localPlayerSlot)
    {
        RefreshSignedInUserOverlay(localPlayerSlot);
        RefreshGamepadLightBarColor(localUserId, LocalUser::LocalPlayerSlotNone);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline void GameStateLocalUserLobby::LoadSignedInUserOverlayCanvas()
    {
        // Load the UI canvas
        const char* uiCanvasAssetPath = GetSignedInUserOverlayCanvasAssetPath();
        UiCanvasManagerBus::BroadcastResult(m_signedInUsersOverlayCanvasEntityId,
                                            &UiCanvasManagerInterface::LoadCanvas,
                                            uiCanvasAssetPath);
        if (!m_signedInUsersOverlayCanvasEntityId.IsValid())
        {
            AZ_Warning("GameStateLocalUserLobby", false, "Could not load %s", uiCanvasAssetPath);
            return;
        }

        // Display the overlay, set it to draw over the top of the main menu,
        // and set it to stay loaded when a level unloads
        UiCanvasBus::Event(m_signedInUsersOverlayCanvasEntityId, &UiCanvasInterface::SetEnabled, true);
        UiCanvasBus::Event(m_signedInUsersOverlayCanvasEntityId, &UiCanvasInterface::SetDrawOrder, 10);
        UiCanvasBus::Event(m_signedInUsersOverlayCanvasEntityId, &UiCanvasInterface::SetKeepLoadedOnLevelUnload, true);

        RefreshAllSignedInUserOverlays();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline void GameStateLocalUserLobby::UnloadSignedInUserOverlayCanvas()
    {
        if (m_signedInUsersOverlayCanvasEntityId.IsValid())
        {
            // Unload the overlay
            UiCanvasManagerBus::Broadcast(&UiCanvasManagerInterface::UnloadCanvas,
                                          m_signedInUsersOverlayCanvasEntityId);
            m_signedInUsersOverlayCanvasEntityId.SetInvalid();
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline const char* GameStateLocalUserLobby::GetSignedInUserOverlayCanvasAssetPath()
    {
        return "@products@/ui/canvases/defaultsignedinusersoverlay.uicanvas";
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline void GameStateLocalUserLobby::RefreshLocalPlayerSlotAssignments()
    {
        // Check whether local users that have been assigned to a local player slot
        // are still signed in and associated with a connected input device. If not,
        // remove them from the local player slot so that it becomes free for other
        // players to join. Note that we ignore the primary user here, as that is a
        // speacial case we handle at all times in LyPlatformServicesSystemComponent
        for (AZ::u32 i = LocalUser::LocalPlayerSlotPrimary + 1; i < LocalUser::LocalPlayerSlotMax; ++i)
        {
            const AzFramework::LocalUserId localUserId = LocalUser::LocalUserRequests::GetLocalUserIdAt(i);
            if (localUserId == AzFramework::LocalUserIdNone)
            {
                // No local user assigned to this slot
                continue;
            }

            const bool isLocalUserSignedIn = IsLocalUserSignedIn(localUserId);
            if (!isLocalUserSignedIn)
            {
                // The local user is no longer signed in, remove them from their local player slot
                LocalUser::LocalUserRequestBus::Broadcast(&LocalUser::LocalUserRequests::RemoveLocalUserIdFromLocalPlayerSlot,
                                                          localUserId);
                continue;
            }

            const bool isAssignedToConnectedInputDevice = IsLocalUserAssociatedWithConnectedInputDevice(localUserId);
            if (!isAssignedToConnectedInputDevice)
            {
                // The local user is no longer associated with a connected input device, remove them from their local player slot
                LocalUser::LocalUserRequestBus::Broadcast(&LocalUser::LocalUserRequests::RemoveLocalUserIdFromLocalPlayerSlot,
                                                          localUserId);
                continue;
            }

            // The local user is still signed in and associated with a connected input device
        }

        // Get all connected controllers...
        AZStd::vector<const AzFramework::InputDevice*> gamepadInputDevices;
        AzFramework::InputDeviceRequests::InputDeviceByIdMap inputDevicesById;
        AzFramework::InputDeviceRequestBus::Broadcast(&AzFramework::InputDeviceRequests::GetInputDevicesById,
                                                      inputDevicesById);
        for (const auto& inputDeviceById : inputDevicesById)
        {
            const AzFramework::InputDevice* inputDevice = inputDeviceById.second;
            if (inputDevice &&
                inputDevice->IsConnected() &&
                AzFramework::InputDeviceGamepad::IsGamepadDevice(inputDeviceById.first))
            {
                // The input device is a connected gamepad
                gamepadInputDevices.push_back(inputDevice);
            }
        }

        // ...sort them by index and then go through to check whether they have been
        // assigned a local user id. If so, auto-assign their local user id into the
        // first available local player slot (unless they've already been assigned).
        AZStd::sort(gamepadInputDevices.begin(), gamepadInputDevices.end(),
            [](const AzFramework::InputDevice* lhs, const AzFramework::InputDevice* rhs)
        {
            return lhs->GetInputDeviceId() < rhs->GetInputDeviceId();
        });
        for (const AzFramework::InputDevice* gamepadDevice : gamepadInputDevices)
        {
            const AzFramework::LocalUserId localUserId = gamepadDevice->GetAssignedLocalUserId();
            if (localUserId == AzFramework::LocalUserIdAny ||
                localUserId == AzFramework::LocalUserIdNone ||
                !IsLocalUserSignedIn(localUserId))
            {
                // The input device has no associated local user id,
                // or is associated with a user that's not signed in.
                continue;
            }

            // Assign the local user id to a local player slot.
            // If it is already assigned this will do nothing.
            LocalUser::LocalUserRequestBus::Broadcast(&LocalUser::LocalUserRequests::AssignLocalUserIdToLocalPlayerSlot,
                                                      localUserId,
                                                      LocalUser::LocalPlayerSlotAny);
        }

        // Lastly, iterate over all the local player slots again and 'collapse' them
        // so that we aren't left with any gaps. For example, if after all the above
        // we are left with user A in local player slot 0 and user B in local player
        // slot 3, the following will result in user B being moved down into slot 1.
        // Note again that we ignore the primary user here, which is a speacial case
        // handled in LyPlatformServicesSystemComponent so that it'll never be empty.
        for (AZ::u32 i = LocalUser::LocalPlayerSlotPrimary + 1; i < LocalUser::LocalPlayerSlotMax; ++i)
        {
            AzFramework::LocalUserId localUserId = LocalUser::LocalUserRequests::GetLocalUserIdAt(i);
            if (localUserId == AzFramework::LocalUserIdNone)
            {
                // No local user assigned to this slot, look for the next occupied slot...
                AZ::u32 j = i + 1;
                for (; j < LocalUser::LocalPlayerSlotMax; ++j)
                {
                    localUserId = LocalUser::LocalUserRequests::GetLocalUserIdAt(j);
                    if (localUserId != AzFramework::LocalUserIdNone)
                    {
                        // ...and move that local user down into the unnocupied slot.
                        LocalUser::LocalUserRequestBus::Broadcast(&LocalUser::LocalUserRequests::AssignLocalUserIdToLocalPlayerSlot,
                                                                  localUserId,
                                                                  i);
                        break;
                    }
                }

                if (j == LocalUser::LocalPlayerSlotMax)
                {
                    // There are no more occupied slots, no need to continue.
                    break;
                }
            }
        }

        // After all this, refresh the gamepad light bar colors (if they exist)
        RefreshAllGamepadLightBarColors();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline bool GameStateLocalUserLobby::IsLocalUserSignedIn(AzFramework::LocalUserId localUserId) const
    {
        bool isLocalUserSignedIn = false;
        LocalUser::LocalUserRequestBus::BroadcastResult(isLocalUserSignedIn,
                                                        &LocalUser::LocalUserRequests::IsLocalUserSignedIn,
                                                        localUserId);
        return isLocalUserSignedIn;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline bool GameStateLocalUserLobby::IsLocalUserAssociatedWithConnectedInputDevice(AzFramework::LocalUserId localUserId) const
    {
        AzFramework::InputDeviceRequests::InputDeviceByIdMap inputDevicesById;
        AzFramework::InputDeviceRequestBus::Broadcast(&AzFramework::InputDeviceRequests::GetInputDevicesByIdWithAssignedLocalUserId,
                                                      inputDevicesById,
                                                      localUserId);
        for (const auto& inputDeviceById : inputDevicesById)
        {
            if (inputDeviceById.second && inputDeviceById.second->IsConnected())
            {
                return true;
            }
        }

        return false;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline void GameStateLocalUserLobby::RefreshAllGamepadLightBarColors()
    {
        AzFramework::InputDeviceRequests::InputDeviceByIdMap inputDevicesById;
        AzFramework::InputDeviceRequestBus::Broadcast(&AzFramework::InputDeviceRequests::GetInputDevicesById,
                                                      inputDevicesById);
        for (const auto& inputDeviceById : inputDevicesById)
        {
            const AzFramework::InputDevice* inputDevice = inputDeviceById.second;
            if (inputDevice && AzFramework::InputDeviceGamepad::IsGamepadDevice(inputDeviceById.first))
            {
                AZ::u32 localPlayerSlot = LocalUser::LocalPlayerSlotNone;
                LocalUser::LocalUserRequestBus::BroadcastResult(localPlayerSlot,
                                                                &LocalUser::LocalUserRequests::GetLocalPlayerSlotOccupiedByLocalUserId,
                                                                inputDevice->GetAssignedLocalUserId());
                RefreshGamepadLightBarColor(inputDeviceById.first, localPlayerSlot);
            }
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline void GameStateLocalUserLobby::RefreshGamepadLightBarColor(AzFramework::LocalUserId localUserId,
                                                                     AZ::u32 localPlayerSlot)
    {
        AzFramework::InputDeviceRequests::InputDeviceByIdMap inputDevicesById;
        AzFramework::InputDeviceRequestBus::Broadcast(&AzFramework::InputDeviceRequests::GetInputDevicesByIdWithAssignedLocalUserId,
                                                      inputDevicesById,
                                                      localUserId);
        for (const auto& inputDeviceById : inputDevicesById)
        {
            if (inputDeviceById.second && AzFramework::InputDeviceGamepad::IsGamepadDevice(inputDeviceById.first))
            {
                RefreshGamepadLightBarColor(inputDeviceById.first, localPlayerSlot);
            }
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline void GameStateLocalUserLobby::RefreshGamepadLightBarColor(const AzFramework::InputDeviceId& inputDeviceId,
                                                                     AZ::u32 localPlayerSlot)
    {
        // Colors with a low saturation (< 75%) tend to get washed out and look mostly white.
        static const AZ::Color s_gamepadLightBarColorsByLocalPlayerSlot[LocalUser::LocalPlayerSlotMax]
        {
            AZ::Color((AZ::u8)0, (AZ::u8)0, (AZ::u8)255, (AZ::u8)255), // Blue
            AZ::Color((AZ::u8)255, (AZ::u8)0, (AZ::u8)0, (AZ::u8)255), // Red
            AZ::Color((AZ::u8)0, (AZ::u8)255, (AZ::u8)0, (AZ::u8)255), // Green
    #if defined(AZ_PLATFORM_PROVO)
            AZ::Color((AZ::u8)255, (AZ::u8)0, (AZ::u8)127, (AZ::u8)255)// Pink
    #else
            AZ::Color((AZ::u8)127, (AZ::u8)255, (AZ::u8)0, (AZ::u8)255)// Yellow
    #endif
        };
        static const AZ::Color s_gamepadLightBarColorNoLocalPlayerSlot = AZ::Color::CreateOne(); // White

        const AZ::Color& lightBarColor = localPlayerSlot < LocalUser::LocalPlayerSlotMax ?
                                         s_gamepadLightBarColorsByLocalPlayerSlot[localPlayerSlot] :
                                         s_gamepadLightBarColorNoLocalPlayerSlot;
        AzFramework::InputLightBarRequestBus::Event(inputDeviceId,
                                                    &AzFramework::InputLightBarRequests::SetLightBarColor,
                                                    lightBarColor);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline void GameStateLocalUserLobby::RefreshAllSignedInUserOverlays()
    {
        for (AZ::u32 localPlayerSlot = 0; localPlayerSlot < LocalUser::LocalPlayerSlotMax; ++localPlayerSlot)
        {
            RefreshSignedInUserOverlay(localPlayerSlot);
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline void GameStateLocalUserLobby::RefreshSignedInUserOverlay(AZ::u32 localPlayerSlot)
    {
        if (!m_signedInUsersOverlayCanvasEntityId.IsValid())
        {
            return;
        }

        const AzFramework::LocalUserId localUserId = LocalUser::LocalUserRequests::GetLocalUserIdAt(localPlayerSlot);
        if (localUserId == AzFramework::LocalUserIdNone)
        {
            SetSignedInUserOverlayEnabled(localPlayerSlot, false);
            return;
        }

        const bool isLocalUserSignedIn = IsLocalUserSignedIn(localUserId);
        if (!isLocalUserSignedIn)
        {
            SetSignedInUserOverlayEnabled(localPlayerSlot, false);
            return;
        }

        AZStd::string localUserName;
        LocalUser::LocalUserRequestBus::BroadcastResult(localUserName,
                                                        &LocalUser::LocalUserRequests::GetLocalUserName,
                                                        localUserId);
        SetSignedInUserOverlayEnabled(localPlayerSlot, true);
        SetSignedInUserOverlayNameText(localPlayerSlot, localUserName);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline void GameStateLocalUserLobby::SetSignedInUserOverlayEnabled(AZ::u32 localPlayerSlot,
                                                                       bool enabled)
    {
        AZ::EntityId userElementName = GetUiElementIdForLocalPlayerSlot("User", localPlayerSlot);
        if (userElementName.IsValid())
        {
            UiElementBus::Event(userElementName, &UiElementInterface::SetIsEnabled, enabled);
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline void GameStateLocalUserLobby::SetSignedInUserOverlayNameText(AZ::u32 localPlayerSlot,
                                                                        const AZStd::string& localUserName)
    {
        AZ::EntityId userNameTextElementName = GetUiElementIdForLocalPlayerSlot("UserName", localPlayerSlot);
        if (userNameTextElementName.IsValid())
        {
            UiTextBus::Event(userNameTextElementName, &UiTextInterface::SetText, localUserName);
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    inline AZ::EntityId GameStateLocalUserLobby::GetUiElementIdForLocalPlayerSlot(const char* elementName,
                                                                                  AZ::u32 localPlayerSlot) const
    {
        char elementNameForSlot[16];
        azsnprintf(elementNameForSlot, 16, "%s%d", elementName, localPlayerSlot);

        AZ::EntityId elementId;
        UiCanvasBus::EventResult(elementId,
                                 m_signedInUsersOverlayCanvasEntityId,
                                 &UiCanvasInterface::FindElementEntityIdByName,
                                 elementNameForSlot);
        return elementId;
    }
}
