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
#include <TestRunner/Python/TestImpactPythonInstrumentedTestRunner.h>
#include <TestRunner/Python/TestImpactPythonInstrumentedNullTestRunner.h>
#include <TestRunner/Python/TestImpactPythonRegularTestRunner.h>
#include <TestRunner/Python/TestImpactPythonRegularNullTestRunner.h>

#include <iostream>
namespace TestImpact
{
    AZStd::optional<Client::TestRunResult> PythonRegularTestRunnerErrorCodeChecker(
        [[maybe_unused]] const typename PythonRegularTestRunner::JobInfo& jobInfo, const JobMeta& meta)
    {
        if (auto result = CheckPythonErrorCode(meta.m_returnCode.value()); result.has_value())
        {
            return result;
        }

        return AZStd::nullopt;
    }

    AZStd::optional<Client::TestRunResult> PythonInstrumentedTestRunnerErrorCodeChecker(
        [[maybe_unused]] const typename PythonInstrumentedTestRunner::JobInfo& jobInfo, const JobMeta& meta)
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
    struct TestJobRunnerTrait<PythonInstrumentedTestRunner>
    {
        using TestEngineJobType = TestEngineInstrumentedRun<PythonTestTarget, TestCoverage>;
    };

    template<>
    struct TestJobRunnerTrait<PythonInstrumentedNullTestRunner>
    {
        using TestEngineJobType = TestEngineInstrumentedRun<PythonTestTarget, TestCoverage>;
    };

    template<>
    struct TestJobRunnerTrait<PythonRegularTestRunner>
    {
        using TestEngineJobType = TestEngineRegularRun<PythonTestTarget>;
    };

    template<>
    struct TestJobRunnerTrait<PythonRegularNullTestRunner>
    {
        using TestEngineJobType = TestEngineRegularRun<PythonTestTarget>;
    };

    PythonTestEngine::PythonTestEngine(
        const RepoPath& repoDir,
        const RepoPath& buildDir,
        const ArtifactDir& artifactDir,
        Policy::TestRunner testRunnerPolicy)
        : m_instrumentedTestJobInfoGenerator(AZStd::make_unique<PythonInstrumentedTestRunJobInfoGenerator>(
              repoDir, buildDir, artifactDir))
        , m_regularTestJobInfoGenerator(AZStd::make_unique<PythonRegularTestRunJobInfoGenerator>(
              repoDir, buildDir, artifactDir))
        , m_instrumentedTestRunner(AZStd::make_unique<PythonInstrumentedTestRunner>())
        , m_instrumentedNullTestRunner(AZStd::make_unique<PythonInstrumentedNullTestRunner>())
        , m_regularTestRunner(AZStd::make_unique<PythonRegularTestRunner>())
        , m_regularNullTestRunner(AZStd::make_unique<PythonRegularNullTestRunner>())
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

    TestEngineRegularRunResult<PythonTestTarget> PythonTestEngine::RegularRun(
        [[maybe_unused]] const AZStd::vector<const PythonTestTarget*>& testTargets,
        [[maybe_unused]] Policy::ExecutionFailure executionFailurePolicy,
        [[maybe_unused]] Policy::TestFailure testFailurePolicy,
        [[maybe_unused]] Policy::TargetOutputCapture targetOutputCapture,
        [[maybe_unused]] AZStd::optional<AZStd::chrono::milliseconds> testTargetTimeout,
        [[maybe_unused]] AZStd::optional<AZStd::chrono::milliseconds> globalTimeout,
        [[maybe_unused]] AZStd::optional<TestEngineJobCompleteCallback<PythonTestTarget>> callback) const
    {
        DeleteArtifactXmls();

        const auto jobInfos = m_regularTestJobInfoGenerator->GenerateJobInfos(testTargets);

        if (m_testRunnerPolicy == Policy::TestRunner::UseNullTestRunner)
        {
            return RunTests(
                m_regularNullTestRunner.get(),
                jobInfos,
                testTargets,
                PythonRegularTestRunnerErrorCodeChecker,
                executionFailurePolicy,
                testFailurePolicy,
                targetOutputCapture,
                testTargetTimeout,
                globalTimeout,
                callback,
                AZStd::nullopt);
        }
        else
        {
            return RunTests(
                m_regularTestRunner.get(),
                jobInfos,
                testTargets,
                PythonRegularTestRunnerErrorCodeChecker,
                executionFailurePolicy,
                testFailurePolicy,
                targetOutputCapture,
                testTargetTimeout,
                globalTimeout,
                callback,
                AZStd::nullopt);
        }
        //
        //return TestEngineRegularRunResult<PythonTestTarget>{};
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
        const auto jobInfos = m_instrumentedTestJobInfoGenerator->GenerateJobInfos(testTargets);

        if (m_testRunnerPolicy == Policy::TestRunner::UseNullTestRunner)
        {
            // We don't delete the artifacts as they have been left by another test runner (e.g. ctest)
            return GenerateInstrumentedRunResult(
            RunTests(
                m_instrumentedNullTestRunner.get(),
                jobInfos,
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
                RunTests(
                    m_instrumentedTestRunner.get(),
                    jobInfos,
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
