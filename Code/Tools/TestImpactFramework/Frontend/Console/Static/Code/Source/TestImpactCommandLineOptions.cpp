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
#include <TestImpactCommandLineOptionsUtils.h>

#include <AzCore/Settings/CommandLine.h>

namespace TestImpact
{
    namespace
    {
        enum 
        {
            // Options
            Config,
            ChangeList,
            OutputChangeList,
            Sequence,
            TestPrioritizationPolicy,
            ExecutionFailurePolicy,
            FailedTestCoveragePolicy,
            TestFailurePolicy,
            IntegrityFailurePolicy,
            TestShardingPolicy,
            TargetOutputCapture,
            MaxConcurrency,
            TestTargetTimeout,
            GlobalTimeout,
            SuiteFilter,
            SafeMode,
            // Values
            None,
            Seed,
            Regular,
            ImpactAnalysis,
            ImpactAnalysisNoWrite,
            ImpactAnalysisOrSeed,
            Locality,
            Abort,
            Continue,
            Ignore,
            StdOut,
            File,
            Remove,
            Keep
        };

        constexpr const char* OptionKeys[] =
        {
            // Options
            "config",
            "changelist",
            "ochangelist",
            "sequence",
            "ppolicy",
            "epolicy",
            "cpolicy",
            "fpolicy",
            "ipolicy",
            "shard",
            "targetout",
            "maxconcurrency",
            "ttimeout",
            "gtimeout",
            "suite",
            "safemode",
            // Values
            "none",
            "seed",
            "regular",
            "tia",
            "tianowrite",
            "tiaorseed",
            "locality",
            "abort",
            "continue",
            "ignore",
            "stdout",
            "file",
            "remove",
            "keep"
        };

        RepoPath ParseConfigurationFile(const AZ::CommandLine& cmd)
        {
            return ParsePathOption(OptionKeys[Config], cmd).value_or(LY_TEST_IMPACT_DEFAULT_CONFIG_FILE);
        }

        AZStd::optional<RepoPath> ParseChangeListFile(const AZ::CommandLine& cmd)
        {
            return ParsePathOption(OptionKeys[ChangeList], cmd);
        }

        bool ParseOutputChangeList(const AZ::CommandLine& cmd)
        {
            return ParseOnOffOption(OptionKeys[OutputChangeList], BinaryStateValue<bool>{ false, true }, cmd).value_or(false);
        }

        TestSequenceType ParseTestSequenceType(const AZ::CommandLine& cmd)
        {
            const AZStd::vector<AZStd::pair<AZStd::string, TestSequenceType>> states =
            {
                {OptionKeys[None], TestSequenceType::None},
                {OptionKeys[Seed], TestSequenceType::Seed},
                {OptionKeys[Regular], TestSequenceType::Regular},
                {OptionKeys[ImpactAnalysis], TestSequenceType::ImpactAnalysis},
                {OptionKeys[ImpactAnalysisNoWrite], TestSequenceType::ImpactAnalysisNoWrite},
                {OptionKeys[ImpactAnalysisOrSeed], TestSequenceType::ImpactAnalysisOrSeed}
            };

            return ParseMultiStateOption(OptionKeys[Sequence], states, cmd).value_or(TestSequenceType::None);
        }

        Policy::TestPrioritization ParseTestPrioritizationPolicy(const AZ::CommandLine& cmd)
        {
            const BinaryStateOption<Policy::TestPrioritization> states =
            {
                {OptionKeys[None], Policy::TestPrioritization::None},
                {OptionKeys[Locality], Policy::TestPrioritization::DependencyLocality}
            };

            return ParseBinaryStateOption(OptionKeys[TestPrioritizationPolicy], states, cmd).value_or(Policy::TestPrioritization::None);
        }

        Policy::ExecutionFailure ParseExecutionFailurePolicy(const AZ::CommandLine& cmd)
        {
            const AZStd::vector<AZStd::pair<AZStd::string, Policy::ExecutionFailure>> states =
            {
                {OptionKeys[Abort], Policy::ExecutionFailure::Abort},
                {OptionKeys[Continue], Policy::ExecutionFailure::Continue},
                {OptionKeys[Ignore], Policy::ExecutionFailure::Ignore}
            };
            return ParseMultiStateOption(OptionKeys[ExecutionFailurePolicy], states, cmd).value_or(Policy::ExecutionFailure::Continue);
        }

