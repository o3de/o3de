///*
// * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of
// * this distribution.
// *
// * SPDX-License-Identifier: Apache-2.0 OR MIT
// *
// */
//
//#include <TestImpactChangeListSerializerInternal.h>
//#include <Metric/TestImpactMetricSerializer.h>
//#include <Target/TestImpactTestTarget.h>
//
//#include <AzCore/JSON/document.h>
//#include <AzCore/JSON/prettywriter.h>
//#include <AzCore/JSON/rapidjson.h>
//#include <AzCore/JSON/stringbuffer.h>
//#include <AzCore/std/time.h>
//
//namespace TestImpact
//{
//    namespace
//    {
//        enum class SequenceType
//        {
//            Regular,
//            ImpactAnalysis,
//            SafeImpactAnalysis,
//            Seed
//        };
//
//        static AZStd::string SerializeSequence(
//            SequenceType sequenceType,
//            AZStd::chrono::milliseconds sequenceDuration,
//            TestSequenceResult result,
//            const ChangeList& changeList,
//            AZStd::optional<AZStd::chrono::milliseconds> testTargetTimeout,
//            AZStd::optional<AZStd::chrono::milliseconds> globalTimeout,
//            size_t maxConcurrency,
//            Policy::ExecutionFailure executionFailurePolicy,
//            Policy::FailedTestCoverage failedTestCoveragePolicy,
//            Policy::TestFailure testFailurePolicy,
//            Policy::IntegrityFailure integrationFailurePolicy,
//            Policy::TestSharding testShardingPolicy,
//            Policy::TargetOutputCapture targetOutputCapturePolicy,
//            AZStd::optional<Policy::TestPrioritization> testPrioritizationPolicy,
//            AZStd::optional<Policy::DynamicDependencyMap> dynamicDependencyMapPolicy,
//            const TestRunMetrics& selectedTestTargets,
//            const TestRunMetrics& discardedTestTargets)
//        {
//            rapidjson::StringBuffer stringBuffer;
//            rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(stringBuffer);
//
//            // Sequence
//            writer.StartObject();
//
//                // Sequence type
//                writer.Key("type");
//                switch (sequenceType)
//                {
//                case SequenceType::Regular:
//                    writer.String("regular");
//                    break;
//
//                case SequenceType::ImpactAnalysis:
//                    writer.String("impact_analysis");
//                    break;
//
//                case SequenceType::SafeImpactAnalysis:
//                    writer.String("safe_impact_analysis");
//                    break;
//
//                case SequenceType::Seed:
//                    writer.String("seed");
//                    break;
//                }
//
//                // For sequences, we use a time line where T0 is the sequence start time and all subsequent start and end times
//                // are relative to this point
//
//                // Start time
//                writer.Key("start_time");
//                writer.Uint64(0);
//
//                // End time
//                // Relative to T0, hence same as sequence duration
//                writer.Key("end_time");
//                writer.Uint64(sequenceDuration.count());
//
//                // Duration
//                writer.Key("duration");
//                writer.Uint64(sequenceDuration.count());
//
//                // Result
//                writer.Key("result");
//                switch (result)
//                {
//                case TestSequenceResult::Failure:
//                    writer.String("failure");
//                    break;
//
//                case TestSequenceResult::Success:
//                    writer.String("success");
//                    break;
//
//                case TestSequenceResult::Timeout:
//                    writer.String("timeout");
//                    break;
//                }
//
//                // Change list
//                writer.Key("change_list");
//                writer.StartObject();
//                SerializeChangeList(changeList, writer);
//                writer.EndObject(); // Change list
//
//                // Config
//                writer.Key("config");
//                writer.StartObject();
//
//                    // Test target timeout
//                    writer.Key("test_target_timeout");
//                    testTargetTimeout.has_value() ? writer.Uint64(testTargetTimeout->count()) : writer.Null();
//
//                    // Global timeout
//                    writer.Key("global_timeout");
//                    globalTimeout.has_value() ? writer.Uint64(globalTimeout->count()) : writer.Null();
//
//                    // Max concurrency
//                    writer.Key("max_concurrency");
//                    writer.Uint64(maxConcurrency);
//
//                    // Policy
//                    writer.Key("policy");
//                    writer.StartObject();
//
//                        // Execution failure
//                        writer.Key("execution_failure");
//                        switch(executionFailurePolicy)
//                        {
//                        case Policy::ExecutionFailure::Abort:
//                            writer.String("abort");
//                            break;
//
//                        case Policy::ExecutionFailure::Continue:
//                            writer.String("continue");
//                            break;
//
//                        case Policy::ExecutionFailure::Ignore:
//                            writer.String("ignore");
//                            break;
//                        }
//
//                        // Coverage failure
//                        writer.Key("coverage_failure");
//                        switch (failedTestCoveragePolicy)
//                        {
//                        case Policy::FailedTestCoverage::Keep:
//                            writer.String("keep");
//                            break;
//
//                        case Policy::FailedTestCoverage::Discard:
//                            writer.String("discard");
//                            break;
//                        }
//
//                        // Test failure
//                        writer.Key("test_failure");
//                        switch (testFailurePolicy)
//                        {
//                        case Policy::TestFailure::Abort:
//                            writer.String("abort");
//                            break;
//
//                        case Policy::TestFailure::Continue:
//                            writer.String("continue");
//                            break;
//                        }
//
//                        // Integrity failure
//                        writer.Key("integrity_failure");
//                        switch (integrationFailurePolicy)
//                        {
//                        case Policy::IntegrityFailure::Abort:
//                            writer.String("abort");
//                            break;
//
//                        case Policy::IntegrityFailure::Continue:
//                            writer.String("continue");
//                            break;
//                        }
//
//                        // Sharding
//                        writer.Key("test_sharding");
//                        switch (testShardingPolicy)
//                        {
//                        case Policy::TestSharding::Always:
//                            writer.String("always");
//                            break;
//
//                        case Policy::TestSharding::Never:
//                            writer.String("never");
//                            break;
//                        }
//
//                        // Target output capture
//                        writer.Key("target_output_capture");
//                        switch (targetOutputCapturePolicy)
//                        {
//                        case Policy::TargetOutputCapture::File:
//                            writer.String("file");
//                            break;
//
//                        case Policy::TargetOutputCapture::None:
//                            writer.String("none");
//                            break;
//
//                        case Policy::TargetOutputCapture::StdOut:
//                            writer.String("stdout");
//                            break;
//
//                        case Policy::TargetOutputCapture::StdOutAndFile:
//                            writer.String("stdout_file");
//                            break;
//                        }
//
//                        // Test prioritization
//                        writer.Key("test_prioritization");
//                        if (testPrioritizationPolicy.has_value())
//                        {
//                            switch (testPrioritizationPolicy.value())
//                            {
//                            case Policy::TestPrioritization::DependencyLocality:
//                                writer.String("dependency_locality");
//                                break;
//
//                            case Policy::TestPrioritization::None:
//                                writer.String("none");
//                                break;
//                            }
//                        }
//                        else
//                        {
//                            writer.Null();
//                        }
//
//                        // Dynamic dependency map
//                        writer.Key("dynamic_dependency_map");
//                        if (dynamicDependencyMapPolicy.has_value())
//                        {
//                            switch (dynamicDependencyMapPolicy.value())
//                            {
//                            case Policy::DynamicDependencyMap::Discard:
//                                writer.String("discard");
//                                break;
//
//                            case Policy::DynamicDependencyMap::Update:
//                                writer.String("continue");
//                                break;
//                            }
//                        }
//                        else
//                        {
//                            writer.Null();
//                        }
//
//                    writer.EndObject(); // Policy
//                writer.EndObject(); // Config
//
//                const auto serializeTestSelection = [&writer](const TestRunMetrics& testSelection)
//                {
//                    // Tests
//                    writer.Key("tests");
//                    writer.StartArray();
//                    for (const auto& testTarget : testSelection.GetTests().GetIncludededTestRuns())
//                    {
//                        writer.String(testTarget.c_str());
//                    }
//                    for (const auto& testTarget : testSelection.GetTests().GetExcludedTestRuns())
//                    {
//                        writer.String(testTarget.c_str());
//                    }
//                    writer.EndArray();
//
//                    // Included tests
//                    writer.Key("included_tests");
//                    writer.StartArray();
//                    for (const auto& testTarget : testSelection.GetTests().GetIncludededTestRuns())
//                    {
//                        writer.String(testTarget.c_str());
//                    }
//                    writer.EndArray();
//
//                    // Excluded tests
//                    writer.Key("excluded_tests");
//                    writer.StartArray();
//                    for (const auto& testTarget : testSelection.GetTests().GetExcludedTestRuns())
//                    {
//                        writer.String(testTarget.c_str());
//                    }
//                    writer.EndArray();
//
//                    // Start time
//                    // Relative to T0
//                    writer.Key("start_time");
//                    writer.Uint64(testSelection.GetStartTimeSinceT0().count());
//
//                    // End time
//                    // Relative to T0
//                    writer.Key("end_time");
//                    writer.Uint64(testSelection.GetEndTimeSinceT0().count());
//
//                    // Duration
//                    writer.Key("duration");
//                    writer.Uint64(testSelection.GetDuration().count());
//
//                    // Test failures
//                    writer.Key("test_failures");
//                    writer.StartArray();
//                    for (const auto& testTarget : testSelection.GetSequenceFailureReport().GetTestRunFailures())
//                    {
//                        writer.String(testTarget.GetTargetName().c_str());
//                    }
//                    writer.EndArray();
//
//                    // Execution failures
//                    writer.Key("execution_failures");
//                    writer.StartArray();
//                    for (const auto& testTarget : testSelection.GetSequenceFailureReport().GetExecutionFailures())
//                    {
//                        writer.String(testTarget.GetTargetName().c_str());
//                    }
//                    writer.EndArray();
//
//                    // Test timeouts
//                    writer.Key("test_timeouts");
//                    writer.StartArray();
//                    for (const auto& testTarget : testSelection.GetSequenceFailureReport().GetTimedOutTests())
//                    {
//                        writer.String(testTarget.GetTargetName().c_str());
//                    }
//                    writer.EndArray();
//
//                    // Unexecuted tests
//                    writer.Key("unexecuted_tests");
//                    writer.StartArray();
//                    for (const auto& testTarget : testSelection.GetSequenceFailureReport().GetUnexecutedTests())
//                    {
//                        writer.String(testTarget.GetTargetName().c_str());
//                    }
//                    writer.EndArray();
//                };
//
//                // Selected tests
//                writer.Key("selected");
//                writer.StartObject();
//
//                // Efficiency
//                writer.Key("efficiency");
//                if (discardedTestTargets.GetTests().GetTotalNumTests() > 0)
//                {
//                    const double totalTests =
//                        selectedTestTargets.GetTests().GetTotalNumTests() + discardedTestTargets.GetTests().GetTotalNumTests();
//                    const double efficiency = selectedTestTargets.GetTests().GetTotalNumTests() / totalTests;
//                    writer.Double(efficiency);
//                }
//                else
//                {
//                    writer.Double(0.);
//                }
//
//                serializeTestSelection(selectedTestTargets);
//
//                writer.EndObject(); // Selected tests
//
//                // Discarded tests
//                writer.Key("discarded");
//                writer.StartObject();
//                serializeTestSelection(selectedTestTargets);
//                writer.EndObject(); // Discarded tests
//
//            writer.EndObject(); // Sequence
//
//            return stringBuffer.GetString(); 
//        }
//    }
//
//    TestRunMetrics::TestRunMetrics(
//        const Client::TestRunSelection& tests,
//        AZStd::chrono::milliseconds t0,
//        AZStd::chrono::milliseconds startTime,
//        AZStd::chrono::milliseconds duration,
//        const Client::SequenceFailureReport& sequenceFailureReport)
//        : m_tests(tests)
//        , m_startTimeSinceT0(startTime - t0)
//        , m_duration(duration)
//        , m_sequenceFailureReport(sequenceFailureReport)
//    {
//    }
//
//    const Client::TestRunSelection& TestRunMetrics::GetTests() const
//    {
//        return m_tests;
//    }
//
//    AZStd::chrono::milliseconds TestRunMetrics::GetStartTimeSinceT0() const
//    {
//        return m_startTimeSinceT0;
//    }
//
//    AZStd::chrono::milliseconds TestRunMetrics::GetEndTimeSinceT0() const
//    {
//        return m_startTimeSinceT0 + m_duration;
//    }
//
//    AZStd::chrono::milliseconds TestRunMetrics::GetDuration() const
//    {
//        return m_duration;
//    }
//
//    const Client::SequenceFailureReport& TestRunMetrics::GetSequenceFailureReport() const
//    {
//        return m_sequenceFailureReport;
//    }
//
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
//        const TestRunMetrics& selectedTests)
//    {
//        return SerializeSequence(
//            SequenceType::Regular,
//            selectedTests.GetDuration(),
//            result,
//            ChangeList {},
//            testTargetTimeout,
//            globalTimeout,
//            maxConcurrency,
//            executionFailurePolicy,
//            failedTestCoveragePolicy,
//            testFailurePolicy,
//            integrationFailurePolicy,
//            testShardingPolicy,
//            targetOutputCapture,
//            AZStd::nullopt,
//            AZStd::nullopt,
//            selectedTests,
//            TestRunMetrics());
//    }
//} // namespace TestImpact
