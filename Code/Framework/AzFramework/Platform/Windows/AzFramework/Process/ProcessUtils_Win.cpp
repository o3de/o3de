/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzFramework/Process/ProcessUtils.h>
#include <AzCore/PlatformIncl.h>
#include <AzCore/std/string/string.h>

#include <tlhelp32.h>

namespace AzFramework::ProcessUtils
{
    int ProcessCount(AZStd::string_view processFilename)
    {
        PROCESSENTRY32 entry;
        entry.dwSize = sizeof(PROCESSENTRY32);
        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
        int foundProcessCount = 0;
        if (Process32First(snapshot, &entry))
        {
            // Build a wide-char string of the server executable name. IE: L"MultiplayerSample.ServerLauncher.exe"
            const AZStd::wstring processFilenameUnicode(processFilename.begin(), processFilename.end());

            while (Process32Next(snapshot, &entry))
            {
                if (!_wcsicmp(entry.szExeFile, processFilenameUnicode.c_str()))
                {
                    ++foundProcessCount;
                }
            }
        }
        CloseHandle(snapshot);

        return foundProcessCount;
    }
}
