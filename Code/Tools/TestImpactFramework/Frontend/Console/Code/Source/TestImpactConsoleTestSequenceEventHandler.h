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

#include <TestImpactFramework/TestImpactTestSequence.h>
#include <TestImpactFramework/TestImpactClientTestSelection.h>
#include <TestImpactFramework/TestImpactClientSequenceReport.h>
#include <TestImpactFramework/TestImpactClientTestRun.h>

#include <AzCore/std/chrono/chrono.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/unordered_set.h>

#pragma once

namespace TestImpact
{
    namespace Console
    {
        void TestSequenceStartCallback(SuiteType suiteType, const Client::TestRunSelection& selectedTests);

        void ImpactAnalysisTestSequenceStartCallback(
            SuiteType suiteType,
            const Client::TestRunSelection& selectedTests,
            const AZStd::vector<AZStd::string>& discardedTests,
            const AZStd::vector<AZStd::string>& draftedTests);

        void SafeImpactAnalysisTestSequenceStartCallback(
            SuiteType suiteType,
            const Client::TestRunSelection& selectedTests,
            const Client::TestRunSelection& discardedTests,
            const AZStd::vector<AZStd::string>& draftedTests);

        void TestSequenceCompleteCallback(const Client::SequenceReport& sequenceReport);

        void ImpactAnalysisTestSequenceCompleteCallback(const Client::ImpactAnalysisSequenceReport& sequenceReport);

        void SafeImpactAnalysisTestSequenceCompleteCallback(const Client::SafeImpactAnalysisSequenceReport& sequenceReport);

        void TestRunCompleteCallback(const Client::TestRun& testRun, size_t numTestRunsCompleted, size_t totalNumTestRuns);
    } // namespace Console
} // namespace TestImpact
