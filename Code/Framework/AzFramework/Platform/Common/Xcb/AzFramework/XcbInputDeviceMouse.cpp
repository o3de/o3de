/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/typetraits/integral_constant.h>
#include <AzFramework/API/ApplicationAPI_Linux.h>
#include <AzFramework/XcbConnectionManager.h>
#include <AzFramework/XcbInputDeviceMouse.h>

namespace AzFramework
{
    xcb_window_t GetSystemCursorFocusWindow()
    {
        void* systemCursorFocusWindow = nullptr;
        AzFramework::InputSystemCursorConstraintRequestBus::BroadcastResult(
            systemCursorFocusWindow, &AzFramework::InputSystemCursorConstraintRequests::GetSystemCursorConstraintWindow);

        if (!systemCursorFocusWindow)
        {
            return XCB_NONE;
        }

        // TODO Clang compile error because cast .... loses information. On GNU/Linux HWND is void* and on 64-bit
        // machines its obviously 64 bit but we receive the window id from m_renderOverlay.winId() which is xcb_window_t 32-bit.

        return static_cast<xcb_window_t>(reinterpret_cast<uint64_t>(systemCursorFocusWindow));
    }

    xcb_connection_t* XcbInputDeviceMouse::s_xcbConnection = nullptr;
    xcb_screen_t* XcbInputDeviceMouse::s_xcbScreen = nullptr;
    bool XcbInputDeviceMouse::m_xfixesInitialized = false;
    bool XcbInputDeviceMouse::m_xInputInitialized = false;

    XcbInputDeviceMouse::XcbInputDeviceMouse(InputDeviceMouse& inputDevice)
        : InputDeviceMouse::Implementation(inputDevice)
        , m_systemCursorState(SystemCursorState::Unknown)
        , m_systemCursorPositionNormalized(0.5f, 0.5f)
        , m_prevConstraintWindow(XCB_NONE)
        , m_focusWindow(XCB_NONE)
        , m_cursorShown(true)
    {
        XcbEventHandlerBus::Handler::BusConnect();

        SetSystemCursorState(SystemCursorState::Unknown);
    }

    XcbInputDeviceMouse::~XcbInputDeviceMouse()
    {
        XcbEventHandlerBus::Handler::BusDisconnect();

        SetSystemCursorState(SystemCursorState::Unknown);
    }

    InputDeviceMouse::Implementation* XcbInputDeviceMouse::Create(InputDeviceMouse& inputDevice)
    {
        auto* interface = AzFramework::XcbConnectionManagerInterface::Get();
        if (!interface)
        {
            AZ_Warning("XcbInput", false, "XCB interface not available");
            return nullptr;
        }

        s_xcbConnection = AzFramework::XcbConnectionManagerInterface::Get()->GetXcbConnection();
        if (!s_xcbConnection)
        {
            AZ_Warning("XcbInput", false, "XCB connection not available");
            return nullptr;
        }

        const xcb_setup_t* xcbSetup = xcb_get_setup(s_xcbConnection);
        s_xcbScreen = xcb_setup_roots_iterator(xcbSetup).data;
        if (!s_xcbScreen)
        {
            AZ_Warning("XcbInput", false, "XCB screen not available");
            return nullptr;
        }

        // Initialize XFixes extension which we use to create pointer barriers.
        if (!InitializeXFixes())
        {
            AZ_Warning("XcbInput", false, "XCB XFixes initialization failed");
            return nullptr;
        }

        // Initialize XInput extension which is used to get RAW Input events.
        if (!InitializeXInput())
        {
            AZ_Warning("XcbInput", false, "XCB XInput initialization failed");
            return nullptr;
        }

        return aznew XcbInputDeviceMouse(inputDevice);
    }

    bool XcbInputDeviceMouse::IsConnected() const
    {
        return true;
    }

