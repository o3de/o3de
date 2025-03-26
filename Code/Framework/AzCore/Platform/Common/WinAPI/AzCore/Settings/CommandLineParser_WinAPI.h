/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>

namespace AZ::Settings::Platform
{
    //! A class to ensure command-line parameters are in utf-8 encoding.
    //! On Windows installations without utf-8 commandline arguments, the passed argc and argv arguments are overwritten. This class may
    //! allocate and stores additional strings, therefore it needs to stay in scope until the arguments are no longer needed.
    class CommandLineConverter
    {
        AZStd::vector<AZStd::string> m_convertedArguments;
        AZStd::vector<char*> m_convertedArgumentPointers;

    public:
        CommandLineConverter(int& argc, char**& argv);
    };
} // namespace AZ::Settings::Platform