        Policy::FailedTestCoverage ParseFailedTestCoveragePolicy(const AZ::CommandLine& cmd)
        {
            const AZStd::vector<AZStd::pair<AZStd::string, Policy::FailedTestCoverage>> states =
            {
                {OptionKeys[Remove], Policy::FailedTestCoverage::Remove},
                {OptionKeys[Keep], Policy::FailedTestCoverage::Keep}
            };

            return ParseMultiStateOption(OptionKeys[FailedTestCoveragePolicy], states, cmd).value_or(Policy::FailedTestCoverage::Keep);
        }

        Policy::TestFailure ParseTestFailurePolicy(const AZ::CommandLine& cmd)
        {
            const BinaryStateValue<Policy::TestFailure> states =
            {
                Policy::TestFailure::Abort,
                Policy::TestFailure::Continue
            };

            return ParseAbortContinueOption(OptionKeys[TestFailurePolicy], states, cmd).value_or(Policy::TestFailure::Abort);
        }

        Policy::IntegrityFailure ParseIntegrityFailurePolicy(const AZ::CommandLine& cmd)
        {
            const BinaryStateValue<Policy::IntegrityFailure> states =
            {
                Policy::IntegrityFailure::Abort,
                Policy::IntegrityFailure::Continue
            };

            return ParseAbortContinueOption(OptionKeys[IntegrityFailurePolicy], states, cmd).value_or(Policy::IntegrityFailure::Abort);
        }

        Policy::TestSharding ParseTestShardingPolicy(const AZ::CommandLine& cmd)
        {
            const BinaryStateValue<Policy::TestSharding> states =
            {
                Policy::TestSharding::Never,
                Policy::TestSharding::Always
            };

            return ParseOnOffOption("shard", states, cmd).value_or(Policy::TestSharding::Never);
        }

        Policy::TargetOutputCapture ParseTargetOutputCapture(const AZ::CommandLine& cmd)
        {
            if (const auto numSwitchValues = cmd.GetNumSwitchValues(OptionKeys[TargetOutputCapture]);
                numSwitchValues)
            {
                AZ_TestImpact_Eval(
                    numSwitchValues <= 2, CommandLineOptionsException, "Unexpected parameters for target output capture option");

                Policy::TargetOutputCapture targetOutputCapture = Policy::TargetOutputCapture::None;
                for (auto i = 0; i < numSwitchValues; i++)
                {
                    const auto option = cmd.GetSwitchValue(OptionKeys[TargetOutputCapture], i);
                    if (option == OptionKeys[StdOut])
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
                    else if (option == OptionKeys[File])
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
                        throw CommandLineOptionsException(
                            AZStd::string::format("Unexpected value for target output capture option: %s", option.c_str()));
                    }
                }

                return targetOutputCapture;
            }

            return Policy::TargetOutputCapture::None;
        }

        AZStd::optional<size_t> ParseMaxConcurrency(const AZ::CommandLine& cmd)
        {
            return ParseUnsignedIntegerOption(OptionKeys[MaxConcurrency], cmd);
        }

        AZStd::optional<AZStd::chrono::milliseconds> ParseTestTargetTimeout(const AZ::CommandLine& cmd)
        {
            return ParseSecondsOption(OptionKeys[TestTargetTimeout], cmd);
        }

        AZStd::optional<AZStd::chrono::milliseconds> ParseGlobalTimeout(const AZ::CommandLine& cmd)
        {
            return ParseSecondsOption(OptionKeys[GlobalTimeout], cmd);
        }

        bool ParseSafeMode(const AZ::CommandLine& cmd)
        {
            const BinaryStateValue<bool> states = { false, true };
            return ParseOnOffOption(OptionKeys[SafeMode], states, cmd).value_or(false);
        }

