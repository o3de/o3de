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

#include <TestImpactCommandLineOptions.h>
#include <TestImpactCommandLineOptionsException.h>

#include <AzCore/Settings/CommandLine.h>

namespace TestImpact
{
    namespace
    {
        RepoPath ParseConfigurationFile(const AZ::CommandLine& cmd)
        {
            const auto numSwitchValues = cmd.GetNumSwitchValues("config");
            if (numSwitchValues)
            {
                AZ_TestImpact_Eval(numSwitchValues == 1, CommandLineOptionsException, "Unexpected number of parameters for config file option");
                const auto value = cmd.GetSwitchValue("config", 0);
                AZ_TestImpact_Eval(!value.empty(), CommandLineOptionsException, "Config file option value is empty");
                return value;
            }

            return LY_TEST_IMPACT_DEFAULT_CONFIG_FILE;
        }

        AZStd::optional<RepoPath> ParseChangeListFile(const AZ::CommandLine& cmd)
        {
            const auto numSwitchValues = cmd.GetNumSwitchValues("changelist");
            if (numSwitchValues)
            {
                AZ_TestImpact_Eval(numSwitchValues == 1, CommandLineOptionsException, "Unexpected number of parameters for change lis file option");
                const auto value = cmd.GetSwitchValue("changelist", 0);
                AZ_TestImpact_Eval(!value.empty(), CommandLineOptionsException, "Change list file option value is empty");
                return value;
            }

            return AZStd::nullopt;
        }

        bool ParseOutputChangeList(const AZ::CommandLine& cmd)
        {
            const auto numSwitchValues = cmd.GetNumSwitchValues("ochangelist");
            if (numSwitchValues)
            {
                AZ_TestImpact_Eval(numSwitchValues == 1, CommandLineOptionsException, "Unexpected number of parameters for output change list option");
                const auto option = cmd.GetSwitchValue("ochangelist", 0);
                if (option == "on")
                {
                    return true;
                }
                else if (option == "off")
                {
                    return false;
                }

                throw CommandLineOptionsException(AZStd::string::format("Unexpected value for output change list option: %s", option.c_str()));
            }

            return false;
        }

        AZStd::optional<TestSequenceType> ParseTestSequenceType(const AZ::CommandLine& cmd)
        {
            const auto numSwitchValues = cmd.GetNumSwitchValues("sequence");
            if (numSwitchValues)
            {
                AZ_TestImpact_Eval(numSwitchValues == 1, CommandLineOptionsException, "Unexpected number of parameters for sequence option");
                const auto option = cmd.GetSwitchValue("sequence", 0);
                if (option == "none")
                {
                    return AZStd::nullopt;
                }
                else if (option == "seed")
                {
                    return TestSequenceType::Seed;
                }
                else if (option == "regular")
                {
                    return TestSequenceType::Regular;
                }
                else if (option == "tia")
                {
                    return TestSequenceType::ImpactAnalysis;
                }
                else if (option == "tiaorseed")
                {
                    return TestSequenceType::ImpactAnalysisOrSeed;
                }

                throw CommandLineOptionsException(AZStd::string::format("Unexpected value for sequence option: %s", option.c_str()));
            }

            return AZStd::nullopt;
        }

        Policy::TestPrioritization ParseTestPrioritizationPolicy(const AZ::CommandLine& cmd)
        {
            const auto numSwitchValues = cmd.GetNumSwitchValues("ppolicy");
            if (numSwitchValues)
            {
                AZ_TestImpact_Eval(numSwitchValues == 1, CommandLineOptionsException, "Unexpected number of parameters for test prioritization policy option");
                const auto option = cmd.GetSwitchValue("ppolicy", 0);
                if (option == "none")
                {
                    return Policy::TestPrioritization::None;
                }
                else if (option == "locality")
                {
                    return Policy::TestPrioritization::DependencyLocality;
                }

                throw CommandLineOptionsException(AZStd::string::format("Unexpected value for test prioritization policy option: %s", option.c_str()));
            }

            return Policy::TestPrioritization::None;
        }

