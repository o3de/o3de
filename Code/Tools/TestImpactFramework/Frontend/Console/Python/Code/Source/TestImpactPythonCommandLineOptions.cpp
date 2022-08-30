/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TestImpactPythonCommandLineOptions.h>
#include <TestImpactCommandLineOptionsUtils.h>
#include <TestImpactFramework/TestImpactUtils.h>

#include <AzCore/Settings/CommandLine.h>

namespace TestImpact
{
    namespace
    {
        enum PythonOptions
        {
            // Options
            NullTestRunnerPolicyKey,
        };

        constexpr const char* OptionKeys[] = {
            // Options
            "usenulltestrunner",
        };

        Policy::TestRunner ParseNullTestRunnerPolicy(const AZ::CommandLine& cmd)
        {
            const BinaryStateValue<Policy::TestRunner> states = { Policy::TestRunner::UseTestRunner, Policy::TestRunner::UseNullTestRunner };

            return ParseOnOffOption(OptionKeys[NullTestRunnerPolicyKey], states, cmd).value_or(Policy::TestRunner::UseTestRunner);
        }
    } // namespace

    PythonCommandLineOptions::PythonCommandLineOptions(int argc, char** argv)
        : CommandLineOptions(argc, argv)
    {
        AZ::CommandLine cmd;
        cmd.Parse(argc, argv);

        m_testRunnerPolicy = ParseNullTestRunnerPolicy(cmd);
    }

    Policy::TestRunner PythonCommandLineOptions::GetTestRunnerPolicy() const
    {
        return m_testRunnerPolicy;
    }

    AZStd::string PythonCommandLineOptions::GetCommandLineUsageString()
    {
        AZStd::string help = CommandLineOptions::GetCommandLineUsageString();
        help +=
            "    -testrunner=<on,off>                                   Whether to use the null test runner (on) or run the tests(off). \n"
            "                                                           If not set, defaults to running tests.                          \n";

        return help;
    }
} // namespace TestImpact
