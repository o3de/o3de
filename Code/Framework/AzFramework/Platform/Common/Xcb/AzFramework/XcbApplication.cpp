/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/XcbApplication.h>
#include <AzFramework/XcbEventHandler.h>

namespace AzFramework
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    class XcbConnectionManagerImpl
        : public XcbConnectionManagerBus::Handler
    {
    public:
        XcbConnectionManagerImpl()
        {
            m_xcbConnection = xcb_connect(nullptr, nullptr);
            AZ_Error("Application", m_xcbConnection != nullptr, "Unable to connect to X11 Server.");
            XcbConnectionManagerBus::Handler::BusConnect();
        }

        ~XcbConnectionManagerImpl() override
        {
            XcbConnectionManagerBus::Handler::BusDisconnect();
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
    XcbApplication::XcbApplication()
    {
        LinuxLifecycleEvents::Bus::Handler::BusConnect();
        m_xcbConnectionManager = AZStd::make_unique<XcbConnectionManagerImpl>();
        if (XcbConnectionManagerInterface::Get() == nullptr)
        {
            XcbConnectionManagerInterface::Register(m_xcbConnectionManager.get());
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    XcbApplication::~XcbApplication()
    {
        if (XcbConnectionManagerInterface::Get() == m_xcbConnectionManager.get())
        {
            XcbConnectionManagerInterface::Unregister(m_xcbConnectionManager.get());
        }
        m_xcbConnectionManager.reset();
        LinuxLifecycleEvents::Bus::Handler::BusDisconnect();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void XcbApplication::PumpSystemEventLoopOnce()
    {
        if (xcb_connection_t* xcbConnection = m_xcbConnectionManager->GetXcbConnection())
        {
            if (xcb_generic_event_t* event = xcb_poll_for_event(xcbConnection))
            {
                XcbEventHandlerBus::Broadcast(&XcbEventHandlerBus::Events::HandleXcbEvent, event);
                free(event);
            }
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void XcbApplication::PumpSystemEventLoopUntilEmpty()
    {
        if (xcb_connection_t* xcbConnection = m_xcbConnectionManager->GetXcbConnection())
        {
            while (xcb_generic_event_t* event = xcb_poll_for_event(xcbConnection))
            {
                XcbEventHandlerBus::Broadcast(&XcbEventHandlerBus::Events::HandleXcbEvent, event);
                free(event);
            }
        }
    }

} // namespace AzFramework
