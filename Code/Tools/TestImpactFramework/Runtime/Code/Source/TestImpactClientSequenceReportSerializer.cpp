/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of
 * this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TestImpactFramework/TestImpactClientSequenceReportSerializer.h>
#include <TestImpactFramework/TestImpactSequenceReportException.h>
#include <TestImpactFramework/TestImpactUtils.h>

#include <AzCore/JSON/document.h>
#include <AzCore/JSON/prettywriter.h>
#include <AzCore/JSON/rapidjson.h>
#include <AzCore/JSON/stringbuffer.h>

namespace TestImpact
{
    namespace
    {
        AZ::u64 TimePointInMsAsInt64(AZStd::chrono::high_resolution_clock::time_point timePoint)
        {
            return AZStd::chrono::duration_cast<AZStd::chrono::milliseconds>(timePoint.time_since_epoch()).count();
        }

        void SerializeTestRunMembers(const Client::TestRun& testRun, rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer)
        {
            // Name
            writer.Key("name");
            writer.String(testRun.GetTargetName().c_str());

            // Command string
            writer.Key("command_string");
            writer.String(testRun.GetCommandString().c_str());

            // Start time
            writer.Key("start_time");
            writer.Int64(TimePointInMsAsInt64(testRun.GetStartTime()));

            // End time
            writer.Key("end_time");
            writer.Int64(TimePointInMsAsInt64(testRun.GetEndTime()));

            // Duration
            writer.Key("duration");
            writer.Uint64(testRun.GetDuration().count());

            // Result
            writer.Key("result");
            writer.String(TestRunResultAsString(testRun.GetResult()).c_str());
        }

        void SerializeTestRun(const Client::TestRun& testRun, rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer)
        {
            writer.StartObject();

                SerializeTestRunMembers(testRun, writer);

            writer.EndObject();
        }

        void SerializeCompletedTestRun(const Client::CompletedTestRun& testRun, rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer)
        {
            writer.StartObject();
            
                SerializeTestRunMembers(testRun, writer);

                 // Number of passing test cases
                writer.Key("num_passing_tests");
                writer.Uint64(testRun.GetTotalNumPassingTests());
            
                // Number of failing test cases
                writer.Key("num_failing_tests");
                writer.Uint64(testRun.GetTotalNumFailingTests());
            
                // Test Suites
                writer.Key("test_suites");
                writer.StartArray();
            
                for (const auto& testSuite : testRun.GetTestSuites())
                {
                    // Test suite
                    writer.StartObject(); 
            
                        // Name
                        writer.Key("name");
                        writer.String(testSuite.GetName().c_str());
            
                        // Test cases
                        writer.Key("test_cases");
                        writer.StartArray();
            
                        for (const auto& testCase : testSuite.GetTestCases())
                        {
                            // Test case
                            writer.StartObject();
            
                                // Name
                                writer.Key("name");
                                writer.String(testCase.GetName().c_str());
            
                                // Result
                                writer.Key("result");
                                writer.String(ClientTestCaseResultAsString(testCase.GetResult()).c_str());
            
                            writer.EndObject(); // Test case
                        }
            
                        writer.EndArray(); // Test cases
            
                    writer.EndObject(); // Test suite
                }
            
                writer.EndArray(); // Test suites
            
            writer.EndObject();
        }

        void SerializeTestRunReport(
            const Client::TestRunReport& testRunReport, rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer)
        {
            writer.StartObject();

                // Result
                writer.Key("result");
                writer.String(TestSequenceResultAsString(testRunReport.GetResult()).c_str());

                // Start time
                writer.Key("start_time");
                writer.Int64(TimePointInMsAsInt64(testRunReport.GetStartTime()));

                // End time
                writer.Key("end_time");
                writer.Int64(TimePointInMsAsInt64(testRunReport.GetEndTime()));

                // Duration
                writer.Key("duration");
                writer.Uint64(testRunReport.GetDuration().count());

                // Number of passing test runs
                writer.Key("num_passing_test_runs");
                writer.Uint64(testRunReport.GetNumPassingTestRuns());

                // Number of failing test runs
                writer.Key("num_failing_test_runs");
                writer.Uint64(testRunReport.GetNumFailingTestRuns());

                // Number of test runs that failed to execute
                writer.Key("num_execution_failure_test_runs");
                writer.Uint64(testRunReport.GetNumExecutionFailureTestRuns());

                // Number of timed out test runs
                writer.Key("num_timed_out_test_runs");
                writer.Uint64(testRunReport.GetNumTimedOutTestRuns());

                // Number of unexecuted test runs
                writer.Key("num_unexecuted_test_runs");
                writer.Uint64(testRunReport.GetNumUnexecutedTestRuns());

                // Passing test runs
                writer.Key("passing_test_runs");
                writer.StartArray();
                for (const auto& testRun : testRunReport.GetPassingTestRuns())
                {
                    SerializeCompletedTestRun(testRun, writer);
                }
                writer.EndArray(); // Passing test runs

                // Failing test runs
                writer.Key("failing_test_runs");
                writer.StartArray();
                for (const auto& testRun : testRunReport.GetFailingTestRuns())
                {
                    SerializeCompletedTestRun(testRun, writer);
                }
                writer.EndArray(); // Failing test runs

                // Execution failures
                writer.Key("execution_failures_test_runs");
                writer.StartArray();
                for (const auto& testRun : testRunReport.GetExecutionFailureTestRuns())
                {
                    SerializeTestRun(testRun, writer);
                }
                writer.EndArray(); // Execution failures

                // Timed out test runs
                writer.Key("timed_out_test_runs");
                writer.StartArray();
                for (const auto& testRun : testRunReport.GetTimedOutTestRuns())
                {
                    SerializeTestRun(testRun, writer);
                }
                writer.EndArray(); // Timed out test runs

                // Unexecuted test runs
                writer.Key("unexecuted_test_runs");
                writer.StartArray();
                for (const auto& testRun : testRunReport.GetUnexecutedTestRuns())
                {
                    SerializeTestRun(testRun, writer);
                }
                writer.EndArray(); // Unexecuted test runs

                // Number of passing tests
                writer.Key("num_passing_tests");
                writer.Uint64(testRunReport.GetTotalNumPassingTests());

                // Number of failing tests
                writer.Key("num_failing_tests");
                writer.Uint64(testRunReport.GetTotalNumFailingTests());

            writer.EndObject();
        }

