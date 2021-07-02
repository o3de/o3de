/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>

namespace AZ
{
    class CommandLine;
}

namespace PythonBindingsExample
{
    struct ApplicationParameters final
    {
        bool Parse(const AZ::CommandLine* commandLine);

        // the arguments
        using StringList = AZStd::vector<AZStd::string>;
        bool m_verbose = false;
        AZStd::string m_pythonFilename;
        StringList m_pythonArgs;
        AZStd::string m_pythonStatement;
        bool m_interactiveMode = false;

    protected:
        void ShowHelp();
    };
}
