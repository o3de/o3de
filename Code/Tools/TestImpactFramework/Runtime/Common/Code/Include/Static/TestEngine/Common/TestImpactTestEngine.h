/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Process/Scheduler/TestImpactProcessScheduler.h>
#include <TestEngine/Common/TestImpactTestEngineException.h>
#include <TestEngine/Common/Job/TestImpactTestEngineJob.h>

#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/vector.h>

#pragma once

namespace TestImpact
{
    template<typename TestTarget>
    class TestEngineRegularRun;

    template<typename TestTarget, typename Coverage>
    class TestEngineInstrumentedRun;

    //! Callback for when a given test engine job has failed and requires determination of the error code it returned.
    template<typename TestJobRunner> 
    using ErrorCodeCheckerCallback = AZStd::function<AZStd::optional<Client::TestRunResult>(const typename TestJobRunner::JobInfo& jobInfo, const JobMeta& meta)>;

    //! Callback for when a given test engine job completes.
    template<typename TestTarget>
    using TestEngineJobCompleteCallback = AZStd::function<void(const TestEngineJob<TestTarget>& testJob)>;

    template<typename TestTarget>
    using TestEngineRegularRunResult = AZStd::pair<TestSequenceResult, AZStd::vector<TestEngineRegularRun<TestTarget>>>;

    template<typename TestTarget, typename Coverage>
    using TestEngineInstrumentedRunResult = AZStd::pair<TestSequenceResult, AZStd::vector<TestEngineInstrumentedRun<TestTarget, Coverage>>>;

    // Calculate the sequence result by analyzing the state of the test targets that were run.
    template<typename TestEngineJobType>
    TestSequenceResult CalculateSequenceResult(
        ProcessSchedulerResult result, const AZStd::vector<TestEngineJobType>& engineJobs, Policy::ExecutionFailure executionFailurePolicy)
    {
        if (result == ProcessSchedulerResult::Timeout)
        {
            // Test job runner timing out overrules all other possible sequence results
            return TestSequenceResult::Timeout;
        }

        bool hasExecutionFailures = false;
        bool hasTestFailures = false;
        for (const auto& engineJob : engineJobs)
        {
            switch (engineJob.GetTestResult())
            {
            case Client::TestRunResult::FailedToExecute:
                {
                    hasExecutionFailures = true;
                    break;
                }
            case Client::TestRunResult::Timeout:
            case Client::TestRunResult::TestFailures:
                {
                    hasTestFailures = true;
                    break;
                }
            default:
                {
                    continue;
                }
            }
        }

        // Execution failure can be considered test passes if a permissive execution failure policy is used, otherwise they are failures
        if ((hasExecutionFailures && executionFailurePolicy != Policy::ExecutionFailure::Ignore) || hasTestFailures)
        {
            return TestSequenceResult::Failure;
        }
        else
        {
            return TestSequenceResult::Success;
        }
    }

    // Map for storing the test engine job data of completed test target runs
    template<typename IdType, typename TestTarget>
    using TestEngineJobMap = AZStd::unordered_map<IdType, TestEngineJob<TestTarget>>;

    // Functor for handling test job runner callbacks
    template<typename TestJobRunner, typename TestTarget>
    class TestJobRunnerCallbackHandler
    {
        using IdType = typename TestJobRunner::JobInfo::IdType;
        using JobInfo = typename TestJobRunner::JobInfo;

    public:
        TestJobRunnerCallbackHandler(
            const AZStd::vector<const TestTarget*>& testTargets,
            TestEngineJobMap<IdType, TestTarget>* engineJobs,
            Policy::ExecutionFailure executionFailurePolicy,
            Policy::TestFailure testFailurePolicy,
            ErrorCodeCheckerCallback<TestJobRunner> errorCodeCheckerCallback,
            AZStd::optional<TestEngineJobCompleteCallback<TestTarget>>* callback)
            : m_testTargets(testTargets)
            , m_engineJobs(engineJobs)
            , m_executionFailurePolicy(executionFailurePolicy)
            , m_testFailurePolicy(testFailurePolicy)
            , m_errorCodeCheckerCallback(errorCodeCheckerCallback)
            , m_callback(callback)
        {
        }