    void XcbInputDeviceMouse::CreateBarriers(xcb_window_t window, bool create)
    {
        // Don't create any barriers if we are debugging. This will cause artifacts but better then
        // a confined cursor during debugging.
        if (AZ::Debug::Trace::IsDebuggerPresent())
        {
            AZ_Warning("XcbInput", false, "Debugger running. Barriers will not be created.");
            return;
        }

        if (create)
        {
            // Destroy barriers if they are active already.
            if (!m_activeBarriers.empty())
            {
                for (const auto& barrier : m_activeBarriers)
                {
                    xcb_xfixes_delete_pointer_barrier_checked(s_xcbConnection, barrier.id);
                }

                m_activeBarriers.clear();
            }

            // Get window information.
            const XcbStdFreePtr<xcb_get_geometry_reply_t> xcbGeometryReply{ xcb_get_geometry_reply(
                s_xcbConnection, xcb_get_geometry(s_xcbConnection, window), NULL) };

            if (!xcbGeometryReply)
            {
                return;
            }

            const xcb_translate_coordinates_cookie_t translate_coord =
                xcb_translate_coordinates(s_xcbConnection, window, s_xcbScreen->root, 0, 0);

            const XcbStdFreePtr<xcb_translate_coordinates_reply_t> xkbTranslateCoordReply{ xcb_translate_coordinates_reply(
                s_xcbConnection, translate_coord, NULL) };

            if (!xkbTranslateCoordReply)
            {
                return;
            }

            const int16_t x0 = xkbTranslateCoordReply->dst_x < 0 ? 0 : xkbTranslateCoordReply->dst_x;
            const int16_t y0 = xkbTranslateCoordReply->dst_y < 0 ? 0 : xkbTranslateCoordReply->dst_y;
            const int16_t x1 = xkbTranslateCoordReply->dst_x + xcbGeometryReply->width;
            const int16_t y1 = xkbTranslateCoordReply->dst_y + xcbGeometryReply->height;

            // ATTN For whatever reason, when making an exact rectangle the pointer will escape the top right corner in some cases. Adding
            // an offset to the lines so that they cross each other prevents that.
            const int16_t offset = 30;

            // Create the left barrier info.
            m_activeBarriers.push_back({ xcb_generate_id(s_xcbConnection), XCB_XFIXES_BARRIER_DIRECTIONS_POSITIVE_X, x0, Clamp(y0 - offset),
                                         x0, Clamp(y1 + offset) });

            // Create the right barrier info.
            m_activeBarriers.push_back({ xcb_generate_id(s_xcbConnection), XCB_XFIXES_BARRIER_DIRECTIONS_NEGATIVE_X, x1, Clamp(y0 - offset),
                                         x1, Clamp(y1 + offset) });

            // Create the top barrier info.
            m_activeBarriers.push_back({ xcb_generate_id(s_xcbConnection), XCB_XFIXES_BARRIER_DIRECTIONS_POSITIVE_Y, Clamp(x0 - offset), y0,
                                         Clamp(x1 + offset), y0 });

            // Create the bottom barrier info.
            m_activeBarriers.push_back({ xcb_generate_id(s_xcbConnection), XCB_XFIXES_BARRIER_DIRECTIONS_NEGATIVE_Y, Clamp(x0 - offset), y1,
                                         Clamp(x1 + offset), y1 });

            // Create the xfixes barriers.
            for (const auto& barrier : m_activeBarriers)
            {
                xcb_void_cookie_t cookie = xcb_xfixes_create_pointer_barrier_checked(
                    s_xcbConnection, barrier.id, window, barrier.x0, barrier.y0, barrier.x1, barrier.y1, barrier.direction, 0, NULL);
                const XcbStdFreePtr<xcb_generic_error_t> xkbError{ xcb_request_check(s_xcbConnection, cookie) };

                AZ_Warning(
                    "XcbInput", !xkbError, "XFixes, failed to create barrier %d at (%d %d %d %d)", barrier.id, barrier.x0, barrier.y0,
                    barrier.x1, barrier.y1);
            }
        }
        else
        {
            for (const auto& barrier : m_activeBarriers)
            {
                xcb_xfixes_delete_pointer_barrier_checked(s_xcbConnection, barrier.id);
            }

            m_activeBarriers.clear();
        }

        xcb_flush(s_xcbConnection);
    }

