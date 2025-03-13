/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/AzCoreAPI.h>

namespace AZ::IO::Posix
{
    //! Enables Virtual Terminal Processing on Windows for th specified file descriptor
    //! This allows ANSI escapes to be supported
    //! https://learn.microsoft.com/en-us/windows/console/console-virtual-terminal-sequences
    AZCORE_API void EnableVirtualTerminalProcessing(int fileDescriptor);

    //! return true the terminal supports color ANSI escape codes
    AZCORE_API bool TerminalSupportsColor();

    //! return true if the open file descriptor is a terminal
    AZCORE_API bool IsATty(int fileDescriptor);

    //! return true if the file descriptor supports ANSI escape sequences
    AZCORE_API bool SupportsAnsiEscapes(int fileDescriptor);
}
