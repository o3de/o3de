/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// LY Base Crashpad Windows implementation

#include <CrashHandler.h>
#include <AzCore/PlatformIncl.h>

namespace CrashHandler
{
    const char* crashHandlerPath = "ToolsCrashUploader.exe";

    const char* CrashHandlerBase::GetCrashHandlerExecutableName() const
    {
        return crashHandlerPath;
    }

    void CrashHandlerBase::GetOSAnnotations(CrashHandlerAnnotations& annotations) const
    {
        annotations["os"] = "windows";

        // Disable warning about GetVersion deprecation
        // Windows now encourages users to check for the specific capability they need rather
        // than relying on a version ID due to badly written/lazy code (If buildID >= NUM then I certainly can do x...)
        // In this case we really just want a build ID to report
AZ_PUSH_DISABLE_WARNING(4996, "-Wdeprecated-declarations")
        DWORD winVersion = GetVersion();
AZ_POP_DISABLE_WARNING

        // VersionMajor lives in the low byte of the low word
        int versionMajor = winVersion & 0xFF;
        // VersionMinor lives in the high byte of the low word
        int versionMinor = (winVersion & (0xFF << 8)) >> 8;

        int osBuild = 0;
        if (winVersion < 0x80000000)
        {
            // BuildID lives in the high word
            osBuild = (winVersion & (0xFFFF << 16)) >> 16;
        }

        std::string annotationBuf;

        annotationBuf = std::to_string(versionMajor) + "." + std::to_string(versionMinor);
        annotations["os.version"] = annotationBuf;

        annotationBuf = std::to_string(osBuild);
        annotations["os.build"] = annotationBuf;

        const size_t kbSize = 1024;
        MEMORYSTATUSEX statex;
        statex.dwLength = sizeof(statex);
        GlobalMemoryStatusEx(&statex);
        annotationBuf = std::to_string(statex.dwMemoryLoad);
        annotations["vm.used"] = annotationBuf;
        annotationBuf = std::to_string(statex.ullTotalPhys / kbSize);
        annotations["vm.total"] = annotationBuf;
        annotationBuf = std::to_string(statex.ullAvailPhys / kbSize);
        annotations["vm.free"] = annotationBuf;
        annotationBuf = std::to_string(statex.ullTotalPageFile / kbSize);
        annotations["vm.swap.size"] = annotationBuf;

        RECT desktopRect;
        const HWND desktopWind = GetDesktopWindow();
        GetWindowRect(desktopWind, &desktopRect);
        annotationBuf = std::to_string(desktopRect.right) + "x" + std::to_string(desktopRect.bottom);
        annotations["resolution"] = annotationBuf;

        ULARGE_INTEGER freeBytes;
        auto queryResult = GetDiskFreeSpaceExA(".", &freeBytes, nullptr, nullptr);
        if (queryResult)
        {
            annotationBuf = std::to_string(freeBytes.QuadPart);
            annotations["disk_free"] = annotationBuf;
        }
        else
        {
            annotations["disk_free"] = "error";
        }
    }

}