    bool XcbInputDeviceMouse::InitializeXFixes()
    {
        m_xfixesInitialized = false;

        // We don't have to free query_extension_reply according to xcb documentation.
        const xcb_query_extension_reply_t* query_extension_reply = xcb_get_extension_data(s_xcbConnection, &xcb_xfixes_id);
        if (!query_extension_reply || !query_extension_reply->present)
        {
            return m_xfixesInitialized;
        }

        const xcb_xfixes_query_version_cookie_t query_cookie = xcb_xfixes_query_version(s_xcbConnection, 5, 0);

        xcb_generic_error_t* error = NULL;
        const XcbStdFreePtr<xcb_xfixes_query_version_reply_t> xkbQueryRequestReply{ xcb_xfixes_query_version_reply(
            s_xcbConnection, query_cookie, &error) };

        if (!xkbQueryRequestReply || error)
        {
            if (error)
            {
                AZ_Warning("XcbInput", false, "Retrieving XFixes version failed : Error code %d", error->error_code);
                free(error);
            }
            return m_xfixesInitialized;
        }
        else if (xkbQueryRequestReply->major_version < 5)
        {
            AZ_Warning("XcbInput", false, "XFixes version fails the minimum version check (%d<5)", xkbQueryRequestReply->major_version);
            return m_xfixesInitialized;
        }

        m_xfixesInitialized = true;

        return m_xfixesInitialized;
    }

    bool XcbInputDeviceMouse::InitializeXInput()
    {
        m_xInputInitialized = false;

        // We don't have to free query_extension_reply according to xcb documentation.
        const xcb_query_extension_reply_t* query_extension_reply = xcb_get_extension_data(s_xcbConnection, &xcb_input_id);
        if (!query_extension_reply || !query_extension_reply->present)
        {
            return m_xInputInitialized;
        }

        const xcb_input_xi_query_version_cookie_t query_version_cookie = xcb_input_xi_query_version(s_xcbConnection, 2, 2);

        xcb_generic_error_t* error = NULL;
        const XcbStdFreePtr<xcb_input_xi_query_version_reply_t> xkbQueryRequestReply{ xcb_input_xi_query_version_reply(
            s_xcbConnection, query_version_cookie, &error) };

        if (!xkbQueryRequestReply || error)
        {
            if (error)
            {
                AZ_Warning("XcbInput", false, "Retrieving XInput version failed : Error code %d", error->error_code);
                free(error);
            }
            return m_xInputInitialized;
        }
        else if (xkbQueryRequestReply->major_version < 2)
        {
            AZ_Warning("XcbInput", false, "XInput version fails the minimum version check (%d<5)", xkbQueryRequestReply->major_version);
            return m_xInputInitialized;
        }

        m_xInputInitialized = true;

        return m_xInputInitialized;
    }

    void XcbInputDeviceMouse::SetEnableXInput(bool enable)
    {
        struct
        {
            xcb_input_event_mask_t head;
            int mask;
        } mask;

        mask.head.deviceid = XCB_INPUT_DEVICE_ALL;
        mask.head.mask_len = 1;

        if (enable)
        {
            mask.mask = XCB_INPUT_XI_EVENT_MASK_RAW_MOTION | XCB_INPUT_XI_EVENT_MASK_RAW_BUTTON_PRESS |
                XCB_INPUT_XI_EVENT_MASK_RAW_BUTTON_RELEASE | XCB_INPUT_XI_EVENT_MASK_MOTION | XCB_INPUT_XI_EVENT_MASK_BUTTON_PRESS |
                XCB_INPUT_XI_EVENT_MASK_BUTTON_RELEASE;
        }
        else
        {
            mask.mask = XCB_NONE;
        }

        xcb_input_xi_select_events(s_xcbConnection, s_xcbScreen->root, 1, &mask.head);

        xcb_flush(s_xcbConnection);
    }

    void XcbInputDeviceMouse::SetSystemCursorState(SystemCursorState systemCursorState)
    {
        if (systemCursorState != m_systemCursorState)
        {
            m_systemCursorState = systemCursorState;

            m_focusWindow = GetSystemCursorFocusWindow();

            HandleCursorState(m_focusWindow, systemCursorState);
        }
    }

