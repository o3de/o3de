/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/XcbApplication.h>
#include <AzFramework/XcbEventHandler.h>
#include <AzFramework/XcbInterface.h>

#include <xcb/xinput.h>

namespace AzFramework
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    class XcbConnectionManagerImpl
        : public XcbConnectionManagerBus::Handler
    {
    public:
        XcbConnectionManagerImpl()
            : m_xcbConnection(xcb_connect(nullptr, nullptr))
        {
            AZ_Error("Application", m_xcbConnection != nullptr, "Unable to connect to X11 Server.");
            XcbConnectionManagerBus::Handler::BusConnect();
        }

        ~XcbConnectionManagerImpl() override
        {
            XcbConnectionManagerBus::Handler::BusDisconnect();
        }

        xcb_connection_t* GetXcbConnection() const override
        {
            return m_xcbConnection.get();
        }

        void SetEnableXInput(xcb_connection_t* connection, bool enable) override
        {
            struct Mask
            {
                xcb_input_event_mask_t head;
                xcb_input_xi_event_mask_t mask;
            };
            const Mask mask {
                /*.head=*/{
                    /*.device_id=*/XCB_INPUT_DEVICE_ALL_MASTER,
                    /*.mask_len=*/1
                },
                /*.mask=*/ enable ?
                    (xcb_input_xi_event_mask_t)(XCB_INPUT_XI_EVENT_MASK_RAW_MOTION | XCB_INPUT_XI_EVENT_MASK_RAW_BUTTON_PRESS | XCB_INPUT_XI_EVENT_MASK_RAW_BUTTON_RELEASE) :
                    (xcb_input_xi_event_mask_t)XCB_NONE
            };

            const xcb_setup_t* xcbSetup = xcb_get_setup(connection);
            const xcb_screen_t* xcbScreen = xcb_setup_roots_iterator(xcbSetup).data;

            xcb_input_xi_select_events(connection, xcbScreen->root, 1, &mask.head);

            xcb_flush(connection);
        }

    private:
        XcbUniquePtr<xcb_connection_t, xcb_disconnect> m_xcbConnection = nullptr;
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
            if (auto event = XcbStdFreePtr<xcb_generic_event_t>{xcb_poll_for_event(xcbConnection)})
            {
                XcbEventHandlerBus::Broadcast(&XcbEventHandlerBus::Events::HandleXcbEvent, event.get());
            }
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void XcbApplication::PumpSystemEventLoopUntilEmpty()
    {
        if (xcb_connection_t* xcbConnection = m_xcbConnectionManager->GetXcbConnection())
        {
            while (auto event = XcbStdFreePtr<xcb_generic_event_t>{xcb_poll_for_event(xcbConnection)})
            {
                XcbEventHandlerBus::Broadcast(&XcbEventHandlerBus::Events::HandleXcbEvent, event.get());
            }
        }
    }

} // namespace AzFramework
