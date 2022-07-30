/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <TestImpactFramework/TestImpactChangeList.h>
#include <TestImpactFramework/TestImpactClientTestSelection.h>
#include <TestImpactFramework/TestImpactClientTestRun.h>
#include <TestImpactFramework/TestImpactClientSequenceReport.h>
#include <TestImpactFramework/TestImpactTestSequence.h>

#include <AzCore/std/chrono/chrono.h>
#include <AzCore/std/functional.h>
#include <AzCore/std/optional.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/vector.h>

namespace TestImpact
{
    template<typename TestTarget, typename ProdutionTarget>
    class ChangeDependencyList;

    template<typename TestTarget, typename ProdutionTarget>
    class BuildTargetList;

    template<typename TestTarget, typename ProdutionTarget>
    class DynamicDependencyMap;

    template<typename TestTarget, typename ProdutionTarget>
    class TestSelectorAndPrioritizer;

    template<typename TestTarget>
    class TestTargetExclusionList;

    //! Callback for a test sequence that isn't using test impact analysis to determine selected tests.
    //! @parm suiteType The test suite to select tests from.
    //! @param tests The tests that will be run for this sequence.
    using TestSequenceStartCallback = AZStd::function<void(SuiteType suiteType, const Client::TestRunSelection& tests)>;

    //! Callback for a test sequence using test impact analysis.
    //! @parm suiteType The test suite to select tests from.
    //! @param selectedTests The tests that have been selected for this run by test impact analysis.
    //! @param discardedTests The tests that have been rejected for this run by test impact analysis. 
    //! @param draftedTests The tests that have been drafted in for this run due to requirements outside of test impact analysis
    //! (e.g. test targets that have been added to the repository since the last test impact analysis sequence or test that failed
    //! to execute previously).
    //! These tests will be run with coverage instrumentation.
    //! @note discardedTests and draftedTests may contain overlapping tests.
    using ImpactAnalysisTestSequenceStartCallback = AZStd::function<void(
        SuiteType suiteType,
        const Client::TestRunSelection& selectedTests,
        const AZStd::vector<AZStd::string>& discardedTests,
        const AZStd::vector<AZStd::string>& draftedTests)>;

    //! Callback for a test sequence using test impact analysis.
    //! @parm suiteType The test suite to select tests from.
    //! @param selectedTests The tests that have been selected for this run by test impact analysis.
    //! @param discardedTests The tests that have been rejected for this run by test impact analysis.
    //! These tests will not be run without coverage instrumentation unless there is an entry in the draftedTests list.
    //! @param draftedTests The tests that have been drafted in for this run due to requirements outside of test impact analysis
    //! (e.g. test targets that have been added to the repository since the last test impact analysis sequence or test that failed
    //! to execute previously).
    //! @note discardedTests and draftedTests may contain overlapping tests.
    using SafeImpactAnalysisTestSequenceStartCallback = AZStd::function<void(
        SuiteType suiteType,
        const Client::TestRunSelection& selectedTests,
        const Client::TestRunSelection& discardedTests,
        const AZStd::vector<AZStd::string>& draftedTests)>;

    //! Callback for end of a test sequence.
    //! @tparam SequenceReportType The report type to be used for the sequence.
    //! @param sequenceReport The completed sequence report.
    template<typename SequenceReportType>
    using TestSequenceCompleteCallback = AZStd::function<void(const SequenceReportType& sequenceReport)>;

    //! Callback for test runs that have completed for any reason.
    //! @param testRunMeta The test that has completed.
    //! @param numTestRunsCompleted The number of test runs that have completed.
    //! @param totalNumTestRuns The total number of test runs in the sequence.
    using TestRunCompleteCallback = AZStd::function<void(Client::TestRunBase& testRun, size_t numTestRunsCompleted, size_t totalNumTestRuns)>;
} // namespace TestImpact
