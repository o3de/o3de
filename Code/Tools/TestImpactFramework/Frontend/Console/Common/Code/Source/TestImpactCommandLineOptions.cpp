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
    namespace CommonOptions
    {
        enum Fields
        {
            // Options
            ConfigKey,
            DataFileKey,
            PreviousRunDataFileKey,
            ChangeListKey,
            SequenceReportKey,
            SequenceKey,
            TestPrioritizationPolicyKey,
            ExecutionFailurePolicyKey,
            FailedTestCoveragePolicyKey,
            TestFailurePolicyKey,
            IntegrityFailurePolicyKey,
            TargetOutputCaptureKey,
            TestTargetTimeoutKey,
            GlobalTimeoutKey,
            SuiteSetKey,
            SuiteLabelExcludeKey,
            DraftFailingTestsKey,
            ExcludedTestsKey,
            SafeModeKey,
            TestRunnerPolicy,
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
            Keep,
            // Checksum
            _CHECKSUM_
        };

        constexpr const char* Keys[] = {
            // Options
            "config",
            "datafile",
            "previousrundatafile",
            "changelist",
            "report",
            "sequence",
            "ppolicy",
            "epolicy",
            "cpolicy",
            "fpolicy",
            "ipolicy",
            "targetout",
            "ttimeout",
            "gtimeout",
            "suites",
            "labelexcludes",
            "draftfailingtests",
            "excluded",
            "safemode",
            "testrunner",
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
            "keep",
        };
    } // namespace CommonOptions

    RepoPath ParseConfigurationFile(const AZ::CommandLine& cmd)
    {
        return ParsePathOption(CommonOptions::Keys[CommonOptions::ConfigKey], cmd).value_or(LY_TEST_IMPACT_DEFAULT_CONFIG_FILE);
    }

    AZStd::optional<RepoPath> ParseDataFile(const AZ::CommandLine& cmd)
    {
        return ParsePathOption(CommonOptions::Keys[CommonOptions::DataFileKey], cmd);
    }

    AZStd::optional<RepoPath> ParsePreviousRunDataFile(const AZ::CommandLine& cmd)
    {
        return ParsePathOption(CommonOptions::Keys[CommonOptions::PreviousRunDataFileKey], cmd);
    }

    AZStd::optional<RepoPath> ParseChangeListFile(const AZ::CommandLine& cmd)
    {
        return ParsePathOption(CommonOptions::Keys[CommonOptions::ChangeListKey], cmd);
    }

    AZStd::optional<RepoPath> ParseSequenceReportFile(const AZ::CommandLine& cmd)
    {
        return ParsePathOption(CommonOptions::Keys[CommonOptions::SequenceReportKey], cmd);
    }

    TestSequenceType ParseTestSequenceType(const AZ::CommandLine& cmd)
    {
        const AZStd::vector<AZStd::pair<AZStd::string, TestSequenceType>> states =
        {
            {CommonOptions::Keys[CommonOptions::None], TestSequenceType::None},
            {CommonOptions::Keys[CommonOptions::Seed], TestSequenceType::Seed},
            {CommonOptions::Keys[CommonOptions::Regular], TestSequenceType::Regular},
            {CommonOptions::Keys[CommonOptions::ImpactAnalysis], TestSequenceType::ImpactAnalysis},
            {CommonOptions::Keys[CommonOptions::ImpactAnalysisNoWrite], TestSequenceType::ImpactAnalysisNoWrite},
            {CommonOptions::Keys[CommonOptions::ImpactAnalysisOrSeed], TestSequenceType::ImpactAnalysisOrSeed}
        };

        return ParseMultiStateOption(CommonOptions::Keys[CommonOptions::SequenceKey], states, cmd).value_or(TestSequenceType::None);
    }

    Policy::TestPrioritization ParseTestPrioritizationPolicy(const AZ::CommandLine& cmd)
    {
        const BinaryStateOption<Policy::TestPrioritization> states =
        {
            {CommonOptions::Keys[CommonOptions::None], Policy::TestPrioritization::None},
            {CommonOptions::Keys[CommonOptions::Locality], Policy::TestPrioritization::DependencyLocality}
        };

        return ParseBinaryStateOption(CommonOptions::Keys[CommonOptions::TestPrioritizationPolicyKey], states, cmd).value_or(Policy::TestPrioritization::None);
    }

    Policy::ExecutionFailure ParseExecutionFailurePolicy(const AZ::CommandLine& cmd)
    {
        const AZStd::vector<AZStd::pair<AZStd::string, Policy::ExecutionFailure>> states =
        {
            {CommonOptions::Keys[CommonOptions::Abort], Policy::ExecutionFailure::Abort},
            {CommonOptions::Keys[CommonOptions::Continue], Policy::ExecutionFailure::Continue},
            {CommonOptions::Keys[CommonOptions::Ignore], Policy::ExecutionFailure::Ignore}
        };
        return ParseMultiStateOption(CommonOptions::Keys[CommonOptions::ExecutionFailurePolicyKey], states, cmd).value_or(Policy::ExecutionFailure::Continue);
    }

    Policy::FailedTestCoverage ParseFailedTestCoveragePolicy(const AZ::CommandLine& cmd)
    {
        const AZStd::vector<AZStd::pair<AZStd::string, Policy::FailedTestCoverage>> states =
        {
            {CommonOptions::Keys[CommonOptions::Discard], Policy::FailedTestCoverage::Discard},
            {CommonOptions::Keys[CommonOptions::Keep], Policy::FailedTestCoverage::Keep}
        };

        return ParseMultiStateOption(CommonOptions::Keys[CommonOptions::FailedTestCoveragePolicyKey], states, cmd).value_or(Policy::FailedTestCoverage::Keep);
    }

    Policy::TestFailure ParseTestFailurePolicy(const AZ::CommandLine& cmd)
    {
        const BinaryStateValue<Policy::TestFailure> states =
        {
            Policy::TestFailure::Abort,
            Policy::TestFailure::Continue
        };

        return ParseAbortContinueOption(CommonOptions::Keys[CommonOptions::TestFailurePolicyKey], states, cmd).value_or(Policy::TestFailure::Abort);
    }

    Policy::IntegrityFailure ParseIntegrityFailurePolicy(const AZ::CommandLine& cmd)
    {
        const BinaryStateValue<Policy::IntegrityFailure> states =
        {
            Policy::IntegrityFailure::Abort,
            Policy::IntegrityFailure::Continue
        };

        return ParseAbortContinueOption(CommonOptions::Keys[CommonOptions::IntegrityFailurePolicyKey], states, cmd).value_or(Policy::IntegrityFailure::Abort);
    }

    Policy::TestRunner ParseTestRunnerPolicy(const AZ::CommandLine& cmd)
    {
        const BinaryStateValue<Policy::TestRunner> states = { Policy::TestRunner::UseLiveTestRunner, Policy::TestRunner::UseNullTestRunner };

        return ParseLiveNullOption(CommonOptions::Keys[CommonOptions::TestRunnerPolicy], states, cmd).value_or(Policy::TestRunner::UseLiveTestRunner);
    }

    Policy::TargetOutputCapture ParseTargetOutputCapture(const AZ::CommandLine& cmd)
    {
        if (const auto numSwitchValues = cmd.GetNumSwitchValues(CommonOptions::Keys[CommonOptions::TargetOutputCaptureKey]);
            numSwitchValues)
        {
            AZ_TestImpact_Eval(
                numSwitchValues <= 2, CommandLineOptionsException, "Unexpected parameters for target output capture option");

            Policy::TargetOutputCapture targetOutputCapture = Policy::TargetOutputCapture::None;
            for (auto i = 0; i < numSwitchValues; i++)
            {
                const auto option = cmd.GetSwitchValue(CommonOptions::Keys[CommonOptions::TargetOutputCaptureKey], i);
                if (option == CommonOptions::Keys[CommonOptions::StdOut])
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
                else if (option == CommonOptions::Keys[CommonOptions::File])
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

    AZStd::optional<AZStd::chrono::milliseconds> ParseTestTargetTimeout(const AZ::CommandLine& cmd)
    {
        return ParseSecondsOption(CommonOptions::Keys[CommonOptions::TestTargetTimeoutKey], cmd);
    }

    AZStd::optional<AZStd::chrono::milliseconds> ParseGlobalTimeout(const AZ::CommandLine& cmd)
    {
        return ParseSecondsOption(CommonOptions::Keys[CommonOptions::GlobalTimeoutKey], cmd);
    }

    bool ParseDraftFailingTests(const AZ::CommandLine& cmd)
    {
        const BinaryStateValue<bool> states = { false, true };
        return ParseOnOffOption(CommonOptions::Keys[CommonOptions::DraftFailingTestsKey], states, cmd).value_or(false);
    }

    SuiteSet ParseSuiteSet(const AZ::CommandLine& cmd)
    {
        return ParseMultiValueOption(CommonOptions::Keys[CommonOptions::SuiteSetKey], cmd);
    }

    SuiteLabelExcludeSet ParseSuiteLabelExcludeSet(const AZ::CommandLine& cmd)
    {
        return ParseMultiValueOption(CommonOptions::Keys[CommonOptions::SuiteLabelExcludeKey], cmd);
    }

    AZStd::vector<ExcludedTarget> ParseExcludedTestsFile(const AZ::CommandLine& cmd)
    {
        AZStd::optional<RepoPath> excludeFilePath = ParsePathOption(CommonOptions::Keys[CommonOptions::ExcludedTestsKey], cmd);
        if (excludeFilePath.has_value())
        {
            return ParseExcludedTestTargetsFromFile(ReadFileContents<CommandLineOptionsException>(excludeFilePath.value()));
        }

        return {};
    }

    bool ParseSafeMode(const AZ::CommandLine& cmd)
    {
        const BinaryStateValue<bool> states = { false, true };
        return ParseOnOffOption(CommonOptions::Keys[CommonOptions::SafeModeKey], states, cmd).value_or(false);
    }

    CommandLineOptions::CommandLineOptions(int argc, char** argv)
    {
        static_assert(CommonOptions::Fields::_CHECKSUM_ == AZStd::size(CommonOptions::Keys));
        AZ::CommandLine cmd;
        cmd.Parse(argc, argv);

        m_configurationFile = ParseConfigurationFile(cmd);
        m_dataFile = ParseDataFile(cmd);
        m_previousRunDataFile = ParsePreviousRunDataFile(cmd);
        m_changeListFile = ParseChangeListFile(cmd);
        m_sequenceReportFile = ParseSequenceReportFile(cmd);
        m_testSequenceType = ParseTestSequenceType(cmd);
        m_testPrioritizationPolicy = ParseTestPrioritizationPolicy(cmd);
        m_executionFailurePolicy = ParseExecutionFailurePolicy(cmd);
        m_failedTestCoveragePolicy = ParseFailedTestCoveragePolicy(cmd);
        m_testFailurePolicy = ParseTestFailurePolicy(cmd);
        m_integrityFailurePolicy = ParseIntegrityFailurePolicy(cmd);
        m_targetOutputCapture = ParseTargetOutputCapture(cmd);
        m_globalTimeout = ParseGlobalTimeout(cmd);
        m_draftFailingTests = ParseDraftFailingTests(cmd);
        m_suiteSet = ParseSuiteSet(cmd);
        m_suiteLabelExcludes = ParseSuiteLabelExcludeSet(cmd);
        m_excludedTests = ParseExcludedTestsFile(cmd);
        m_safeMode = ParseSafeMode(cmd);
        m_testTargetTimeout = ParseTestTargetTimeout(cmd);
        m_testRunnerPolicy = ParseTestRunnerPolicy(cmd);
    }

    bool CommandLineOptions::HasSafeMode() const
    {
        return m_safeMode;
    }

    bool CommandLineOptions::HasDataFilePath() const
    {
        return m_dataFile.has_value();
    }

    bool CommandLineOptions::HasPreviousRunDataFilePath() const
    {
        return m_previousRunDataFile.has_value();
    }
 
    bool CommandLineOptions::HasChangeListFilePath() const
    {
        return m_changeListFile.has_value();
    }

    bool CommandLineOptions::HasSequenceReportFilePath() const
    {
        return m_sequenceReportFile.has_value();
    }

    bool CommandLineOptions::HasDraftFailingTests() const
    {
        return m_draftFailingTests;
    }

    const AZStd::optional<RepoPath>& CommandLineOptions::GetDataFilePath() const
    {
        return m_dataFile;
    }

    const AZStd::optional<RepoPath>& CommandLineOptions::GetPreviousRunDataFilePath() const
    {
        return m_previousRunDataFile;
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

    Policy::TargetOutputCapture CommandLineOptions::GetTargetOutputCapture() const
    {
        return m_targetOutputCapture;
    }

    Policy::TestRunner CommandLineOptions::GetTestRunnerPolicy() const
    {
        return m_testRunnerPolicy;
    }

    const AZStd::optional<AZStd::chrono::milliseconds>& CommandLineOptions::GetTestTargetTimeout() const
    {
        return m_testTargetTimeout;
    }

    const AZStd::optional<AZStd::chrono::milliseconds>& CommandLineOptions::GetGlobalTimeout() const
    {
        return m_globalTimeout;
    }

    const SuiteSet& CommandLineOptions::GetSuiteSet() const
    {
        return m_suiteSet;
    }

    const SuiteLabelExcludeSet& CommandLineOptions::GetSuiteLabelExcludeSet() const
    {
        return m_suiteLabelExcludes;
    }

    bool CommandLineOptions::HasExcludedTests() const
    {
        return !m_excludedTests.empty();
    }

    const AZStd::vector<ExcludedTarget>& CommandLineOptions::GetExcludedTests() const
    {
        return m_excludedTests;
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
            "    -previousrundatafile=<filename>                             Optional path to a test impact data file that will used instead of that\n"
            "                                                                specified in the config file.\n"
            "    -excluded=<filename>                                        Optional path to a test target exclusion file that will be used instead of\n"
            "                                                                that specified in the config file\n"
            "    -changelist=<filename>                                      Path to the JSON of source file changes to perform test impact \n"
            "                                                                analysis on.\n"
            "    -report=<filename>                                          Path to where the sequence report file will be written (if this option \n"
            "                                                                is not specified, no report will be written).\n"
            "    -ttimeout=<seconds>                                         Timeout value to terminate individual test targets should it be \n"
            "                                                                tests is run without instrumentation after the set of selected \n"
            "                                                                instrumented tests is run (this has the effect of ensuring all \n"
            "                                                                tests are run regardless).\n"
            "    -gtimeout=<seconds>                                         Global timeout value to terminate the entire test sequence should it \n"
            "                                                                be exceeded.\n"
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
            "    -cpolicy=<remove, keep>                                     Policy for handling the coverage data of failing tests, where 'discard' \n"
            "                                                                will discard the coverage data produced by the failing tests, causing \n"
            "                                                                them to be drafted into future test runs and 'keep' will keep any existing \n"
            "                                                                coverage data and update the coverage data for failed tests that produce \n"
            "                                                                coverage.\n"
            "    -targetout=<stdout, file>                                   Capture of individual test run stdout, where 'stdout' will capture \n"
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
            "    -safemode=<on,off>                                          Flag to specify a safe mode sequence where the set of unselected \n"
            "    -testrunner=<live,null>                                     Whether to use the null test runner (on) or run the tests (off). \n"
            "                                                                If not set, defaults to running the tests.                          \n"
            "    -suite=<...>                                                The test suites to select from for this test sequence.\n"
            "    -labelexcludes=<...>                                        The list of labels that will exclude any tests with any of these labels\n"
            "                                                                in their suite.";

        return help;
    }
} // namespace TestImpact
