/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TestImpactFramework/TestImpactUtils.h>

#include <Target/Native/TestImpactnativeTestTarget.h>
#include <TestEngine/Common/TestImpactTestEngineException.h>
#include <TestEngine/TestImpactTestEngine.h>
#include <TestRunner/Common/TestImpactErrorCodeChecker.h>
#include <TestRunner/Native/TestImpactNativeErrorCodeHandler.h>
#include <TestRunner/Native/Enumeration/TestImpactNativeTestEnumerator.h>
#include <TestRunner/Native/Run/TestImpactNativeInstrumentedTestRunner.h>
#include <TestRunner/Native/Run/TestImpactNativeRegularTestRunner.h>
#include <TestRunner/Native/Job/TestImpactNativeTestJobInfoGenerator.h>

#include <AzCore/std/containers/unordered_map.h>

namespace TestImpact
{
    namespace
    {
        // Calculate the sequence result by analysing the state of the test targets that were run.
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

        // Map for storing the test engine job data of completed test target runs
        template<typename IdType>
        using TestEngineJobMap = AZStd::unordered_map<IdType, TestEngineJob<NativeBuildTargetTraits>>;

        //!
        AZStd::optional<Client::TestRunResult> StandAloneHandler(ReturnCode returnCode)
        {
            if (returnCode == 0)
            {
                return AZStd::nullopt;
            }
            else
            {
                return Client::TestRunResult::FailedToExecute;
            }
        }

        // Functor for handling test job runner callbacks
        template<typename TestJobRunner>
        class TestJobRunnerCallbackHandler
        {
            using IdType = typename TestJobRunner::JobInfo::IdType;
            using JobInfo = typename TestJobRunner::JobInfo;
        public:
            TestJobRunnerCallbackHandler(
                const AZStd::vector<const NativeTestTarget*>& testTargets,
                TestEngineJobMap<IdType>* engineJobs,
                Policy::ExecutionFailure executionFailurePolicy,
                Policy::TestFailure testFailurePolicy,
                AZStd::vector<ErrorCodeHandler>&& standAloneErrorCodeHandlers,
                AZStd::vector<ErrorCodeHandler>&& testRunnerErrorCodeHandlers,
                AZStd::optional<TestEngineJobCompleteCallback>* callback)
                : m_testTargets(testTargets)
                , m_engineJobs(engineJobs)
                , m_executionFailurePolicy(executionFailurePolicy)
                , m_testFailurePolicy(testFailurePolicy)
                , m_standAloneErrorCodeChecker(AZStd::move(standAloneErrorCodeHandlers))
                , m_testRunnerErrorCodeChecker(AZStd::move(testRunnerErrorCodeHandlers))
                , m_callback(callback)
            {
            }

            [[nodiscard]] ProcessCallbackResult operator()(
                const typename JobInfo& jobInfo, const TestImpact::JobMeta& meta, StdContent&& std)
            {
                const auto id = jobInfo.GetId().m_value;
                const auto& args = jobInfo.GetCommand().m_args;
                const auto* target = m_testTargets[id];
                const auto result = GetClientTestRunResultForMeta(jobInfo, meta);

                // Place the test engine job associated with this test run into the map along with its client test run result so
                // that it can be retrieved when the sequence has ended (and any associated artifacts processed)
                const auto& [it, success] = m_engineJobs->emplace(
                    id,
                    TestEngineJob<NativeBuildTargetTraits>(
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
                    const ErrorCodeChecker& checker =
                        jobInfo.GetLaunchMethod() == LaunchMethod::StandAlone ? m_standAloneErrorCodeChecker : m_testRunnerErrorCodeChecker;
                    if (const auto result = checker.CheckErrorCode(meta.m_returnCode.value());
                        result.has_value())
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
                default:
                    throw(
                        TestEngineException(AZStd::string::format("Unexpected job result: %u", static_cast<unsigned int>(meta.m_result))));
                }
            }

        private:
            const AZStd::vector<const NativeTestTarget*>& m_testTargets;
            TestEngineJobMap<typename IdType>* m_engineJobs;
            Policy::ExecutionFailure m_executionFailurePolicy;
            Policy::TestFailure m_testFailurePolicy;
            ErrorCodeChecker m_standAloneErrorCodeChecker;
            ErrorCodeChecker m_testRunnerErrorCodeChecker;
            AZStd::optional<TestEngineJobCompleteCallback>* m_callback;
        };

