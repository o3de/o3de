/*
 * All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 * its licensors.
 *
 * For complete copyright and license terms please see the LICENSE at the root of this
 * distribution (the "License"). All use of this software is governed by the License,
 * or, if provided, by the license below or the license accompanying this file. Do not
 * remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
