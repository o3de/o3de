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
#include <TestRunner/Python/TestImpactPythonErrorCodeChecker.h>
#include <TestEngine/Python/TestImpactPythonTestEngine.h>
#include <TestRunner/Python/Job/TestImpactPythonTestJobInfoGenerator.h>
#include <TestRunner/Python/TestImpactPythonTestRunner.h>
#include <TestRunner/Python/TestImpactPythonNullTestRunner.h>

#include <iostream>
namespace TestImpact
{
    AZStd::optional<Client::TestRunResult> PythonInstrumentedTestRunnerErrorCodeChecker(
        [[maybe_unused]] const typename PythonTestRunner::JobInfo& jobInfo, const JobMeta& meta)
    {
        // The PyTest error code for test failures overlaps with the Python error code for script error so we have no way of
        // discerning at the job meta level whether a test failure or script execution error we will assume the tests failed for now
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

    template<>
    struct TestJobRunnerTrait<PythonTestRunner>
    {
        using TestEngineJobType = TestEngineInstrumentedRun<PythonTestTarget, TestCoverage>;
    };

    template<>
    struct TestJobRunnerTrait<PythonNullTestRunner>
    {
        using TestEngineJobType = TestEngineInstrumentedRun<PythonTestTarget, TestCoverage>;
    };

    PythonTestEngine::PythonTestEngine(
        const RepoPath& repoDir,
        const RepoPath& buildDir,
        const ArtifactDir& artifactDir,
        Policy::TestRunner testRunnerPolicy)
        : m_testJobInfoGenerator(AZStd::make_unique<PythonTestRunJobInfoGenerator>(
              repoDir, buildDir, artifactDir))
        , m_testRunner(AZStd::make_unique<PythonTestRunner>(artifactDir))
        , m_nullTestRunner(AZStd::make_unique<PythonNullTestRunner>(artifactDir))
        , m_artifactDir(artifactDir)
        , m_testRunnerPolicy(testRunnerPolicy)
    {
    }

    PythonTestEngine::~PythonTestEngine() = default;

    void PythonTestEngine::DeleteArtifactXmls() const
    {
        DeleteFiles(m_artifactDir.m_testRunArtifactDirectory, "*.xml");
        DeleteFiles(m_artifactDir.m_coverageArtifactDirectory, "*.pycoverage");
    }

    TestEngineInstrumentedRunResult<PythonTestTarget, TestCoverage>
        PythonTestEngine::
        InstrumentedRun(
        const AZStd::vector<const PythonTestTarget*>& testTargets,
        Policy::ExecutionFailure executionFailurePolicy,
        Policy::IntegrityFailure integrityFailurePolicy,
        Policy::TestFailure testFailurePolicy,
        Policy::TargetOutputCapture targetOutputCapture,
        AZStd::optional<AZStd::chrono::milliseconds> testTargetTimeout,
        AZStd::optional<AZStd::chrono::milliseconds> globalTimeout,
        AZStd::optional<TestEngineJobCompleteCallback<PythonTestTarget>> callback) const
    {
        if (m_testRunnerPolicy == Policy::TestRunner::UseNullTestRunner)
        {
            // We don't delete the artifacts as they have been left by another test runner (e.g. ctest)
            return GenerateInstrumentedRunResult(
            GenerateJobInfosAndRunTests(
                m_nullTestRunner.get(),
                m_testJobInfoGenerator.get(),
                testTargets,
                PythonInstrumentedTestRunnerErrorCodeChecker,
                executionFailurePolicy,
                testFailurePolicy,
                targetOutputCapture,
                testTargetTimeout,
                globalTimeout,
                callback,
                std::nullopt), // For real-time stdout/err output of test targets
            integrityFailurePolicy);
        }
        else
        {;
            DeleteArtifactXmls();
            return GenerateInstrumentedRunResult(
                GenerateJobInfosAndRunTests(
                    m_testRunner.get(),
                    m_testJobInfoGenerator.get(),
                    testTargets,
                    PythonInstrumentedTestRunnerErrorCodeChecker,
                    executionFailurePolicy,
                    testFailurePolicy,
                    targetOutputCapture,
                    testTargetTimeout,
                    globalTimeout,
                    callback,
                    std::nullopt), // For real-time stdout/err output of test targets
                integrityFailurePolicy);
        }
    }
} // namespace TestImpact
