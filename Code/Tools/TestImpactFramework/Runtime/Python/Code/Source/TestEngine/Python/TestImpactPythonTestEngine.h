/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <TestImpactFramework/TestImpactConfiguration.h>
#include <TestImpactFramework/TestImpactClientTestRun.h>
#include <TestImpactFramework/TestImpactTestSequence.h>

#include <TestEngine/Common/Enumeration/TestImpactTestEngineEnumeration.h>
#include <TestEngine/Common/Run/TestImpactTestEngineInstrumentedRun.h>
#include <TestEngine/Common/TestImpactTestEngine.h>
#include <TestRunner/Common/Run/TestImpactTestCoverage.h>

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

namespace TestImpact
{
    class PythonTestTarget;
    class PythonTestRunJobInfoGenerator;
    class PythonTestEnumerator;
    class PythonTestRunner;
    class PythonNullTestRunner;

    //! Provides the front end for performing test enumerations and test runs.
    class PythonTestEngine
    {
    public:
        using TestTarget = PythonTestTarget;

        //! Configures the test engine with the necessary path information for launching test targets and managing the artifacts they
        //! produce.
        //! @param repoDir Root path to where the repository is located.
        //! @param buildDir Path to the build folder where the binaries are built.
        //! @param artifactDir Path to the transient directory where test artifacts are produced.
        //! @param useNullTestRunner If true, uses the null test runner, otherwise uses the standard test runner.
        PythonTestEngine(
            const RepoPath& repoDir,
            const RepoPath& buildDir,
            const ArtifactDir& artifactDir,
            Policy::TestRunner testRunnerPolicy);

        ~PythonTestEngine();

        //! Performs a test run with instrumentation and, for each test target, returns the test run results, coverage data and metrics
        //! about the run.
        //! @param testTargets The test targets to run.
        //! @param executionFailurePolicy Policy for how test execution failures should be handled.
        //! @param integrityFailurePolicy Policy for how integrity failures of the test impact data and source tree model should be handled.
        //! @param testFailurePolicy Policy for how test targets with failing tests should be handled.
        //! @param targetOutputCapture Policy for how test target standard output should be captured and handled.
        //! @param testTargetTimeout The maximum duration a test target may be in-flight for before being forcefully terminated (infinite if
        //! empty).
        //! @param globalTimeout The maximum duration the enumeration sequence may run before being forcefully terminated (infinite if
        //! empty).
        //! @param callback The client callback function to handle completed test target runs.
        //! @ returns The sequence result and the test run results and test coverages for the test targets that were run.
        [[nodiscard]] TestEngineInstrumentedRunResult<TestTarget, TestCoverage>
        InstrumentedRun(
            const AZStd::vector<const PythonTestTarget*>& testTargets,
            Policy::ExecutionFailure executionFailurePolicy,
            Policy::IntegrityFailure integrityFailurePolicy,
            Policy::TestFailure testFailurePolicy,
            Policy::TargetOutputCapture targetOutputCapture,
            AZStd::optional<AZStd::chrono::milliseconds> testTargetTimeout,
            AZStd::optional<AZStd::chrono::milliseconds> globalTimeout,
            AZStd::optional<TestEngineJobCompleteCallback<PythonTestTarget>> callback) const;
    private:
        //! Cleans up the artifacts directory of any artifacts from previous runs.
        void DeleteArtifactXmls() const;

        AZStd::unique_ptr<PythonTestRunJobInfoGenerator> m_testJobInfoGenerator;
        AZStd::unique_ptr<PythonTestRunner> m_testRunner;
        AZStd::unique_ptr<PythonNullTestRunner> m_nullTestRunner;
        ArtifactDir m_artifactDir;
        Policy::TestRunner m_testRunnerPolicy;
    };
} // namespace TestImpact
