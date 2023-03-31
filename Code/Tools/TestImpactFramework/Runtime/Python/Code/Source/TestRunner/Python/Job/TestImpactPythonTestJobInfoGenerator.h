/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <TestImpactFramework/TestImpactConfiguration.h>

#include <Target/Python/TestImpactPythonTestTarget.h>
#include <TestRunner/Common/Job/TestImpactTestJobInfoGenerator.h>
#include <TestRunner/Python/TestImpactPythonInstrumentedTestRunner.h>
#include <TestRunner/Python/TestImpactPythonRegularTestRunner.h>

namespace TestImpact
{
    //! Generates job information for the instrumented test job runner.
    class PythonInstrumentedTestRunJobInfoGenerator
        : public TestJobInfoGeneratorBase<PythonInstrumentedTestRunnerBase, PythonTestTarget>
    {
    public:
        //! Configures the test job info generator with the necessary path information for launching test targets.
        //! @param repoDir Root path to where the repository is located.
        //! @param buildDir Path to where the target binaries are found.
        //! @param artifactDir Path to the transient directory where test artifacts are produced.
        PythonInstrumentedTestRunJobInfoGenerator(
            const RepoPath& repoDir, const RepoPath& buildDir, const ArtifactDir& artifactDir);

        // TestJobInfoGeneratorBase overrides...
        PythonInstrumentedTestRunnerBase::JobInfo GenerateJobInfo(const PythonTestTarget* testTarget, PythonInstrumentedTestRunnerBase::JobInfo::Id jobId) const;

    private:
        RepoPath m_repoDir;
        RepoPath m_buildDir;
        ArtifactDir m_artifactDir;
    };

    //! Generates job information for the regular test job runner.
    class PythonRegularTestRunJobInfoGenerator
        : public TestJobInfoGeneratorBase<PythonRegularTestRunnerBase, PythonTestTarget>
    {
    public:
        //! Configures the test job info generator with the necessary path information for launching test targets.
        //! @param repoDir Root path to where the repository is located.
        //! @param buildDir Path to where the target binaries are found.
        //! @param artifactDir Path to the transient directory where test artifacts are produced.
        PythonRegularTestRunJobInfoGenerator(
            const RepoPath& repoDir, const RepoPath& buildDir, const ArtifactDir& artifactDir);

        // TestJobInfoGeneratorBase overrides...
        PythonRegularTestRunnerBase::JobInfo GenerateJobInfo(const PythonTestTarget* testTarget, PythonRegularTestRunnerBase::JobInfo::Id jobId) const;

    private:
        RepoPath m_repoDir;
        RepoPath m_buildDir;
        ArtifactDir m_artifactDir;
    };
} // namespace TestImpact
