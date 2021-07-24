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
#if PAL_TRAIT_LINUX_WINDOW_MANAGER_XCB
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
#endif // PAL_TRAIT_LINUX_WINDOW_MANAGER_XCB

    ////////////////////////////////////////////////////////////////////////////////////////////////
    class ApplicationLinux
        : public Application::Implementation
        , public LinuxLifecycleEvents::Bus::Handler
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        AZ_CLASS_ALLOCATOR(ApplicationLinux, AZ::SystemAllocator, 0);
        ApplicationLinux();
        ~ApplicationLinux() override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Application::Implementation
        void PumpSystemEventLoopOnce() override;
        void PumpSystemEventLoopUntilEmpty() override;
    private:

#if PAL_TRAIT_LINUX_WINDOW_MANAGER_XCB
        AZStd::unique_ptr<LinuxXcbConnectionManager>    m_xcbConnectionManager;
#endif // PAL_TRAIT_LINUX_WINDOW_MANAGER_XCB

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

#if PAL_TRAIT_LINUX_WINDOW_MANAGER_XCB
        m_xcbConnectionManager = AZStd::make_unique<LinuxXcbConnectionManagerImpl>();
        if (LinuxXcbConnectionManagerInterface::Get() == nullptr)
        {
            LinuxXcbConnectionManagerInterface::Register(m_xcbConnectionManager.get());
        }
#endif // PAL_TRAIT_LINUX_WINDOW_MANAGER_XCB
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    ApplicationLinux::~ApplicationLinux()
    {
#if PAL_TRAIT_LINUX_WINDOW_MANAGER_XCB
        if (LinuxXcbConnectionManagerInterface::Get() == m_xcbConnectionManager.get())
        {
            LinuxXcbConnectionManagerInterface::Unregister(m_xcbConnectionManager.get());
        }
        m_xcbConnectionManager.reset();
#endif // PAL_TRAIT_LINUX_WINDOW_MANAGER_XCB        
        LinuxLifecycleEvents::Bus::Handler::BusDisconnect();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void ApplicationLinux::PumpSystemEventLoopOnce()
    {
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void ApplicationLinux::PumpSystemEventLoopUntilEmpty()
    {
    }
} // namespace AzFramework
