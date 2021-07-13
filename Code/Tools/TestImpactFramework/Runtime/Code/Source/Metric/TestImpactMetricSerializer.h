///*
// * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of
// * this distribution.
// *
// * SPDX-License-Identifier: Apache-2.0 OR MIT
// *
// */
//
//#pragma once
//
//#include <TestImpactFramework/TestImpactChangeList.h>
//#include <TestImpactFramework/TestImpactTestSequence.h>
//#include <TestImpactFramework/TestImpactClientFailureReport.h>
//#include <TestImpactFramework/TestImpactClientTestSelection.h>
//
//#include <AzCore/std/chrono/chrono.h>
//
//namespace TestImpact
//{
//    class TestRunMetrics
//    {
//    public:
//        TestRunMetrics() = default;
//        TestRunMetrics(
//            const Client::TestRunSelection& tests,
//            AZStd::chrono::milliseconds t0,
//            AZStd::chrono::milliseconds startTime,
//            AZStd::chrono::milliseconds duration,
//            const Client::SequenceFailureReport& sequenceFailureReport);
//
//        const Client::TestRunSelection& GetTests() const;
//        AZStd::chrono::milliseconds GetStartTimeSinceT0() const;
//        AZStd::chrono::milliseconds GetEndTimeSinceT0() const;
//        AZStd::chrono::milliseconds GetDuration() const;
//        const Client::SequenceFailureReport& GetSequenceFailureReport() const;
//
//    private:
//        Client::TestRunSelection m_tests;
//        AZStd::chrono::milliseconds m_startTimeSinceT0;
//        AZStd::chrono::milliseconds m_duration;
//        Client::SequenceFailureReport m_sequenceFailureReport;
//    };
//
//    AZStd::string SerializeRegularSequence(
//        TestSequenceResult result,
//        AZStd::optional<AZStd::chrono::milliseconds> testTargetTimeout,
//        AZStd::optional<AZStd::chrono::milliseconds> globalTimeout,
//        size_t maxConcurrency,
//        Policy::ExecutionFailure executionFailurePolicy,
//        Policy::FailedTestCoverage failedTestCoveragePolicy,
//        Policy::TestFailure testFailurePolicy,
//        Policy::IntegrityFailure integrationFailurePolicy,
//        Policy::TestSharding testShardingPolicy,
//        Policy::TargetOutputCapture targetOutputCapture,
//        const TestRunMetrics& selectedTestMetrics);
//
//    void SerializeImpactAnalysisSequence();
//
//    void SerializeSafeImpactAnalysisSequence();
//
//    void SerializeSeedSequence();
//}