        Policy::ExecutionFailure ParseExecutionFailurePolicy(const AZ::CommandLine& cmd)
        {
            const auto numSwitchValues = cmd.GetNumSwitchValues("epolicy");
            if (numSwitchValues)
            {
                AZ_TestImpact_Eval(numSwitchValues == 1, CommandLineOptionsException, "Unexpected number of parameters for test execution failure policy option");
                const auto option = cmd.GetSwitchValue("epolicy", 0);
                if (option == "abort")
                {
                    return Policy::ExecutionFailure::Abort;
                }
                else if (option == "continue")
                {
                    return Policy::ExecutionFailure::Continue;
                }
                else if (option == "ignore")
                {
                    return Policy::ExecutionFailure::Ignore;
                }

                throw CommandLineOptionsException(AZStd::string::format("Unexpected value for test execution failure policy option: %s", option.c_str()));
            }

            return Policy::ExecutionFailure::Continue;
        }

        Policy::ExecutionFailureDrafting ParseExecutionFailureDraftingPolicy(const AZ::CommandLine& cmd)
        {
            const auto numSwitchValues = cmd.GetNumSwitchValues("rexecfailures");
            if (numSwitchValues)
            {
                AZ_TestImpact_Eval(numSwitchValues == 1, CommandLineOptionsException, "Unexpected parameters for test execution failure drafting policy option");
                const auto option = cmd.GetSwitchValue("rexecfailures", 0);
                if (option == "on")
                {
                    return Policy::ExecutionFailureDrafting::Always;
                }
                else if (option == "off")
                {
                    return Policy::ExecutionFailureDrafting::Never;
                }

                throw CommandLineOptionsException(AZStd::string::format("Unexpected value for test execution failure drafting policy option: %s", option.c_str()));
            }

            return Policy::ExecutionFailureDrafting::Always;
        }

        Policy::TestFailure ParseTestFailurePolicy(const AZ::CommandLine& cmd)
        {
            const auto numSwitchValues = cmd.GetNumSwitchValues("fpolicy");
            if (numSwitchValues)
            {
                AZ_TestImpact_Eval(numSwitchValues == 1, CommandLineOptionsException, "Unexpected parameters for test failure policy option");
                const auto option = cmd.GetSwitchValue("fpolicy", 0);
                if (option == "abort")
                {
                    return Policy::TestFailure::Abort;
                }
                else if (option == "continue")
                {
                    return Policy::TestFailure::Continue;
                }

                throw CommandLineOptionsException(AZStd::string::format("Unexpected value for test failure policy option: %s", option.c_str()));
            }

            return Policy::TestFailure::Abort;
        }

        Policy::IntegrityFailure ParseIntegrityFailurePolicy(const AZ::CommandLine& cmd)
        {
            const auto numSwitchValues = cmd.GetNumSwitchValues("ipolicy");
            if (numSwitchValues)
            {
                AZ_TestImpact_Eval(numSwitchValues == 1, CommandLineOptionsException, "Unexpected parameters for integrity failure policy option");
                const auto option = cmd.GetSwitchValue("ipolicy", 0);
                if (option == "abort")
                {
                    return Policy::IntegrityFailure::Abort;
                }
                else if (option == "continue")
                {
                    return Policy::IntegrityFailure::Continue;
                }

                throw CommandLineOptionsException(AZStd::string::format("Unexpected value for integrity failure policy option: %s", option.c_str()));
            }

            return Policy::IntegrityFailure::Abort;
        }

        Policy::TestSharding ParseTestShading(const AZ::CommandLine& cmd)
        {
            const auto numSwitchValues = cmd.GetNumSwitchValues("shard");
            if (numSwitchValues)
            {
                AZ_TestImpact_Eval(numSwitchValues == 1, CommandLineOptionsException, "Unexpected parameters for test sharding option");
                const auto option = cmd.GetSwitchValue("shard", 0);
                if (option == "on")
                {
                    return Policy::TestSharding::Always;
                }
                else if (option == "off")
                {
                    return Policy::TestSharding::Never;
                }

                throw CommandLineOptionsException(AZStd::string::format("Unexpected value for test sharding option: %s", option.c_str()));
            }

            return Policy::TestSharding::Never;
        }

