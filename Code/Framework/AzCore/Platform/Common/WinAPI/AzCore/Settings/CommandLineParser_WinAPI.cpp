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
    // The option prefix of --, - and / are used by default to indicate
    // if a command line token is a option on Windows API compatible OS
    // Ex. App.exe --foo -bar /baz positional_argument
    CommandLineOptionPrefixArray GetDefaultOptionPrefixes()
    {
        return CommandLineOptionPrefixArray{ "--", "-", "/" };
    }
}
