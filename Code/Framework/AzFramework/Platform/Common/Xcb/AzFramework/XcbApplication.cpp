/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/API/ApplicationAPI_Platform.h>
#include <AzFramework/Application/Application.h>
#include "Application_Linux_xcb.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
#if PAL_TRAIT_LINUX_WINDOW_MANAGER_XCB
    ////////////////////////////////////////////////////////////////////////////////////////////////
    class LinuxXcbConnectionManagerImpl
        : public LinuxXcbConnectionManagerBus::Handler
    {
    public:
        LinuxXcbConnectionManagerImpl()
        {
            m_xcbConnection = xcb_connect(nullptr, nullptr);
            AZ_Error("ApplicationLinux", m_xcbConnection != nullptr, "Unable to connect to X11 Server.");
            LinuxXcbConnectionManagerBus::Handler::BusConnect();
        }

        ~LinuxXcbConnectionManagerImpl()
        {
            LinuxXcbConnectionManagerBus::Handler::BusDisconnect();
            xcb_disconnect(m_xcbConnection);   
        }

        xcb_connection_t* GetXcbConnection() const override
        {
            return m_xcbConnection;
        }

    private:
        xcb_connection_t*   m_xcbConnection = nullptr;
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    ApplicationLinux_xcb::ApplicationLinux_xcb()
    {
        LinuxLifecycleEvents::Bus::Handler::BusConnect();
        m_xcbConnectionManager = AZStd::make_unique<LinuxXcbConnectionManagerImpl>();
        if (LinuxXcbConnectionManagerInterface::Get() == nullptr)
        {
            LinuxXcbConnectionManagerInterface::Register(m_xcbConnectionManager.get());
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    ApplicationLinux_xcb::~ApplicationLinux_xcb()
    {
        if (LinuxXcbConnectionManagerInterface::Get() == m_xcbConnectionManager.get())
        {
            LinuxXcbConnectionManagerInterface::Unregister(m_xcbConnectionManager.get());
        }
        m_xcbConnectionManager.reset();
        LinuxLifecycleEvents::Bus::Handler::BusDisconnect();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void ApplicationLinux_xcb::PumpSystemEventLoopOnce()
    {
        if (xcb_connection_t* xcbConnection = m_xcbConnectionManager->GetXcbConnection())
        {
            if (xcb_generic_event_t* event = xcb_poll_for_event(xcbConnection))
            {
                LinuxXcbEventHandlerBus::Broadcast(&LinuxXcbEventHandlerBus::Events::HandleXcbEvent, event);
                free(event);
            }
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void ApplicationLinux_xcb::PumpSystemEventLoopUntilEmpty()
    {
        if (xcb_connection_t* xcbConnection = m_xcbConnectionManager->GetXcbConnection())
        {
            while (xcb_generic_event_t* event = xcb_poll_for_event(xcbConnection))
            {
                LinuxXcbEventHandlerBus::Broadcast(&LinuxXcbEventHandlerBus::Events::HandleXcbEvent, event);
                free(event);
            }
        }
    }

#endif // PAL_TRAIT_LINUX_WINDOW_MANAGER_XCB        

} // namespace AzFramework
