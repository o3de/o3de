/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/PlatformIncl.h>
#include <AzCore/Module/Environment.h>
#include <AzFramework/Input/Devices/Mouse/InputDeviceMouse.h>
#include <AzFramework/Input/Devices/Mouse/InputDeviceMouse_Windows.h>
#include <AzFramework/Input/Buses/Notifications/RawInputNotificationBus_Platform.h>
#include <AzFramework/Input/Buses/Requests/InputSystemCursorRequestBus.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace
{
    // These are scarcely documented, and do not seem to be publically accessible:
    // - https://msdn.microsoft.com/en-us/library/windows/desktop/ms645546(v=vs.85).aspx
    // - https://msdn.microsoft.com/en-us/library/ff543440.aspx
    const USHORT RAW_INPUT_MOUSE_USAGE_PAGE = 0x01;
    const USHORT RAW_INPUT_MOUSE_USAGE = 0x02;

    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Get the focus window that should be used to clip and/or normalize the system cursor.
    //! \return The HWND that should currently be considered the applictaion's focus window.
    HWND GetSystemCursorFocusWindow()
    {
        void* systemCursorFocusWindow = nullptr;
        AzFramework::InputSystemCursorConstraintRequestBus::BroadcastResult(
            systemCursorFocusWindow,
            &AzFramework::InputSystemCursorConstraintRequests::GetSystemCursorConstraintWindow);
        return systemCursorFocusWindow ? static_cast<HWND>(systemCursorFocusWindow) : ::GetFocus();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    AZ::EnvironmentVariable<int> InputDeviceMouseWindows::s_instanceCount = nullptr;

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputDeviceMouseWindows::InputDeviceMouseWindows(InputDeviceMouse& inputDevice)
        : InputDeviceMouse::Implementation(inputDevice)
        , m_systemCursorState(SystemCursorState::Unknown)
        , m_hasFocus(false)
        , m_lastMouseMoveEventFlags(0)
        , m_lastMouseMoveEventAbsolutePosition()
    {
        memset(&m_lastClientRect, 0, sizeof(m_lastClientRect));

        static const char* s_mouseCountEnvironmentVarName = "InputDeviceMouseInstanceCount";
        s_instanceCount = AZ::Environment::FindVariable<int>(s_mouseCountEnvironmentVarName);
        if (!s_instanceCount)
        {
            s_instanceCount = AZ::Environment::CreateVariable<int>(s_mouseCountEnvironmentVarName, 1);

            // Register for raw mouse input
            RAWINPUTDEVICE rawInputDevice;
            rawInputDevice.usUsagePage = RAW_INPUT_MOUSE_USAGE_PAGE;
            rawInputDevice.usUsage = RAW_INPUT_MOUSE_USAGE;
            rawInputDevice.dwFlags = 0;
            rawInputDevice.hwndTarget = 0;
            const BOOL result = RegisterRawInputDevices(&rawInputDevice, 1, sizeof(rawInputDevice));
            AZ_Assert(result, "Failed to register raw input device: mouse");
            AZ_UNUSED(result);
        }
        else
        {
            s_instanceCount.Set(s_instanceCount.Get() + 1);
        }

        RawInputNotificationBusWindows::Handler::BusConnect();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputDeviceMouseWindows::~InputDeviceMouseWindows()
    {
        RawInputNotificationBusWindows::Handler::BusDisconnect();

        // Cleanup system cursor visibility and constraint
        SetSystemCursorState(SystemCursorState::Unknown);

        int instanceCount = s_instanceCount.Get();
        if (--instanceCount == 0)
        {
            // Deregister from raw mouse input
            RAWINPUTDEVICE rawInputDevice;
            rawInputDevice.usUsagePage = RAW_INPUT_MOUSE_USAGE_PAGE;
            rawInputDevice.usUsage     = RAW_INPUT_MOUSE_USAGE;
            rawInputDevice.dwFlags     = RIDEV_REMOVE;
            rawInputDevice.hwndTarget  = 0;
            const BOOL result = RegisterRawInputDevices(&rawInputDevice, 1, sizeof(rawInputDevice));
            AZ_Assert(result, "Failed to deregister raw input device: mouse");
            AZ_UNUSED(result);

            s_instanceCount.Reset();
        }
        else
        {
            s_instanceCount.Set(instanceCount);
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool InputDeviceMouseWindows::IsConnected() const
    {
        // If necessary, we can register raw input devices using RIDEV_DEVNOTIFY in order to receive
        // WM_INPUT_DEVICE_CHANGE Windows messages in the WndProc function. These could then be sent
        // using an EBus (RawInputNotificationBusWindows?) and used to keep track of connected state.
        //
        // Doing this would allow (in one respect force) us to distinguish between multiple physical
        // devices of the same type. But given that support for multiple mice is a fairly niche need
        // we'll keep things simple (for now) and assume there is one (and only one) mouse connected
        // at all times. In practice this means that if multiple physical mice are connected we will
        // process input from them all, but treat all the input as if it comes from the same device.
        //
        // If it becomes necessary to determine the connected state of mouse devices (and/or support
        // distinguishing between multiple physical mice), we should implement this function as well
        // call BroadcastInputDeviceConnectedEvent/BroadcastInputDeviceDisconnectedEvent when needed.
        //
        // Note that doing so will require modifying how we create and manage mouse input devices in
        // InputSystemComponent/InputSystemComponentWin in order to create multiple InputDeviceMouse
        // instances (somehow associating each with a RID_DEVICE_INFO_MOUSE), and also modifying the
        // InputDeviceMouseWindows::OnRawInputEvent function to filter incoming events by device id.
        return true;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceMouseWindows::SetSystemCursorState(SystemCursorState systemCursorState)
    {
        if (m_captureCursor && systemCursorState != m_systemCursorState)
        {
            m_systemCursorState = systemCursorState;
            RefreshSystemCursorClippingConstraint();
            RefreshSystemCursorVisibility();
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    SystemCursorState InputDeviceMouseWindows::GetSystemCursorState() const
    {
        return m_systemCursorState;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceMouseWindows::SetSystemCursorPositionNormalized(AZ::Vector2 positionNormalized)
    {
        HWND focusWindow = GetSystemCursorFocusWindow();
        if (!focusWindow)
        {
            return;
        }

        // Get the content (client) rect of the focus window in screen coordinates, excluding the system taskbar
        RECT clientRect = GetConstrainedClientRect(focusWindow);
        const float clientWidth = static_cast<float>(clientRect.right - clientRect.left);
        const float clientHeight = static_cast<float>(clientRect.bottom - clientRect.top);

        // De-normalize the position relative to the focus window, then transform to screen coords
        POINT cursorPos;
        cursorPos.x = static_cast<LONG>(positionNormalized.GetX() * clientWidth);
        cursorPos.y = static_cast<LONG>(positionNormalized.GetY() * clientHeight);
        ::ClientToScreen(focusWindow, &cursorPos);
        ::SetCursorPos(cursorPos.x, cursorPos.y);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    AZ::Vector2 InputDeviceMouseWindows::GetSystemCursorPositionNormalized() const
    {
        HWND focusWindow = GetSystemCursorFocusWindow();
        if (!focusWindow)
        {
            return AZ::Vector2::CreateZero();
        }

        // Get the position of the cursor relative to the focus window
        POINT cursorPos;
        ::GetCursorPos(&cursorPos);
        ::ScreenToClient(focusWindow, &cursorPos);

        // Get the content (client) rect of the focus window
        RECT clientRect = GetConstrainedClientRect(focusWindow);

        // If the window is 0-sized in at least one dimension, just return a cursor position of (0, 0).
        // This can happen when a window gets hidden by pressing Windows-D to go to the desktop.
        if ((clientRect.left == clientRect.right) || (clientRect.top == clientRect.bottom))
        {
            return AZ::Vector2(0.0f, 0.0f);
        }

        // Normalize the cursor position relative to the content (client rect) of the focus window
        const float clientRectWidth = static_cast<float>(clientRect.right - clientRect.left);
        const float clientRectHeight = static_cast<float>(clientRect.bottom - clientRect.top);
        const float normalizedCursorPositionX = static_cast<float>(cursorPos.x) / clientRectWidth;
        const float normalizedCursorPositionY = static_cast<float>(cursorPos.y) / clientRectHeight;

        return AZ::Vector2(normalizedCursorPositionX, normalizedCursorPositionY);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceMouseWindows::TickInputDevice()
    {
        // The input event loop is pumped by the system on Windows so all raw input events for this
        // frame have already been dispatched. But they are all queued until ProcessRawEventQueues
        // is called below so that all raw input events are processed at the same time every frame.

        const bool hadFocus = m_hasFocus;
        m_hasFocus = ::GetFocus() != nullptr;
        if (m_hasFocus)
        {
            RECT clientRect;
            HWND focusWindow = GetSystemCursorFocusWindow();
            ::GetClientRect(focusWindow, &clientRect);
            ::ClientToScreen(focusWindow, (LPPOINT)&clientRect.left);  // Converts the top-left point
            ::ClientToScreen(focusWindow, (LPPOINT)&clientRect.right); // Converts the bottom-right point
            if (!hadFocus ||
                clientRect.top != m_lastClientRect.top ||
                clientRect.left != m_lastClientRect.left ||
                clientRect.right != m_lastClientRect.right ||
                clientRect.bottom != m_lastClientRect.bottom)
            {
                // We have to refresh the system cursor clip rect each time the application gains
                // focus, changes resolution, or transitions between fullscreen and windowed mode.
                // This is in order to combat the cursor being unclipped by the system or another
                // application which can happen as a result of the cursor being a shared resource.
                RefreshSystemCursorClippingConstraint();
            }
            memcpy(&m_lastClientRect, &clientRect, sizeof(m_lastClientRect));

            // Process raw event queues once each frame while this thread's message queue has focus
            ProcessRawEventQueues();
        }
        else if (hadFocus)
        {
            // The window attached to this thread's message queue no longer has focus, process any
            // events that are queued, before resetting the state of all associated input channels.
            ProcessRawEventQueues();
            ResetInputChannelStates();
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceMouseWindows::OnRawInputEvent(const RAWINPUT& rawInput)
    {
        if (rawInput.header.dwType != RIM_TYPEMOUSE || !::GetFocus())
        {
            return;
        }

        const RAWMOUSE& rawMouseData = rawInput.data.mouse;
        const USHORT buttonFlags = rawMouseData.usButtonFlags;

        // Left button
        if (buttonFlags & RI_MOUSE_LEFT_BUTTON_DOWN)
        {
            QueueRawButtonEvent(InputDeviceMouse::Button::Left, true);
        }
        if (buttonFlags & RI_MOUSE_LEFT_BUTTON_UP)
        {
            QueueRawButtonEvent(InputDeviceMouse::Button::Left, false);
        }

        // Right button
        if (buttonFlags & RI_MOUSE_RIGHT_BUTTON_DOWN)
        {
            QueueRawButtonEvent(InputDeviceMouse::Button::Right, true);
        }
        if (buttonFlags & RI_MOUSE_RIGHT_BUTTON_UP)
        {
            QueueRawButtonEvent(InputDeviceMouse::Button::Right, false);
        }

        // Middle button
        if (buttonFlags & RI_MOUSE_MIDDLE_BUTTON_DOWN)
        {
            QueueRawButtonEvent(InputDeviceMouse::Button::Middle, true);
        }
        if (buttonFlags & RI_MOUSE_MIDDLE_BUTTON_UP)
        {
            QueueRawButtonEvent(InputDeviceMouse::Button::Middle, false);
        }

        // X1 button (deprecated)
        if (buttonFlags & RI_MOUSE_BUTTON_4_DOWN)
        {
            QueueRawButtonEvent(InputDeviceMouse::Button::Other1, true);
        }
        if (buttonFlags & RI_MOUSE_BUTTON_4_UP)
        {
            QueueRawButtonEvent(InputDeviceMouse::Button::Other1, false);
        }

        // X2 button (deprecated)
        if (buttonFlags & RI_MOUSE_BUTTON_5_DOWN)
        {
            QueueRawButtonEvent(InputDeviceMouse::Button::Other2, true);
        }
        if (buttonFlags & RI_MOUSE_BUTTON_5_UP)
        {
            QueueRawButtonEvent(InputDeviceMouse::Button::Other2, false);
        }

        // Scroll wheel
        if (buttonFlags & RI_MOUSE_WHEEL)
        {
            QueueRawMovementEvent(InputDeviceMouse::Movement::Z, static_cast<short>(rawMouseData.usButtonData));
        }

        // Mouse movement
        if (rawMouseData.usFlags == MOUSE_MOVE_RELATIVE)
        {
            QueueRawMovementEvent(InputDeviceMouse::Movement::X, static_cast<float>(rawMouseData.lLastX));
            QueueRawMovementEvent(InputDeviceMouse::Movement::Y, static_cast<float>(rawMouseData.lLastY));
            m_lastMouseMoveEventFlags = rawMouseData.usFlags;
        }
        else if (rawMouseData.usFlags & MOUSE_MOVE_ABSOLUTE)
        {
            const bool isVirtualDesktop = (rawMouseData.usFlags & MOUSE_VIRTUAL_DESKTOP) != 0;
            const int screenWidth = GetSystemMetrics(isVirtualDesktop ? SM_CXVIRTUALSCREEN : SM_CXSCREEN);
            const int screenHeight = GetSystemMetrics(isVirtualDesktop ? SM_CYVIRTUALSCREEN : SM_CYSCREEN);
            const float absoluteX = (static_cast<float>(rawMouseData.lLastX) / static_cast<float>(USHRT_MAX)) * static_cast<float>(screenWidth);
            const float absoluteY = (static_cast<float>(rawMouseData.lLastY) / static_cast<float>(USHRT_MAX)) * static_cast<float>(screenHeight);

            if (m_lastMouseMoveEventFlags & MOUSE_MOVE_ABSOLUTE)
            {
                // Only calculate and send the delta if we have previously cached a valid position
                const float deltaX = absoluteX - m_lastMouseMoveEventAbsolutePosition.GetX();
                const float deltaY = absoluteY - m_lastMouseMoveEventAbsolutePosition.GetY();
                QueueRawMovementEvent(InputDeviceMouse::Movement::X, deltaX);
                QueueRawMovementEvent(InputDeviceMouse::Movement::Y, deltaY);
            }

            m_lastMouseMoveEventFlags = rawMouseData.usFlags;
            m_lastMouseMoveEventAbsolutePosition.SetX(absoluteX);
            m_lastMouseMoveEventAbsolutePosition.SetY(absoluteY);
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceMouseWindows::RefreshSystemCursorClippingConstraint()
    {
        HWND focusWindow = GetSystemCursorFocusWindow();
        if (!focusWindow)
        {
            return;
        }

        const bool shouldBeConstrained = (m_systemCursorState == SystemCursorState::ConstrainedAndHidden) ||
                                         (m_systemCursorState == SystemCursorState::ConstrainedAndVisible);
        if (!shouldBeConstrained)
        {
            // Unconstrain the cursor
            ::ClipCursor(NULL);
            return;
        }

        // Constrain the cursor to the client (content) rect of the focus window
        RECT clientRect = GetConstrainedClientRect(focusWindow);
        ::ClipCursor(&clientRect);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceMouseWindows::RefreshSystemCursorVisibility()
    {
        const bool shouldBeHidden = (m_systemCursorState == SystemCursorState::ConstrainedAndHidden) ||
                                    (m_systemCursorState == SystemCursorState::UnconstrainedAndHidden);

        // The Windows system ShowCursor function stores and returns an application specific display
        // counter, and the cursor is displayed only when the display count is greater than or equal
        // to zero: https://msdn.microsoft.com/en-us/library/windows/desktop/ms648396(v=vs.85).aspx
        if (shouldBeHidden)
        {
            while (::ShowCursor(false) >= 0) {}
        }
        else
        {
            while (::ShowCursor(true) < 0) {}
        }
    }

    RECT InputDeviceMouseWindows::GetConstrainedClientRect(HWND focusWindow) const
    {
        // Get the client rect of the focus window in screen coordinates
        RECT clientRect;
        ::GetClientRect(focusWindow, &clientRect);
        ::ClientToScreen(focusWindow, (LPPOINT)&clientRect.left); // Converts the top-left point
        ::ClientToScreen(focusWindow, (LPPOINT)&clientRect.right); // Converts the bottom-right point

        // Note that this returns a rectangle that could overlap other things like the system taskbar.
        // We're making the assumption that the implementation of NativeWindow is drawing the window above everything,
        // including the taskbar, so that any mouse movements and clicks within the window won't be able to change focus
        // to the taskbar, popup email notifications, etc. If NativeWindow *doesn't* draw the window above everything,
        // then the way we handle hidden contrained mouse inputs will need to change to ensure that it doesn't accidentally
        // send clicks and movements to overlapping windows of higher priority.

        return clientRect;
    }

} // namespace AzFramework
