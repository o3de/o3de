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

#include <TestEngine/TestImpactTestEngine.h>
#include <TestEngine/Enumeration/TestImpactTestEnumerator.h>
#include <TestEngine/Run/TestImpactInstrumentedTestRunner.h>
#include <TestEngine/Run/TestImpactTestRunner.h>
#include <TestEngine/JobRunner/TestImpactTestJobInfoGenerator.h>

#include <AzCore/std/containers/unordered_map.h>

namespace TestImpact
{
    namespace ErrorCodes
    {
        namespace OpenCppCoverage
        {
            static constexpr ReturnCode InvalidArgs = -1618178468;
        }
    }

    namespace ErrorCodes
    {
        namespace GTest
        {
            static constexpr ReturnCode Unsuccessful = 1;
        }
    }

    namespace ErrorCodes
    {
        namespace AZTestRunner
        {
            static constexpr ReturnCode InvalidArgs = 101;
            static constexpr ReturnCode FailedToFindTargetBinary = 102;
            static constexpr ReturnCode SymbolNotFound = 103;
            static constexpr ReturnCode ModuleSkipped = 104;
        }
    }

    namespace
    {
        // this will change when timeout result implemented in process scheduler!!!
        template<typename TestEngineJobType>
        TestSequenceResult CalculateSequenceResult(const AZStd::vector<TestEngineJobType>& engineJobs, ExecutionFailurePolicy executionFailurePolicy)
        {
            bool hasExecutionFailures = false;
            bool hasTestFailures = false;
            size_t i = 0;
            for (const auto& engineJob : engineJobs)
            {
                switch (engineJob.GetTestResult())
                {
                case TestResult::FailedToExecute:
                {
                    hasExecutionFailures = true;
                    break;
                }
                /*case TestResult::NotExecuted:
                {
                    hasTestTimeouts = true;
                    break;
                }*/
                case TestResult::Timeout:
                case TestResult::TestFailures:
                {
                    hasTestFailures = true;
                    break;
                }
                default:
                {
                    continue;
                }
                }i++;
            }

            if ((hasExecutionFailures && executionFailurePolicy != ExecutionFailurePolicy::Ignore) || hasTestFailures)
            {
                return TestSequenceResult::Failure;
            }
            else
            {
                return TestSequenceResult::Success;
            }
        }

        TestResult GetTestResultForMeta(const JobMeta& meta)
        {
            if (meta.m_returnCode.has_value())
            {
                switch (meta.m_returnCode.value())
                {
                case ErrorCodes::OpenCppCoverage::InvalidArgs:
                case ErrorCodes::AZTestRunner::InvalidArgs:
                case ErrorCodes::AZTestRunner::FailedToFindTargetBinary:
                case ErrorCodes::AZTestRunner::ModuleSkipped:
                case ErrorCodes::AZTestRunner::SymbolNotFound:
                    return TestResult::FailedToExecute;
                case ErrorCodes::GTest::Unsuccessful:
                    return TestResult::TestFailures;
                default:
                    break;
                }
            }

            switch (meta.m_result)
            {
            case JobResult::ExecutedWithFailure:
                return TestResult::TestFailures;
            case JobResult::ExecutedWithSuccess:
                return TestResult::AllTestsPass;
            case JobResult::NotExecuted:
            case JobResult::Terminated:
                return TestResult::NotRun;
            case JobResult::TimedOut:
                return TestResult::Timeout;
            default:
                throw(/*TestEngine*/Exception(AZStd::string::format("Unexpected job result: %u", static_cast<unsigned int>(meta.m_result))));
            }
        }
    }

    TestEngine::TestEngine(
        const AZ::IO::Path& sourceDir,
        const AZ::IO::Path& targetBinaryDir,
        const AZ::IO::Path& cacheDir,
        const AZ::IO::Path& artifactDir,
        const AZ::IO::Path& testRunnerBinary,
        const AZ::IO::Path& instrumentBinary,
        size_t maxConcurrentRuns)
        : m_maxConcurrentRuns(maxConcurrentRuns)
        , m_testJobInfoGenerator(AZStd::make_unique<TestJobInfoGenerator>(sourceDir, targetBinaryDir, cacheDir, artifactDir, testRunnerBinary, instrumentBinary))
    {
    }

    TestEngine::~TestEngine() = default;

