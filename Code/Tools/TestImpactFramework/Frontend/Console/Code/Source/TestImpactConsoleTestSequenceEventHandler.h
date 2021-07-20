/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TestImpactFramework/TestImpactTestSequence.h>
#include <TestImpactFramework/TestImpactClientTestSelection.h>
#include <TestImpactFramework/TestImpactClientFailureReport.h>
#include <TestImpactFramework/TestImpactClientTestRun.h>

#include <AzCore/std/chrono/chrono.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/unordered_set.h>

#pragma once

namespace TestImpact
{
    namespace Console
    {
        //! Event handler for all test sequence types.
        class TestSequenceEventHandler
        {
        public:
            explicit TestSequenceEventHandler(SuiteType suiteFilter);

            //! TestSequenceStartCallback.
            void operator()(Client::TestRunSelection&& selectedTests);

            //! ImpactAnalysisTestSequenceStartCallback.
            void operator()(
                Client::TestRunSelection&& selectedTests,
                AZStd::vector<AZStd::string>&& discardedTests,
                AZStd::vector<AZStd::string>&& draftedTests);

            //! SafeImpactAnalysisTestSequenceStartCallback.
            void operator()(
                Client::TestRunSelection&& selectedTests,
                Client::TestRunSelection&& discardedTests,
                AZStd::vector<AZStd::string>&& draftedTests);

            //! TestSequenceCompleteCallback.
            void operator()(
                Client::SequenceFailure&& failureReport,
                AZStd::chrono::milliseconds duration);

            //! SafeTestSequenceCompleteCallback.
            void operator()(
                Client::SequenceFailure&& selectedFailureReport,
                Client::SequenceFailure&& discardedFailureReport,
                AZStd::chrono::milliseconds selectedDuration,
                AZStd::chrono::milliseconds discaredDuration);

            //! TestRunCompleteCallback.
            void operator()(Client::TestRun&& test);

        private:
            void ClearState();

            SuiteType m_suiteFilter;
            size_t m_numTests = 0;
            size_t m_numTestsComplete = 0;
        };
    } // namespace Console
} // namespace TestImpact
