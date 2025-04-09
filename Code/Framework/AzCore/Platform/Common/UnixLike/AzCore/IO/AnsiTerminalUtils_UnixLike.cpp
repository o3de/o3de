/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/Utils/Utils.h>

namespace AZ::IO::Posix
{
    void EnableVirtualTerminalProcessing(int)
    {
        // Terminals on UnixLike platforms support virtual terminal
        // sequences by default
    }

    bool TerminalSupportsColor()
    {
        char envBuffer[256];
        if (auto termEnvOutcome = AZ::Utils::GetEnv(envBuffer, "TERM");
            termEnvOutcome)
        {
            AZStd::string_view termView = termEnvOutcome.GetValue();
            return termView == "xterm"
                || termView == "xterm-color"
                || termView == "xterm-256color"
                || termView == "screen"
                || termView == "screen-256color"
                || termView == "tmux"
                || termView == "tmux-256color"
                || termView == "rxvt-unicode"
                || termView == "rxvt-unicode-256color"
                || termView == "linux";
        }

        return false;
    }
    // return true if the file descriptor is referring to a terminal
    bool IsATty(int fileDescriptor)
    {
        return isatty(fileDescriptor);
    }
}
