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
#include <TestEngine/Common/TestImpactErrorCodeChecker.h>
#include <TestRunner/Native/TestImpactNativeErrorCodeHandler.h>
#include <TestRunner/Native/TestImpactNativeTestEnumerator.h>
#include <TestRunner/Native/TestImpactNativeInstrumentedTestRunner.h>
#include <TestRunner/Native/TestImpactNativeRegularTestRunner.h>
#include <TestEngine/Native/Job/TestImpactNativeTestJobInfoGenerator.h>

namespace TestImpact
{
    //!
    AZStd::optional<Client::TestRunResult> StandAloneErrorHandler(ReturnCode returnCode)
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

    //!
    class RegularTestJobRunnerCallbackHandler
        : public TestJobRunnerCallbackHandler<NativeRegularTestRunner, NativeTestTarget>
    {
    public:
        RegularTestJobRunnerCallbackHandler(
            const AZStd::vector<const NativeTestTarget*>& testTargets,
            TestEngineJobMap<NativeRegularTestRunner::JobInfo::IdType, NativeTestTarget>* engineJobs,
            Policy::ExecutionFailure executionFailurePolicy,
            Policy::TestFailure testFailurePolicy,
            AZStd::optional<TestEngineJobCompleteCallback<NativeTestTarget>>* callback)
            : TestJobRunnerCallbackHandler(
                    testTargets,
                    engineJobs,
                    executionFailurePolicy,
                    testFailurePolicy,
                    AZStd::vector<ErrorCodeHandler>{ GetNativeTestLibraryErrorCodeHandler(),
                                                    [](ReturnCode returnCode)
                                                    {
                                                       return StandAloneErrorHandler(returnCode);
                                                    } },
                    AZStd::vector<ErrorCodeHandler>{ GetNativeTestRunnerErrorCodeHandler(), GetNativeTestLibraryErrorCodeHandler() },
                    callback)
        {
        }
    };

    //!
    class InstrumentedRegularTestJobRunnerCallbackHandler
        : public TestJobRunnerCallbackHandler<NativeInstrumentedTestRunner, NativeTestTarget>
    {
    public:
        InstrumentedRegularTestJobRunnerCallbackHandler(
            const AZStd::vector<const NativeTestTarget*>& testTargets,
            TestEngineJobMap<NativeInstrumentedTestRunner::JobInfo::IdType, NativeTestTarget>* engineJobs,
            Policy::ExecutionFailure executionFailurePolicy,
            Policy::TestFailure testFailurePolicy,
            AZStd::optional<TestEngineJobCompleteCallback<NativeTestTarget>>* callback)
            : TestJobRunnerCallbackHandler(
                    testTargets,
                    engineJobs,
                    executionFailurePolicy,
                    testFailurePolicy,
                    AZStd::vector<ErrorCodeHandler>{ GetNativeInstrumentationErrorCodeHandler(), GetNativeTestLibraryErrorCodeHandler(),
                                                    [](ReturnCode returnCode)
                                                    {
                                                       return StandAloneErrorHandler(returnCode);
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
        using TestEngineJobType = TestEngineEnumeration<NativeTestTarget>;
        using TestJobRunnerCallbackHandlerType = RegularTestJobRunnerCallbackHandler; // INCORRECT!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    };

    // Type trait for the test runner
    template<>
    struct TestJobRunnerTrait<NativeRegularTestRunner>
    {
        using TestEngineJobType = TestEngineRegularRun<NativeTestTarget>;
        using TestJobRunnerCallbackHandlerType = RegularTestJobRunnerCallbackHandler;
    };

    // Type trait for the instrumented test runner
    template<>
    struct TestJobRunnerTrait<NativeInstrumentedTestRunner>
    {
        using TestEngineJobType = TestEngineInstrumentedRun<NativeTestTarget>;
        using TestJobRunnerCallbackHandlerType = InstrumentedRegularTestJobRunnerCallbackHandler;
    };

    TestEngine::TestEngine(
        const RepoPath& sourceDir,
        const RepoPath& targetBinaryDir,
        [[maybe_unused]]const RepoPath& cacheDir,
        const RepoPath& artifactDir,
        const RepoPath& testRunnerBinary,
        const RepoPath& instrumentBinary,
        size_t maxConcurrentRuns)
        : m_maxConcurrentRuns(maxConcurrentRuns)
        , m_regularTestJobInfoGenerator(AZStd::make_unique<NativeRegularTestRunJobInfoGenerator>(
            sourceDir,
            targetBinaryDir,
            artifactDir,
            testRunnerBinary))
        , m_instrumentedTestJobInfoGenerator(AZStd::make_unique<NativeInstrumentedTestRunJobInfoGenerator>(
            sourceDir,
            targetBinaryDir,
            artifactDir,
            testRunnerBinary,
            instrumentBinary))
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

    AZStd::pair<TestSequenceResult, AZStd::vector<TestEngineRegularRun<NativeTestTarget>>> TestEngine::RegularRun(
        const AZStd::vector<const NativeTestTarget*>& testTargets,
        Policy::ExecutionFailure executionFailurePolicy,
        Policy::TestFailure testFailurePolicy,
        Policy::TargetOutputCapture targetOutputCapture,
        AZStd::optional<AZStd::chrono::milliseconds> testTargetTimeout,
        AZStd::optional<AZStd::chrono::milliseconds> globalTimeout,
        AZStd::optional<TestEngineJobCompleteCallback<NativeTestTarget>> callback) const
    {
        DeleteArtifactXmls();

        return RunTests(
            m_testRunner.get(),
            m_regularTestJobInfoGenerator.get(),
            testTargets,
            executionFailurePolicy,
            testFailurePolicy,
            targetOutputCapture,
            testTargetTimeout,
            globalTimeout,
            callback
            );
    }

    AZStd::pair<TestSequenceResult, AZStd::vector<TestEngineInstrumentedRun<NativeTestTarget>>> TestEngine::InstrumentedRun(
        const AZStd::vector<const NativeTestTarget*>& testTargets,
        Policy::ExecutionFailure executionFailurePolicy,
        Policy::IntegrityFailure integrityFailurePolicy,
        Policy::TestFailure testFailurePolicy,
        Policy::TargetOutputCapture targetOutputCapture,
        AZStd::optional<AZStd::chrono::milliseconds> testTargetTimeout,
        AZStd::optional<AZStd::chrono::milliseconds> globalTimeout,
        AZStd::optional<TestEngineJobCompleteCallback<NativeTestTarget>> callback) const
    {
        DeleteArtifactXmls();

        auto [result, engineRuns] = RunTests(
            m_instrumentedTestRunner.get(),
            m_instrumentedTestJobInfoGenerator.get(),
            testTargets,
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

        return { result, engineRuns };
    }
} // namespace TestImpact
