/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/PlatformIncl.h>

#include <io.h>

namespace AZ::IO::Posix
{
    void EnableVirtualTerminalProcessing(int fileDescriptor)
    {
        const auto fileHandle = reinterpret_cast<HANDLE>(_get_osfhandle(fileDescriptor));
        DWORD currentMode{};
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
        GetConsoleMode(fileHandle, &currentMode);
        SetConsoleMode(fileHandle, currentMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
#endif
    }

    bool TerminalSupportsColor()
    {
        return true;
    }
    // return true if the file descriptor is refering to a terminal
    bool IsATty(int fileDescriptor)
    {
        return _isatty(fileDescriptor);
    }
}
