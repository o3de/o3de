/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/string/string.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/PlatformIncl.h>

namespace LuxCoreUI
{
    void LaunchLuxCoreUI(const AZStd::string& luxCoreExeFullPath, const AZStd::string& commandLine)
    {
        STARTUPINFO si;
        PROCESS_INFORMATION pi;

        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        ZeroMemory(&pi, sizeof(pi));

        AZStd::wstring luxCoreExeFullPathW;
        AZStd::to_wstring(luxCoreExeFullPathW, luxCoreExeFullPath.c_str());
        AZStd::wstring commandLineW;
        AZStd::to_wstring(commandLineW, commandLine.c_str());

        // start the program up
        CreateProcessW(luxCoreExeFullPathW.c_str(),   // the path
            commandLineW.data(),        // Command line
            NULL,           // Process handle not inheritable
            NULL,           // Thread handle not inheritable
            FALSE,          // Set handle inheritance to FALSE
            0,              // No creation flags
            NULL,           // Use parent's environment block
            NULL,           // Use parent's starting directory 
            &si,            // Pointer to STARTUPINFO structure
            &pi             // Pointer to PROCESS_INFORMATION structure (removed extra parentheses)
        );
        // Close process and thread handles. 
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
}
