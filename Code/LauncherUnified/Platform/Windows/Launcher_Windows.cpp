/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Launcher.h>

#include <AzCore/Math/Vector2.h>
#include <AzCore/Memory/SystemAllocator.h>

int APIENTRY WinMain([[maybe_unused]] HINSTANCE hInstance, [[maybe_unused]] HINSTANCE hPrevInstance, [[maybe_unused]] LPSTR lpCmdLine, [[maybe_unused]] int nCmdShow)
{
    const AZ::Debug::Trace tracer;
    InitRootDir();

    using namespace O3DELauncher;

    PlatformMainInfo mainInfo;

    mainInfo.m_instance = GetModuleHandle(0);

    mainInfo.CopyCommandLine(__argc, __argv);

    AZ::AllocatorInstance<AZ::SystemAllocator>::Create();

    ReturnCode status = Run(mainInfo);

#if !defined(_RELEASE)
    bool noPrompt = (strstr(mainInfo.m_commandLine, "-noprompt") != nullptr);
#else
    bool noPrompt = false;
#endif // !defined(_RELEASE)

    if (!noPrompt && status != ReturnCode::Success)
    {
        MessageBoxA(0, GetReturnCodeString(status), "Error", MB_OK | MB_DEFAULT_DESKTOP_ONLY | MB_ICONERROR);
    }

    // there is no way to transfer ownership of the allocator to the component application
    // without altering the app descriptor, so it must be destroyed here
    AZ::AllocatorInstance<AZ::SystemAllocator>::Destroy();

    return static_cast<int>(status);
}

void CVar_OnViewportPosition(const AZ::Vector2& value)
{
    if (HWND windowHandle = GetActiveWindow())
    {
        SetWindowPos(windowHandle, nullptr,
            static_cast<int>(value.GetX()),
            static_cast<int>(value.GetY()),
            0, 0, SWP_NOOWNERZORDER | SWP_NOSIZE);
    }
}
