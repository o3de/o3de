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
#include <TestEngine/Native/TestImpactNativeTestEngine.h>
#include <TestRunner/Common/TestImpactErrorCodeChecker.h>
#include <TestRunner/Native/TestImpactNativeErrorCodeHandler.h>
#include <TestRunner/Native/Enumeration/TestImpactNativeTestEnumerator.h>
#include <TestRunner/Native/Run/TestImpactNativeInstrumentedTestRunner.h>
#include <TestRunner/Native/Run/TestImpactNativeRegularTestRunner.h>
#include <TestRunner/Native/Job/TestImpactNativeTestJobInfoGenerator.h>

namespace TestImpact
{
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

    class RegularTestJobRunnerCallbackHandler
        : public TestJobRunnerCallbackHandler<NativeRegularTestRunner, typename NativeBuildTargetTraits::TestTarget>
    {
        using ParentHandler = TestJobRunnerCallbackHandler<NativeRegularTestRunner, typename NativeBuildTargetTraits::TestTarget>;

    public:
        RegularTestJobRunnerCallbackHandler(
            const AZStd::vector<const NativeTestTarget*>& testTargets,
            TestEngineJobMap<NativeRegularTestRunner::JobInfo::IdType, typename NativeBuildTargetTraits::TestTarget>* engineJobs,
            Policy::ExecutionFailure executionFailurePolicy,
            Policy::TestFailure testFailurePolicy,
            AZStd::optional<TestEngineJobCompleteCallback<typename NativeBuildTargetTraits::TestTarget>>* callback)
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
        : public TestJobRunnerCallbackHandler<NativeInstrumentedTestRunner, typename NativeBuildTargetTraits::TestTarget>
    {
        using ParentHandler = TestJobRunnerCallbackHandler<NativeInstrumentedTestRunner, typename NativeBuildTargetTraits::TestTarget>;

    public:
        InstrumentedRegularTestJobRunnerCallbackHandler(
            const AZStd::vector<const NativeTestTarget*>& testTargets,
            TestEngineJobMap<NativeInstrumentedTestRunner::JobInfo::IdType, typename NativeBuildTargetTraits::TestTarget>* engineJobs,
            Policy::ExecutionFailure executionFailurePolicy,
            Policy::TestFailure testFailurePolicy,
            AZStd::optional<TestEngineJobCompleteCallback<typename NativeBuildTargetTraits::TestTarget>>* callback)
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

    // Type trait for the test enumerator
    template<>
    struct TestJobRunnerTrait<NativeTestEnumerator>
    {
        using TestEngineJobType = TestEngineEnumeration<typename NativeBuildTargetTraits::TestTarget>;
        using TestJobRunnerCallbackHandlerType = RegularTestJobRunnerCallbackHandler; // INCORRECT!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    };

    // Type trait for the test runner
    template<>
    struct TestJobRunnerTrait<NativeRegularTestRunner>
    {
        using TestEngineJobType = TestEngineRegularRun<typename NativeBuildTargetTraits::TestTarget>;
        using TestJobRunnerCallbackHandlerType = RegularTestJobRunnerCallbackHandler;
    };

    // Type trait for the instrumented test runner
    template<>
    struct TestJobRunnerTrait<NativeInstrumentedTestRunner>
    {
        using TestEngineJobType = TestEngineInstrumentedRun<typename NativeBuildTargetTraits::TestTarget>;
        using TestJobRunnerCallbackHandlerType = InstrumentedRegularTestJobRunnerCallbackHandler;
    };
                
    // Helper function to compile the run type specific test engine jobs from their associated jobs and payloads
    template<typename TestJobRunner>
    AZStd::vector<TestEngineJobType<TestJobRunner>> CompileTestEngineRuns(
        const AZStd::vector<const NativeTestTarget*>& testTargets,
        AZStd::vector<typename TestJobRunner::Job>& runnerjobs,
        TestEngineJobMap<typename TestJobRunner::JobInfo::IdType, typename NativeBuildTargetTraits::TestTarget>&& engineJobs)
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
                    TestEngineJob<typename NativeBuildTargetTraits::TestTarget>(target, args, {}, Client::TestRunResult::NotRun, "", ""),
                    {});
                engineRuns.push_back(AZStd::move(run));
            }
        }

        return engineRuns;
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

    AZStd::pair<TestSequenceResult, AZStd::vector<TestEngineRegularRun<typename NativeBuildTargetTraits::TestTarget>>> TestEngine::RegularRun(
        const AZStd::vector<const NativeTestTarget*>& testTargets,
        Policy::ExecutionFailure executionFailurePolicy,
        Policy::TestFailure testFailurePolicy,
        Policy::TargetOutputCapture targetOutputCapture,
        AZStd::optional<AZStd::chrono::milliseconds> testTargetTimeout,
        AZStd::optional<AZStd::chrono::milliseconds> globalTimeout,
        AZStd::optional<TestEngineJobCompleteCallback<typename NativeBuildTargetTraits::TestTarget>> callback) const
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

    AZStd::pair<TestSequenceResult, AZStd::vector<TestEngineInstrumentedRun<typename NativeBuildTargetTraits::TestTarget>>> TestEngine::InstrumentedRun(
        const AZStd::vector<const NativeTestTarget*>& testTargets,
        Policy::ExecutionFailure executionFailurePolicy,
        Policy::IntegrityFailure integrityFailurePolicy,
        Policy::TestFailure testFailurePolicy,
        Policy::TargetOutputCapture targetOutputCapture,
        AZStd::optional<AZStd::chrono::milliseconds> testTargetTimeout,
        AZStd::optional<AZStd::chrono::milliseconds> globalTimeout,
        AZStd::optional<TestEngineJobCompleteCallback<typename NativeBuildTargetTraits::TestTarget>> callback) const
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
