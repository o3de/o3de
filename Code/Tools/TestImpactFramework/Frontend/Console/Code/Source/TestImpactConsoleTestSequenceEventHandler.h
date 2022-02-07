/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
        //! Handler for TestSequenceStartCallback event.
        void TestSequenceStartCallback(SuiteType suiteType, const Client::TestRunSelection& selectedTests);

        //! Handler for TestSequenceStartCallback event.
        void ImpactAnalysisTestSequenceStartCallback(
            SuiteType suiteType,
            const Client::TestRunSelection& selectedTests,
            const AZStd::vector<AZStd::string>& discardedTests,
            const AZStd::vector<AZStd::string>& draftedTests);

        //! Handler for SafeImpactAnalysisTestSequenceStartCallback event.
        void SafeImpactAnalysisTestSequenceStartCallback(
            SuiteType suiteType,
            const Client::TestRunSelection& selectedTests,
            const Client::TestRunSelection& discardedTests,
            const AZStd::vector<AZStd::string>& draftedTests);

        //! Handler for RegularTestSequenceCompleteCallback event.
        void RegularTestSequenceCompleteCallback(const Client::RegularSequenceReport& sequenceReport);

        //! Handler for SeedTestSequenceCompleteCallback event.
        void SeedTestSequenceCompleteCallback(const Client::SeedSequenceReport& sequenceReport);

        //! Handler for ImpactAnalysisTestSequenceCompleteCallback event.
        void ImpactAnalysisTestSequenceCompleteCallback(const Client::ImpactAnalysisSequenceReport& sequenceReport);

        //! Handler for SafeImpactAnalysisTestSequenceCompleteCallback event.
        void SafeImpactAnalysisTestSequenceCompleteCallback(const Client::SafeImpactAnalysisSequenceReport& sequenceReport);

        //! Handler for TestRunCompleteCallback event.
        void TestRunCompleteCallback(const Client::TestRunBase& testRun, size_t numTestRunsCompleted, size_t totalNumTestRuns);
    } // namespace Console
} // namespace TestImpact
