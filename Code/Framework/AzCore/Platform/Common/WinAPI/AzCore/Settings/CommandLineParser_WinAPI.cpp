/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "CommandLineParser_WinAPI.h"
#include <AzCore/Settings/CommandLineParser.h>
#include <AzCore/std/string/conversions.h>
#include <Windows.h>
#include <shellapi.h>

namespace AZ::Settings::Platform
{
    // The option prefixes of `--`, `-` and `/` are used by default to indicate
    // if a command line token should be parsed as an option on Windows
    // Ex. App.exe --foo -bar /baz positional_argument
    CommandLineOptionPrefixArray GetDefaultOptionPrefixes()
    {
        return CommandLineOptionPrefixArray{ "--", "-", "/" };
    }

    CommandLineConverter::CommandLineConverter(int& argc, char**& argv)
    {
        // Some test cases create an instance of ToolsTestApplication with new commandline parameters, which are different than the
        // application commandline parameters retrieved by GetCommandLineW(). These test cases also set the argv[0] to nullptr, so it can be
        // detected here.
        if (argc >= 1 && argv[0] == nullptr)
        {
            return;
        }

        // Read arguments form GetCommandLineW(), convert them to utf-8, and write them back to argc+argv. This is the most convenient way
        // on Windows to get the arguments as utf-8 encoded strings.

        int argcW;
        LPWSTR* argvW{ CommandLineToArgvW(GetCommandLineW(), &argcW) };

        m_convertedArguments.resize(argcW);
        m_convertedArgumentPointers.resize(argcW);

        for (int i{ 0 }; i < argcW; i++)
        {
            AZStd::to_string(m_convertedArguments[i], argvW[i]); // Convert wchar to utf-8 char
            m_convertedArgumentPointers[i] = m_convertedArguments[i].data();
        }

        LocalFree(argvW);
        argc = argcW;
        argv = m_convertedArgumentPointers.data();
    }
} // namespace AZ::Settings::Platform
