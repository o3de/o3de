/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <GameState/GameState.h>

#include <LocalUser/LocalUserNotificationBus.h>

#include <AzFramework/API/ApplicationAPI.h>
#include <AzFramework/Input/Buses/Notifications/InputDeviceNotificationBus.h>
#include <AzFramework/Input/Events/InputChannelEventListener.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace GameStateSamples
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Game state that acts a lobby by assigning local user ids into local player slots as needed.
    class GameStateLocalUserLobby : public GameState::IGameState
                                  , public AzFramework::InputChannelEventListener
                                  , public AzFramework::InputDeviceNotificationBus::Handler
                                  , public AzFramework::ApplicationLifecycleEvents::Bus::Handler
                                  , public LocalUser::LocalUserNotificationBus::Handler
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        // Allocator
        AZ_CLASS_ALLOCATOR(GameStateLocalUserLobby, AZ::SystemAllocator);

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Type Info
        AZ_RTTI(GameStateLocalUserLobby, "{E6D54EAF-F826-4EEE-91CD-60A052DA55E4}", IGameState);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Default constructor
        GameStateLocalUserLobby() = default;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Default destructor
        ~GameStateLocalUserLobby() override = default;

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

    protected:
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
        //! \ref AzFramework::InputDeviceNotifications::OnInputDeviceDisconnectedEvent
        void OnInputDeviceDisconnectedEvent(const AzFramework::InputDevice& inputDevice) override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::ApplicationLifecycleEvents::OnApplicationUnconstrained
        void OnApplicationUnconstrained(AzFramework::ApplicationLifecycleEvents::Event /*lastEvent*/) override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref LocalUser::LocalUserNotifications::OnLocalUserIdAssignedToLocalPlayerSlot
        void OnLocalUserIdAssignedToLocalPlayerSlot(AzFramework::LocalUserId localUserId,
                                                    AZ::u32 newLocalPlayerSlot,
                                                    AZ::u32 previousLocalPlayerSlot) override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref LocalUser::LocalUserNotifications::OnLocalUserIdRemovedFromLocalPlayerSlot
        void OnLocalUserIdRemovedFromLocalPlayerSlot(AzFramework::LocalUserId localUserId,
                                                     AZ::u32 localPlayerSlot) override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Convenience functions to load and unload the signed in user overlay UI canvas.
        ///@{
        virtual void LoadSignedInUserOverlayCanvas();
        virtual void UnloadSignedInUserOverlayCanvas();
        virtual const char* GetSignedInUserOverlayCanvasAssetPath();
        ///@}

    protected:
        void RefreshLocalPlayerSlotAssignments();
        bool IsLocalUserSignedIn(AzFramework::LocalUserId localUserId) const;
        bool IsLocalUserAssociatedWithConnectedInputDevice(AzFramework::LocalUserId localUserId) const;

        void RefreshAllGamepadLightBarColors();
        void RefreshGamepadLightBarColor(AzFramework::LocalUserId localUserId, AZ::u32 localPlayerSlot);
        void RefreshGamepadLightBarColor(const AzFramework::InputDeviceId& inputDeviceId, AZ::u32 localPlayerSlot);

        void RefreshAllSignedInUserOverlays();
        void RefreshSignedInUserOverlay(AZ::u32 localPlayerSlot);

        void SetSignedInUserOverlayEnabled(AZ::u32 localPlayerSlot,
                                           bool enabled);
        void SetSignedInUserOverlayNameText(AZ::u32 localPlayerSlot,
                                            const AZStd::string& localUserName);
        AZ::EntityId GetUiElementIdForLocalPlayerSlot(const char* elementName,
                                                      AZ::u32 localPlayerSlot) const;

    protected:
        ////////////////////////////////////////////////////////////////////////////////////////////
        // Variables
        AZ::EntityId m_signedInUsersOverlayCanvasEntityId;
        bool         m_shouldRefreshLevelListDisplay = false;
    };
} // namespace GameStateSamples

// Include the implementation inline so the class can be instantiated outside of the gem.
#include <GameStateSamples/GameStateLocalUserLobby.inl>