    void XcbInputDeviceMouse::HandleCursorState(xcb_window_t window, SystemCursorState systemCursorState)
    {
        bool confined = false, cursorShown = true;
        switch (systemCursorState)
        {
        case SystemCursorState::ConstrainedAndHidden:
            {
                //!< Constrained to the application's main window and hidden
                confined = true;
                cursorShown = false;
            }
            break;
        case SystemCursorState::ConstrainedAndVisible:
            {
                //!< Constrained to the application's main window and visible
                confined = true;
            }
            break;
        case SystemCursorState::UnconstrainedAndHidden:
            {
                //!< Free to move outside the main window but hidden while inside
                cursorShown = false;
            }
            break;
        case SystemCursorState::UnconstrainedAndVisible:
            {
                //!< Free to move outside the application's main window and visible
            }
        case SystemCursorState::Unknown:
        default:
            break;
        }

        // ATTN GetSystemCursorFocusWindow when getting out of the play in editor will return XCB_NONE
        // We need however the window id to reset the cursor.
        if (XCB_NONE == window && (confined || cursorShown))
        {
            // Reuse the previous window to reset states.
            window = m_prevConstraintWindow;
            m_prevConstraintWindow = XCB_NONE;
        }
        else
        {
            // Remember the window we used to modify cursor and barrier states.
            m_prevConstraintWindow = window;
        }

        SetEnableXInput(!cursorShown);

        CreateBarriers(window, confined);
        ShowCursor(window, cursorShown);
    }

    SystemCursorState XcbInputDeviceMouse::GetSystemCursorState() const
    {
        return m_systemCursorState;
    }

    void XcbInputDeviceMouse::SetSystemCursorPositionNormalizedInternal(xcb_window_t window, AZ::Vector2 positionNormalized)
    {
        // TODO Basically not done at all. Added only the basic functions needed.
        const XcbStdFreePtr<xcb_get_geometry_reply_t> xkbGeometryReply{ xcb_get_geometry_reply(
            s_xcbConnection, xcb_get_geometry(s_xcbConnection, window), NULL) };

        if (!xkbGeometryReply)
        {
            return;
        }

        const int16_t x = static_cast<int16_t>(positionNormalized.GetX() * xkbGeometryReply->width);
        const int16_t y = static_cast<int16_t>(positionNormalized.GetY() * xkbGeometryReply->height);

        xcb_warp_pointer(s_xcbConnection, XCB_NONE, window, 0, 0, 0, 0, x, y);

        xcb_flush(s_xcbConnection);
    }

    void XcbInputDeviceMouse::SetSystemCursorPositionNormalized(AZ::Vector2 positionNormalized)
    {
        const xcb_window_t window = GetSystemCursorFocusWindow();
        if (XCB_NONE == window)
        {
            return;
        }

        SetSystemCursorPositionNormalizedInternal(window, positionNormalized);
    }

    AZ::Vector2 XcbInputDeviceMouse::GetSystemCursorPositionNormalizedInternal(xcb_window_t window) const
    {
        AZ::Vector2 position = AZ::Vector2::CreateZero();

        const xcb_query_pointer_cookie_t pointer = xcb_query_pointer(s_xcbConnection, window);

        const XcbStdFreePtr<xcb_query_pointer_reply_t> xkbQueryPointerReply{ xcb_query_pointer_reply(s_xcbConnection, pointer, NULL) };

        if (!xkbQueryPointerReply)
        {
            return position;
        }

        const XcbStdFreePtr<xcb_get_geometry_reply_t> xkbGeometryReply{ xcb_get_geometry_reply(
            s_xcbConnection, xcb_get_geometry(s_xcbConnection, window), NULL) };

        if (!xkbGeometryReply)
        {
            return position;
        }

        AZ_Assert(xkbGeometryReply->width != 0, "xkbGeometry response width must be non-zero. (%d)", xkbGeometryReply->width);
        const float normalizedCursorPostionX = static_cast<float>(xkbQueryPointerReply->win_x) / xkbGeometryReply->width;

        AZ_Assert(xkbGeometryReply->height != 0, "xkbGeometry response height must be non-zero. (%d)", xkbGeometryReply->height);
        const float normalizedCursorPostionY = static_cast<float>(xkbQueryPointerReply->win_y) / xkbGeometryReply->height;

        position = AZ::Vector2(normalizedCursorPostionX, normalizedCursorPostionY);

        return position;
    }

