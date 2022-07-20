/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <TestImpactFramework/TestImpactClientTestRun.h>
#include <TestImpactFramework/TestImpactRuntime.h>
#include <TestImpactFramework/TestImpactTestSequence.h>

#include <TestEngine/Common/Enumeration/TestImpactTestEngineEnumeration.h>
#include <TestEngine/Common/Run/TestImpactTestEngineInstrumentedRun.h>
#include <TestEngine/Common/TestImpactTestEngine.h>

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

namespace TestImpact
{
    class PythonTestTarget;
    class PythonTestRunJobInfoGenerator;
    class PythonTestEnumerator;
    class PythonTestRunner;

    //! Provides the front end for performing test enumerations and test runs.
    class PythonTestEngine
    {
    public:

        //!
        PythonTestEngine(RepoPath repoDir, RepoPath pythonBinary, RepoPath buildDir, RepoPath artifactDir);

        ~PythonTestEngine();

        //!
        [[nodiscard]] AZStd::pair<TestSequenceResult, AZStd::vector<TestEngineInstrumentedRun<PythonTestTarget, TestCaseCoverage>>> InstrumentedRun(
            const AZStd::vector<const PythonTestTarget*>& testTargets,
            Policy::ExecutionFailure executionFailurePolicy,
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
        RepoPath m_artifactDir;
    };
} // namespace TestImpact
