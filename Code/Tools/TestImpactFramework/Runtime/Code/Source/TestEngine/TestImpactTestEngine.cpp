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
    // Known error codes for test instrumentation, test runner and unit test library
    namespace ErrorCodes
    {
        namespace OpenCppCoverage
        {
            static constexpr ReturnCode InvalidArgs = -1618178468;
        }

        namespace GTest
        {
            static constexpr ReturnCode Unsuccessful = 1;
        }

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
        //! Calculate the sequence result by analysing the state of the test targets that were run.
        template<typename TestEngineJobType>
        TestSequenceResult CalculateSequenceResult(
            ProcessSchedulerResult result,
            const AZStd::vector<TestEngineJobType>& engineJobs,
            Policy::ExecutionFailure executionFailurePolicy)
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

        Client::TestRunResult GetClientTestRunResultForMeta(const JobMeta& meta)
        {
            // Attempt to determine why a given test target executed successfully but return with an error code
            if (meta.m_returnCode.has_value())
            {
                switch (meta.m_returnCode.value())
                {
                // We will consider test targets that technically execute but their launcher or unit test library return a know error
                // code that pertains to incorrect argument usage as test targets that failed to execute
                case ErrorCodes::OpenCppCoverage::InvalidArgs:
                case ErrorCodes::AZTestRunner::InvalidArgs:
                case ErrorCodes::AZTestRunner::FailedToFindTargetBinary:
                case ErrorCodes::AZTestRunner::ModuleSkipped:
                case ErrorCodes::AZTestRunner::SymbolNotFound:
                    return Client::TestRunResult::FailedToExecute;
                // The trivial case: the test target has failing tests
                case ErrorCodes::GTest::Unsuccessful:
                    return Client::TestRunResult::TestFailures;
                default:
                    break;
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
            // NotExecuted happens when a test is queued for launch but the test runner terminates the sequence (either due to client abort
            // or due to the sequence timer expiring) whereas Terminated happens when the aformentioned scenarios happen when the test target
            // is in flight
            case JobResult::NotExecuted:
            case JobResult::Terminated:
                return Client::TestRunResult::NotRun;
            // The individual timer for the test target expired
            case JobResult::Timeout:
                return Client::TestRunResult::Timeout;
            default:
                throw(/*TestEngine*/Exception(AZStd::string::format("Unexpected job result: %u", static_cast<unsigned int>(meta.m_result))));
            }
        }

        template<typename IdType>
        using TestEngineJobMap = AZStd::unordered_map<IdType, TestEngineJob>;

        // Functor for handling test job runner callbacks
        template<typename TestJobRunner, typename TestEngineJobCallback>
        class TestJobRunnerCallbackHandler
        {
            using IdType = typename TestJobRunner::JobInfo::IdType;
            using JobInfo = typename TestJobRunner::JobInfo;
        public:
            TestJobRunnerCallbackHandler(
                const AZStd::vector<const TestTarget*>& testTargets,
                TestEngineJobMap<IdType>* engineJobs,
                AZStd::optional<TestEngineJobCallback>* callback)
                : m_testTargets(testTargets)
                , m_engineJobs(engineJobs)
                , m_callback(callback)
            {
            }

            void operator()(const typename JobInfo& jobInfo, const TestImpact::JobMeta& meta)
            {
                const auto id = jobInfo.GetId().m_value;
                const auto& args = jobInfo.GetCommand().m_args;
                const auto* target = m_testTargets[id];
                const auto result = GetClientTestRunResultForMeta(meta);

                // Place the test engine job associated with this test run into the map along with its client test run result so
                // that it can be retrieved when the sequence has ended (and any associated artifacts processed)
                const auto& [it, success] = m_engineJobs->emplace(id, TestEngineJob(target, args, meta, result));
                
                if (m_callback->has_value())
                {
                    (*m_callback).value()(it->second);
                }
            }

        private:
            const AZStd::vector<const TestTarget*>& m_testTargets;
            TestEngineJobMap<typename IdType>* m_engineJobs;
            AZStd::optional<TestEngineJobCallback>* m_callback;
        };

        template<typename TestJobRunner, typename TestEngineRun>
        AZStd::vector<TestEngineRun> CompileTestEngineRuns(
            const AZStd::vector<const TestTarget*>& testTargets,
            AZStd::vector<typename TestJobRunner::Job>& runnerjobs,
            TestEngineJobMap<typename TestJobRunner::JobInfo::IdType>&& engineJobs)
        {
            AZStd::vector<TestEngineRun> engineRuns;
            engineRuns.reserve(testTargets.size());

            for (auto& job : runnerjobs)
            {
                const auto id = job.GetJobInfo().GetId().m_value;
                if (auto it = engineJobs.find(id);
                    it != engineJobs.end())
                {
                    // An entry in the test engine job map means that this job was acted upon (an attempt to execute)
                    auto& engineJob = it->second;
                    TestEngineRun run(AZStd::move(engineJob), job.ReleasePayload());
                    engineRuns.push_back(AZStd::move(run));
                }
                else
                {
                    // No entry in the test engine job map means that this job never had the opportunity to be acted upon (the sequence
                    // was terminated whilst this job was still queued up for execution)
                    const auto& args = job.GetJobInfo().GetCommand().m_args;
                    const auto* target = testTargets[id];
                    TestEngineRun run(TestEngineJob(target, args, {}, Client::TestRunResult::NotRun), {});
                    engineRuns.push_back(AZStd::move(run));
                }
            }

            return engineRuns;
        }
    }

    TestEngine::TestEngine(
        const RepoPath& sourceDir,
        const RepoPath& targetBinaryDir,
        const RepoPath& cacheDir,
        const RepoPath& artifactDir,
        const RepoPath& testRunnerBinary,
        const RepoPath& instrumentBinary,
        size_t maxConcurrentRuns)
        : m_maxConcurrentRuns(maxConcurrentRuns)
        , m_testJobInfoGenerator(AZStd::make_unique<TestJobInfoGenerator>(sourceDir, targetBinaryDir, cacheDir, artifactDir, testRunnerBinary, instrumentBinary))
        , m_testEnumerator(AZStd::make_unique<TestEnumerator>(maxConcurrentRuns))
        , m_instrumentedTestRunner(AZStd::make_unique<InstrumentedTestRunner>(maxConcurrentRuns))
        , m_testRunner(AZStd::make_unique<TestRunner>(maxConcurrentRuns))
    {
    }

    TestEngine::~TestEngine() = default;

    AZStd::pair<TestSequenceResult, AZStd::vector<TestEngineEnumeration>> TestEngine::UpdateEnumerationCache(
        const AZStd::vector<const TestTarget*>& testTargets,
        Policy::ExecutionFailure executionFailurePolicy,
        AZStd::optional<AZStd::chrono::milliseconds> testTargetTimeout,
        AZStd::optional<AZStd::chrono::milliseconds> globalTimeout,
        AZStd::optional<TestEngineEnumerationCallback> callback)
    {
        TestEngineJobMap<TestEnumerator::JobInfo::IdType> engineJobs;
        const auto jobInfos = m_testJobInfoGenerator->GenerateTestEnumerationJobInfos(testTargets, TestEnumerator::JobInfo::CachePolicy::Write);
    
        const auto cacheExceptionPolicy = (executionFailurePolicy == Policy::ExecutionFailure::Abort)
            ? TestEnumerator::CacheExceptionPolicy::OnCacheWriteFailure
            : TestEnumerator::CacheExceptionPolicy::Never;
    
        const auto jobExecutionPolicy = executionFailurePolicy == Policy::ExecutionFailure::Abort
            ? (TestEnumerator::JobExceptionPolicy::OnExecutedWithFailure | TestEnumerator::JobExceptionPolicy::OnFailedToExecute)
            : TestEnumerator::JobExceptionPolicy::Never;

        TestJobRunnerCallbackHandler<TestEnumerator, TestEngineEnumerationCallback> jobCallback(testTargets, &engineJobs, &callback);
        auto [result, runnerJobs] = m_testEnumerator->Enumerate(
            jobInfos,
            cacheExceptionPolicy,
            jobExecutionPolicy,
            testTargetTimeout,
            globalTimeout,
            jobCallback);

        auto engineRuns = CompileTestEngineRuns<TestEnumerator, TestEngineEnumeration>(testTargets, runnerJobs, AZStd::move(engineJobs));
        return { CalculateSequenceResult(result, engineRuns, executionFailurePolicy), AZStd::move(engineRuns) };
    }

    AZStd::pair<TestSequenceResult, AZStd::vector<TestEngineRegularRun>> TestEngine::RegularRun(
        const AZStd::vector<const TestTarget*>& testTargets,
        [[maybe_unused]]Policy::TestSharding testShardingPolicy,
        Policy::ExecutionFailure executionFailurePolicy,
        Policy::TestFailure testFailurePolicy,
        [[maybe_unused]]TargetOutputCapture targetOutputCapture,
        AZStd::optional<AZStd::chrono::milliseconds> testTargetTimeout,
        AZStd::optional<AZStd::chrono::milliseconds> globalTimeout,
        AZStd::optional<TestEngineRunCallback> callback)
    {
        TestEngineJobMap<TestRunner::JobInfo::IdType> engineJobs;
        const auto jobInfos = m_testJobInfoGenerator->GenerateRegularTestRunJobInfos(testTargets);

        auto jobExecutionPolicy = TestRunner::JobExceptionPolicy::Never;
        if (executionFailurePolicy == Policy::ExecutionFailure::Abort)
        {
            jobExecutionPolicy |= TestRunner::JobExceptionPolicy::OnExecutedWithFailure;
        }

        if (testFailurePolicy == Policy::TestFailure::Abort)
        {
            jobExecutionPolicy |= TestRunner::JobExceptionPolicy::OnExecutedWithFailure;
        }

        TestJobRunnerCallbackHandler<TestRunner, TestEngineRunCallback> jobCallback(testTargets, &engineJobs, &callback);
        auto [result, runnerJobs] = m_testRunner->RunTests(
            jobInfos,
            jobExecutionPolicy,
            testTargetTimeout,
            globalTimeout,
            jobCallback);

        auto engineRuns = CompileTestEngineRuns<TestRunner, TestEngineRegularRun>(testTargets, runnerJobs, AZStd::move(engineJobs));
        return { CalculateSequenceResult(result, engineRuns, executionFailurePolicy), AZStd::move(engineRuns) };
    }

    AZStd::pair<TestSequenceResult, AZStd::vector<TestEngineInstrumentedRun>> TestEngine::InstrumentedRun(
        const AZStd::vector<const TestTarget*>& testTargets,
        [[maybe_unused]] Policy::TestSharding testShardingPolicy,
        Policy::ExecutionFailure executionFailurePolicy,
        Policy::IntegrityFailure integrityFailurePolicy,
        Policy::TestFailure testFailurePolicy,
        [[maybe_unused]]TargetOutputCapture targetOutputCapture,
        AZStd::optional<AZStd::chrono::milliseconds> testTargetTimeout,
        AZStd::optional<AZStd::chrono::milliseconds> globalTimeout,
        AZStd::optional<TestEngineRunCallback> callback)
    {
        TestEngineJobMap<InstrumentedTestRunner::JobInfo::IdType> engineJobs;
        const auto jobInfos = m_testJobInfoGenerator->GenerateInstrumentedTestRunJobInfos(testTargets, CoverageLevel::Source);
        auto jobExecutionPolicy = InstrumentedTestRunner::JobExceptionPolicy::Never;

        if (executionFailurePolicy == Policy::ExecutionFailure::Abort)
        {
            jobExecutionPolicy |= InstrumentedTestRunner::JobExceptionPolicy::OnExecutedWithFailure;
        }

        if (testFailurePolicy == Policy::TestFailure::Abort)
        {
            jobExecutionPolicy |= InstrumentedTestRunner::JobExceptionPolicy::OnExecutedWithFailure;
        }

        const auto coverageExceptionPolicy = (integrityFailurePolicy == Policy::IntegrityFailure::Abort)
            ? InstrumentedTestRunner::CoverageExceptionPolicy::OnEmptyCoverage
            : InstrumentedTestRunner::CoverageExceptionPolicy::Never;

        TestJobRunnerCallbackHandler<InstrumentedTestRunner, TestEngineRunCallback> jobCallback(testTargets, &engineJobs, &callback);
        auto [result, runnerJobs] = m_instrumentedTestRunner->RunInstrumentedTests(
            jobInfos,
            coverageExceptionPolicy,
            jobExecutionPolicy,
            testTargetTimeout,
            globalTimeout,
            jobCallback);

        auto engineRuns = CompileTestEngineRuns<InstrumentedTestRunner, TestEngineInstrumentedRun>(testTargets, runnerJobs, AZStd::move(engineJobs));

        // do test for passing tests with no coverage as these are integrity failrues
        // (failing tests with no coverage must be considered as test launch failures)

        return { CalculateSequenceResult(result, engineRuns, executionFailurePolicy), AZStd::move(engineRuns) };
    }
}
