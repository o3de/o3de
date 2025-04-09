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
    xcb_window_t GetSystemCursorFocusWindow(xcb_connection_t* connection)
    {
        void* systemCursorFocusWindow = nullptr;
        AzFramework::InputSystemCursorConstraintRequestBus::BroadcastResult(
            systemCursorFocusWindow, &AzFramework::InputSystemCursorConstraintRequests::GetSystemCursorConstraintWindow);

        if (systemCursorFocusWindow)
        {
            return static_cast<xcb_window_t>(reinterpret_cast<uint64_t>(systemCursorFocusWindow));
        }

        // EWMH-compliant window managers set the "_NET_ACTIVE_WINDOW" property
        // of the X server's root window to the currently active window. This
        // retrieves value of that property.

        // Get the atom for the _NET_ACTIVE_WINDOW property
        constexpr int propertyNameLength = 18;
        xcb_generic_error_t* error = nullptr;
        XcbStdFreePtr<xcb_intern_atom_reply_t> activeWindowAtom {xcb_intern_atom_reply(
            connection,
            xcb_intern_atom(connection, /*only_if_exists=*/ 1, propertyNameLength, "_NET_ACTIVE_WINDOW"),
            &error
        )};
        if (!activeWindowAtom || error)
        {
            if (error)
            {
                AZ_Warning("XcbInput", false, "Retrieving _NET_ACTIVE_WINDOW atom failed : Error code %d", error->error_code);
                free(error);
            }
            return XCB_WINDOW_NONE;
        }

        // Get the root window
        const xcb_window_t rootWId = xcb_setup_roots_iterator(xcb_get_setup(connection)).data->root;

        // Fetch the value of the root window's _NET_ACTIVE_WINDOW property
        XcbStdFreePtr<xcb_get_property_reply_t> property {xcb_get_property_reply(
            connection,
            xcb_get_property(
                /*c=*/connection,
                /*_delete=*/ 0,
                /*window=*/rootWId,
                /*property=*/activeWindowAtom->atom,
                /*type=*/XCB_ATOM_WINDOW,
                /*long_offset=*/0,
                /*long_length=*/1
            ),
            &error
        )};

        if (!property || error)
        {
            if (error)
            {
                AZ_Warning("XcbInput", false, "Retrieving _NET_ACTIVE_WINDOW atom failed : Error code %d", error->error_code);
                free(error);
            }
            return XCB_WINDOW_NONE;
        }

        return *static_cast<xcb_window_t*>(xcb_get_property_value(property.get()));
    }

    xcb_connection_t* XcbInputDeviceMouse::s_xcbConnection = nullptr;
    xcb_screen_t* XcbInputDeviceMouse::s_xcbScreen = nullptr;
    bool XcbInputDeviceMouse::m_xfixesInitialized = false;
    bool XcbInputDeviceMouse::m_xInputInitialized = false;

    XcbInputDeviceMouse::XcbInputDeviceMouse(InputDeviceMouse& inputDevice)
        : InputDeviceMouse::Implementation(inputDevice)
        , m_systemCursorState(SystemCursorState::Unknown)
        , m_systemCursorPositionNormalized(0.5f, 0.5f)
        , m_focusWindow(XCB_WINDOW_NONE)
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
        const auto* interface = AzFramework::XcbConnectionManagerInterface::Get();
        if (!interface)
        {
            AZ_Warning("XcbInput", false, "XCB interface not available");
            return nullptr;
        }

        s_xcbConnection = interface->GetXcbConnection();
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
        if (AZ::Debug::Trace::Instance().IsDebuggerPresent())
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
                s_xcbConnection, xcb_get_geometry(s_xcbConnection, window), nullptr) };

            if (!xcbGeometryReply)
            {
                return;
            }

            const xcb_translate_coordinates_cookie_t translate_coord =
                xcb_translate_coordinates(s_xcbConnection, window, s_xcbScreen->root, 0, 0);

            const XcbStdFreePtr<xcb_translate_coordinates_reply_t> xkbTranslateCoordReply{ xcb_translate_coordinates_reply(
                s_xcbConnection, translate_coord, nullptr) };

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
                    s_xcbConnection, barrier.id, window, barrier.x0, barrier.y0, barrier.x1, barrier.y1, barrier.direction, 0, nullptr);
                const XcbStdFreePtr<xcb_generic_error_t> xcbError{ xcb_request_check(s_xcbConnection, cookie) };

                AZ_Warning(
                    "XcbInput", !xcbError, "XFixes, failed to create barrier %d at (%d %d %d %d)", barrier.id, barrier.x0, barrier.y0,
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

        xcb_generic_error_t* error = nullptr;
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

        xcb_generic_error_t* error = nullptr;
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

    void XcbInputDeviceMouse::SetSystemCursorState(SystemCursorState systemCursorState)
    {
        if (m_captureCursor) {
            if (systemCursorState != m_systemCursorState) {
                m_systemCursorState = systemCursorState;

                m_focusWindow = GetSystemCursorFocusWindow(s_xcbConnection);

                HandleCursorState(m_focusWindow, systemCursorState);
            }

        }
    }

    void XcbInputDeviceMouse::HandleCursorState(xcb_window_t window, SystemCursorState systemCursorState)
    {
        if (m_captureCursor)
        {
            const bool confined = (systemCursorState == SystemCursorState::ConstrainedAndHidden) ||
                (systemCursorState == SystemCursorState::ConstrainedAndVisible);
            const bool cursorShown = (systemCursorState == SystemCursorState::ConstrainedAndVisible) ||
                (systemCursorState == SystemCursorState::UnconstrainedAndVisible);

            CreateBarriers(window, confined);
            ShowCursor(window, cursorShown);
        }
    }

    SystemCursorState XcbInputDeviceMouse::GetSystemCursorState() const
    {
        return m_systemCursorState;
    }

    void XcbInputDeviceMouse::SetSystemCursorPositionNormalizedInternal(xcb_window_t window, AZ::Vector2 positionNormalized)
    {
        // TODO Basically not done at all. Added only the basic functions needed.
        const XcbStdFreePtr<xcb_get_geometry_reply_t> xcbGeometryReply{ xcb_get_geometry_reply(
            s_xcbConnection, xcb_get_geometry(s_xcbConnection, window), nullptr) };

        if (!xcbGeometryReply)
        {
            return;
        }

        const int16_t x = static_cast<int16_t>(positionNormalized.GetX() * xcbGeometryReply->width);
        const int16_t y = static_cast<int16_t>(positionNormalized.GetY() * xcbGeometryReply->height);

        xcb_warp_pointer(s_xcbConnection, XCB_WINDOW_NONE, window, 0, 0, 0, 0, x, y);

        xcb_flush(s_xcbConnection);
    }

    void XcbInputDeviceMouse::SetSystemCursorPositionNormalized(AZ::Vector2 positionNormalized)
    {
        const xcb_window_t window = GetSystemCursorFocusWindow(s_xcbConnection);
        if (XCB_WINDOW_NONE == window)
        {
            return;
        }

        SetSystemCursorPositionNormalizedInternal(window, positionNormalized);
    }

    AZ::Vector2 XcbInputDeviceMouse::GetSystemCursorPositionNormalizedInternal(xcb_window_t window) const
    {
        AZ::Vector2 position = AZ::Vector2::CreateZero();

        const xcb_query_pointer_cookie_t pointer = xcb_query_pointer(s_xcbConnection, window);

        const XcbStdFreePtr<xcb_query_pointer_reply_t> xkbQueryPointerReply{ xcb_query_pointer_reply(s_xcbConnection, pointer, nullptr) };

        if (!xkbQueryPointerReply)
        {
            return position;
        }

        const XcbStdFreePtr<xcb_get_geometry_reply_t> xkbGeometryReply{ xcb_get_geometry_reply(
            s_xcbConnection, xcb_get_geometry(s_xcbConnection, window), nullptr) };

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
        const xcb_window_t window = GetSystemCursorFocusWindow(s_xcbConnection);
        if (XCB_WINDOW_NONE == window)
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

        const XcbStdFreePtr<xcb_generic_error_t> xcbError{ xcb_request_check(s_xcbConnection, cookie) };

        if (xcbError)
        {
            AZ_Warning("XcbInput", false, "ShowCursor failed: %d", xcbError->error_code);

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

                // Valuators in XCB are things that don't have a binary set of states but rather a range of values.
                // In this case, they represent an axis of motion.

                // The valuator mask is a bitset of which axes have changed and have values in this motion event.
                //  For every bit set in the valuator mask, there will be a corresponding entry in the axis values, 
                // and the length of the axis values array will be the number of bits that are set in this valuator mask.

                // For example, x and y movement mouse axis movement will have bits 0x1 (x) or 0x2 (y), or 0x3 if both.
                // In testing, I always saw 'both' (so 0x3) for all mouse movements.  There are no constants for these
                // that I can find since each bit just represents a possible axis with the convention being the first two
                // being the first two axes of the device, which for mice, is going to be the normal x and y axes.

                // Anything beyond that is a completely different axis, such as the mouse wheel, z axis, other things
                // special mice can do.
                uint32_t *mask = xcb_input_raw_button_press_valuator_mask(mouseMotionEvent);
                if (!mask)
                {
                    // something is broken with this event - at the very least, it cannot contain mouse movement.
                    break;
                }

                if ((mask[0] & 0x3) == 0)
                {
                    // x and y movement are not present in the mask.  This is not necessarily a wheel roll,
                    // but its also not x and y movement.  What it is depends on how many axes the device actually has.
                    // Since we capture wheel roll as button presses (above), ignore this.
                    break;
                }

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

    void XcbInputDeviceMouse::HandleXcbEvent(xcb_generic_event_t* event)
    {
        switch (event->response_type & ~0x80)
        {
        // XInput raw events are sent from the server as a XCB_GE_GENERIC
        // event. A XCB_GE_GENERIC event is typecast to a
        // xcb_ge_generic_event_t, which is distinct from a
        // xcb_generic_event_t, and exists so that X11 extensions can extend
        // the event emission beyond the size that a normal X11 event could
        // contain.
        case XCB_GE_GENERIC:
            {
                const xcb_ge_generic_event_t* genericEvent = reinterpret_cast<const xcb_ge_generic_event_t*>(event);
                HandleRawInputEvents(genericEvent);
            }
            break;
        case XCB_FOCUS_IN:
            {
                const xcb_focus_in_event_t* focusInEvent = reinterpret_cast<const xcb_focus_in_event_t*>(event);
                if (m_focusWindow != focusInEvent->event)
                {
                    if (m_focusWindow != XCB_WINDOW_NONE)
                    {
                        HandleCursorState(m_focusWindow, SystemCursorState::UnconstrainedAndVisible);
                    }

                    m_focusWindow = focusInEvent->event;

                    // If the cursor state is Unknown, then calling HandleCursorState would hide the cursor, 
                    // but we should only hide the cursor if m_systemCursorState is explicitly hidden.
                    if(SystemCursorState::Unknown != m_systemCursorState) 
                    {
                        HandleCursorState(m_focusWindow, m_systemCursorState);
                    }
                }

                auto* interface = AzFramework::XcbConnectionManagerInterface::Get();
                interface->SetEnableXInput(interface->GetXcbConnection(), true);
            }
            break;
        case XCB_FOCUS_OUT:
            {
                const xcb_focus_out_event_t* focusOutEvent = reinterpret_cast<const xcb_focus_out_event_t*>(event);
                HandleCursorState(focusOutEvent->event, SystemCursorState::UnconstrainedAndVisible);

                ProcessRawEventQueues();
                ResetInputChannelStates();

                m_focusWindow = XCB_WINDOW_NONE;

                auto* interface = AzFramework::XcbConnectionManagerInterface::Get();
                interface->SetEnableXInput(interface->GetXcbConnection(), false);
            }
            break;
        }
    }
} // namespace AzFramework
