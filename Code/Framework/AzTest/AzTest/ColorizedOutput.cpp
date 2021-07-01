/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/UnitTest/UnitTest.h>
#include <gtest/gtest.h>
#include <stdarg.h>

namespace UnitTest
{
    namespace Platform
    {
        void EnableVirtualConsoleProcessingForStdout();
        bool TerminalSupportsColor();
    }
    static const char* GetAnsiColorCode(GTestColor color)
    {
        switch (color)
        {
        case COLOR_RED:
            return "1";
        case COLOR_GREEN:
            return "2";
        case COLOR_YELLOW:
            return "3";
        default:
            return nullptr;
        };
    }

    // Returns true if and only if Google Test should use colors in the output.
    static bool ShouldUseColor(bool stdout_is_tty)
    {
        const char* const gtest_color = testing::GTEST_FLAG(color).c_str();

        if (azstricmp(gtest_color, "auto") == 0)
        {
            // On non-Windows platforms, we rely on the TERM variable.
            
            return stdout_is_tty && Platform::TerminalSupportsColor();
        }

        return azstricmp(gtest_color, "yes") == 0 || azstricmp(gtest_color, "true") == 0 || azstricmp(gtest_color, "t") == 0 || strcmp(gtest_color, "1") == 0;
    }

    void ColoredPrintf(GTestColor color, const char* fmt, ...)
    {
        va_list args;
        va_start(args, fmt);

        static const bool in_color_mode = ShouldUseColor(testing::internal::posix::IsATTY(testing::internal::posix::FileNo(stdout)) != 0);
        const bool use_color = in_color_mode && (color != COLOR_DEFAULT);

        if (!use_color)
        {
            vprintf(fmt, args);
            va_end(args);
            return;
        }

        Platform::EnableVirtualConsoleProcessingForStdout();
        printf("\033[0;3%sm", GetAnsiColorCode(color));
        vprintf(fmt, args);
        printf("\033[m");  // Resets the terminal to default.
        va_end(args);
    }
}
