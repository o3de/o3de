/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TestImpactFramework/TestImpactUtils.h>

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
            ConfigKey,
            DataFileKey,
            ChangeListKey,
            SequenceReportKey,
            SequenceKey,
            TestPrioritizationPolicyKey,
            ExecutionFailurePolicyKey,
            FailedTestCoveragePolicyKey,
            TestFailurePolicyKey,
            IntegrityFailurePolicyKey,
            TestShardingPolicyKey,
            TargetOutputCaptureKey,
            MaxConcurrencyKey,
            TestTargetTimeoutKey,
            GlobalTimeoutKey,
            SuiteFilterKey,
            SafeModeKey,
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
            Discard,
            Keep
        };

        constexpr const char* OptionKeys[] =
        {
            // Options
            "config",
            "datafile",
            "changelist",
            "report",
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
            "discard",
            "keep"
        };

        RepoPath ParseConfigurationFile(const AZ::CommandLine& cmd)
        {
            return ParsePathOption(OptionKeys[ConfigKey], cmd).value_or(LY_TEST_IMPACT_DEFAULT_CONFIG_FILE);
        }

        AZStd::optional<RepoPath> ParseDataFile(const AZ::CommandLine& cmd)
        {
            return ParsePathOption(OptionKeys[DataFileKey], cmd);
        }

        AZStd::optional<RepoPath> ParseChangeListFile(const AZ::CommandLine& cmd)
        {
            return ParsePathOption(OptionKeys[ChangeListKey], cmd);
        }

        AZStd::optional<RepoPath> ParseSequenceReportFile(const AZ::CommandLine& cmd)
        {
            return ParsePathOption(OptionKeys[SequenceReportKey], cmd);
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

            return ParseMultiStateOption(OptionKeys[SequenceKey], states, cmd).value_or(TestSequenceType::None);
        }

        Policy::TestPrioritization ParseTestPrioritizationPolicy(const AZ::CommandLine& cmd)
        {
            const BinaryStateOption<Policy::TestPrioritization> states =
            {
                {OptionKeys[None], Policy::TestPrioritization::None},
                {OptionKeys[Locality], Policy::TestPrioritization::DependencyLocality}
            };

            return ParseBinaryStateOption(OptionKeys[TestPrioritizationPolicyKey], states, cmd).value_or(Policy::TestPrioritization::None);
        }

        Policy::ExecutionFailure ParseExecutionFailurePolicy(const AZ::CommandLine& cmd)
        {
            const AZStd::vector<AZStd::pair<AZStd::string, Policy::ExecutionFailure>> states =
            {
                {OptionKeys[Abort], Policy::ExecutionFailure::Abort},
                {OptionKeys[Continue], Policy::ExecutionFailure::Continue},
                {OptionKeys[Ignore], Policy::ExecutionFailure::Ignore}
            };
            return ParseMultiStateOption(OptionKeys[ExecutionFailurePolicyKey], states, cmd).value_or(Policy::ExecutionFailure::Continue);
        }

        Policy::FailedTestCoverage ParseFailedTestCoveragePolicy(const AZ::CommandLine& cmd)
        {
            const AZStd::vector<AZStd::pair<AZStd::string, Policy::FailedTestCoverage>> states =
            {
                {OptionKeys[Discard], Policy::FailedTestCoverage::Discard},
                {OptionKeys[Keep], Policy::FailedTestCoverage::Keep}
            };

            return ParseMultiStateOption(OptionKeys[FailedTestCoveragePolicyKey], states, cmd).value_or(Policy::FailedTestCoverage::Keep);
        }

        Policy::TestFailure ParseTestFailurePolicy(const AZ::CommandLine& cmd)
        {
            const BinaryStateValue<Policy::TestFailure> states =
            {
                Policy::TestFailure::Abort,
                Policy::TestFailure::Continue
            };

            return ParseAbortContinueOption(OptionKeys[TestFailurePolicyKey], states, cmd).value_or(Policy::TestFailure::Abort);
        }

        Policy::IntegrityFailure ParseIntegrityFailurePolicy(const AZ::CommandLine& cmd)
        {
            const BinaryStateValue<Policy::IntegrityFailure> states =
            {
                Policy::IntegrityFailure::Abort,
                Policy::IntegrityFailure::Continue
            };

            return ParseAbortContinueOption(OptionKeys[IntegrityFailurePolicyKey], states, cmd).value_or(Policy::IntegrityFailure::Abort);
        }

        Policy::TestSharding ParseTestShardingPolicy(const AZ::CommandLine& cmd)
        {
            const BinaryStateValue<Policy::TestSharding> states =
            {
                Policy::TestSharding::Never,
                Policy::TestSharding::Always
            };

            return ParseOnOffOption(OptionKeys[TestShardingPolicyKey], states, cmd).value_or(Policy::TestSharding::Never);
        }

        Policy::TargetOutputCapture ParseTargetOutputCapture(const AZ::CommandLine& cmd)
        {
            if (const auto numSwitchValues = cmd.GetNumSwitchValues(OptionKeys[TargetOutputCaptureKey]);
                numSwitchValues)
            {
                AZ_TestImpact_Eval(
                    numSwitchValues <= 2, CommandLineOptionsException, "Unexpected parameters for target output capture option");

                Policy::TargetOutputCapture targetOutputCapture = Policy::TargetOutputCapture::None;
                for (auto i = 0; i < numSwitchValues; i++)
                {
                    const auto option = cmd.GetSwitchValue(OptionKeys[TargetOutputCaptureKey], i);
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
            return ParseUnsignedIntegerOption(OptionKeys[MaxConcurrencyKey], cmd);
        }

        AZStd::optional<AZStd::chrono::milliseconds> ParseTestTargetTimeout(const AZ::CommandLine& cmd)
        {
            return ParseSecondsOption(OptionKeys[TestTargetTimeoutKey], cmd);
        }

        AZStd::optional<AZStd::chrono::milliseconds> ParseGlobalTimeout(const AZ::CommandLine& cmd)
        {
            return ParseSecondsOption(OptionKeys[GlobalTimeoutKey], cmd);
        }

        bool ParseSafeMode(const AZ::CommandLine& cmd)
        {
            const BinaryStateValue<bool> states = { false, true };
            return ParseOnOffOption(OptionKeys[SafeModeKey], states, cmd).value_or(false);
        }

        SuiteType ParseSuiteFilter(const AZ::CommandLine& cmd)
        {
            const AZStd::vector<AZStd::pair<AZStd::string, SuiteType>> states =
            {
                { SuiteTypeAsString(SuiteType::Main), SuiteType::Main },
                { SuiteTypeAsString(SuiteType::Periodic), SuiteType::Periodic },
                { SuiteTypeAsString(SuiteType::Sandbox), SuiteType::Sandbox },
                { SuiteTypeAsString(SuiteType::AWSI), SuiteType::AWSI }
            };

            return ParseMultiStateOption(OptionKeys[SuiteFilterKey], states, cmd).value_or(SuiteType::Main);
        }
    }

    CommandLineOptions::CommandLineOptions(int argc, char** argv)
    {
        AZ::CommandLine cmd;
        cmd.Parse(argc, argv);

        m_configurationFile = ParseConfigurationFile(cmd);
        m_dataFile = ParseDataFile(cmd);
        m_changeListFile = ParseChangeListFile(cmd);
        m_sequenceReportFile = ParseSequenceReportFile(cmd);
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

    bool CommandLineOptions::HasDataFilePath() const
    {
        return m_dataFile.has_value();
    }
 
    bool CommandLineOptions::HasChangeListFilePath() const
    {
        return m_changeListFile.has_value();
    }

    bool CommandLineOptions::HasSequenceReportFilePath() const
    {
        return m_sequenceReportFile.has_value();
    }

    bool CommandLineOptions::HasSafeMode() const
    {
        return m_safeMode;
    }

    const AZStd::optional<RepoPath>& CommandLineOptions::GetDataFilePath() const
    {
        return m_dataFile;
    }

    const AZStd::optional<RepoPath>& CommandLineOptions::GetChangeListFilePath() const
    {
        return m_changeListFile;
    }

    const AZStd::optional<RepoPath>& CommandLineOptions::GetSequenceReportFilePath() const
    {
        return m_sequenceReportFile;
    }

    const RepoPath& CommandLineOptions::GetConfigurationFilePath() const
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
            "    -config=<filename>                                          Path to the configuration file for the TIAF runtime (default: \n"
            "                                                                <tiaf binay build dir>.<tiaf binary build type>.json).\n"
            "    -datafile=<filename>                                        Optional path to a test impact data file that will used instead of that\n"
            "                                                                specified in the config file.\n"
            "    -changelist=<filename>                                      Path to the JSON of source file changes to perform test impact \n"
            "                                                                analysis on.\n"
            "    -report=<filename>                                          Path to where the sequence report file will be written (if this option \n"
            "                                                                is not specified, no report will be written).\n"
            "    -gtimeout=<seconds>                                         Global timeout value to terminate the entire test sequence should it \n"
            "                                                                be exceeded.\n"
            "    -ttimeout=<seconds>                                         Timeout value to terminate individual test targets should it be \n"
            "                                                                exceeded.\n"
            "    -sequence=<none, seed, regular, tia, tianowrite, tiaorseed> The type of test sequence to perform, where 'none' runs no tests and\n"
            "                                                                will report a all tests successful, 'seed' removes any prior coverage \n"
            "                                                                data and runs all test targets with instrumentation to reseed the \n"
            "                                                                data from scratch, 'regular' runs all of the test targets without any \n"
            "                                                                instrumentation to generate coverage data(any prior coverage data is \n"
            "                                                                left intact), 'tia' uses any prior coverage data to run the instrumented \n"
            "                                                                subset of selected tests(if no prior coverage data a regular run is \n"
            "                                                                performed instead), 'tianowrite' uses any prior coverage data to run the \n"
            "                                                                uninstrumented subset of selected tests (if no prior coverage data a \n"
            "                                                                regular run is performed instead). The coverage data is not updated with \n"
            "                                                                the subset of selected tests and 'tiaorseed' uses any prior coverage data \n"
            "                                                                to run the instrumented subset of selected tests (if no prior coverage \n"
            "                                                                data a seed run is performed instead).\n"
            "    -safemode=<on,off>                                          Flag to specify a safe mode sequence where the set of unselected \n"
            "                                                                tests is run without instrumentation after the set of selected \n"
            "                                                                instrumented tests is run (this has the effect of ensuring all \n"
            "                                                                tests are run regardless).\n"
            "    -shard=<on,off>                                             Break any test targets with a sharding policy into the number of \n"
            "                                                                shards according to the maximum concurrency value.\n"
            "    -cpolicy=<remove, keep>                                     Policy for handling the coverage data of failing tests, where 'discard' \n"
            "                                                                will discard the coverage data produced by the failing tests, causing \n"
            "                                                                them to be drafted into future test runs and 'keep' will keep any existing \n"
            "                                                                coverage data and update the coverage data for failed tests that produce \n"
            "                                                                coverage.\n"
            "    -targetout=<sdtout, file>                                   Capture of individual test run stdout, where 'stdout' will capture \n"
            "                                                                each individual test target's stdout and output each one to stdout \n"
            "                                                                and 'file' will capture each individual test target's stdout and output \n"
            "                                                                each one individually to a file (multiple values are accepted).\n"
            "    -epolicy=<abort, continue, ignore>                          Policy for handling test execution failure (test targets could not be \n"
            "                                                                launched due to the binary not being built, incorrect paths, etc.), \n"
            "                                                                where 'abort' will abort the entire test sequence upon the first test\n"
            "                                                                target execution failure and report a failure(along with the return \n"
            "                                                                code of the test target that failed to launch), 'continue' will continue \n"
            "                                                                with the test sequence in the event of test target execution failures\n"
            "                                                                and treat the test targets that failed to launch as test failures\n"
            "                                                                (along with the return codes of the test targets that failed to \n"
            "                                                                launch), 'ignore' will continue with the test sequence in the event of \n"
            "                                                                test target execution failures and treat the test targets that failed\n"
            "                                                                to launch as test passes(along with the return codes of the test \n"
            "                                                                targets that failed to launch).\n"
            "    -fpolicy <abort, continue>                                  Policy for handling test failures (test targets report failing tests), \n"
            "                                                                where 'abort' will abort the entire test sequence upon the first test \n"
            "                                                                failure and report a failure and 'continue' will continue with the test\n"
            "                                                                sequence in the event of test failures and report the test failures.\n"
            "    -ipolicy=<abort, seed, rerun>                               Policy for handling coverage data integrity failures, where 'abort' will \n"
            "                                                                abort the test sequence and report a failure, 'seed' will attempt another \n"
            "                                                                sequence using the seed sequence type, otherwise will abort and report \n"
            "                                                                a failure (this option has no effect for regular and seed sequence \n"
            "                                                                types) and 'rerun' will attempt another sequence using the regular \n"
            "                                                                sequence type, otherwise will abort and report a failure(this option has \n"
            "                                                                no effect for regular sequence type).\n"
            "    -ppolicy=<none, locality>                                   Policy for prioritizing selected test targets, where 'none' will not \n"
            "                                                                attempt any test target prioritization and 'locality' will attempt to \n"
            "                                                                prioritize test targets according to the locality of their covering \n"
            "                                                                production targets in the dependency graph(if no dependency graph data \n"
            "                                                                available, no prioritization will occur).\n"
            "    -maxconcurrency=<number>                                    The maximum number of concurrent test targets/shards to be in flight at \n"
            "                                                                any given moment.\n"
            "    -suite=<main, periodic, sandbox, awsi>                      The test suite to select from for this test sequence.";

        return help;
    }
} // namespace TestImpact