        class RegularTestJobRunnerCallbackHandler
            : public TestJobRunnerCallbackHandler<NativeRegularTestRunner>
        {
            using ParentHandler = TestJobRunnerCallbackHandler<NativeRegularTestRunner>;

        public:
            RegularTestJobRunnerCallbackHandler(
                const AZStd::vector<const NativeTestTarget*>& testTargets,
                TestEngineJobMap<NativeRegularTestRunner::JobInfo::IdType>* engineJobs,
                Policy::ExecutionFailure executionFailurePolicy,
                Policy::TestFailure testFailurePolicy,
                AZStd::optional<TestEngineJobCompleteCallback>* callback)
                : ParentHandler(
                      testTargets,
                      engineJobs,
                      executionFailurePolicy,
                      testFailurePolicy,
                      AZStd::vector<ErrorCodeHandler>{ GetNativeTestLibraryErrorCodeHandler(),
                                                       [](ReturnCode returnCode)
                                                       {
                                                           return StandAloneHandler(returnCode);
                                                       } },
                      AZStd::vector<ErrorCodeHandler>{ GetNativeTestRunnerErrorCodeHandler(), GetNativeTestLibraryErrorCodeHandler() },
                      callback)
            {
            }
        };

        class InstrumentedRegularTestJobRunnerCallbackHandler
            : public TestJobRunnerCallbackHandler<NativeInstrumentedTestRunner>
        {
            using ParentHandler = TestJobRunnerCallbackHandler<NativeInstrumentedTestRunner>;

        public:
            InstrumentedRegularTestJobRunnerCallbackHandler(
                const AZStd::vector<const NativeTestTarget*>& testTargets,
                TestEngineJobMap<NativeInstrumentedTestRunner::JobInfo::IdType>* engineJobs,
                Policy::ExecutionFailure executionFailurePolicy,
                Policy::TestFailure testFailurePolicy,
                AZStd::optional<TestEngineJobCompleteCallback>* callback)
                : ParentHandler(
                      testTargets,
                      engineJobs,
                      executionFailurePolicy,
                      testFailurePolicy,
                      AZStd::vector<ErrorCodeHandler>{ GetNativeInstrumentationErrorCodeHandler(), GetNativeTestLibraryErrorCodeHandler(),
                                                       [](ReturnCode returnCode)
                                                       {
                                                           return StandAloneHandler(returnCode);
                                                       } },
                      AZStd::vector<ErrorCodeHandler>{ GetNativeInstrumentationErrorCodeHandler(), GetNativeTestRunnerErrorCodeHandler(),
                                                       GetNativeTestLibraryErrorCodeHandler() },
                      callback)
            {
            }
        };

        // Helper trait for identifying the test engine job specialization for a given test job runner
        template<typename TestJobRunner>
        struct TestJobRunnerTrait
        {};

        // Helper function for getting the type directly of the test job runner trait
        template<typename TestJobRunner>
        using TestEngineJobType = typename TestJobRunnerTrait<TestJobRunner>::TestEngineJobType;

        // Helper function for getting the type directly of the test job runner trait
        template<typename TestJobRunner>
        using TestJobRunnerCallbackHandlerType = typename TestJobRunnerTrait<TestJobRunner>::TestJobRunnerCallbackHandlerType;

        // Type trait for the test enumerator
        template<>
        struct TestJobRunnerTrait<NativeTestEnumerator>
        {
            using TestEngineJobType = TestEngineEnumeration<NativeBuildTargetTraits>;
            using TestJobRunnerCallbackHandlerType = RegularTestJobRunnerCallbackHandler; // INCORRECT!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        };

        // Type trait for the test runner
        template<>
        struct TestJobRunnerTrait<NativeRegularTestRunner>
        {
            using TestEngineJobType = TestEngineRegularRun<NativeBuildTargetTraits>;
            using TestJobRunnerCallbackHandlerType = RegularTestJobRunnerCallbackHandler;
        };

