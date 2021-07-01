/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/PlatformIncl.h>
#include <ConsoleApi.h>

namespace UnitTest
{
    namespace Platform
    {
        void EnableVirtualConsoleProcessingForStdout()
        {
            const HANDLE stdout_handle = GetStdHandle(STD_OUTPUT_HANDLE);
            DWORD currentMode{};
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
            GetConsoleMode(stdout_handle, &currentMode);
            SetConsoleMode(stdout_handle, currentMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
#endif
        }

        bool TerminalSupportsColor()
        {
            return true;
        }
    }
}
