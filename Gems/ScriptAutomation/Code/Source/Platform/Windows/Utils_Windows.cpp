/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/string/string.h>
#include <AzCore/PlatformIncl.h>

namespace AZ::Platform
{
    bool LaunchProgram(const AZStd::string& progPath, const AZStd::string& arguments)
    {
        bool result = false;

        AZStd::wstring exeW;
        AZStd::to_wstring(exeW, progPath.c_str());
        AZStd::wstring argumentsW;
        AZStd::to_wstring(argumentsW, arguments.c_str());

        STARTUPINFOW si;
        PROCESS_INFORMATION pi;

        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        ZeroMemory(&pi, sizeof(pi));

        // start the program up
        result = CreateProcess(exeW.data(),   // the path
            argumentsW.data(),        // Command line
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

        return result;
    }
}
