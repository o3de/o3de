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

namespace TestImpact
{
    AZStd::optional<Client::TestRunResult> PythonRegularTestRunnerErrorCodeChecker(
        [[maybe_unused]] const typename PythonRegularTestRunner::JobInfo& jobInfo, const JobMeta& meta)
    {
        return CheckPythonErrorCode(meta.m_returnCode.value());
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

    // Type trait for the instrumented test runner
    template<>
    struct TestJobRunnerTrait<PythonInstrumentedTestRunner>
    {
        using TestEngineJobType = TestEngineInstrumentedRun<PythonTestTarget, TestCoverage>;
    };

    // Type trait for the instrumented null test runner
    template<>
    struct TestJobRunnerTrait<PythonInstrumentedNullTestRunner>
    {
        using TestEngineJobType = TestEngineInstrumentedRun<PythonTestTarget, TestCoverage>;
    };

    // Type trait for the regular test runner
    template<>
    struct TestJobRunnerTrait<PythonRegularTestRunner>
    {
        using TestEngineJobType = TestEngineRegularRun<PythonTestTarget>;
    };

    // Type trait for the regular null test runner
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

    void PythonTestEngine::DeleteXmlArtifacts() const
    {
        DeleteFiles(m_artifactDir.m_testRunArtifactDirectory, "*.xml");
        DeleteFiles(m_artifactDir.m_coverageArtifactDirectory, "*.pycoverage");
    }

    TestEngineRegularRunResult<PythonTestTarget> PythonTestEngine::RegularRun(
        const AZStd::vector<const PythonTestTarget*>& testTargets,
        Policy::ExecutionFailure executionFailurePolicy,
        Policy::TestFailure testFailurePolicy,
        Policy::TargetOutputCapture targetOutputCapture,
        AZStd::optional<AZStd::chrono::milliseconds> testTargetTimeout,
        AZStd::optional<AZStd::chrono::milliseconds> globalTimeout) const
    {
        DeleteXmlArtifacts();

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
                globalTimeout);
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
                globalTimeout);
        }
    }

    TestEngineInstrumentedRunResult<PythonTestTarget, TestCoverage>
        PythonTestEngine::
        InstrumentedRun(
        const AZStd::vector<const PythonTestTarget*>& testTargets,
        Policy::ExecutionFailure executionFailurePolicy,
        Policy::TestFailure testFailurePolicy,
        Policy::TargetOutputCapture targetOutputCapture,
        AZStd::optional<AZStd::chrono::milliseconds> testTargetTimeout,
        AZStd::optional<AZStd::chrono::milliseconds> globalTimeout) const
    {
        const auto jobInfos = m_instrumentedTestJobInfoGenerator->GenerateJobInfos(testTargets);

        if (m_testRunnerPolicy == Policy::TestRunner::UseNullTestRunner)
        {
            // We don't delete the artifacts as they have been left by another test runner (e.g. ctest)
            return RunTests(
                m_instrumentedNullTestRunner.get(),
                jobInfos,
                testTargets,
                PythonInstrumentedTestRunnerErrorCodeChecker,
                executionFailurePolicy,
                testFailurePolicy,
                targetOutputCapture,
                testTargetTimeout,
                globalTimeout);
        }
        else
        {
            return RunTests(
                    m_instrumentedTestRunner.get(),
                    jobInfos,
                    testTargets,
                    PythonInstrumentedTestRunnerErrorCodeChecker,
                    executionFailurePolicy,
                    testFailurePolicy,
                    targetOutputCapture,
                    testTargetTimeout,
                    globalTimeout);
        }
    }
} // namespace TestImpact
