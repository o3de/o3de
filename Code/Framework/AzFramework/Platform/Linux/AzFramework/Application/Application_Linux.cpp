/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Application/Application.h>

#include "Application_Linux_xcb.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    Application::Implementation* Application::Implementation::Create()
    {
#if PAL_TRAIT_LINUX_WINDOW_MANAGER_XCB
        return aznew ApplicationLinux_xcb();
#elif PAL_TRAIT_LINUX_WINDOW_MANAGER_WAYLAND
        #error "Linux Window Manager Wayland not supported."
        return nullptr;
#elif PAL_TRAIT_LINUX_WINDOW_MANAGER_XLIB
        #error "Linux Window Manager XLIB not supported."
        return nullptr;
#else
        #error "Linux Window Manager not recognized."
        return nullptr;
#endif // PAL_TRAIT_LINUX_WINDOW_MANAGER_XCB
    }

} // namespace AzFramework
