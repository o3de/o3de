/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzFramework/API/ApplicationAPI_Platform.h>
#include <AzFramework/Application/Application.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{

#if PAL_TRAIT_LINUX_WINDOW_MANAGER_XCB

    ////////////////////////////////////////////////////////////////////////////////////////////////
    class ApplicationLinux_xcb
        : public Application::Implementation
        , public LinuxLifecycleEvents::Bus::Handler
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        AZ_CLASS_ALLOCATOR(ApplicationLinux_xcb, AZ::SystemAllocator, 0);
        ApplicationLinux_xcb();
        ~ApplicationLinux_xcb() override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Application::Implementation
        void PumpSystemEventLoopOnce() override;
        void PumpSystemEventLoopUntilEmpty() override;

    private:
        AZStd::unique_ptr<LinuxXcbConnectionManager>    m_xcbConnectionManager;
    };

#endif // PAL_TRAIT_LINUX_WINDOW_MANAGER_XCB

} // namespace AzFramework