        [[nodiscard]] ProcessCallbackResult operator()(const typename JobInfo& jobInfo, const TestImpact::JobMeta& meta, StdContent&& std)
        {
            const auto id = jobInfo.GetId().m_value;
            const auto& args = jobInfo.GetCommand().m_args;
            const auto* target = m_testTargets[id];
            const auto result = GetClientTestRunResultForMeta(jobInfo, meta);

            // Place the test engine job associated with this test run into the map along with its client test run result so
            // that it can be retrieved when the sequence has ended (and any associated artifacts processed)
            const auto& [it, success] = m_engineJobs->emplace(
                id,
                TestEngineJob<TestTarget>(
                    target, args, meta, result, AZStd::move(std.m_out.value_or("")), AZStd::move(std.m_err.value_or(""))));

            if (m_callback->has_value())
            {
                (*m_callback).value()(it->second);
            }

            if ((result == Client::TestRunResult::FailedToExecute && m_executionFailurePolicy == Policy::ExecutionFailure::Abort) ||
                (result == Client::TestRunResult::TestFailures && m_testFailurePolicy == Policy::TestFailure::Abort))
            {
                return ProcessCallbackResult::Abort;
            }

            return ProcessCallbackResult::Continue;
        }

        // Deduces the run result for a given test target based on how the process exited and known return values
        Client::TestRunResult GetClientTestRunResultForMeta(const typename JobInfo& jobInfo, const JobMeta& meta)
        {
            // Attempt to determine why a given test target executed successfully but return with an error code
            if (meta.m_returnCode.has_value())
            {
                if (const auto result = m_errorCodeCheckerCallback(jobInfo, meta); result.has_value())
                {
                    return result.value();
                }
            }

            switch (meta.m_result)
            {
            // If the test target executed successfully but returned in an unknown abnormal state it's probably because a test caused
            // an unhandled exception, segfault or any other of the weird and wonderful ways a badly behaving test can terminate
            case JobResult::ExecutedWithFailure:
                return Client::TestRunResult::TestFailures;
            // The trivial case: all of the tests in the test target passed
            case JobResult::ExecutedWithSuccess:
                return Client::TestRunResult::AllTestsPass;
            // NotExecuted happens when a test is queued for launch but the test runner terminates the sequence (either due to client
            // abort or due to the sequence timer expiring) whereas Terminated happens when the aforementioned scenarios happen when the
            // test target is in flight
            case JobResult::NotExecuted:
            case JobResult::Terminated:
                return Client::TestRunResult::NotRun;
            // The individual timer for the test target expired
            case JobResult::Timeout:
                return Client::TestRunResult::Timeout;
            case JobResult::FailedToExecute:
                return Client::TestRunResult::FailedToExecute;
            default:
                throw(TestEngineException(AZStd::string::format("Unexpected job result: %u", static_cast<unsigned int>(meta.m_result))));
            }
        }

    private:
        const AZStd::vector<const TestTarget*>& m_testTargets;
        TestEngineJobMap<typename IdType, TestTarget>* m_engineJobs;
        Policy::ExecutionFailure m_executionFailurePolicy;
        Policy::TestFailure m_testFailurePolicy;
        ErrorCodeCheckerCallback<TestJobRunner> m_errorCodeCheckerCallback;
        AZStd::optional<TestEngineJobCompleteCallback<TestTarget>>* m_callback;
    };

    // Helper trait for identifying the test engine job specialization for a given test job runner
    template<typename TestJobRunner>
    struct TestJobRunnerTrait
    {
    };

    // Helper function for getting the type directly of the test job runner trait
    template<typename TestJobRunner>
    using TestEngineJobType = typename TestJobRunnerTrait<TestJobRunner>::TestEngineJobType;

    // Helper function to compile the run type specific test engine jobs from their associated jobs and payloads
    template<typename TestJobRunner, typename TestTarget>
    AZStd::vector<TestEngineJobType<TestJobRunner>> CompileTestEngineRuns(
        const AZStd::vector<const TestTarget*>& testTargets,
        AZStd::vector<typename TestJobRunner::Job>& runnerjobs,
        TestEngineJobMap<typename TestJobRunner::JobInfo::IdType, TestTarget>&& engineJobs)
    {
        AZStd::vector<TestEngineJobType<TestJobRunner>> engineRuns;
        engineRuns.reserve(testTargets.size());

        for (auto& job : runnerjobs)
        {
            const auto id = job.GetJobInfo().GetId().m_value;
            if (auto it = engineJobs.find(id); it != engineJobs.end())
            {
                // An entry in the test engine job map means that this job was acted upon (an attempt to execute, successful or
                // otherwise)
                auto& engineJob = it->second;
                TestEngineJobType<TestJobRunner> run(AZStd::move(engineJob), job.ReleasePayload());
                engineRuns.push_back(AZStd::move(run));
            }
            else
            {
                // No entry in the test engine job map means that this job never had the opportunity to be acted upon (the sequence
                // was terminated whilst this job was still queued up for execution)
                const auto& args = job.GetJobInfo().GetCommand().m_args;
                const auto* target = testTargets[id];
                TestEngineJobType<TestJobRunner> run(
                    TestEngineJob<TestTarget>(target, args, {}, Client::TestRunResult::NotRun, "", ""), {});
                engineRuns.push_back(AZStd::move(run));
            }
        }

        return engineRuns;
    }

