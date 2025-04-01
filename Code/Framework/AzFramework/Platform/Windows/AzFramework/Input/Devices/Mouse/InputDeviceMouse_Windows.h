/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/PlatformIncl.h>
#include <AzCore/Module/Environment.h>
#include <AzFramework/Input/Devices/Mouse/InputDeviceMouse.h>
#include <AzFramework/Input/Buses/Notifications/RawInputNotificationBus_Platform.h>
#include <AzFramework/Input/Buses/Requests/InputSystemCursorRequestBus.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Platform specific implementation for Windows mouse input devices
    class InputDeviceMouseWindows : public InputDeviceMouse::Implementation
                                  , public RawInputNotificationBusWindows::Handler
    {
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Count of the number instances of this class that have been created
        static AZ::EnvironmentVariable<int> s_instanceCount;

    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        // Allocator
        AZ_CLASS_ALLOCATOR(InputDeviceMouseWindows, AZ::SystemAllocator);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Constructor
        //! \param[in] inputDevice Reference to the input device being implemented
        InputDeviceMouseWindows(InputDeviceMouse& inputDevice);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Destructor
        ~InputDeviceMouseWindows() override;

    private:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputDeviceMouse::Implementation::IsConnected
        bool IsConnected() const override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputDeviceMouse::Implementation::SetSystemCursorState
        void SetSystemCursorState(SystemCursorState systemCursorState) override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputDeviceMouse::Implementation::GetSystemCursorState
        SystemCursorState GetSystemCursorState() const override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputDeviceMouse::Implementation::SetSystemCursorPositionNormalized
        void SetSystemCursorPositionNormalized(AZ::Vector2 positionNormalized) override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputDeviceMouse::Implementation::GetSystemCursorPositionNormalized
        AZ::Vector2 GetSystemCursorPositionNormalized() const override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::InputDeviceMouse::Implementation::TickInputDevice
        void TickInputDevice() override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref AzFramework::RawInputNotificationsWindows::OnRawInputEvent
        void OnRawInputEvent(const RAWINPUT& rawInput) override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Refresh system cursor clipping constraint
        void RefreshSystemCursorClippingConstraint();

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Refresh system cursor viibility
        void RefreshSystemCursorVisibility();

        //! Return the size of the client window constrained to the desktop work area.
        RECT GetConstrainedClientRect(HWND focusWindow) const;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! The current system cursor state
        SystemCursorState m_systemCursorState{};

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Does the window attached to the input (main) thread's message queue have focus?
        // https://msdn.microsoft.com/en-us/library/windows/desktop/ms646294(v=vs.85).aspx
        bool m_hasFocus{};

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! The client rect of the window obtained the last time this input device was ticked.
        RECT m_lastClientRect{};

        //! The flags sent with the last received MOUSE_MOVE_RELATIVE or MOUSE_MOVE_ABSOLUTE event.
        USHORT m_lastMouseMoveEventFlags{};

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! The last absolute mouse position, reported by a MOUSE_MOVE_ABSOLUTE raw input API event,
        //! which will more than likely only ever be received when running a remote desktop session.
        AZ::Vector2 m_lastMouseMoveEventAbsolutePosition{};
    };
} // namespace AzFramework
