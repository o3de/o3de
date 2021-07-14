/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
