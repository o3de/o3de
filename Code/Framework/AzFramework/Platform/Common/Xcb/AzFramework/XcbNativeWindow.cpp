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
#include <xcb/randr.h>

namespace AzFramework
{
    [[maybe_unused]] const char XcbErrorWindow[] = "XcbNativeWindow";
    static constexpr uint8_t s_XcbFormatDataSize = 32; // Format indicator for xcb for client messages
    static constexpr uint16_t s_DefaultXcbWindowBorderWidth = 4; // The default border with in pixels if a border was specified

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
    void XcbNativeWindow::InitWindowInternal(const AZStd::string& title, const WindowGeometry& geometry, const WindowStyleMasks& styleMasks)
    {
        // Get the parent window
        const xcb_setup_t* xcbSetup = xcb_get_setup(m_xcbConnection);
        m_xcbRootScreen = xcb_setup_roots_iterator(xcbSetup).data;
        xcb_window_t xcbParentWindow = m_xcbRootScreen->root;

        // Create an XCB window from the connection
        m_xcbWindow = xcb_generate_id(m_xcbConnection);

        uint16_t borderWidth = 0;
        m_styleMask = styleMasks.m_platformAgnosticStyleMask;
        if ((m_styleMask & WindowStyleMasks::WINDOW_STYLE_BORDERED) || (m_styleMask & WindowStyleMasks::WINDOW_STYLE_RESIZEABLE))
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

        SetWindowSizeHints();

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

    void XcbNativeWindow::SetWindowSizeHints()
    {
        struct XcbSizeHints
        {
            uint32_t flags;
            int32_t  x, y;
            int32_t  width, height;
            int32_t  minWidth, minHeight;
            int32_t  maxWidth, maxHeight;
            int32_t  widthInc, heightInc;
            int32_t  minAspectNum, minAspectDen;
            int32_t  maxAspectNum, maxAspectDen;
            int32_t  baseWidth, baseHeight;
            uint32_t winGravity;
        };

        enum XcbSizeHintsFlags
        {
            USPosition  = 1U << 0,
            USSize      = 1U << 1,
            PPosition   = 1U << 2,
            PSize       = 1U << 3,
            PMinSize    = 1U << 4,
            PMaxSize    = 1U << 5,
            PResizeInc  = 1U << 6,
            PAspect     = 1U << 7,
            PWinGravity = 1U << 9
        };

        XcbSizeHints hints{};

        if ((m_styleMask & WindowStyleMasks::WINDOW_STYLE_RESIZEABLE) == 0)
        {
            hints.flags      |= XcbSizeHintsFlags::PMaxSize | XcbSizeHintsFlags::PMinSize,
            hints.minWidth   = static_cast<int32_t>(m_width);
            hints.minHeight  = static_cast<int32_t>(m_height);
            hints.maxWidth   = static_cast<int32_t>(m_width);
            hints.maxHeight  = static_cast<int32_t>(m_height);
        }

        xcb_void_cookie_t xcbCheckResult;
        xcbCheckResult = xcb_change_property(m_xcbConnection,
                                             XCB_PROP_MODE_REPLACE,
                                             m_xcbWindow,
                                             XCB_ATOM_WM_NORMAL_HINTS,
                                             XCB_ATOM_WM_SIZE_HINTS,
                                             32,
                                             18,
                                             &hints);
        AZ_Assert(ValidateXcbResult(xcbCheckResult), "Failed to set window size hints.");
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

        _NET_ACTIVE_WINDOW = GetAtom("_NET_ACTIVE_WINDOW");
        _NET_WM_BYPASS_COMPOSITOR = GetAtom("_NET_WM_BYPASS_COMPOSITOR");

        // ---------------------------------------------------------------------
        // Handle all WM Protocols atoms.
        //

        WM_PROTOCOLS = GetAtom("WM_PROTOCOLS");

        // This atom is used to close a window. Emitted when user clicks the close button.
        WM_DELETE_WINDOW = GetAtom("WM_DELETE_WINDOW");
        _NET_WM_PING = GetAtom("_NET_WM_PING");

        const AZStd::array atoms {WM_DELETE_WINDOW, _NET_WM_PING};

        xcb_change_property(
            m_xcbConnection, XCB_PROP_MODE_REPLACE, m_xcbWindow, WM_PROTOCOLS, XCB_ATOM_ATOM, 32, atoms.size(), atoms.data());

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
    void XcbNativeWindow::ResizeClientArea(WindowSize clientAreaSize, const WindowPosOptions& options)
    {
        const uint32_t values[] = { clientAreaSize.m_width, clientAreaSize.m_height };
        
        if (m_activated)
        {
            xcb_unmap_window(m_xcbConnection, m_xcbWindow);
        }
        xcb_configure_window(m_xcbConnection, m_xcbWindow, XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT, values);
        
        if (m_activated)
        {
            xcb_map_window(m_xcbConnection, m_xcbWindow);
            xcb_flush(m_xcbConnection);
        }
        //Notify the RHI to rebuild swapchain and swapchain images after updating the surface
        WindowSizeChanged(clientAreaSize.m_width, clientAreaSize.m_height);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool XcbNativeWindow::SupportsClientAreaResize() const
    {
        return true;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    uint32_t XcbNativeWindow::GetDisplayRefreshRate() const
    {
        constexpr uint32_t defaultRefreshRate = 60;
        xcb_generic_error_t* error = nullptr;

        xcb_screen_iterator_t iter = xcb_setup_roots_iterator(xcb_get_setup(m_xcbConnection));
        xcb_screen_t* screen = iter.data;

        const xcb_query_extension_reply_t* extReply = xcb_get_extension_data(m_xcbConnection, &xcb_randr_id);
        if (!extReply || !extReply->present)
        {
            AZ_WarningOnce(XcbErrorWindow, extReply != nullptr, "Failed to get extension RandR, defaulting to display rate of %d", defaultRefreshRate);
            return defaultRefreshRate;
        }

        int positionX = 0;
        int positionY = 0;

        //Get the position of the window globally.
        {
            xcb_translate_coordinates_cookie_t translateCookie = xcb_translate_coordinates(
                m_xcbConnection,
                m_xcbWindow,
                screen->root,
                0,
                0
                );
            xcb_translate_coordinates_reply_t* translateReply = xcb_translate_coordinates_reply(
                m_xcbConnection,
                translateCookie,
                &error
                );

            if (error || translateReply == nullptr)
            {
                AZ_Warning(XcbErrorWindow, error != nullptr, "Failed to translate window coordinates.");
                free(error);
                free(translateReply);
                return defaultRefreshRate;
            }

            positionX = translateReply->dst_x;
            positionY = translateReply->dst_y;
            free(translateReply);
        }

        xcb_randr_get_screen_resources_current_cookie_t resCookie = xcb_randr_get_screen_resources_current(m_xcbConnection, screen->root);
        xcb_randr_get_screen_resources_current_reply_t* resReply = xcb_randr_get_screen_resources_current_reply(
            m_xcbConnection,
            resCookie,
            &error);

        if (error || resReply == nullptr)
        {
            AZ_Warning(XcbErrorWindow, error != nullptr, "Failed to get screen resources.");
            free(error);
            return defaultRefreshRate;
        }

        xcb_randr_crtc_t* crtcs = xcb_randr_get_screen_resources_current_crtcs(resReply);
        int crtcLength = xcb_randr_get_screen_resources_current_crtcs_length(resReply);

        uint32_t refreshRate = 0;

        for (int i = 0; i < crtcLength; i++)
        {
            xcb_randr_get_crtc_info_cookie_t crtcInfoCookie = xcb_randr_get_crtc_info(m_xcbConnection, crtcs[i], XCB_TIME_CURRENT_TIME);
            xcb_randr_get_crtc_info_reply_t* crtcInfoReply = xcb_randr_get_crtc_info_reply(m_xcbConnection, crtcInfoCookie, &error);

            if (error || crtcInfoReply == nullptr)
            {
                free(error);
                continue;
            }

            int crtcX = crtcInfoReply->x;
            int crtcY = crtcInfoReply->y;
            int crtcW = crtcInfoReply->width;
            int crtcH = crtcInfoReply->height;

            if (positionX + m_width > crtcX && positionX < crtcX + crtcW &&
                positionY + m_height > crtcY && positionY < crtcY + crtcH)
            {
                const uint32_t modeId = crtcInfoReply->mode;

                xcb_randr_mode_info_t* modes = xcb_randr_get_screen_resources_current_modes(resReply);
                int modesLength = xcb_randr_get_screen_resources_current_modes_length(resReply);

                for (int mi = 0; mi < modesLength; mi++)
                {
                    auto mode = modes[mi];
                    if (mode.id != modeId)
                        continue;

                    if(mode.htotal == 0 || mode.vtotal == 0)
                    {
                        AZ_WarningOnce(XcbErrorWindow, false, "V/H total is 0");
                        return defaultRefreshRate;
                    }

                    double refreshRatePrecise = static_cast<double>(mode.dot_clock) /
                        (mode.htotal * mode.vtotal);

                    //Most of the time the refresh rate is 159.798 or 59.98
                    refreshRate = static_cast<uint32_t>(ceil(refreshRatePrecise));
                }

                free(crtcInfoReply);
                break;
            }

            free(crtcInfoReply);
        }

        if (refreshRate == 0)
        {
            AZ_Warning(XcbErrorWindow, refreshRate == 0, "Failed to get CRTC refresh rate for window.");
            free(resReply);
            return defaultRefreshRate;
        }

        free(resReply);
        return refreshRate;
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

                if ((cme->type == WM_PROTOCOLS) && (cme->format == s_XcbFormatDataSize))
                {
                    const xcb_atom_t protocolAtom = cme->data.data32[0];
                    if (protocolAtom == WM_DELETE_WINDOW)
                    {
                        Deactivate();

                        ApplicationRequests::Bus::Broadcast(&ApplicationRequests::ExitMainLoop);
                    }
                    else if (protocolAtom == _NET_WM_PING && cme->window != m_xcbRootScreen->root)
                    {
                        xcb_client_message_event_t reply = *cme;
                        reply.response_type = XCB_CLIENT_MESSAGE;
                        reply.window = m_xcbRootScreen->root;

                        xcb_send_event(m_xcbConnection, 0, m_xcbRootScreen->root, XCB_EVENT_MASK_STRUCTURE_NOTIFY | XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT, reinterpret_cast<const char*>(&reply));
                        xcb_flush(m_xcbConnection);
                    }
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
