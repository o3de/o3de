/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Launcher.h>

#include <CryCommon/CryLibrary.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Memory/SystemAllocator.h>

int APIENTRY WinMain([[maybe_unused]] HINSTANCE hInstance, [[maybe_unused]] HINSTANCE hPrevInstance, [[maybe_unused]] LPSTR lpCmdLine, [[maybe_unused]] int nCmdShow)
{
    InitRootDir();

    using namespace O3DELauncher;

    PlatformMainInfo mainInfo;

    mainInfo.m_instance = GetModuleHandle(0);

    mainInfo.CopyCommandLine(__argc, __argv);

    // Prevent allocator from growing in small chunks
    // Pre-create our system allocator and configure it to ask for larger chunks from the OS
    // Creating this here to be consistent with other platforms
    AZ::SystemAllocator::Descriptor sysHeapDesc;
    sysHeapDesc.m_heap.m_systemChunkSize = 64 * 1024 * 1024;
    AZ::AllocatorInstance<AZ::SystemAllocator>::Create(sysHeapDesc);

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

#if !defined(AZ_MONOLITHIC_BUILD)

    {
        // HACK HACK HACK - is this still needed?!?!
        // CrySystem module can get loaded multiple times (even from within CrySystem itself)
        // so we will release it as many times as it takes until it actually unloads.
        void* hModule = CryLoadLibraryDefName("CrySystem");
        if (hModule)
        {
            // loop until we fail (aka unload the DLL)
            while (CryFreeLibrary(hModule))
            {
                ;
            }
        }
    }
#endif // !defined(AZ_MONOLITHIC_BUILD)

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