        Policy::TargetOutputCapture ParseTargetOutputCapture(const AZ::CommandLine& cmd)
        {
            const auto numSwitchValues = cmd.GetNumSwitchValues("targetout");
            if (numSwitchValues)
            {
                AZ_TestImpact_Eval(numSwitchValues <= 2, CommandLineOptionsException, "Unexpected parameters for target output capture option");
                Policy::TargetOutputCapture targetOutputCapture = Policy::TargetOutputCapture::None;
                for (auto i = 0; i < numSwitchValues; i++)
                {
                    const auto option = cmd.GetSwitchValue("targetout", i);
                    if (option == "stdout")
                    {
                        if (targetOutputCapture == Policy::TargetOutputCapture::File)
                        {
                            targetOutputCapture = Policy::TargetOutputCapture::StdOutAndFile;
                        }
                        else
                        {
                            targetOutputCapture = Policy::TargetOutputCapture::StdOut;
                        }
                    }
                    else if (option == "file")
                    {
                        if (targetOutputCapture == Policy::TargetOutputCapture::StdOut)
                        {
                            targetOutputCapture = Policy::TargetOutputCapture::StdOutAndFile;
                        }
                        else
                        {
                            targetOutputCapture = Policy::TargetOutputCapture::File;
                        }
                    }
                    else
                    {
                        throw CommandLineOptionsException(AZStd::string::format("Unexpected value for target output capture option: %s", option.c_str()));
                    }
                }

                return targetOutputCapture;
            }

            return Policy::TargetOutputCapture::None;
        }

        size_t ParseUnsignedIntegerOrThrow(const char* str)
        {
            char* end = nullptr;
            auto value = strtoul(str, &end, 0);
            AZ_TestImpact_Eval(end != str, CommandLineOptionsException, AZStd::string::format("Couldn't parse unsigned integer option value: %s", str));
            return aznumeric_caster(value);
        }

        AZStd::optional<size_t> ParseMaxConcurrency(const AZ::CommandLine& cmd)
        {
            const auto numSwitchValues = cmd.GetNumSwitchValues("maxconcurrency");
            if (numSwitchValues)
            {
                AZ_TestImpact_Eval(numSwitchValues == 1, CommandLineOptionsException, "Unexpected parameters for max concurrency option");
                return ParseUnsignedIntegerOrThrow(cmd.GetSwitchValue("maxconcurrency", 0).c_str());
            }

            return AZStd::nullopt;
        }

        AZStd::optional<AZStd::chrono::milliseconds> ParseTestTargetTimeout(const AZ::CommandLine& cmd)
        {
            const auto numSwitchValues = cmd.GetNumSwitchValues("ttimeout");
            if (numSwitchValues)
            {
                AZ_TestImpact_Eval(numSwitchValues == 1, CommandLineOptionsException, "Unexpected parameters for test target timeout option");
                return AZStd::chrono::seconds(ParseUnsignedIntegerOrThrow(cmd.GetSwitchValue("ttimeout", 0).c_str()));
            }

            return AZStd::nullopt;
        }

        AZStd::optional<AZStd::chrono::milliseconds> ParseGlobalTimeout(const AZ::CommandLine& cmd)
        {
            const auto numSwitchValues = cmd.GetNumSwitchValues("gtimeout");
            if (numSwitchValues)
            {
                AZ_TestImpact_Eval(numSwitchValues == 1, CommandLineOptionsException, "Unexpected parameters for global timeout option");
                return AZStd::chrono::seconds(ParseUnsignedIntegerOrThrow(cmd.GetSwitchValue("gtimeout", 0).c_str()));
            }

            return AZStd::nullopt;
        }

        bool ParseSafeMode(const AZ::CommandLine& cmd)
        {
            const auto numSwitchValues = cmd.GetNumSwitchValues("safemode");
            if (numSwitchValues)
            {
                AZ_TestImpact_Eval(numSwitchValues == 1, CommandLineOptionsException, "Unexpected parameters for safe mode option");
                const auto option = cmd.GetSwitchValue("safemode", 0);
                if (option == "on")
                {
                    return true;
                }
                else if (option == "off")
                {
                    return false;
                }

                throw CommandLineOptionsException(AZStd::string::format("Unexpected value for test execution failure drafting policy option: %s", option.c_str()));
            }

            return false;
        }

        AZStd::unordered_set<AZStd::string> ParseSuitesFilter(const AZ::CommandLine& cmd)
        {
            AZStd::unordered_set<AZStd::string> suitesFilter;
            const auto numSwitchValues = cmd.GetNumSwitchValues("suites");
            if (numSwitchValues)
            {
                for (auto i = 0; i < numSwitchValues; i++)
                {
                    const auto value = cmd.GetSwitchValue("suites", i);
                    AZ_TestImpact_Eval(!value.empty(), CommandLineOptionsException, "Suites option value is empty");
                    if (value == "*")
                    {
                        AZ_TestImpact_Eval(suitesFilter.empty(), CommandLineOptionsException, "The * suite cannot be used with other suites");
                    }

                    suitesFilter.insert(value);
                }
            }

            if (suitesFilter.find("*") != suitesFilter.end())
            {
                return {};
            }

            return suitesFilter;
        }
    }

