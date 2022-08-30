/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <TestImpactCommandLineOptions.h>

namespace TestImpact
{
    //! Representation of the command line options specific to the python runtime supplied to the console frontend application.
    class PythonCommandLineOptions
        : public CommandLineOptions
    {
    public:
        PythonCommandLineOptions(int argc, char** argv);

        //! Compiles the python command line usage to a string.
        static AZStd::string GetCommandLineUsageString();

        //! Returns the test runner policy to use
        Policy::TestRunner GetTestRunnerPolicy() const;

    private:
        Policy::TestRunner m_testRunnerPolicy = Policy::TestRunner::UseNullTestRunner;
    };
} // namespace TestImpact
