/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
