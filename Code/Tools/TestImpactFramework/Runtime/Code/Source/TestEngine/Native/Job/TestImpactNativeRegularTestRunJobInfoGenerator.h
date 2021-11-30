/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Target/Native/TestImpactNativeTestTarget.h>
#include <TestEngine/Common/Job/TestImpactTestJobInfoGenerator.h>
#include <TestRunner/Native/TestImpactNativeRegularTestRunner.h>

namespace TestImpact
{
    //! Generates job information for the different test job runner types.
    class NativeRegularTestRunJobInfoGenerator
        : public TestJobInfoGenerator<NativeRegularTestRunner, NativeTestTarget>
    {
    public:
        //! Configures the test job info generator with the necessary path information for launching test targets.
        //! @param sourceDir Root path where source files are found (including subfolders).
        //! @param targetBinaryDir Path to where the test target binaries are found.
        //! @param artifactDir Path to the transient directory where test artifacts are produced.
        //! @param testRunnerBinary Path to the binary responsible for launching test targets that have the TestRunner launch method.
        NativeRegularTestRunJobInfoGenerator(
            const RepoPath& sourceDir,
            const RepoPath& targetBinaryDir,
            const RepoPath& artifactDir,
            const RepoPath& testRunnerBinary);

        //! Generates the information for a test run job.
        //! @param testTarget The test target to generate the job information for.
        //! @param jobId The id to assign for this job.
        NativeRegularTestRunner::JobInfo GenerateJobInfo(
            const NativeTestTarget* testTarget, NativeRegularTestRunner::JobInfo::Id jobId) const;

    private:
        RepoPath m_sourceDir;
        RepoPath m_targetBinaryDir;
        RepoPath m_artifactDir;
        RepoPath m_testRunnerBinary;
    };
}
