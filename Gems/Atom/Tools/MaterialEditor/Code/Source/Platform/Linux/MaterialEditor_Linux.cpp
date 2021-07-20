/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/string/string.h>
#include <QWidget>
#include <AzFramework/Windowing/WindowBus.h>

namespace Platform
{
    void LoadPluginDependencies()
    {
        AZ_Warning("Material Editor", false, "LoadPluginDependencies() function is not implemented");
    }

    void ProcessInput(void* message)
    {
        AZ_Warning("Material Editor", false, "ProcessInput() function is not implemented");
    }

    AzFramework::NativeWindowHandle GetWindowHandle(WId winId)
    {
        AZ_Warning("Material Editor", false, "GetWindowHandle() function is not implemented");
        AZ_UNUSED(winId);
        return nullptr;
    }

    AzFramework::WindowSize GetClientAreaSize(AzFramework::NativeWindowHandle window)
    {
        AZ_Warning("Material Editor", false, "GetClientAreaSize() function is not implemented");
        AZ_UNUSED(window);
        return AzFramework::WindowSize{1,1};
    }
}
