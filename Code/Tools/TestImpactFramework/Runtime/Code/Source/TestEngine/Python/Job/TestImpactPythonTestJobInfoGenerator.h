/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Target/Python/TestImpactPythonTestTarget.h>
#include <TestEngine/Common/Job/TestImpactTestJobInfoGenerator.h>
#include <TestRunner/Python/TestImpactPythonTestRunner.h>

namespace TestImpact
{
    //! Generates job information for the different test job runner types.
    class PythonTestRunJobInfoGenerator
        : public TestJobInfoGenerator<PythonTestRunner, PythonTestTarget>
    {
    public:
        //! Configures the test job info generator with the necessary path information for launching test targets.
        //! @param pythonBinary Path to the Python binray.
        //! @param buildDir Path to where the target binaries are found.
        //! @param artifactDir Path to the transient directory where test artifacts are produced.
        PythonTestRunJobInfoGenerator(
            RepoPath repoDir, RepoPath pythonBinary, RepoPath buildDir, RepoPath artifactDir);

        //! Generates the information for a test run job.
        //! @param testTarget The test target to generate the job information for.
        //! @param jobId The id to assign for this job.
        PythonTestRunner::JobInfo GenerateJobInfo(const PythonTestTarget* testTarget, PythonTestRunner::JobInfo::Id jobId) const;

    private:
        RepoPath m_repoDir;
        RepoPath m_pythonBinary;
        RepoPath m_buildDir;
        RepoPath m_artifactDir;
    };
} // namespace TestImpact
