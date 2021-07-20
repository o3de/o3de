/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/string/string.h>
#include <AzFramework/API/ApplicationAPI.h>
#include <QDir>
#include <QWidget>
#include <MaterialEditor_Traits_Platform.h>

namespace Platform
{
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
