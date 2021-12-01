/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TestImpactFramework/TestImpactUtils.h>

#include <Target/Python/TestImpactPythonTestTarget.h>
#include <TestEngine/Common/TestImpactTestEngineException.h>
#include <TestEngine/Python/TestImpactPythonErrorCodeChecker.h>
#include <TestEngine/Python/Job/TestImpactPythonTestJobInfoGenerator.h>
#include <TestEngine/Python/TestImpactPythonTestEngine.h>
#include <TestRunner/Python/TestImpactPythonTestRunner.h>

namespace TestImpact
{
    AZStd::optional<Client::TestRunResult> PythonInstrumentedTestRunnerErrorCodeChecker(
        [[maybe_unused]] const typename PythonTestRunner::JobInfo& jobInfo, const JobMeta& meta)
    {
        // The PyTest error code for test failures overlaps with the Python error codes so we have no way of discerning at the
        // job meta level whether a test failure or script execution error we will assume the tests failed for now
        if (auto result = CheckPyTestErrorCode(meta.m_returnCode.value()); result.has_value())
        {
            return result;
        }

        if (auto result = CheckPythonErrorCode(meta.m_returnCode.value()); result.has_value())
        {
            return result;
        }

        return AZStd::nullopt;
    }

    //!
    class PythonTestJobRunnerCallbackHandler
        : public TestJobRunnerCallbackHandler<PythonTestRunner, PythonTestTarget>
    {
    public:
        PythonTestJobRunnerCallbackHandler(
            const AZStd::vector<const PythonTestTarget*>& testTargets,
            TestEngineJobMap<PythonTestRunner::JobInfo::IdType, PythonTestTarget>* engineJobs,
            Policy::ExecutionFailure executionFailurePolicy,
            Policy::TestFailure testFailurePolicy,
            AZStd::optional<TestEngineJobCompleteCallback<PythonTestTarget>>* callback)
            : TestJobRunnerCallbackHandler(
                  testTargets,
                  engineJobs,
                  executionFailurePolicy,
                  testFailurePolicy,
                  PythonInstrumentedTestRunnerErrorCodeChecker,
                  callback)
        {
        }
    };

    // Type trait for the test runner
    template<>
    struct TestJobRunnerTrait<PythonTestRunner>
    {
        using TestEngineJobType = TestEngineInstrumentedRun<PythonTestTarget, TestCaseCoverage>;
        using TestJobRunnerCallbackHandlerType = PythonTestJobRunnerCallbackHandler;
    };

    PythonTestEngine::PythonTestEngine(RepoPath repoDir, RepoPath pythonBinary, RepoPath buildDir, RepoPath artifactDir)
        : m_testJobInfoGenerator(AZStd::make_unique<PythonTestRunJobInfoGenerator>(
              AZStd::move(repoDir), AZStd::move(pythonBinary), AZStd::move(buildDir), artifactDir))
        , m_testRunner(AZStd::make_unique<PythonTestRunner>())
        , m_artifactDir(artifactDir)
    {
    }

    PythonTestEngine::~PythonTestEngine() = default;

    void PythonTestEngine::DeleteArtifactXmls() const
    {
        DeleteFiles(m_artifactDir, "*.xml");
    }

    AZStd::pair<TestSequenceResult, AZStd::vector<TestEngineInstrumentedRun<PythonTestTarget, TestCaseCoverage>>> PythonTestEngine::
        InstrumentedRun(
        const AZStd::vector<const PythonTestTarget*>& testTargets,
        Policy::ExecutionFailure executionFailurePolicy,
        Policy::TestFailure testFailurePolicy,
        Policy::TargetOutputCapture targetOutputCapture,
        AZStd::optional<AZStd::chrono::milliseconds> testTargetTimeout,
        AZStd::optional<AZStd::chrono::milliseconds> globalTimeout,
        AZStd::optional<TestEngineJobCompleteCallback<PythonTestTarget>> callback) const
    {
        DeleteArtifactXmls();

        return GenerateJobInfosAndRunTests(
            m_testRunner.get(),
            m_testJobInfoGenerator.get(),
            testTargets,
            executionFailurePolicy,
            testFailurePolicy,
            targetOutputCapture,
            testTargetTimeout,
            globalTimeout,
            callback);
    }
} // namespace TestImpact
