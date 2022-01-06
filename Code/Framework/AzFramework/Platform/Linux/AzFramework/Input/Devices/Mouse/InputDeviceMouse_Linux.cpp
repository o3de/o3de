/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#if PAL_TRAIT_LINUX_WINDOW_MANAGER_XCB
#include <AzFramework/XcbInputDeviceMouse.h>
#endif

namespace AzFramework
{
    InputDeviceMouse::Implementation* InputDeviceMouse::Implementation::Create(InputDeviceMouse& inputDevice)
    {
#if PAL_TRAIT_LINUX_WINDOW_MANAGER_XCB
        return XcbInputDeviceMouse::Create(inputDevice);
#elif PAL_TRAIT_LINUX_WINDOW_MANAGER_WAYLAND
#error "Linux Window Manager Wayland not supported."
        return nullptr;
#else
#error "Linux Window Manager not recognized."
        return nullptr;
#endif // PAL_TRAIT_LINUX_WINDOW_MANAGER_XCB
    }
} // namespace AzFramework
