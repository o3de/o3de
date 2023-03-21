/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <TestImpactFramework/TestImpactClientSequenceReport.h>
#include <TestImpactFramework/TestImpactClientTestRun.h>
#include <TestImpactFramework/TestImpactClientTestSelection.h>
#include <TestImpactFramework/TestImpactTestSequence.h>

#include <AzCore/EBus/EBus.h>

namespace TestImpact
{
    //! Base Bus for test sequence notifications.
    class TestSequenceNotificationsBase
        : public AZ::EBusTraits
    {
    public:
        // EBusTraits overrides ...
        static constexpr AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        static constexpr AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;

        virtual ~TestSequenceNotificationsBase() = default;

        //! Callback for test runs that have completed for any reason.
        //! @param testRunMeta The test that has completed.
        //! @param numTestRunsCompleted The number of test runs that have completed.
        //! @param totalNumTestRuns The total number of test runs in the sequence.
        virtual void OnTestRunComplete(
            [[maybe_unused]] const Client::TestRunBase& testRun,
            [[maybe_unused]] size_t numTestRunsCompleted,
            [[maybe_unused]] size_t totalNumTestRuns)
        {
        }

        //! Callback for realtime standard output and error of test targets.
        //! @param stdOutDelta The string delta of the standard output since the last notification.
        //! @param stdErrDelta The string delta of the standard error since the last notification.
        virtual void OnRealtimeStdContent([[maybe_unused]] const AZStd::string& stdOutDelta, [[maybe_unused]] const AZStd::string& stdErrDelta)
        {
        }
    };

    //! Base bus for non-impact analysis test sequence notifications.
    class NonImpactAnalysisTestSequenceNotificationsBase
        : public TestSequenceNotificationsBase
    {
    public:
        virtual ~NonImpactAnalysisTestSequenceNotificationsBase() = default;

        //! Callback for a test sequence that isn't using test impact analysis to determine selected tests.
        //! @param suiteSet The test suites to select tests from.
        //! @param suiteLabelExcludeSet Any tests with suites that match a label from this set will be excluded.
        //! @param selectedTests The tests that will be run for this sequence.
        virtual void OnTestSequenceStart(
            [[maybe_unused]] const SuiteSet& suiteSet,
            [[maybe_unused]] const SuiteLabelExcludeSet& suiteLabelExcludeSet,
            [[maybe_unused]] const Client::TestRunSelection& selectedTests)
        {
        }
    };

    //! Bus for regular test sequence notifications.
    class RegularTestSequenceNotifications
        : public NonImpactAnalysisTestSequenceNotificationsBase
    {
    public:
        //! Callback for end of a test sequence.
        //! @param sequenceReport The completed sequence report.
        virtual void OnTestSequenceComplete([[maybe_unused]] const Client::RegularSequenceReport& sequenceReport)
        {
        }
    };

    //! Bus for seed test sequence notifications.
    class SeedTestSequenceNotifications
        : public NonImpactAnalysisTestSequenceNotificationsBase
    {
    public:
        //! Callback for end of a test sequence.
        //! @param sequenceReport The completed sequence report.
        virtual void OnTestSequenceComplete([[maybe_unused]] const Client::SeedSequenceReport& sequenceReport)
        {
        }
    };

    //! Bus for impact analysis test sequence notifications.
    class ImpactAnalysisTestSequenceNotifications
        : public TestSequenceNotificationsBase
    {
    public:
        //! Callback for a test sequence using test impact analysis.
        //! @param suiteSet The test suites suite to select tests from.
        //! @param suiteLabelExcludeSet Any tests with suites that match a label from this set will be excluded.
        //! @param selectedTests The tests that have been selected for this run by test impact analysis.
        //! @param discardedTests The tests that have been rejected for this run by test impact analysis.
        //! @param draftedTests The tests that have been drafted in for this run due to requirements outside of test impact analysis
        //! (e.g. test targets that have been added to the repository since the last test impact analysis sequence or test that failed
        //! to execute previously).
        //! These tests will be run with coverage instrumentation.
        //! @note discardedTests and draftedTests may contain overlapping tests.
        virtual void OnTestSequenceStart(
            [[maybe_unused]] const SuiteSet& suiteSet,
            [[maybe_unused]] const SuiteLabelExcludeSet& suiteLabelExcludeSet,
            [[maybe_unused]] const Client::TestRunSelection& selectedTests,
            [[maybe_unused]] const AZStd::vector<AZStd::string>& discardedTests,
            [[maybe_unused]] const AZStd::vector<AZStd::string>& draftedTests)
        {
        }

        //! Callback for end of a test sequence.
        //! @param sequenceReport The completed sequence report.
        virtual void OnTestSequenceComplete([[maybe_unused]] const Client::ImpactAnalysisSequenceReport& sequenceReport)
        {
        }
    };

    //! Bus for safe impact analysis test sequence notifications.
    class SafeImpactAnalysisTestSequenceNotifications : public TestSequenceNotificationsBase
    {
    public:
        //! Callback for a test sequence using test impact analysis.
        //! @parm suiteSet The test suites to select tests from.
        //! @param suiteLabelExcludeSet Any tests with suites that match a label from this set will be excluded.
        //! @param selectedTests The tests that have been selected for this run by test impact analysis.
        //! @param discardedTests The tests that have been rejected for this run by test impact analysis.
        //! These tests will not be run without coverage instrumentation unless there is an entry in the draftedTests list.
        //! @param draftedTests The tests that have been drafted in for this run due to requirements outside of test impact analysis
        //! (e.g. test targets that have been added to the repository since the last test impact analysis sequence or test that failed
        //! to execute previously).
        //! @note discardedTests and draftedTests may contain overlapping tests.
        virtual void OnTestSequenceStart(
            [[maybe_unused]] const SuiteSet& suiteSet,
            [[maybe_unused]] const SuiteLabelExcludeSet& suiteLabelExcludeSet,
            [[maybe_unused]] const Client::TestRunSelection& selectedTests,
            [[maybe_unused]] const Client::TestRunSelection& discardedTests,
            [[maybe_unused]] const AZStd::vector<AZStd::string>& draftedTests)
        {
        }

        //! Callback for end of a test sequence.
        //! @param sequenceReport The completed sequence report.
        virtual void OnTestSequenceComplete([[maybe_unused]] const Client::SafeImpactAnalysisSequenceReport& sequenceReport)
        {
        }
    };

    using TestSequenceNotificationsBaseBus = AZ::EBus<TestSequenceNotificationsBase>;
    using NonImpactAnalysisTestSequenceNotificationsBaseBus = AZ::EBus<NonImpactAnalysisTestSequenceNotificationsBase>;
    using RegularTestSequenceNotificationBus = AZ::EBus<RegularTestSequenceNotifications>;
    using SeedTestSequenceNotificationBus = AZ::EBus<SeedTestSequenceNotifications>;
    using ImpactAnalysisTestSequenceNotificationBus = AZ::EBus<ImpactAnalysisTestSequenceNotifications>;
    using SafeImpactAnalysisTestSequenceNotificationBus = AZ::EBus<SafeImpactAnalysisTestSequenceNotifications>;
} // namespace TestImpact