    AZStd::string CommandLineOptions::GetCommandLineUsageString()
    {
        AZStd::string help =
            "usage: tiaf [options]\n"
            "  options:\n"
            "    -config=<filename>                              Path to the configuration file for the TIAF runtime (default: \n"
            "                                                    <tiaf binay build dir>.<tiaf binary build type>.json).\n"
            "    -changelist=<filename>                          Path to the JSON of source file changes to perform test impact \n"
            "                                                    analysis on.\n"
            "    -gtimeout=<seconds>                             Global timeout value to terminate the entire test sequence should it \n"
            "                                                    be exceeded.\n"
            "    -ttimeout=<seconds>                             Timeout value to terminate individual test targets should it be \n"
            "                                                    exceeded.\n"
            "    -sequence=<none, seed, regular, tia, tiaorseed> The type of test sequence to perform, where none runs no tests and\n"
            "                                                    will report a all tests successful, seed removes any prior coverage \n"
            "                                                    data and runs all test targets with instrumentation to reseed the \n"
            "                                                    data from scratch, regular runs all of the test targets without any \n"
            "                                                    instrumentation to generate coverage data(any prior coverage data is \n"
            "                                                    left intact), tia uses any prior coverage data to run the instrumented \n"
            "                                                    subset of selected tests(if no prior coverage data a regular run is \n"
            "                                                    performed instead) and tiaorseed uses any prior coverage data to run \n"
            "                                                    the instrumented subset of selected tests(if no prior coverage data a \n"
            "                                                    seed run is performed instead).\n"
            "    -safemode=<on,off>                              Flag to specify a safe mode sequence where the set of unselected \n"
            "                                                    tests is run without instrumentation after the set of selected \n"
            "                                                    instrumented tests is run (this has the effect of ensuring all \n"
            "                                                    tests are run regardless).\n"
            "    -shard=<on,off>                                 Break any test targets with a sharding policy into the number of \n"
            "                                                    shards according to the maximum concurrency value.\n"
            "    -rexecfailures=<on,off>                         Attempt to execute test targets that previously failed to execute.\n"
            "    -targetout=<sdtout, file>                       Capture of individual test run stdout, where stdout will capture \n"
            "                                                    each individual test target's stdout and output each one to stdout \n"
            "                                                    and file will capture each individual test target's stdout and output \n"
            "                                                    each one individually to a file (multiple values are accepted).\n"
            "    -epolicy=<abort, continue, ignore>              Policy for handling test execution failure (test targets could not be \n"
            "                                                    launched due to the binary not being built, incorrect paths, etc.), \n"
            "                                                    where abort will abort the entire test sequence upon the first test\n"
            "                                                    target execution failureand report a failure(along with the return \n"
            "                                                    code of the test target that failed to launch), continue will continue \n"
            "                                                    with the test sequence in the event of test target execution failures\n"
            "                                                    and treat the test targets that failed to launch as as test failures\n"
            "                                                    (along with the return codes of the test targets that failed to \n"
            "                                                    launch), ignore will continue with the test sequence in the event of \n"
            "                                                    test target execution failuresand treat the test targets that failed\n"
            "                                                    to launch as as test passes(along with the return codes of the test \n"
            "                                                    targets that failed to launch).\n"
            "    -fpolicy <abort, continue>                      Policy for handling test failures (test targets report failing tests), \n"
            "                                                    where abort will abort the entire test sequenceupon the first test \n"
            "                                                    failureand report a failure and continue will continue with the test\n"
            "                                                    sequence in the event of test failuresand report the test failures.\n"
            "    -ipolicy=<abort, seed, rerun>                   Policy for handling coverage data integrity failures, where abort will \n"
            "                                                    abort the test sequenceand report a failure, seed will attempt another \n"
            "                                                    sequence using the seed sequence type, otherwise will abort and report \n"
            "                                                    a failure (this option has no effect for regularand seed sequence \n"
            "                                                    types) and rerun will attempt another sequence using the regular \n"
            "                                                    sequence type, otherwise will abortand report a failure(this option has \n"
            "                                                    no effect for regular sequence type).\n"
            "    -ppolicy=<none, locality>                       Policy for prioritizing selected test targets, where none will not \n"
            "                                                    attempt any test target prioritization and locality will attempt to \n"
            "                                                    prioritize test targets according to the locality of their covering \n"
            "                                                    production targets in the dependency graph(if no dependency graph data \n"
            "                                                    available, no prioritization will occur).\n"
            "    -maxconcurrency=<number>                        The maximum number of concurrent test targets/shards to be in flight at \n"
            "                                                    any given moment.\n"
            "    -ochangelist=<on,off>                           Outputs the change list used for test selection.\n"
            "    -suites=<names>                                 The test suites to select from for this test sequence (multiple values are \n"
            "                                                    allowed). The suite all has special significance and will allow tests from \n"
            "                                                    any suite to be selected, however this particular suite is mutually exclusive\n"
            "                                                    with other suite. Note: this option is only applicable to the regular sequence\n"
            "                                                    and, if safe mode is enables, the tia and tiaorseed sequences.";

        return help;
    }