        SuiteType ParseSuiteFilter(const AZ::CommandLine& cmd)
        {
            const AZStd::vector<AZStd::pair<AZStd::string, SuiteType>> states =
            {
                {GetSuiteTypeName(SuiteType::Main), SuiteType::Main},
                {GetSuiteTypeName(SuiteType::Periodic), SuiteType::Periodic},
                {GetSuiteTypeName(SuiteType::Sandbox), SuiteType::Sandbox}
            };

            return ParseMultiStateOption(OptionKeys[SuiteFilter], states, cmd).value_or(SuiteType::Main);
        }
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
        m_failedTestCoveragePolicy = ParseFailedTestCoveragePolicy(cmd);
        m_testFailurePolicy = ParseTestFailurePolicy(cmd);
        m_integrityFailurePolicy = ParseIntegrityFailurePolicy(cmd);
        m_testShardingPolicy = ParseTestShardingPolicy(cmd);
        m_targetOutputCapture = ParseTargetOutputCapture(cmd);
        m_maxConcurrency = ParseMaxConcurrency(cmd);
        m_testTargetTimeout = ParseTestTargetTimeout(cmd);
        m_globalTimeout = ParseGlobalTimeout(cmd);
        m_safeMode = ParseSafeMode(cmd);
        m_suiteFilter = ParseSuiteFilter(cmd);
    }
 
    bool CommandLineOptions::HasChangeListFile() const
    {
        return m_changeListFile.has_value();
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

    TestSequenceType CommandLineOptions::GetTestSequenceType() const
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
    
    Policy::FailedTestCoverage CommandLineOptions::GetFailedTestCoveragePolicy() const
    {
        return m_failedTestCoveragePolicy;
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

    SuiteType CommandLineOptions::GetSuiteFilter() const
    {
        return m_suiteFilter;
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
            "    -cpolicy=<remove, keep>                         Policy for handling the coverage data of failed tests (both test that \n"
            "                                                    failed to execute and tests that ran but failed), where remove will \n"
            "                                                    remove the failed tests from the all coverage data(causing them to be \n"
            "                                                    drafted into future test runs) and keep will keep any existing coverage \n"
            "                                                    data and update the coverage data for failed tests that produce coverage.\n"
            "    -targetout=<sdtout, file>                       Capture of individual test run stdout, where stdout will capture \n"
            "                                                    each individual test target's stdout and output each one to stdout \n"
            "                                                    and file will capture each individual test target's stdout and output \n"
            "                                                    each one individually to a file (multiple values are accepted).\n"
            "    -epolicy=<abort, continue, ignore>              Policy for handling test execution failure (test targets could not be \n"
            "                                                    launched due to the binary not being built, incorrect paths, etc.), \n"
            "                                                    where abort will abort the entire test sequence upon the first test\n"
            "                                                    target execution failure and report a failure(along with the return \n"
            "                                                    code of the test target that failed to launch), continue will continue \n"
            "                                                    with the test sequence in the event of test target execution failures\n"
            "                                                    and treat the test targets that failed to launch as as test failures\n"
            "                                                    (along with the return codes of the test targets that failed to \n"
            "                                                    launch), ignore will continue with the test sequence in the event of \n"
            "                                                    test target execution failures and treat the test targets that failed\n"
            "                                                    to launch as test passes(along with the return codes of the test \n"
            "                                                    targets that failed to launch).\n"
            "    -fpolicy <abort, continue>                      Policy for handling test failures (test targets report failing tests), \n"
            "                                                    where abort will abort the entire test sequence upon the first test \n"
            "                                                    failure and report a failure and continue will continue with the test\n"
            "                                                    sequence in the event of test failures and report the test failures.\n"
            "    -ipolicy=<abort, seed, rerun>                   Policy for handling coverage data integrity failures, where abort will \n"
            "                                                    abort the test sequenceand report a failure, seed will attempt another \n"
            "                                                    sequence using the seed sequence type, otherwise will abort and report \n"
            "                                                    a failure (this option has no effect for regular and seed sequence \n"
            "                                                    types) and rerun will attempt another sequence using the regular \n"
            "                                                    sequence type, otherwise will abort and report a failure(this option has \n"
            "                                                    no effect for regular sequence type).\n"
            "    -ppolicy=<none, locality>                       Policy for prioritizing selected test targets, where none will not \n"
            "                                                    attempt any test target prioritization and locality will attempt to \n"
            "                                                    prioritize test targets according to the locality of their covering \n"
            "                                                    production targets in the dependency graph(if no dependency graph data \n"
            "                                                    available, no prioritization will occur).\n"
            "    -maxconcurrency=<number>                        The maximum number of concurrent test targets/shards to be in flight at \n"
            "                                                    any given moment.\n"
            "    -ochangelist=<on,off>                           Outputs the change list used for test selection.\n"
            "    -suite=<main, periodic, sandbox>                The test suite to select from for this test sequence.";

        return help;
    }
} // namespace TestImpact
