/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Application/Application.h>
#include <sys/resource.h>

#if PAL_TRAIT_LINUX_WINDOW_MANAGER_XCB
#include <AzFramework/XcbApplication.h>
#endif

constexpr rlim_t g_minimumOpenFileHandles = 65536L;

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    Application::Implementation* Application::Implementation::Create()
    {
        // The default open file limit for processes may not be enough for O3DE applications. 
        // We will need to increase to the recommended value if the current open file limit
        // is not sufficient.
        rlimit currentLimit;
        int get_limit_result = getrlimit(RLIMIT_NOFILE, &currentLimit);
        AZ_Warning("Application", get_limit_result == 0, "Unable to read current ulimit open file limits");
        if ((get_limit_result == 0) && (currentLimit.rlim_cur < g_minimumOpenFileHandles || currentLimit.rlim_max < g_minimumOpenFileHandles))
        {
            rlimit newLimit;
            newLimit.rlim_cur = g_minimumOpenFileHandles; // Soft Limit
            newLimit.rlim_max = g_minimumOpenFileHandles; // Hard Limit
            [[maybe_unused]] int set_limit_result = setrlimit(RLIMIT_NOFILE, &newLimit);
            AZ_Assert(set_limit_result == 0, "Unable to update open file limits");
        }
        
#if PAL_TRAIT_LINUX_WINDOW_MANAGER_XCB
        return aznew XcbApplication();
#elif PAL_TRAIT_LINUX_WINDOW_MANAGER_WAYLAND
        #error "Linux Window Manager Wayland not supported."
        return nullptr;
#else
        #error "Linux Window Manager not recognized."
        return nullptr;
#endif // PAL_TRAIT_LINUX_WINDOW_MANAGER_XCB
    }

} // namespace AzFramework