    //AZStd::vector<TestEngineEnumeration> TestEngine::UpdateEnumerationCache(
    //    const AZStd::vector<const TestTarget*> testTargets,
    //    ExecutionFailurePolicy executionFailurePolicy,
    //    AZStd::optional<AZStd::chrono::milliseconds> testTargetTimeout,
    //    AZStd::optional<AZStd::chrono::milliseconds> globalTimeout,
    //    AZStd::optional<TestEngineEnumerationCallback> callback)
    //{
    //    AZStd::vector<TestEnumerator::JobInfo> jobInfos;
    //    AZStd::unordered_map<TestEnumerator::JobInfo::IdType, TestEngineJob> engineJobs;
    //    AZStd::vector<TestEngineEnumeration> engineEnumerations;
    //    //engineEnumerations.reserve(testTargets.size());
    //
    //    //for (size_t jobId = 0; jobId < testTargets.size(); jobId++)
    //    //{
    //    //    jobInfos.push_back(
    //    //        m_testJobInfoGenerator->GenerateTestEnumerationJobInfo(testTargets[jobId], { jobId }, TestEnumerator::JobInfo::CachePolicy::Write));
    //    //}
    //
    //    //const auto jobCallback = [&testTargets, &engineJobs, &callback]
    //    //(const TestImpact::TestEnumerator::JobInfo& jobInfo, const TestImpact::JobMeta& meta)
    //    //{
    //    //    const auto& [item, success] =
    //    //        engineJobs.emplace(jobInfo.GetId().m_value, TestEngineJob(testTargets[jobInfo.GetId().m_value], jobInfo.GetCommand().m_args, meta));
    //    //    if (callback.has_value())
    //    //    {
    //    //        (*callback)(item->second);
    //    //    }
    //    //};
    //
    //    //TestImpact::TestEnumerator testEnumerator(jobCallback, m_maxConcurrentRuns, testTargetTimeout, globalTimeout);
    //
    //    //const auto cacheExceptionPolicy = (executionFailurePolicy == ExecutionFailurePolicy::Abort)
    //    //    ? TestEnumerator::CacheExceptionPolicy::OnCacheWriteFailure
    //    //    : TestEnumerator::CacheExceptionPolicy::Never;
    //
    //    //const auto jobExecutionPolicy = executionFailurePolicy == ExecutionFailurePolicy::Abort
    //    //    ? (TestEnumerator::JobExceptionPolicy::OnExecutedWithFailure | TestEnumerator::JobExceptionPolicy::OnFailedToExecute)
    //    //    : TestEnumerator::JobExceptionPolicy::Never;
    //
    //    //auto enumerationJobs = testEnumerator.Enumerate(jobInfos, cacheExceptionPolicy, jobExecutionPolicy);
    //    //for (auto& job : enumerationJobs)
    //    //{
    //    //    auto engineJob = engineJobs.find(job.GetJobInfo().GetId().m_value);
    //    //    // DO EVAL!!
    //    //    TestEngineEnumeration enumeration(AZStd::move(engineJob->second), job.ReleasePayload());
    //    //    engineEnumerations.push_back(AZStd::move(enumeration));
    //    //}
    //
    //    return engineEnumerations;
    //}

    AZStd::pair<TestSequenceResult, AZStd::vector<TestEngineRegularRun>> TestEngine::RegularRun(
        const AZStd::vector<const TestTarget*> testTargets,
        [[maybe_unused]] TestShardingPolicy testShardingPolicy,
        ExecutionFailurePolicy executionFailurePolicy,
        TestFailurePolicy testFailurePolicy,
        [[maybe_unused]]TargetOutputCapture targetOutputCapture, // NOT IMPLEMENTED YET
        AZStd::optional<AZStd::chrono::milliseconds> testTargetTimeout,
        AZStd::optional<AZStd::chrono::milliseconds> globalTimeout,
        AZStd::optional<TestEngineRunCallback> callback)
    {
        AZStd::vector<TestRunner::JobInfo> jobInfos;
        AZStd::unordered_map<TestRunner::JobInfo::IdType, AZStd::pair<TestEngineJob, TestResult>> engineJobs;
        AZStd::vector<TestEngineRegularRun> engineRuns;
        engineRuns.reserve(testTargets.size());

        for (size_t jobId = 0; jobId < testTargets.size(); jobId++)
        {
            jobInfos.push_back(
                m_testJobInfoGenerator->GenerateRegularTestRunJobInfo(testTargets[jobId], { jobId }));
        }

        const auto jobCallback = [&testTargets, &engineJobs, &callback]
        (const TestImpact::TestRunner::JobInfo& jobInfo, const TestImpact::JobMeta& meta)
        {
            const auto& [item, success] =
                engineJobs.emplace(jobInfo.GetId().m_value, AZStd::pair<TestEngineJob, TestResult>{ TestEngineJob(testTargets[jobInfo.GetId().m_value], jobInfo.GetCommand().m_args, meta), GetTestResultForMeta(meta) });
            if (callback.has_value())
            {
                (*callback)(item->second.first, item->second.second);
            }
        };

        TestImpact::TestRunner testRunner(jobCallback, m_maxConcurrentRuns, testTargetTimeout, globalTimeout);

        auto jobExecutionPolicy = TestRunner::JobExceptionPolicy::Never;

        if (executionFailurePolicy == ExecutionFailurePolicy::Abort)
        {
            jobExecutionPolicy |= TestRunner::JobExceptionPolicy::OnExecutedWithFailure;
        }

        if (testFailurePolicy == TestFailurePolicy::Abort)
        {
            jobExecutionPolicy |= TestRunner::JobExceptionPolicy::OnExecutedWithFailure;
        }

        auto runJobs = testRunner.RunTests(jobInfos, jobExecutionPolicy);
        for (auto& job : runJobs)
        {
            if (auto engineJob = engineJobs.find(job.GetJobInfo().GetId().m_value);
                engineJob != engineJobs.end())
            {
                TestEngineRegularRun run(AZStd::move(engineJob->second.first), job.ReleasePayload(), engineJob->second.second);
                engineRuns.push_back(AZStd::move(run));
            }
            else
            {
                TestEngineRegularRun run(TestEngineJob(testTargets[job.GetJobInfo().GetId().m_value], job.GetJobInfo().GetCommand().m_args, {}), {}, TestResult::NotRun);
                engineRuns.push_back(AZStd::move(run));
            }
        }

        const auto sequenceResult = CalculateSequenceResult(engineRuns, executionFailurePolicy);
        return { sequenceResult, AZStd::move(engineRuns) };
    }