    AZ::Vector2 XcbInputDeviceMouse::GetSystemCursorPositionNormalized() const
    {
        const xcb_window_t window = GetSystemCursorFocusWindow();
        if (XCB_NONE == window)
        {
            return AZ::Vector2::CreateZero();
        }

        return GetSystemCursorPositionNormalizedInternal(window);
    }

    void XcbInputDeviceMouse::TickInputDevice()
    {
        ProcessRawEventQueues();
    }

    void XcbInputDeviceMouse::ShowCursor(xcb_window_t window, bool show)
    {
        xcb_void_cookie_t cookie;
        if (show)
        {
            cookie = xcb_xfixes_show_cursor_checked(s_xcbConnection, window);
        }
        else
        {
            cookie = xcb_xfixes_hide_cursor_checked(s_xcbConnection, window);
        }

        const XcbStdFreePtr<xcb_generic_error_t> xkbError{ xcb_request_check(s_xcbConnection, cookie) };

        if (xkbError)
        {
            AZ_Warning("XcbInput", false, "ShowCursor failed: %d", xkbError->error_code);

            return;
        }

        // ATTN In the following part we will when cursor gets hidden store the position of the cursor in screen space
        // not window space. We use that to re-position when showing the cursor again. Is this the correct
        // behavior?

        const bool cursorWasHidden = !m_cursorShown;
        m_cursorShown = show;
        if (!m_cursorShown)
        {
            m_cursorHiddenPosition = GetSystemCursorPositionNormalizedInternal(s_xcbScreen->root);

            SetSystemCursorPositionNormalized(AZ::Vector2(0.5f, 0.5f));
        }
        else if (cursorWasHidden)
        {
            SetSystemCursorPositionNormalizedInternal(s_xcbScreen->root, m_cursorHiddenPosition);
        }

        xcb_flush(s_xcbConnection);
    }

    void XcbInputDeviceMouse::HandleButtonPressEvents(uint32_t detail, bool pressed)
    {
        bool isWheel;
        float wheelDirection;
        const auto* button = InputChannelFromMouseEvent(detail, isWheel, wheelDirection);
        if (button)
        {
            QueueRawButtonEvent(*button, pressed);
        }
        if (isWheel)
        {
            float axisValue = MAX_XI_WHEEL_SENSITIVITY * wheelDirection;
            QueueRawMovementEvent(InputDeviceMouse::Movement::Z, axisValue);
        }
    }

    void XcbInputDeviceMouse::HandlePointerMotionEvents(const xcb_generic_event_t* event)
    {
        const xcb_input_motion_event_t* mouseMotionEvent = reinterpret_cast<const xcb_input_motion_event_t*>(event);

        m_systemCursorPosition[0] = mouseMotionEvent->event_x;
        m_systemCursorPosition[1] = mouseMotionEvent->event_y;
    }

    void XcbInputDeviceMouse::HandleRawInputEvents(const xcb_ge_generic_event_t* event)
    {
        const xcb_ge_generic_event_t* genericEvent = reinterpret_cast<const xcb_ge_generic_event_t*>(event);
        switch (genericEvent->event_type)
        {
        case XCB_INPUT_RAW_BUTTON_PRESS:
            {
                const xcb_input_raw_button_press_event_t* mouseButtonEvent =
                    reinterpret_cast<const xcb_input_raw_button_press_event_t*>(event);
                HandleButtonPressEvents(mouseButtonEvent->detail, true);
            }
            break;
        case XCB_INPUT_RAW_BUTTON_RELEASE:
            {
                const xcb_input_raw_button_release_event_t* mouseButtonEvent =
                    reinterpret_cast<const xcb_input_raw_button_release_event_t*>(event);
                HandleButtonPressEvents(mouseButtonEvent->detail, false);
            }
            break;
        case XCB_INPUT_RAW_MOTION:
            {
                const xcb_input_raw_motion_event_t* mouseMotionEvent = reinterpret_cast<const xcb_input_raw_motion_event_t*>(event);

                int axisLen = xcb_input_raw_button_press_axisvalues_length(mouseMotionEvent);
                const xcb_input_fp3232_t* axisvalues = xcb_input_raw_button_press_axisvalues_raw(mouseMotionEvent);
                for (int i = 0; i < axisLen; ++i)
                {
                    const float axisValue = fp3232ToFloat(axisvalues[i]);

                    switch (i)
                    {
                    case 0:
                        QueueRawMovementEvent(InputDeviceMouse::Movement::X, axisValue);
                        break;
                    case 1:
                        QueueRawMovementEvent(InputDeviceMouse::Movement::Y, axisValue);
                        break;
                    }
                }
            }
            break;
        }
    }

