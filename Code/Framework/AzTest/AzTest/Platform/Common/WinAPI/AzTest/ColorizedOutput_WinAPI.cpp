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
