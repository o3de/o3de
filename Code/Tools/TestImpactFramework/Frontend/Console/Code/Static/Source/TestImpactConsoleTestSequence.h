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
        class TestSequence
        {
        public:
            TestSequence(const AZStd::unordered_set<AZStd::string>* suiteFilter);

            // TestSequenceStartCallback
            void operator()(Client::TestRunSelection&& selectedTests);

            // ImpactAnalysisTestSequenceStartCallback
            void operator()(
                Client::TestRunSelection&& selectedTests,
                AZStd::vector<AZStd::string>&& discardedTests,
                AZStd::vector<AZStd::string>&& draftedTests);

            // SafeImpactAnalysisTestSequenceStartCallback
            void operator()(
                Client::TestRunSelection&& selectedTests,
                Client::TestRunSelection&& discardedTests,
                AZStd::vector<AZStd::string>&& draftedTests);

            // TestSequenceCompleteCallback
            void operator()(
                Client::SequenceFailure&& failureReport,
                AZStd::chrono::milliseconds duration);

            // SafeTestSequenceCompleteCallback
            void operator()(
                Client::SequenceFailure&& selectedFailureReport,
                Client::SequenceFailure&& discardedFailureReport,
                AZStd::chrono::milliseconds duration);

            // TestRunCompleteCallback
            void operator()(Client::TestRun&& test);

        private:
            void ClearState();

            const AZStd::unordered_set<AZStd::string>* m_suiteFilter = nullptr;
            size_t m_numTests = 0;
            size_t m_numTestsComplete = 0;
        };
    }
}
