/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <TestImpactFramework/TestImpactTestSequenceBus.h>

#include <AzCore/std/chrono/chrono.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/string/string.h>

namespace TestImpact
{
    namespace Console
    {
        //! Console output dumping for test target standard output and error.
        enum class ConsoleOutputMode : AZ::u8
        {
            Buffered, //!< Output will be dumped to console once the test target execution has completed.
            Realtime //!< Output will be dumped to console in realtime.
        };

        //! Base class for all test sequence notification handlers.
        class TestSequenceNotificationHandlerBase
            : private TestSequenceNotificationsBaseBus::Handler
        {
        public:
            TestSequenceNotificationHandlerBase(ConsoleOutputMode consoleOutputMode);
            virtual ~TestSequenceNotificationHandlerBase();

        private:
            // TestSequenceNotificationsBaseBus overrides ...
            void OnTestRunComplete(const Client::TestRunBase& testRun, size_t numTestRunsCompleted, size_t totalNumTestRuns) override;
            void OnRealtimeStdContent(const AZStd::string& stdOutDelta, const AZStd::string& stdErrDelta) override;

            ConsoleOutputMode m_consoleOutputMode;
        };

        //! Base class for non-impact analysis test sequence notificaion handlers.
        class NonImpactAnalysisTestSequenceNotificationHandlerBase
            : public TestSequenceNotificationHandlerBase
            , private NonImpactAnalysisTestSequenceNotificationsBaseBus::Handler
        {
        public:
            NonImpactAnalysisTestSequenceNotificationHandlerBase(ConsoleOutputMode consoleOutputMode);
            ~NonImpactAnalysisTestSequenceNotificationHandlerBase();

        private:
            // NonImpactAnalysisTestSequenceNotificationsBaseBus overrides ...
            void OnTestSequenceStart(
                const SuiteSet& suiteSet,
                const SuiteLabelExcludeSet& suiteLabelExcludeSet,
                const Client::TestRunSelection& selectedTests) override;
        };

        //! Handler for regular test sequence notifications.
        class RegularTestSequenceNotificationHandler
            : public NonImpactAnalysisTestSequenceNotificationHandlerBase
            , private RegularTestSequenceNotificationBus::Handler
        {
        public:
            RegularTestSequenceNotificationHandler(ConsoleOutputMode consoleOutputMode);
            ~RegularTestSequenceNotificationHandler();

        private:
            // RegularTestSequenceNotificationBus overrides ...
            void OnTestSequenceComplete(const Client::RegularSequenceReport& sequenceReport) override;
        };

        //! Handler for seed test sequence notifications.
        class SeedTestSequenceNotificationHandler
            : public NonImpactAnalysisTestSequenceNotificationHandlerBase
            , private SeedTestSequenceNotificationBus::Handler
        {
        public:
            SeedTestSequenceNotificationHandler(ConsoleOutputMode consoleOutputMode);
            ~SeedTestSequenceNotificationHandler();

        private:
            // SeedTestSequenceNotificationBus overrides ...
            void OnTestSequenceComplete(const Client::SeedSequenceReport& sequenceReport) override;
        };

        //! Handler for impact analysis test sequence notifications.
        class ImpactAnalysisTestSequenceNotificationHandler
            : public TestSequenceNotificationHandlerBase
            , private ImpactAnalysisTestSequenceNotificationBus::Handler
        {
        public:
            ImpactAnalysisTestSequenceNotificationHandler(ConsoleOutputMode consoleOutputMode);
            ~ImpactAnalysisTestSequenceNotificationHandler();

        private:
            // ImpactAnalysisTestSequenceNotificationBus overrides ...
            void OnTestSequenceStart(
                const SuiteSet& suiteSet,
                const SuiteLabelExcludeSet& suiteLabelExcludeSet,
                const Client::TestRunSelection& selectedTests,
                const AZStd::vector<AZStd::string>& discardedTests,
                const AZStd::vector<AZStd::string>& draftedTests) override;
            void OnTestSequenceComplete(const Client::ImpactAnalysisSequenceReport& sequenceReport) override;
        };

        //! Handler for safe impact analysis test sequence notifications.
        class SafeImpactAnalysisTestSequenceNotificationHandler
            : public TestSequenceNotificationHandlerBase
            , private SafeImpactAnalysisTestSequenceNotificationBus::Handler
        {
        public:
            SafeImpactAnalysisTestSequenceNotificationHandler(ConsoleOutputMode consoleOutputMode);
            ~SafeImpactAnalysisTestSequenceNotificationHandler();

        private:
            // SafeImpactAnalysisTestSequenceNotificationBus overrides ...
            void OnTestSequenceStart(
                const SuiteSet& suiteSet,
                const SuiteLabelExcludeSet& suiteLabelExcludeSet,
                const Client::TestRunSelection& selectedTests,
                const Client::TestRunSelection& discardedTests,
                const AZStd::vector<AZStd::string>& draftedTests) override;
            void OnTestSequenceComplete(const Client::SafeImpactAnalysisSequenceReport& sequenceReport) override;
        };
    } // namespace Console
} // namespace TestImpact
