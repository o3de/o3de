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
            "testrunner",
        };

        Policy::TestRunner ParseNullTestRunnerPolicy(const AZ::CommandLine& cmd)
        {
            const BinaryStateValue<Policy::TestRunner> states = { Policy::TestRunner::UseNullTestRunner, Policy::TestRunner::UseTestRunner };

            return ParseOnOffOption(OptionKeys[NullTestRunnerPolicyKey], states, cmd).value_or(Policy::TestRunner::UseNullTestRunner);
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
            "    -testrunner=<on,off>                                   Whether to use the test runner (on) or the nulltestrunner(off). \n"
            "                                                           If not set, defaults to null test runner                        \n";

        return help;
    }
} // namespace TestImpact
