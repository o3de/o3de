/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    ApplicationLinux::~ApplicationLinux()
    {
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
