/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Settings/CommandLineParser.h>

namespace AZ::Settings::Platform
{
    // The option prefixes of `--` and `-` are used by default to indicate
    // if a command line token should be parsed as an option on Unix-like OS
    // Ex. App.exe --foo -bar positional_argument
    CommandLineOptionPrefixArray GetDefaultOptionPrefixes()
    {
        return CommandLineOptionPrefixArray{ "--", "-" };
    }
}
