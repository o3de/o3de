/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/base.h>
#include <gtest/gtest.h>

namespace UnitTest
{
    namespace Platform
    {
        void EnableVirtualConsoleProcessingForStdout()
        {
            // UnixLike tty doesn't need ansi escapes enabled for them
        }

        bool TerminalSupportsColor()
        {
            const char* const term = testing::internal::posix::GetEnv("TERM");
            const bool term_supports_color = term && (
                azstricmp(term, "xterm") == 0 ||
                azstricmp(term, "xterm-color") == 0 ||
                azstricmp(term, "xterm-256color") == 0 ||
                azstricmp(term, "screen") == 0 ||
                azstricmp(term, "screen-256color") == 0 ||
                azstricmp(term, "tmux") == 0 ||
                azstricmp(term, "tmux-256color") == 0 ||
                azstricmp(term, "rxvt-unicode") == 0 ||
                azstricmp(term, "rxvt-unicode-256color") == 0 ||
                azstricmp(term, "linux") == 0 ||
                azstricmp(term, "cygwin") == 0
                );

            return term_supports_color;
        }
    }
}