    //! Helper function to run the specified tests.
    template<typename TestJobRunner, typename TestTarget>
    auto RunTests(
        TestJobRunner* testRunner,
        const AZStd::vector<typename TestJobRunner::JobInfo>& jobInfos,
        const AZStd::vector<const TestTarget*>& testTargets,
        ErrorCodeCheckerCallback<TestJobRunner> errorCheckerCallback,
        Policy::ExecutionFailure executionFailurePolicy,
        Policy::TestFailure testFailurePolicy,
        Policy::TargetOutputCapture targetOutputCapture,
        AZStd::optional<AZStd::chrono::milliseconds> testTargetTimeout,
        AZStd::optional<AZStd::chrono::milliseconds> globalTimeout,
        AZStd::optional<TestEngineJobCompleteCallback<TestTarget>> jobCallback,
        AZStd::optional<typename TestJobRunner::StdContentCallback> stdContentCallback)
    {
        TestEngineJobMap<typename TestJobRunner::JobInfo::IdType, TestTarget> engineJobs;
        auto [result, runnerJobs] = testRunner->RunTests(
            jobInfos,
            targetOutputCapture == Policy::TargetOutputCapture::None ? StdOutputRouting::None : StdOutputRouting::ToParent,
            targetOutputCapture == Policy::TargetOutputCapture::None ? StdErrorRouting::None : StdErrorRouting::ToParent,
            testTargetTimeout,
            globalTimeout,
            TestJobRunnerCallbackHandler<TestJobRunner, TestTarget>(
                testTargets,
                &engineJobs,
                executionFailurePolicy,
                testFailurePolicy,
                errorCheckerCallback,
                &jobCallback),
            stdContentCallback);

        auto engineRuns = CompileTestEngineRuns<TestJobRunner, TestTarget>(testTargets, runnerJobs, AZStd::move(engineJobs));
        return AZStd::pair{ CalculateSequenceResult(result, engineRuns, executionFailurePolicy), AZStd::move(engineRuns) };
    }

    //! Helper function to generate the test engine job infos and the proceed to running the tests.
    template<typename TestJobRunner, typename TestJobInfoGenerator, typename TestTarget>
    auto GenerateJobInfosAndRunTests(
        TestJobRunner* testRunner,
        TestJobInfoGenerator* jobInfoGenerator,
        const AZStd::vector<const TestTarget*>& testTargets,
        ErrorCodeCheckerCallback<TestJobRunner> errorCheckerCallback,
        Policy::ExecutionFailure executionFailurePolicy,
        Policy::TestFailure testFailurePolicy,
        Policy::TargetOutputCapture targetOutputCapture,
        AZStd::optional<AZStd::chrono::milliseconds> testTargetTimeout,
        AZStd::optional<AZStd::chrono::milliseconds> globalTimeout,
        AZStd::optional<TestEngineJobCompleteCallback<TestTarget>> jobCallback,
        AZStd::optional<typename TestJobRunner::StdContentCallback> stdContentCallback)
    {
        return RunTests(
            testRunner,
            jobInfoGenerator->GenerateJobInfos(testTargets),
            testTargets,
            errorCheckerCallback,
            executionFailurePolicy,
            testFailurePolicy,
            targetOutputCapture,
            testTargetTimeout,
            globalTimeout,
            jobCallback,
            stdContentCallback
        );
    }

    template<typename TestEngineJob>
    auto GenerateInstrumentedRunResult(const AZStd::pair<TestSequenceResult, AZStd::vector<TestEngineJob>>& engineJobs, Policy::IntegrityFailure integrityFailurePolicy)
    {
        const auto& [result, engineRuns] = engineJobs;

        // Now that we know the true result of successful jobs that return non-zero we can deduce if we have any integrity failures
        // where a test target ran and completed its tests without incident yet failed to produce coverage data
        if (integrityFailurePolicy == Policy::IntegrityFailure::Abort)
        {
            for (const auto& engineRun : engineRuns)
            {
                if (const auto testResult = engineRun.GetTestResult();
                    testResult == Client::TestRunResult::AllTestsPass || testResult == Client::TestRunResult::TestFailures)
                {
                    AZ_TestImpact_Eval(
                        engineRun.GetCoverge().has_value(),
                        TestEngineException,
                        AZStd::string::format(
                            "Test target %s completed its test run but failed to produce coverage data",
                            engineRun.GetTestTarget()->GetName().c_str()));
                }
            }
        }

        return AZStd::pair{ result, engineRuns };
    }
} // namespace TestImpact
