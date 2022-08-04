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
#include <TestRunner/Python/Run/TestImpactPythonTestCoverage.h>

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
        using TestTargetType = PythonTestTarget;
        using TestCaseCoverageType = PythonTestCoverage;
        //!
        PythonTestEngine(
            const RepoPath& repoDir,
            const RepoPath& pythonBinary,
            const RepoPath& buildDir,
            const ArtifactDir& artifactDir,
            bool useNullTestRunner = false);

        ~PythonTestEngine();

        //!
        [[nodiscard]] TestEngineInstrumentedRunResult<TestTargetType, TestCaseCoverageType>
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
        bool m_useNullTestRunner = false;
    };
} // namespace TestImpact
