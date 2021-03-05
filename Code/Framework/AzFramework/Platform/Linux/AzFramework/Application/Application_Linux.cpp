/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