    CommandLineOptions::CommandLineOptions(int argc, char** argv)
    {
        AZ::CommandLine cmd;
        cmd.Parse(argc, argv);

        m_configurationFile = ParseConfigurationFile(cmd);
        m_changeListFile = ParseChangeListFile(cmd);
        m_outputChangeList = ParseOutputChangeList(cmd);
        m_testSequenceType = ParseTestSequenceType(cmd);
        m_testPrioritizationPolicy = ParseTestPrioritizationPolicy(cmd);
        m_executionFailurePolicy = ParseExecutionFailurePolicy(cmd);
        m_executionFailureDraftingPolicy = ParseExecutionFailureDraftingPolicy(cmd);
        m_testFailurePolicy = ParseTestFailurePolicy(cmd);
        m_integrityFailurePolicy = ParseIntegrityFailurePolicy(cmd);
        m_testShardingPolicy = ParseTestShading(cmd);
        m_targetOutputCapture = ParseTargetOutputCapture(cmd);
        m_maxConcurrency = ParseMaxConcurrency(cmd);
        m_testTargetTimeout = ParseTestTargetTimeout(cmd);
        m_globalTimeout = ParseGlobalTimeout(cmd);
        m_safeMode = ParseSafeMode(cmd);
        m_suitesFilter = ParseSuitesFilter(cmd);
    }
 
    bool CommandLineOptions::HasChangeListFile() const
    {
        return m_changeListFile.has_value();
    }

    bool CommandLineOptions::HasTestSequence() const
    {
        return m_testSequenceType.has_value();
    }

    bool CommandLineOptions::HasSafeMode() const
    {
        return m_safeMode;
    }

    const AZStd::optional<RepoPath>& CommandLineOptions::GetChangeListFile() const
    {
        return m_changeListFile;
    }

    bool CommandLineOptions::HasOutputChangeList() const
    {
        return m_outputChangeList;
    }

    const RepoPath& CommandLineOptions::GetConfigurationFile() const
    {
        return m_configurationFile;
    }

    const AZStd::optional<TestSequenceType>& CommandLineOptions::GetTestSequenceType() const
    {
        return m_testSequenceType;
    }
    
    Policy::TestPrioritization CommandLineOptions::GetTestPrioritizationPolicy() const
    {
        return m_testPrioritizationPolicy;
    }
    
    Policy::ExecutionFailure CommandLineOptions::GetExecutionFailurePolicy() const
    {
        return m_executionFailurePolicy;
    }
    
    Policy::ExecutionFailureDrafting CommandLineOptions::GetExecutionFailureDraftingPolicy() const
    {
        return m_executionFailureDraftingPolicy;
    }
    
    Policy::TestFailure CommandLineOptions::GetTestFailurePolicy() const
    {
        return m_testFailurePolicy;
    }

    Policy::IntegrityFailure CommandLineOptions::GetIntegrityFailurePolicy() const
    {
        return m_integrityFailurePolicy;
    }
    
    Policy::TestSharding CommandLineOptions::GetTestShardingPolicy() const
    {
        return m_testShardingPolicy;
    }
    
    Policy::TargetOutputCapture CommandLineOptions::GetTargetOutputCapture() const
    {
        return m_targetOutputCapture;
    }
    
    const AZStd::optional<size_t>& CommandLineOptions::GetMaxConcurrency() const
    {
        return m_maxConcurrency;
    }
    
    const AZStd::optional<AZStd::chrono::milliseconds>& CommandLineOptions::GetTestTargetTimeout() const
    {
        return m_testTargetTimeout;
    }
    
    const AZStd::optional<AZStd::chrono::milliseconds>& CommandLineOptions::GetGlobalTimeout() const
    {
        return m_globalTimeout;
    }

    const AZStd::unordered_set<AZStd::string>& CommandLineOptions::GetSuitesFilter() const
    {
        return m_suitesFilter;
    }
}
