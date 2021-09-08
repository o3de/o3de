/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/API/ApplicationAPI_Platform.h>
#include <AzFramework/Application/Application.h>
#include <AzFramework/Windowing/NativeWindow.h>
#include <xcb/xcb.h>

#include "NativeWindow_Linux_xcb.h"

namespace AzFramework
{
#if PAL_TRAIT_LINUX_WINDOW_MANAGER_XCB
    static constexpr uint8_t s_XcbFormatDataSize = 32;
    static constexpr uint16_t s_DefaultXcbWindowBorderWidth = 4;

    ////////////////////////////////////////////////////////////////////////////////////////////////
    NativeWindowImpl_Linux_xcb::NativeWindowImpl_Linux_xcb() 
        : NativeWindow::Implementation()
    {
        if (auto xcbConnectionManager = AzFramework::LinuxXcbConnectionManagerInterface::Get();
            xcbConnectionManager != nullptr)
        {
            m_xcbConnection = xcbConnectionManager->GetXcbConnection();
        }
        AZ_Error("AtomVulkan_RHI", m_xcbConnection!=nullptr, "Unable to get XCB Connection");
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    NativeWindowImpl_Linux_xcb::~NativeWindowImpl_Linux_xcb()
    {
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void NativeWindowImpl_Linux_xcb::InitWindow([[maybe_unused]]const AZStd::string& title,
                                                const WindowGeometry& geometry,
                                                [[maybe_unused]]const WindowStyleMasks& styleMasks)
    {
        // Get the parent window 
        const xcb_setup_t* xcbSetup = xcb_get_setup(m_xcbConnection);
        xcb_screen_t * xcbRootScreen = xcb_setup_roots_iterator(xcbSetup).data;
        xcb_window_t xcbParentWindow = xcbRootScreen->root;

        // Create an XCB window from the connection
        m_xcbWindow = xcb_generate_id(m_xcbConnection);

        uint16_t borderWidth = 0;
        const uint32_t mask = styleMasks.m_platformAgnosticStyleMask;
        if ((mask & WindowStyleMasks::WINDOW_STYLE_BORDERED) ||
            (mask & WindowStyleMasks::WINDOW_STYLE_RESIZEABLE))
        {
            borderWidth = s_DefaultXcbWindowBorderWidth;
        }

        uint32_t eventMask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
        
        uint32_t valueList[] = { xcbRootScreen->black_pixel, 
                                 XCB_EVENT_MASK_STRUCTURE_NOTIFY };

        xcb_void_cookie_t xcb_check_result;

        xcb_check_result = xcb_create_window_checked(m_xcbConnection,
                                                     XCB_COPY_FROM_PARENT,                         //depth
                                                     m_xcbWindow,                                  // Window ID
                                                     xcbParentWindow,                              // parent.
                                                     aznumeric_cast<int16_t>(geometry.m_posX),     // X
                                                     aznumeric_cast<int16_t>(geometry.m_posY),     // Y
                                                     aznumeric_cast<int16_t>(geometry.m_width),    // Width
                                                     aznumeric_cast<int16_t>(geometry.m_height),   // Height
                                                     borderWidth,                                  // Border Width
                                                     XCB_WINDOW_CLASS_INPUT_OUTPUT,                // Class
                                                     xcbRootScreen->root_visual,
                                                     eventMask,
                                                     valueList);

        AZ_Assert(ValidateXcbResult(xcb_check_result), "Failed to create xcb window.");

        SetWindowTitle(title);

        // Setup the window close event 
        const static char* wmProtocolString = "WM_PROTOCOLS";
        
        xcb_intern_atom_cookie_t cookie_protocol = xcb_intern_atom(m_xcbConnection, 1, strlen(wmProtocolString), wmProtocolString);
        xcb_intern_atom_reply_t* reply_protocol = xcb_intern_atom_reply(m_xcbConnection, cookie_protocol, nullptr);
        AZ_Error("AtomVulkan_RHI", reply_protocol!=NULL, "Unable to query xcb '%s' atom", wmProtocolString);
        m_xcbAtomProtocols = reply_protocol->atom;

        const static char* wmDeleteWindowString = "WM_DELETE_WINDOW";
        xcb_intern_atom_cookie_t cookie_delete_window = xcb_intern_atom(m_xcbConnection, 0, strlen(wmDeleteWindowString), wmDeleteWindowString);
        xcb_intern_atom_reply_t* reply_delete_window = xcb_intern_atom_reply(m_xcbConnection, cookie_delete_window, nullptr);
        AZ_Error("AtomVulkan_RHI", reply_delete_window!=NULL, "Unable to query xcb '%s' atom", wmDeleteWindowString);
        m_xcbAtomDeleteWindow = reply_delete_window->atom;

        xcb_check_result = xcb_change_property_checked(m_xcbConnection, 
                                                       XCB_PROP_MODE_REPLACE, 
                                                       m_xcbWindow,
			                                           m_xcbAtomProtocols, 
                                                       XCB_ATOM_ATOM, 
                                                       s_XcbFormatDataSize, 
                                                       1,
			                                           &m_xcbAtomDeleteWindow);
                                                    
        AZ_Assert(ValidateXcbResult(xcb_check_result), "Failed to change atom property for WM_CLOSE event");

        m_width = geometry.m_width;
        m_height = geometry.m_height;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void NativeWindowImpl_Linux_xcb::Activate()
    {
        LinuxXcbEventHandlerBus::Handler::BusConnect();

        if (!m_activated) // nothing to do if window was already activated
        {
            m_activated = true;

            xcb_map_window(m_xcbConnection, m_xcbWindow);
            xcb_flush(m_xcbConnection);
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void NativeWindowImpl_Linux_xcb::Deactivate()
    {
        if (m_activated) // nothing to do if window was already deactivated
        {
            m_activated = false;

            WindowNotificationBus::Event(reinterpret_cast<NativeWindowHandle>(m_xcbWindow), &WindowNotificationBus::Events::OnWindowClosed);

            xcb_unmap_window(m_xcbConnection, m_xcbWindow);
            xcb_flush(m_xcbConnection);
        }
        LinuxXcbEventHandlerBus::Handler::BusDisconnect();
    }    

    ////////////////////////////////////////////////////////////////////////////////////////////////
    NativeWindowHandle NativeWindowImpl_Linux_xcb::GetWindowHandle() const
    {
        return reinterpret_cast<NativeWindowHandle>(m_xcbWindow);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void NativeWindowImpl_Linux_xcb::SetWindowTitle(const AZStd::string& title)
    {
        xcb_void_cookie_t xcb_check_result;
        xcb_check_result = xcb_change_property(m_xcbConnection,
                                               XCB_PROP_MODE_REPLACE,
                                               m_xcbWindow,
                                               XCB_ATOM_WM_NAME,
                                               XCB_ATOM_STRING,
                                               8,
                                               static_cast<uint32_t>(title.size()),
                                               title.c_str());
        AZ_Assert(ValidateXcbResult(xcb_check_result), "Failed to set window title.");
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void NativeWindowImpl_Linux_xcb::ResizeClientArea(WindowSize clientAreaSize)
    {
        const static uint32_t values[] = { clientAreaSize.m_width, clientAreaSize.m_height };

        xcb_configure_window (m_xcbConnection, m_xcbWindow, XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT, values);
        
        m_width = clientAreaSize.m_width;
        m_height = clientAreaSize.m_height;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    uint32_t NativeWindowImpl_Linux_xcb::GetDisplayRefreshRate() const
    {
        //Using 60 for now until proper support is added
        return 60;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool NativeWindowImpl_Linux_xcb::ValidateXcbResult(xcb_void_cookie_t cookie)
    {
        bool result = true;
        if (xcb_generic_error_t* error = xcb_request_check(m_xcbConnection, cookie))
        {
            AZ_TracePrintf("Error","Error code %d", error->error_code);
            result = false;
        }
        return result;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void NativeWindowImpl_Linux_xcb::HandleXcbEvent(xcb_generic_event_t* event)
    {
        switch (event->response_type & 0x7f)
        {
            case XCB_CONFIGURE_NOTIFY:
            {
                xcb_configure_notify_event_t *cne = reinterpret_cast<xcb_configure_notify_event_t *>(event);
                if ((cne->width != m_width) ||
                    (cne->height != m_height))
                {
                    WindowSizeChanged(static_cast<uint32_t>(cne->width), 
                                      static_cast<uint32_t>(cne->height));
                }
                break;
            }
            case XCB_CLIENT_MESSAGE:
            {
                xcb_client_message_event_t *cme = reinterpret_cast<xcb_client_message_event_t *>(event);
	            if ((cme->type == m_xcbAtomProtocols) && 
                    (cme->format == s_XcbFormatDataSize) &&
			        (cme->data.data32[0] == m_xcbAtomDeleteWindow))
                {
                    Deactivate();
                    ApplicationRequests::Bus::Broadcast(&ApplicationRequests::ExitMainLoop);
                }
                break;
            }
        }
        AZ_UNUSED(event);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void NativeWindowImpl_Linux_xcb::WindowSizeChanged(const uint32_t width, const uint32_t height)
    {
        if (m_width != width || m_height != height)
        {
            m_width = width;
            m_height = height;

            if (m_activated)
            {
                WindowNotificationBus::Event(reinterpret_cast<NativeWindowHandle>(m_xcbWindow), &WindowNotificationBus::Events::OnWindowResized, width, height);
            }
        }
    }
#endif // PAL_TRAIT_LINUX_WINDOW_MANAGER_XCB

} // namespace AzFramework
