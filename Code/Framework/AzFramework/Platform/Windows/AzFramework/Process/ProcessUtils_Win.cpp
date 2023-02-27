/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzFramework/Process/ProcessUtils.h>
#include <tlhelp32.h>

namespace AzFramework::ProcessUtils
{
    bool TerminateProcess(const AZStd::string& processFilename)
    {
        PROCESSENTRY32 entry;
        entry.dwSize = sizeof(PROCESSENTRY32);
        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
        bool foundAndTerminatedProcess = false;
        if (Process32First(snapshot, &entry))
        {
            // Build a wide-char string of the server executable name. IE: L"MultiplayerSample.ServerLauncher.exe"
            const AZStd::wstring processFilenameUnicode(processFilename.begin(), processFilename.end());

            while (Process32Next(snapshot, &entry))
            {
                if (!_wcsicmp(entry.szExeFile, processFilenameUnicode.c_str()))
                {
                    HANDLE serverLauncherProcess = OpenProcess(PROCESS_TERMINATE, false, entry.th32ProcessID);
                    foundAndTerminatedProcess = ::TerminateProcess(serverLauncherProcess, 1);
                    CloseHandle(serverLauncherProcess);
                }
            }
        }
        CloseHandle(snapshot);

        return foundAndTerminatedProcess;
    }
}