    void XcbInputDeviceMouse::PollSpecialEvents()
    {
        while (xcb_generic_event_t* genericEvent = xcb_poll_for_queued_event(s_xcbConnection))
        {
            // TODO Is the following correct? If we are showing the cursor, don't poll RAW Input events.
            switch (genericEvent->response_type & ~0x80)
            {
            case XCB_GE_GENERIC:
                {
                    const xcb_ge_generic_event_t* geGenericEvent = reinterpret_cast<const xcb_ge_generic_event_t*>(genericEvent);

                    // Only handle raw inputs if we have focus.
                    // Handle Raw Input events first.
                    if ((geGenericEvent->event_type == XCB_INPUT_RAW_BUTTON_PRESS) ||
                        (geGenericEvent->event_type == XCB_INPUT_RAW_BUTTON_RELEASE) ||
                        (geGenericEvent->event_type == XCB_INPUT_RAW_MOTION))
                    {
                        HandleRawInputEvents(geGenericEvent);

                        free(genericEvent);
                    }
                }
                break;
            }
        }
    }

    void XcbInputDeviceMouse::HandleXcbEvent(xcb_generic_event_t* event)
    {
        switch (event->response_type & ~0x80)
        {
        // QT5 is using by default XInput which means we do need to check for XCB_GE_GENERIC event to parse all mouse related events.
        case XCB_GE_GENERIC:
            {
                const xcb_ge_generic_event_t* genericEvent = reinterpret_cast<const xcb_ge_generic_event_t*>(event);

                // Handling RAW Inputs here works in GameMode but not in Editor mode because QT is
                // not handling RAW input events and passing to.
                if (!m_cursorShown)
                {
                    // Handle Raw Input events first.
                    if ((genericEvent->event_type == XCB_INPUT_RAW_BUTTON_PRESS) ||
                        (genericEvent->event_type == XCB_INPUT_RAW_BUTTON_RELEASE) || (genericEvent->event_type == XCB_INPUT_RAW_MOTION))
                    {
                        HandleRawInputEvents(genericEvent);
                    }
                }
                else
                {
                    switch (genericEvent->event_type)
                    {
                    case XCB_INPUT_BUTTON_PRESS:
                        {
                            const xcb_input_button_press_event_t* mouseButtonEvent =
                                reinterpret_cast<const xcb_input_button_press_event_t*>(genericEvent);
                            HandleButtonPressEvents(mouseButtonEvent->detail, true);
                        }
                        break;
                    case XCB_INPUT_BUTTON_RELEASE:
                        {
                            const xcb_input_button_release_event_t* mouseButtonEvent =
                                reinterpret_cast<const xcb_input_button_release_event_t*>(genericEvent);
                            HandleButtonPressEvents(mouseButtonEvent->detail, false);
                        }
                        break;
                    case XCB_INPUT_MOTION:
                        {
                            HandlePointerMotionEvents(event);
                        }
                        break;
                    }
                }
            }
            break;
        case XCB_FOCUS_IN:
            {
                const xcb_focus_in_event_t* focusInEvent = reinterpret_cast<const xcb_focus_in_event_t*>(event);
                if (m_focusWindow != focusInEvent->event)
                {
                    m_focusWindow = focusInEvent->event;
                    HandleCursorState(m_focusWindow, m_systemCursorState);
                }
            }
            break;
        case XCB_FOCUS_OUT:
            {
                const xcb_focus_out_event_t* focusOutEvent = reinterpret_cast<const xcb_focus_out_event_t*>(event);
                HandleCursorState(focusOutEvent->event, SystemCursorState::UnconstrainedAndVisible);

                ProcessRawEventQueues();
                ResetInputChannelStates();

                m_focusWindow = XCB_NONE;
            }
            break;
        }
    }
} // namespace AzFramework
