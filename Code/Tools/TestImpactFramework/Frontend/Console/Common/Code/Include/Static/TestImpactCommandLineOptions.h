/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <TestImpactFramework/TestImpactTestSequence.h>
#include <TestImpactFramework/TestImpactRepoPath.h>
#include <TestImpactFramework/TestImpactConfiguration.h>

#include <AzCore/std/chrono/chrono.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/optional.h>
#include <AzCore/std/containers/vector.h>

namespace TestImpact
{
    //! The type of test sequence to run.
    enum class TestSequenceType
    {
        None, //!< Runs no tests and will report all tests successful.
        Seed, //!< Removes any prior coverage data and runs all test targets with instrumentation to reseed the data from scratch.
        Regular, //!< Runs all of the test targets without any instrumentation to generate coverage data (any prior coverage data is left intact).
        ImpactAnalysis, //!< Uses any prior coverage data to run the instrumented subset of selected tests (if no prior coverage data a regular run is performed instead).
        ImpactAnalysisNoWrite, //!< Uses any prior coverage data to run the uninstrumented subset of selected tests (if no prior coverage data a regular run is performed instead).
                               //!< The coverage data is not updated with the subset of selected tests.
        ImpactAnalysisOrSeed //!< Uses any prior coverage data to run the instrumented subset of selected tests (if no prior coverage data a seed run is performed instead).
    };

    //! Representation of the command line options supplied to the console frontend application.
    class CommandLineOptions
    {
    public:
        CommandLineOptions(int argc, char** argv);

        //! Compiles the command line usage to a string.
        static AZStd::string GetCommandLineUsageString();

        //! Returns true if a test impact data file path has been supplied, otherwise false.
        bool HasDataFilePath() const;

        //! Returns true if a previous run data file path has been supplied, otherwise false.
        bool HasPreviousRunDataFilePath() const;

        //! Returns true if a change list file path has been supplied, otherwise false.
        bool HasChangeListFilePath() const;

        //! Returns true if a sequence report file path has been supplied, otherwise false.
        bool HasSequenceReportFilePath() const;

        //! Returns true if the draft failing tests option has been enabled, otherwise false.
        bool HasDraftFailingTests() const;

        //! Returns the path to the runtime configuration file.
        const RepoPath& GetConfigurationFilePath() const;

        //! Returns the path to the test impact data file (if any).
        const AZStd::optional<RepoPath>& GetDataFilePath() const;

        //! Returns the path to the previous run data file (if any).
        const AZStd::optional<RepoPath>& GetPreviousRunDataFilePath() const;

        //! Returns the path to the change list file (if any).
        const AZStd::optional<RepoPath>& GetChangeListFilePath() const;

        //! Returns the path to the sequence report file (if any).
        const AZStd::optional<RepoPath>& GetSequenceReportFilePath() const;

        //! Returns the test sequence type to run.
        TestSequenceType GetTestSequenceType() const;

        //! Returns the test prioritization policy to use.
        Policy::TestPrioritization GetTestPrioritizationPolicy() const;

        //! Returns the test execution failure policy to use.
        Policy::ExecutionFailure GetExecutionFailurePolicy() const;

        //! Returns failed test coverage drafting policy to use.
        Policy::FailedTestCoverage GetFailedTestCoveragePolicy() const;

        //! Returns the test failure policy to use.
        Policy::TestFailure GetTestFailurePolicy() const;

        //! Returns the integration failure policy to use.
        Policy::IntegrityFailure GetIntegrityFailurePolicy() const;

        //! Returns the test target standard output capture policy to use.
        Policy::TargetOutputCapture GetTargetOutputCapture() const;

        //! Returns the test runner policy to use
        Policy::TestRunner GetTestRunnerPolicy() const;

        //! Returns the individual test target timeout to use (if any).
        const AZStd::optional<AZStd::chrono::milliseconds>& GetTestTargetTimeout() const;

        //! Returns the global test sequence timeout to use (if any).
        const AZStd::optional<AZStd::chrono::milliseconds>& GetGlobalTimeout() const;

        //! Returns the filter for test suites that will be allowed to be run.
        const SuiteSet& GetSuiteSet() const;

        //! Returns the test suite label exclude set that will be used to exclude any test targets with any matching suite labels.
        const SuiteLabelExcludeSet& GetSuiteLabelExcludeSet() const;

        //! Returns the tests to exclude from this run of TIAF (if any).
        const AZStd::vector<ExcludedTarget>& GetExcludedTests() const;

        //! Returns true if we have tests to exclude that have been loaded from the exclude file, otherwise false.
        bool HasExcludedTests() const;

        //! Returns true if the safe mode option has been enabled, otherwise false.
        bool HasSafeMode() const;

    private:
        RepoPath m_configurationFile;
        AZStd::optional<RepoPath> m_dataFile;
        AZStd::optional<RepoPath> m_previousRunDataFile;
        AZStd::optional<RepoPath> m_changeListFile;
        AZStd::optional<RepoPath> m_sequenceReportFile;
        TestSequenceType m_testSequenceType;
        Policy::TestPrioritization m_testPrioritizationPolicy = Policy::TestPrioritization::None;
        Policy::ExecutionFailure m_executionFailurePolicy = Policy::ExecutionFailure::Continue;
        Policy::FailedTestCoverage m_failedTestCoveragePolicy = Policy::FailedTestCoverage::Keep;
        Policy::TestFailure m_testFailurePolicy = Policy::TestFailure::Abort;
        Policy::IntegrityFailure m_integrityFailurePolicy = Policy::IntegrityFailure::Abort;
        Policy::TargetOutputCapture m_targetOutputCapture = Policy::TargetOutputCapture::None;
        Policy::TestRunner m_testRunnerPolicy = Policy::TestRunner::UseLiveTestRunner;
        AZStd::optional<AZStd::chrono::milliseconds> m_testTargetTimeout;
        AZStd::optional<AZStd::chrono::milliseconds> m_globalTimeout;
        SuiteSet m_suiteSet;
        SuiteLabelExcludeSet m_suiteLabelExcludes;
        bool m_draftFailingTests = false;
        AZStd::vector<ExcludedTarget> m_excludedTests;
        bool m_safeMode = false;
    };
} // namespace TestImpact
