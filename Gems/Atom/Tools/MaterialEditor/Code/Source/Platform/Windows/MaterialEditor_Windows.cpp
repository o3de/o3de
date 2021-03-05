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
#include <AzFramework/API/ApplicationAPI.h>
#include <QDir>
#include <QWidget>
#include <MaterialEditor_Traits_Platform.h>

namespace Platform
{
    void LoadPluginDependencies()
    {
        // Ensure that the QtForPython compatible version of python is loaded 
        const char* engineRoot = nullptr;
        AzFramework::ApplicationRequests::Bus::BroadcastResult(engineRoot, &AzFramework::ApplicationRequests::GetEngineRoot);
        AZ_Assert(engineRoot != nullptr, "AzFramework::ApplicationRequests::GetEngineRoot failed");

        QString resolvedPythonPath(DEFAULT_LY_PYTHONHOME);
        AZ_TracePrintf("LoadPluginDependencies", "Adding Python to DLL directories: %s\n", resolvedPythonPath.toUtf8().data());
        SetDllDirectoryA(resolvedPythonPath.toUtf8().data());
    }

    void ProcessInput(void* message)
    {
        MSG* msg = (MSG*)message;

        // Ensure that the Windows WM_INPUT messages get passed through to the AzFramework input system,
        // but only while in game mode so we don't accumulate raw input events before we start actually
        // ticking the input devices, otherwise the queued events will get sent when entering game mode.
        if (msg->message == WM_INPUT)
        {
            UINT rawInputSize;
            const UINT rawInputHeaderSize = sizeof(RAWINPUTHEADER);
            GetRawInputData((HRAWINPUT)msg->lParam, RID_INPUT, NULL, &rawInputSize, rawInputHeaderSize);

            LPBYTE rawInputBytes = new BYTE[rawInputSize];
            GetRawInputData((HRAWINPUT)msg->lParam, RID_INPUT, rawInputBytes, &rawInputSize, rawInputHeaderSize);

            RAWINPUT* rawInput = (RAWINPUT*)rawInputBytes;

            AzFramework::RawInputNotificationBusWindows::Broadcast(
                &AzFramework::RawInputNotificationBusWindows::Events::OnRawInputEvent, *rawInput);
        }
    }

    AzFramework::NativeWindowHandle GetWindowHandle(WId winId)
    {
        return reinterpret_cast<HWND>(winId);
    }

    AzFramework::WindowSize GetClientAreaSize(AzFramework::NativeWindowHandle window)
    {
        RECT r;
        if (GetWindowRect(reinterpret_cast<HWND>(window), &r))
        {
            return AzFramework::WindowSize{aznumeric_cast<uint32_t>(r.right - r.left), aznumeric_cast<uint32_t>(r.bottom - r.top)};
        }
        else
        {
            AZ_Assert(false, "Failed to get dimensions for window");
            return AzFramework::WindowSize{};
        }
    }
}