        void SerializeTestSelection(
            const Client::TestRunSelection& testSelection, rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer)
        {
            writer.StartObject();

                // Total number of test runs
                writer.Key("num_total_tests");
                writer.Uint64(testSelection.GetTotalNumTests());

                // Number of included test runs
                writer.Key("num_included_tests");
                writer.Uint64(testSelection.GetNumIncludedTestRuns());

                // Number of excluded test runs
                writer.Key("num_excluded_tests");
                writer.Uint64(testSelection.GetNumExcludedTestRuns());

                // Included test runs
                writer.Key("included_test_runs");
                writer.StartArray();
                for (const auto& testRun : testSelection.GetIncludededTestRuns())
                {
                    writer.String(testRun.c_str());
                }
                writer.EndArray(); // Included test runs

                // Excluded test runs
                writer.Key("excluded_test_runs");
                writer.StartArray();
                for (const auto& testRun : testSelection.GetExcludedTestRuns())
                {
                    writer.String(testRun.c_str());
                }
                writer.EndArray(); // Excluded test runs

            writer.EndObject();
        }

        void SerializePolicyStateBaseMembers(const PolicyStateBase& policyState, rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer)
        {
            // Execution failure
            writer.Key("execution_failure");
            writer.String(ExecutionFailurePolicyAsString(policyState.m_executionFailurePolicy).c_str());
            
            // Failed test coverage
            writer.Key("coverage_failure");
            writer.String(FailedTestCoveragePolicyAsString(policyState.m_failedTestCoveragePolicy).c_str());
            
            // Test failure
            writer.Key("test_failure");
            writer.String(TestFailurePolicyAsString(policyState.m_testFailurePolicy).c_str());
            
            // Integrity failure
            writer.Key("integrity_failure");
            writer.String(IntegrityFailurePolicyAsString(policyState.m_integrityFailurePolicy).c_str());
            
            // Test sharding
            writer.Key("test_sharding");
            writer.String(TestShardingPolicyAsString(policyState.m_testShardingPolicy).c_str());
            
            // Target output capture
            writer.Key("target_output_capture");
            writer.String(TargetOutputCapturePolicyAsString(policyState.m_targetOutputCapture).c_str());
        }

        void SerializePolicyStateMembers(
            const SequencePolicyState& policyState, rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer)
        {
            SerializePolicyStateBaseMembers(policyState.m_basePolicies, writer);

            // Test prioritization
            writer.Key("test_prioritization");
            writer.String("");

            // Dynamic dependency map
            writer.Key("dynamic_dependency_map");
            writer.String("");
        }

        void SerializePolicyStateMembers(
            const SafeImpactAnalysisSequencePolicyState& policyState, rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer)
        {
            SerializePolicyStateBaseMembers(policyState.m_basePolicies, writer);

            // Test prioritization
            writer.Key("test_prioritization");
            writer.String(TestPrioritizationPolicyAsString(policyState.m_testPrioritizationPolicy).c_str());

            // Dynamic dependency map
            writer.Key("dynamic_dependency_map");
            writer.String("");
        }

        void SerializePolicyStateMembers(
            const ImpactAnalysisSequencePolicyState& policyState, rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer)
        {
            SerializePolicyStateBaseMembers(policyState.m_basePolicies, writer);

            // Test prioritization
            writer.Key("test_prioritization");
            writer.String(TestPrioritizationPolicyAsString(policyState.m_testPrioritizationPolicy).c_str());

            // Dynamic dependency map
            writer.Key("dynamic_dependency_map");
            writer.String(DynamicDependencyMapPolicyAsString(policyState.m_dynamicDependencyMap).c_str());
        }