    AZStd::pair<TestSequenceResult, AZStd::vector<TestEngineInstrumentedRun>> TestEngine::InstrumentedRun(
        const AZStd::vector<const TestTarget*> testTargets,
        [[maybe_unused]] TestShardingPolicy testShardingPolicy,
        ExecutionFailurePolicy executionFailurePolicy,
        IntegrityFailurePolicy integrityFailurePolicy,
        TestFailurePolicy testFailurePolicy,
        [[maybe_unused]]TargetOutputCapture targetOutputCapture,
        AZStd::optional<AZStd::chrono::milliseconds> testTargetTimeout,
        AZStd::optional<AZStd::chrono::milliseconds> globalTimeout,
        AZStd::optional<TestEngineRunCallback> callback)
    {
        AZStd::vector<InstrumentedTestRunner::JobInfo> jobInfos;
        AZStd::unordered_map<InstrumentedTestRunner::JobInfo::IdType, AZStd::pair<TestEngineJob, TestResult>> engineJobs;
        AZStd::vector<TestEngineInstrumentedRun> engineRuns;
        engineRuns.reserve(testTargets.size());

        for (size_t jobId = 0; jobId < testTargets.size(); jobId++)
        {
            jobInfos.push_back(
                m_testJobInfoGenerator->GenerateInstrumentedTestRunJobInfo(testTargets[jobId], { jobId }, CoverageLevel::Source));
        }

        const auto jobCallback = [&testTargets, &engineJobs, &callback]
        (const TestImpact::InstrumentedTestRunner::JobInfo& jobInfo, const TestImpact::JobMeta& meta)
        {
            const auto& [item, success] =
                engineJobs.emplace(jobInfo.GetId().m_value, AZStd::pair<TestEngineJob, TestResult>{ TestEngineJob(testTargets[jobInfo.GetId().m_value], jobInfo.GetCommand().m_args, meta), GetTestResultForMeta(meta) });
            if (callback.has_value())
            {
                (*callback)(item->second.first, item->second.second);
            }
        };

        TestImpact::InstrumentedTestRunner testRunner(jobCallback, m_maxConcurrentRuns, testTargetTimeout, globalTimeout);

        auto jobExecutionPolicy = InstrumentedTestRunner::JobExceptionPolicy::Never;

        if (executionFailurePolicy == ExecutionFailurePolicy::Abort)
        {
            jobExecutionPolicy |= InstrumentedTestRunner::JobExceptionPolicy::OnExecutedWithFailure;
        }

        if (testFailurePolicy == TestFailurePolicy::Abort)
        {
            jobExecutionPolicy |= TestRunner::JobExceptionPolicy::OnExecutedWithFailure;
        }

        const auto coverageExceptionPolicy = (integrityFailurePolicy == IntegrityFailurePolicy::Abort)
            ? InstrumentedTestRunner::CoverageExceptionPolicy::OnEmptyCoverage
            : InstrumentedTestRunner::CoverageExceptionPolicy::Never;

        auto runJobs = testRunner.RunInstrumentedTests(jobInfos, coverageExceptionPolicy, jobExecutionPolicy);
        for (auto& job : runJobs)
        {
            if (auto engineJob = engineJobs.find(job.GetJobInfo().GetId().m_value);
                engineJob != engineJobs.end())
            {
                TestEngineInstrumentedRun run(AZStd::move(engineJob->second.first), job.ReleasePayload(), engineJob->second.second);
                engineRuns.push_back(AZStd::move(run));
            }
            else
            {
                TestEngineInstrumentedRun run(TestEngineJob(testTargets[job.GetJobInfo().GetId().m_value], job.GetJobInfo().GetCommand().m_args, {}), {}, TestResult::NotRun);
                engineRuns.push_back(AZStd::move(run));
            }
        }

        // do test for passing tests with no coverage as these are integrity failrues
        // (failing tests with no coverage must be considered as test launch failures)

        const auto sequenceResult = CalculateSequenceResult(engineRuns, executionFailurePolicy);
        return { sequenceResult, AZStd::move(engineRuns) };
    }
}
