/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/API/ApplicationAPI_Platform.h>
#include <AzFramework/Application/Application.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    class ApplicationLinux
        : public Application::Implementation
        , public LinuxLifecycleEvents::Bus::Handler
#if AZ_LINUX_WINDOW_MANAGER_XCB
        , public LinuxXCBConnectionManager::Bus::Handler
#endif // AZ_LINUX_WINDOW_MANAGER_XCB
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        AZ_CLASS_ALLOCATOR(ApplicationLinux, AZ::SystemAllocator, 0);
        ApplicationLinux();
        ~ApplicationLinux() override;

#if AZ_LINUX_WINDOW_MANAGER_XCB
        virtual xcb_connection_t* GetXCBConnection() const override;
#endif // AZ_LINUX_WINDOW_MANAGER_XCB
        

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Application::Implementation
        void PumpSystemEventLoopOnce() override;
        void PumpSystemEventLoopUntilEmpty() override;
    private:
#if AZ_LINUX_WINDOW_MANAGER_XCB
        xcb_connection_t    * m_xcbConnection = NULL;
#endif // AZ_LINUX_WINDOW_MANAGER_XCB

    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    Application::Implementation* Application::Implementation::Create()
    {
        return aznew ApplicationLinux();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    ApplicationLinux::ApplicationLinux()
    {
        LinuxLifecycleEvents::Bus::Handler::BusConnect();

#if AZ_LINUX_WINDOW_MANAGER_XCB
        m_xcbConnection = xcb_connect(NULL, NULL);
        AZ_Error("ApplicationLinux", m_xcbConnection != NULL, "Unable to connect to X11 Server.");

        LinuxXCBConnectionManager::Bus::Handler::BusConnect();
#endif // AZ_LINUX_WINDOW_MANAGER_XCB
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    ApplicationLinux::~ApplicationLinux()
    {
#if AZ_LINUX_WINDOW_MANAGER_XCB
        if (m_xcbConnection!=NULL)
        {
            xcb_disconnect(m_xcbConnection);
        }
        LinuxXCBConnectionManager::Bus::Handler::BusDisconnect();

#endif // AZ_LINUX_WINDOW_MANAGER_XCB
        LinuxLifecycleEvents::Bus::Handler::BusDisconnect();
    }

#if AZ_LINUX_WINDOW_MANAGER_XCB
    xcb_connection_t* ApplicationLinux::GetXCBConnection() const
    {
        return m_xcbConnection;
    }
#endif // AZ_LINUX_WINDOW_MANAGER_XCB

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void ApplicationLinux::PumpSystemEventLoopOnce()
    {
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void ApplicationLinux::PumpSystemEventLoopUntilEmpty()
    {
    }
} // namespace AzFramework