        template<typename PolicyStateType>
        void SerializeSequenceReportBaseMembers(
            const Client::SequenceReportBase<PolicyStateType>& sequenceReport, rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer)
        {
            // Type
            writer.Key("type");
            writer.String(SequenceReportTypeAsString(sequenceReport.GetType()).c_str());

            // Maximum concurrency
            writer.Key("max_concurrency");
            writer.Uint64(sequenceReport.GetMaxConcurrency());

            // Policies
            writer.Key("policy");
            writer.StartObject();
                SerializePolicyStateMembers(sequenceReport.GetPolicyState(), writer);
            writer.EndObject(); // Policies

            // Suite
            writer.Key("suite");
            writer.String(SuiteTypeAsString(sequenceReport.GetSuite()).c_str());

            // Selected test runs
            writer.Key("selected_test_runs");
            SerializeTestSelection(sequenceReport.GetSelectedTestRuns(), writer);

            // Selected test run report
            writer.Key("selected_test_run_report");
            SerializeTestRunReport(sequenceReport.GetSelectedTestRunReport(), writer);

            // Start time
            writer.Key("start_time");
            writer.Int64(TimePointInMsAsInt64(sequenceReport.GetStartTime()));

            // End time
            writer.Key("end_time");
            writer.Int64(TimePointInMsAsInt64(sequenceReport.GetEndTime()));

            // Duration
            writer.Key("duration");
            writer.Uint64(sequenceReport.GetDuration().count());

            // Result
            writer.Key("result");
            writer.String(TestSequenceResultAsString(sequenceReport.GetResult()).c_str());

            // Total number of test runs
            writer.Key("total_num_passing_test_runs");
            writer.Uint64(sequenceReport.GetTotalNumTestRuns());

             // Total number of passing test runs
            writer.Key("total_num_passing_test_runs");
            writer.Uint64(sequenceReport.GetTotalNumPassingTestRuns());

            // Total number of failing test runs
            writer.Key("total_num_failing_test_runs");
            writer.Uint64(sequenceReport.GetTotalNumFailingTestRuns());

            // Total number of test runs that failed to execute
            writer.Key("total_num_execution_failure_test_runs");
            writer.Uint64(sequenceReport.GetTotalNumExecutionFailureTestRuns());

            // Total number of timed out test runs
            writer.Key("total_num_timed_out_test_runs");
            writer.Uint64(sequenceReport.GetTotalNumTimedOutTestRuns());

            // Total number of unexecuted test runs
            writer.Key("total_num_unexecuted_test_runs");
            writer.Uint64(sequenceReport.GetTotalNumUnexecutedTestRuns());
        }

        template<typename PolicyStateType>
        void SerializeDraftingSequenceReportMembers(
            const Client::DraftingSequenceReport<PolicyStateType>& sequenceReport, rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer)
        {
            // Drafted test runs
            writer.Key("drafted_test_runs");
            writer.StartArray();
            for (const auto& testRun : sequenceReport.GetDraftedTestRuns())
            {
                writer.String(testRun.c_str());
            }
            writer.EndArray(); // Drafted test runs

            // Drafted test run report
            writer.Key("drafted_test_run_report");
            SerializeTestRunReport(sequenceReport.GetDraftedTestRunReport(), writer);
        }
    }

    AZStd::string SerializeSequenceReport(const Client::SequenceReport& sequenceReport)
    {
        rapidjson::StringBuffer stringBuffer;
        rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(stringBuffer);

        writer.StartObject();
            SerializeSequenceReportBaseMembers(sequenceReport, writer);
        writer.EndObject();

        return stringBuffer.GetString();
    }

    AZStd::string SerializeSequenceReport(const Client::ImpactAnalysisSequenceReport& sequenceReport)
    {
        rapidjson::StringBuffer stringBuffer;
        rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(stringBuffer);

        writer.StartObject();
            SerializeDraftingSequenceReportMembers(sequenceReport, writer);

            // Discarded test runs
            writer.Key("discarded_test_runs");
            writer.StartArray();
            for (const auto& testRun : sequenceReport.GetDiscardedTestRuns())
            {
                writer.String(testRun.c_str());
            }
            writer.EndArray(); // Discarded test runs

        writer.EndObject();

        return stringBuffer.GetString();
    }

    AZStd::string SerializeSequenceReport(const Client::SafeImpactAnalysisSequenceReport& sequenceReport)
    {
        rapidjson::StringBuffer stringBuffer;
        rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(stringBuffer);

        writer.StartObject();
            SerializeDraftingSequenceReportMembers(sequenceReport, writer);

            // Discarded test runs
            SerializeTestSelection(sequenceReport.GetDiscardedTestRuns(), writer);

            // Discarded test run report
            SerializeTestRunReport(sequenceReport.GetDiscardedTestRunReport(), writer);

        writer.EndObject();

        return stringBuffer.GetString();
    }
} // namespace TestImpact
