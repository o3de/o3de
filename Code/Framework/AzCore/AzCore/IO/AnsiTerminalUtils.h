/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

namespace AZ::IO::Posix
{
    //! Enables Virtual Terminal Processing on Windows for th specified file descriptor
    //! This allows ANSI escapes to be supported
    //! https://learn.microsoft.com/en-us/windows/console/console-virtual-terminal-sequences
    void EnableVirtualTerminalProcessing(int fileDescriptor);

    //! return true the terminal supports color ANSI escape codes
    bool TerminalSupportsColor();

    //! return true if the open file descriptor is a terminal
    bool IsATty(int fileDescriptor);

    //! return true if the file descriptor supports ANSI escape sequences
    bool SupportsAnsiEscapes(int fileDescriptor);
}
