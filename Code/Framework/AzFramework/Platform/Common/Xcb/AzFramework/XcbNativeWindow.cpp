/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Application/Application.h>
#include <AzFramework/Windowing/NativeWindow.h>
#include <AzFramework/XcbConnectionManager.h>
#include <AzFramework/XcbInterface.h>
#include <AzFramework/XcbNativeWindow.h>

#include <xcb/xcb.h>

namespace AzFramework
{
    [[maybe_unused]] const char XcbErrorWindow[] = "XcbNativeWindow";
    static constexpr uint8_t s_XcbFormatDataSize = 32; // Format indicator for xcb for client messages
    static constexpr uint16_t s_DefaultXcbWindowBorderWidth = 4; // The default border with in pixels if a border was specified
    static constexpr uint8_t s_XcbResponseTypeMask = 0x7f; // Mask to extract the specific event type from an xcb event

#define _NET_WM_STATE_REMOVE 0l
#define _NET_WM_STATE_ADD 1l
#define _NET_WM_STATE_TOGGLE 2l

    ////////////////////////////////////////////////////////////////////////////////////////////////
    XcbNativeWindow::XcbNativeWindow()
        : NativeWindow::Implementation()
        , m_xcbConnection(nullptr)
        , m_xcbRootScreen(nullptr)
        , m_xcbWindow(XCB_NONE)
    {
        if (auto xcbConnectionManager = AzFramework::XcbConnectionManagerInterface::Get(); xcbConnectionManager != nullptr)
        {
            m_xcbConnection = xcbConnectionManager->GetXcbConnection();
        }
        AZ_Error(XcbErrorWindow, m_xcbConnection != nullptr, "Unable to get XCB Connection");
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    XcbNativeWindow::~XcbNativeWindow()
    {
        if (XCB_NONE != m_xcbWindow)
        {
            xcb_destroy_window(m_xcbConnection, m_xcbWindow);
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void XcbNativeWindow::InitWindow(const AZStd::string& title, const WindowGeometry& geometry, const WindowStyleMasks& styleMasks)
    {
        // Get the parent window
        const xcb_setup_t* xcbSetup = xcb_get_setup(m_xcbConnection);
        m_xcbRootScreen = xcb_setup_roots_iterator(xcbSetup).data;
        xcb_window_t xcbParentWindow = m_xcbRootScreen->root;

        // Create an XCB window from the connection
        m_xcbWindow = xcb_generate_id(m_xcbConnection);

        uint16_t borderWidth = 0;
        const uint32_t mask = styleMasks.m_platformAgnosticStyleMask;
        if ((mask & WindowStyleMasks::WINDOW_STYLE_BORDERED) || (mask & WindowStyleMasks::WINDOW_STYLE_RESIZEABLE))
        {
            borderWidth = s_DefaultXcbWindowBorderWidth;
        }

        uint32_t eventMask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;

        const uint32_t interestedEvents = XCB_EVENT_MASK_STRUCTURE_NOTIFY | XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_KEY_RELEASE |
            XCB_EVENT_MASK_FOCUS_CHANGE | XCB_EVENT_MASK_PROPERTY_CHANGE;
        uint32_t valueList[] = { m_xcbRootScreen->black_pixel, interestedEvents };

        xcb_void_cookie_t xcbCheckResult;

        xcbCheckResult = xcb_create_window_checked(
            m_xcbConnection, XCB_COPY_FROM_PARENT, m_xcbWindow, xcbParentWindow, aznumeric_cast<int16_t>(geometry.m_posX),
            aznumeric_cast<int16_t>(geometry.m_posY), aznumeric_cast<int16_t>(geometry.m_width), aznumeric_cast<int16_t>(geometry.m_height),
            borderWidth, XCB_WINDOW_CLASS_INPUT_OUTPUT, m_xcbRootScreen->root_visual, eventMask, valueList);

        AZ_Assert(ValidateXcbResult(xcbCheckResult), "Failed to create xcb window.");

        SetWindowTitle(title);

        m_posX = geometry.m_posX;
        m_posY = geometry.m_posY;
        m_width = geometry.m_width;
        m_height = geometry.m_height;

        InitializeAtoms();

        xcb_client_message_event_t event;
        event.response_type = XCB_CLIENT_MESSAGE;
        event.type = _NET_REQUEST_FRAME_EXTENTS;
        event.window = m_xcbWindow;
        event.format = 32;
        event.sequence = 0;
        event.data.data32[0] = 0l;
        event.data.data32[1] = 0l;
        event.data.data32[2] = 0l;
        event.data.data32[3] = 0l;
        event.data.data32[4] = 0l;
        xcbCheckResult = xcb_send_event(
            m_xcbConnection, 1, m_xcbRootScreen->root, XCB_EVENT_MASK_STRUCTURE_NOTIFY | XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT,
            (const char*)&event);
        AZ_Assert(ValidateXcbResult(xcbCheckResult), "Failed to set _NET_REQUEST_FRAME_EXTENTS");

        // The WM will be able to kill the application if it gets unresponsive.
        int32_t pid = getpid();
        xcb_change_property(m_xcbConnection, XCB_PROP_MODE_REPLACE, m_xcbWindow, _NET_WM_PID, XCB_ATOM_CARDINAL, 32, 1, &pid);

        xcb_flush(m_xcbConnection);
    }

    xcb_atom_t XcbNativeWindow::GetAtom(const char* atomName)
    {
        xcb_intern_atom_cookie_t intern_atom_cookie = xcb_intern_atom(m_xcbConnection, 0, strlen(atomName), atomName);
        XcbStdFreePtr<xcb_intern_atom_reply_t> xkbinternAtom{ xcb_intern_atom_reply(m_xcbConnection, intern_atom_cookie, NULL) };

        if (!xkbinternAtom)
        {
            AZ_Error(XcbErrorWindow, xkbinternAtom != nullptr, "Unable to query xcb '%s' atom", atomName);
            return XCB_NONE;
        }

        return xkbinternAtom->atom;
    }

    int XcbNativeWindow::SetAtom(xcb_window_t window, xcb_atom_t atom, xcb_atom_t type, size_t len, void* data)
    {
        xcb_void_cookie_t cookie = xcb_change_property_checked(m_xcbConnection, XCB_PROP_MODE_REPLACE, window, atom, type, 32, len, data);
        XcbStdFreePtr<xcb_generic_error_t> xkbError{ xcb_request_check(m_xcbConnection, cookie) };

        if (!xkbError)
        {
            return 0;
        }

        return xkbError->error_code;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////

    void XcbNativeWindow::InitializeAtoms()
    {
        AZStd::vector<xcb_atom_t> Atoms;

        _NET_ACTIVE_WINDOW = GetAtom("_NET_ACTIVE_WINDOW");
        _NET_WM_BYPASS_COMPOSITOR = GetAtom("_NET_WM_BYPASS_COMPOSITOR");

        // ---------------------------------------------------------------------
        // Handle all WM Protocols atoms.
        //

        WM_PROTOCOLS = GetAtom("WM_PROTOCOLS");

        // This atom is used to close a window. Emitted when user clicks the close button.
        WM_DELETE_WINDOW = GetAtom("WM_DELETE_WINDOW");

        Atoms.push_back(WM_DELETE_WINDOW);

        xcb_change_property(
            m_xcbConnection, XCB_PROP_MODE_REPLACE, m_xcbWindow, WM_PROTOCOLS, XCB_ATOM_ATOM, 32, Atoms.size(), Atoms.data());

        xcb_flush(m_xcbConnection);

        // ---------------------------------------------------------------------
        // Handle all WM State atoms.
        //

        _NET_WM_STATE = GetAtom("_NET_WM_STATE");
        _NET_WM_STATE_FULLSCREEN = GetAtom("_NET_WM_STATE_FULLSCREEN");
        _NET_WM_STATE_MAXIMIZED_VERT = GetAtom("_NET_WM_STATE_MAXIMIZED_VERT");
        _NET_WM_STATE_MAXIMIZED_HORZ = GetAtom("_NET_WM_STATE_MAXIMIZED_HORZ");
        _NET_MOVERESIZE_WINDOW = GetAtom("_NET_MOVERESIZE_WINDOW");
        _NET_REQUEST_FRAME_EXTENTS = GetAtom("_NET_REQUEST_FRAME_EXTENTS");
        _NET_FRAME_EXTENTS = GetAtom("_NET_FRAME_EXTENTS");
        _NET_WM_PID = GetAtom("_NET_WM_PID");
    }

    void XcbNativeWindow::GetWMStates()
    {
        xcb_get_property_cookie_t cookie = xcb_get_property(m_xcbConnection, 0, m_xcbWindow, _NET_WM_STATE, XCB_ATOM_ATOM, 0, 1024);

        xcb_generic_error_t* error = nullptr;
        XcbStdFreePtr<xcb_get_property_reply_t> xkbGetPropertyReply{ xcb_get_property_reply(m_xcbConnection, cookie, &error) };

        if (!xkbGetPropertyReply || error || !((xkbGetPropertyReply->format == 32) && (xkbGetPropertyReply->type == XCB_ATOM_ATOM)))
        {
            AZ_Warning("ApplicationLinux", false, "Acquiring _NET_WM_STATE information from the WM failed.");

            if (error)
            {
                AZ_TracePrintf("Error", "Error code %d", error->error_code);
                free(error);
            }
            return;
        }

        m_fullscreenState = false;
        m_horizontalyMaximized = false;
        m_verticallyMaximized = false;

        const xcb_atom_t* states = static_cast<const xcb_atom_t*>(xcb_get_property_value(xkbGetPropertyReply.get()));
        for (int i = 0; i < xkbGetPropertyReply->length; i++)
        {
            if (states[i] == _NET_WM_STATE_FULLSCREEN)
            {
                m_fullscreenState = true;
            }
            else if (states[i] == _NET_WM_STATE_MAXIMIZED_HORZ)
            {
                m_horizontalyMaximized = true;
            }
            else if (states[i] == _NET_WM_STATE_MAXIMIZED_VERT)
            {
                m_verticallyMaximized = true;
            }
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void XcbNativeWindow::Activate()
    {
        XcbEventHandlerBus::Handler::BusConnect();

        if (!m_activated) // nothing to do if window was already activated
        {
            m_activated = true;

            xcb_map_window(m_xcbConnection, m_xcbWindow);
            xcb_flush(m_xcbConnection);
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void XcbNativeWindow::Deactivate()
    {
        if (m_activated) // nothing to do if window was already deactivated
        {
            m_activated = false;

            WindowNotificationBus::Event(reinterpret_cast<NativeWindowHandle>(m_xcbWindow), &WindowNotificationBus::Events::OnWindowClosed);

            xcb_unmap_window(m_xcbConnection, m_xcbWindow);
            xcb_flush(m_xcbConnection);
        }
        XcbEventHandlerBus::Handler::BusDisconnect();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    NativeWindowHandle XcbNativeWindow::GetWindowHandle() const
    {
        return reinterpret_cast<NativeWindowHandle>(m_xcbWindow);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void XcbNativeWindow::SetWindowTitle(const AZStd::string& title)
    {
        // Set the title of both the window and the task bar by using
        // a buffer to hold the title twice, separated by a null-terminator
        auto doubleTitleSize = (title.size() + 1) * 2;
        AZStd::string doubleTitle(doubleTitleSize, '\0');
        azstrncpy(doubleTitle.data(), doubleTitleSize, title.c_str(), title.size());
        azstrncpy(&doubleTitle.data()[title.size() + 1], title.size(), title.c_str(), title.size());

        xcb_void_cookie_t xcbCheckResult;
        xcbCheckResult = xcb_change_property(
            m_xcbConnection, XCB_PROP_MODE_REPLACE, m_xcbWindow, XCB_ATOM_WM_CLASS, XCB_ATOM_STRING, 8, static_cast<uint32_t>(doubleTitle.size()),
            doubleTitle.c_str());
        AZ_Assert(ValidateXcbResult(xcbCheckResult), "Failed to set window title.");
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void XcbNativeWindow::ResizeClientArea(WindowSize clientAreaSize)
    {
        const uint32_t values[] = { clientAreaSize.m_width, clientAreaSize.m_height };

        xcb_configure_window(m_xcbConnection, m_xcbWindow, XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT, values);

        m_width = clientAreaSize.m_width;
        m_height = clientAreaSize.m_height;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    uint32_t XcbNativeWindow::GetDisplayRefreshRate() const
    {
        // [GFX TODO][GHI - 2678]
        // Using 60 for now until proper support is added

        return 60;
    }

    bool XcbNativeWindow::GetFullScreenState() const
    {
        return m_fullscreenState;
    }

    void XcbNativeWindow::SetFullScreenState(bool fullScreenState)
    {
        // TODO This is a pretty basic full-screen implementation using WM's _NET_WM_STATE_FULLSCREEN state.
        // Do we have to provide also the old way?

        GetWMStates();

        xcb_client_message_event_t event;
        event.response_type = XCB_CLIENT_MESSAGE;
        event.type = _NET_WM_STATE;
        event.window = m_xcbWindow;
        event.format = 32;
        event.sequence = 0;
        event.data.data32[0] = fullScreenState ? _NET_WM_STATE_ADD : _NET_WM_STATE_REMOVE;
        event.data.data32[1] = _NET_WM_STATE_FULLSCREEN;
        event.data.data32[2] = 0;
        event.data.data32[3] = 1;
        event.data.data32[4] = 0;
        [[maybe_unused]] xcb_void_cookie_t xcbCheckResult = xcb_send_event(
            m_xcbConnection, 1, m_xcbRootScreen->root, XCB_EVENT_MASK_STRUCTURE_NOTIFY | XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT,
            (const char*)&event);
        AZ_Assert(ValidateXcbResult(xcbCheckResult), "Failed to set _NET_WM_STATE_FULLSCREEN");

        // Also try to disable/enable the compositor if possible. Might help in some cases.
        const long _NET_WM_BYPASS_COMPOSITOR_HINT_ON = m_fullscreenState ? 1 : 0;
        SetAtom(m_xcbWindow, _NET_WM_BYPASS_COMPOSITOR, XCB_ATOM_CARDINAL, 32, (char*)&_NET_WM_BYPASS_COMPOSITOR_HINT_ON);

        if (!fullScreenState)
        {
            if (m_horizontalyMaximized || m_verticallyMaximized)
            {
                printf("Remove maximized state.\n");
                xcb_client_message_event_t event;
                event.response_type = XCB_CLIENT_MESSAGE;
                event.type = _NET_WM_STATE;
                event.window = m_xcbWindow;
                event.format = 32;
                event.sequence = 0;
                event.data.data32[0] = _NET_WM_STATE_MAXIMIZED_VERT;
                event.data.data32[1] = _NET_WM_STATE_MAXIMIZED_HORZ;
                event.data.data32[2] = 0;
                event.data.data32[3] = 0;
                event.data.data32[4] = 0;
                [[maybe_unused]] xcb_void_cookie_t xcbCheckResult = xcb_send_event(
                    m_xcbConnection, 1, m_xcbRootScreen->root, XCB_EVENT_MASK_STRUCTURE_NOTIFY | XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT,
                    (const char*)&event);
                AZ_Assert(
                    ValidateXcbResult(xcbCheckResult), "Failed to remove _NET_WM_STATE_MAXIMIZED_VERT | _NET_WM_STATE_MAXIMIZED_HORZ");
            }
        }

        xcb_flush(m_xcbConnection);
        m_fullscreenState = fullScreenState;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool XcbNativeWindow::ValidateXcbResult(xcb_void_cookie_t cookie)
    {
        bool result = true;
        if (xcb_generic_error_t* error = xcb_request_check(m_xcbConnection, cookie))
        {
            AZ_TracePrintf("Error", "Error code %d", error->error_code);
            result = false;
        }
        return result;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void XcbNativeWindow::HandleXcbEvent(xcb_generic_event_t* event)
    {
        switch (event->response_type & s_XcbResponseTypeMask)
        {
        case XCB_CONFIGURE_NOTIFY:
            {
                xcb_configure_notify_event_t* cne = reinterpret_cast<xcb_configure_notify_event_t*>(event);
                if ((cne->width != m_width) || (cne->height != m_height))
                {
                    WindowSizeChanged(aznumeric_cast<uint32_t>(cne->width), aznumeric_cast<uint32_t>(cne->height));
                }
                break;
            }
        case XCB_CLIENT_MESSAGE:
            {
                xcb_client_message_event_t* cme = reinterpret_cast<xcb_client_message_event_t*>(event);

                if ((cme->type == WM_PROTOCOLS) && (cme->format == s_XcbFormatDataSize) && (cme->data.data32[0] == WM_DELETE_WINDOW))
                {
                    Deactivate();

                    ApplicationRequests::Bus::Broadcast(&ApplicationRequests::ExitMainLoop);
                }
                break;
            }
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void XcbNativeWindow::WindowSizeChanged(const uint32_t width, const uint32_t height)
    {
        if (m_width != width || m_height != height)
        {
            m_width = width;
            m_height = height;

            if (m_activated)
            {
                WindowNotificationBus::Event(
                    reinterpret_cast<NativeWindowHandle>(m_xcbWindow), &WindowNotificationBus::Events::OnWindowResized, width, height);
            }
        }
    }
} // namespace AzFramework