        // Type trait for the instrumented test runner
        template<>
        struct TestJobRunnerTrait<NativeInstrumentedTestRunner>
        {
            using TestEngineJobType = TestEngineInstrumentedRun<NativeBuildTargetTraits>;
            using TestJobRunnerCallbackHandlerType = InstrumentedRegularTestJobRunnerCallbackHandler;
        };
                
        // Helper function to compile the run type specific test engine jobs from their associated jobs and payloads
        template<typename TestJobRunner>
        AZStd::vector<TestEngineJobType<TestJobRunner>> CompileTestEngineRuns(
            const AZStd::vector<const NativeTestTarget*>& testTargets,
            AZStd::vector<typename TestJobRunner::Job>& runnerjobs,
            TestEngineJobMap<typename TestJobRunner::JobInfo::IdType>&& engineJobs)
        {
            AZStd::vector<TestEngineJobType<TestJobRunner>> engineRuns;
            engineRuns.reserve(testTargets.size());

            for (auto& job : runnerjobs)
            {
                const auto id = job.GetJobInfo().GetId().m_value;
                if (auto it = engineJobs.find(id);
                    it != engineJobs.end())
                {
                    // An entry in the test engine job map means that this job was acted upon (an attempt to execute, successful or otherwise)
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
                        TestEngineJob<NativeBuildTargetTraits>(target, args, {}, Client::TestRunResult::NotRun, "", ""), {});
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
        , m_testJobInfoGenerator(AZStd::make_unique<NativeTestJobInfoGenerator>(
            sourceDir, targetBinaryDir, cacheDir, artifactDir, testRunnerBinary, instrumentBinary))
        , m_testEnumerator(AZStd::make_unique<NativeTestEnumerator>(maxConcurrentRuns))
        , m_instrumentedTestRunner(AZStd::make_unique<NativeInstrumentedTestRunner>(maxConcurrentRuns))
        , m_testRunner(AZStd::make_unique<NativeRegularTestRunner>(maxConcurrentRuns))
        , m_artifactDir(artifactDir)
    {
    }

    TestEngine::~TestEngine() = default;

    void TestEngine::DeleteArtifactXmls() const
    {
        DeleteFiles(m_artifactDir, "*.xml");
    }

    //AZStd::pair<TestSequenceResult, AZStd::vector<TestEngineEnumeration>> TestEngine::UpdateEnumerationCache(
    //    const AZStd::vector<const NativeTestTarget*>& testTargets,
    //    Policy::ExecutionFailure executionFailurePolicy,
    //    Policy::TestFailure testFailurePolicy,
    //    AZStd::optional<AZStd::chrono::milliseconds> testTargetTimeout,
    //    AZStd::optional<AZStd::chrono::milliseconds> globalTimeout,
    //    AZStd::optional<TestEngineJobCompleteCallback> callback) const
    //{
    //    TestEngineJobMap<TestEnumerator::JobInfo::IdType> engineJobs;
    //    const auto jobInfos = m_testJobInfoGenerator->GenerateTestEnumerationJobInfos(testTargets, TestEnumerator::JobInfo::CachePolicy::Write);
    //
    //    auto [result, runnerJobs] = m_testEnumerator->Enumerate(
    //        jobInfos,
    //        testTargetTimeout,
    //        globalTimeout,
    //        TestJobRunnerCallbackHandler<TestEnumerator>(testTargets, &engineJobs, executionFailurePolicy, testFailurePolicy, &callback));
    //
    //    auto engineRuns = CompileTestEngineRuns<TestEnumerator>(testTargets, runnerJobs, AZStd::move(engineJobs));
    //    return { CalculateSequenceResult(result, engineRuns, executionFailurePolicy), AZStd::move(engineRuns) };
    //}

    //!
    template<typename TestRunner, typename TestTarget>
     AZStd::pair<ProcessSchedulerResult, AZStd::vector<TestEngineJobType<TestRunner>>> RunTests(
        TestRunner* testRunner,
        const AZStd::vector<const TestTarget*>& testTargets,
        const AZStd::vector<typename TestRunner::JobInfo>& jobInfos ,
        Policy::ExecutionFailure executionFailurePolicy,
        Policy::TestFailure testFailurePolicy,
        Policy::TargetOutputCapture targetOutputCapture,
        AZStd::optional<AZStd::chrono::milliseconds> testTargetTimeout,
        AZStd::optional<AZStd::chrono::milliseconds> globalTimeout,
        AZStd::optional<TestEngineJobCompleteCallback> callback)
    {
        TestEngineJobMap<TestRunner::JobInfo::IdType> engineJobs;
        auto [result, runnerJobs] = testRunner->RunTests(
            jobInfos,
            targetOutputCapture == Policy::TargetOutputCapture::None ? StdOutputRouting::None : StdOutputRouting::ToParent,
            targetOutputCapture == Policy::TargetOutputCapture::None ? StdErrorRouting::None : StdErrorRouting::ToParent,
            testTargetTimeout,
            globalTimeout,
            TestJobRunnerCallbackHandlerType<TestRunner>(
                testTargets,
                &engineJobs,
                executionFailurePolicy,
                testFailurePolicy,
                &callback),
            AZStd::nullopt);
        
        return { result, CompileTestEngineRuns<TestRunner>(testTargets, runnerJobs, AZStd::move(engineJobs)) };
    }

    AZStd::pair<TestSequenceResult, AZStd::vector<TestEngineRegularRun<NativeBuildTargetTraits>>> TestEngine::RegularRun(
        const AZStd::vector<const NativeTestTarget*>& testTargets,
        Policy::ExecutionFailure executionFailurePolicy,
        Policy::TestFailure testFailurePolicy,
        Policy::TargetOutputCapture targetOutputCapture,
        AZStd::optional<AZStd::chrono::milliseconds> testTargetTimeout,
        AZStd::optional<AZStd::chrono::milliseconds> globalTimeout,
        AZStd::optional<TestEngineJobCompleteCallback> callback) const
    {
        DeleteArtifactXmls();

        auto [result, engineRuns] = RunTests(
            m_testRunner.get(),
            testTargets,
            m_testJobInfoGenerator->GenerateRegularTestRunJobInfos(testTargets),
            executionFailurePolicy,
            testFailurePolicy,
            targetOutputCapture,
            testTargetTimeout,
            globalTimeout,
            callback
            );

        return { CalculateSequenceResult(result, engineRuns, executionFailurePolicy), AZStd::move(engineRuns) };
    }

    AZStd::pair<TestSequenceResult, AZStd::vector<TestEngineInstrumentedRun<NativeBuildTargetTraits>>> TestEngine::InstrumentedRun(
        const AZStd::vector<const NativeTestTarget*>& testTargets,
        Policy::ExecutionFailure executionFailurePolicy,
        Policy::IntegrityFailure integrityFailurePolicy,
        Policy::TestFailure testFailurePolicy,
        Policy::TargetOutputCapture targetOutputCapture,
        AZStd::optional<AZStd::chrono::milliseconds> testTargetTimeout,
        AZStd::optional<AZStd::chrono::milliseconds> globalTimeout,
        AZStd::optional<TestEngineJobCompleteCallback> callback) const
    {
        DeleteArtifactXmls();

        auto [result, engineRuns] = RunTests(
            m_instrumentedTestRunner.get(),
            testTargets,
            m_testJobInfoGenerator->GenerateInstrumentedTestRunJobInfos(testTargets, CoverageLevel::Source),
            executionFailurePolicy,
            testFailurePolicy,
            targetOutputCapture,
            testTargetTimeout,
            globalTimeout,
            callback
            );

        // Now that we know the true result of successful jobs that return non-zero we can deduce if we have any integrity failures
        // where a test target ran and completed its tests without incident yet failed to produce coverage data
        if (integrityFailurePolicy == Policy::IntegrityFailure::Abort)
        {
            for (const auto& engineRun : engineRuns)
            {
                if (const auto testResult = engineRun.GetTestResult();
                    testResult == Client::TestRunResult::AllTestsPass || testResult == Client::TestRunResult::TestFailures)
                {
                    AZ_TestImpact_Eval(engineRun.GetTestCoverge().has_value(), TestEngineException, AZStd::string::format(
                        "Test target %s completed its test run but failed to produce coverage data", engineRun.GetTestTarget()->GetName().c_str()));
                }
            }
        }

        return { CalculateSequenceResult(result, engineRuns, executionFailurePolicy), AZStd::move(engineRuns) };
    }
} // namespace TestImpact
