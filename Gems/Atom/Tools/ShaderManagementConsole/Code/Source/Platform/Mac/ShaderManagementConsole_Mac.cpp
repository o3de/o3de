/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates, or
* a third party where indicated.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <AzCore/std/string/string.h>
#include <QWidget>
#include <AzFramework/Windowing/WindowBus.h>

namespace Platform
{
    void ProcessInput(void* message)
    {
        AZ_Warning("Shader Management Console", false, "ProcessInput() function is not implemented");
    }

    AzFramework::NativeWindowHandle GetWindowHandle(WId winId)
    {
        AZ_Warning("Shader Management Console", false, "GetWindowHandle() function is not implemented");
        AZ_UNUSED(winId);
        return nullptr;
    }

    AzFramework::WindowSize GetClientAreaSize(AzFramework::NativeWindowHandle window)
    {
        AZ_Warning("Shader Management Console", false, "GetClientAreaSize() function is not implemented");
        AZ_UNUSED(window);
        return AzFramework::WindowSize{1,1};
    }
}
