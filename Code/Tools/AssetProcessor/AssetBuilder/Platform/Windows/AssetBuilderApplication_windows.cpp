/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <AssetBuilderApplication.h>

#include "shlobj.h"

namespace AssetBuilderApplicationPrivate
{
    BOOL WINAPI CtrlHandlerRoutine(DWORD dwCtrlType)
    {
        (void)dwCtrlType;

        // Terminate the process when CTRL+C is pressed
        // Builder processes load user-code and we couldn't expect that every single gem
        // written by every single external developer be able to shut down cleanly.

        TerminateProcess(GetCurrentProcess(), UINT(-1)); // dont ever return a success error code from a terminated process.
        return TRUE;
    }
}

void AssetBuilderApplication::InstallCtrlHandler()
{
    ::SetConsoleCtrlHandler(AssetBuilderApplicationPrivate::CtrlHandlerRoutine, TRUE);
}