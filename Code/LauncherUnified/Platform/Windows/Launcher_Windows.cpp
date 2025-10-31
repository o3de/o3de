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

#if O3DE_HEADLESS_SERVER
int main(int argc, char* argv[])
{
    int argCount = argc;
    char** argValues = argv;
#else
int APIENTRY WinMain([[maybe_unused]] HINSTANCE hInstance, [[maybe_unused]] HINSTANCE hPrevInstance, [[maybe_unused]] LPSTR lpCmdLine, [[maybe_unused]] int nCmdShow)
{
    int argCount = __argc;
    char** argValues = __argv;
#endif // O3DE_HEADLESS_SERVER

    const AZ::Debug::Trace tracer;
    InitRootDir();

    using namespace O3DELauncher;

    PlatformMainInfo mainInfo;

    mainInfo.m_instance = GetModuleHandle(nullptr);

    mainInfo.CopyCommandLine(argCount, argValues);

    ReturnCode status = Run(mainInfo);

#if !O3DE_HEADLESS_SERVER

    #if !defined(_RELEASE)
    bool noPrompt = (strstr(mainInfo.m_commandLine, "-noprompt") != nullptr);
#else
    bool noPrompt = false;
#endif // !defined(_RELEASE)

    if (!noPrompt && status != ReturnCode::Success)
    {
        MessageBoxA(0, GetReturnCodeString(status), "Error", MB_OK | MB_DEFAULT_DESKTOP_ONLY | MB_ICONERROR);
    }
#endif // !O3DE_HEADLESS_SERVER

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
