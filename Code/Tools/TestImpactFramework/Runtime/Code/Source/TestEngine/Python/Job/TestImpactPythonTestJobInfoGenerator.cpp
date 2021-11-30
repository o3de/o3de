/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TestEngine/Common/Job/TestImpactTestJobInfoUtils.h>
#include <TestEngine/Python/Job/TestImpactPythonTestJobInfoGenerator.h>

namespace TestImpact
{
    PythonTestRunJobInfoGenerator::PythonTestRunJobInfoGenerator(RepoPath pythonBinary, RepoPath buildDir, RepoPath artifactDir)
        : m_pythonBinary(AZStd::move(pythonBinary))
        , m_buildDir(AZStd::move(buildDir))
        , m_artifactDir(AZStd::move(artifactDir))
    {
    }

    PythonTestRunner::JobInfo PythonTestRunJobInfoGenerator::GenerateJobInfo(
        const PythonTestTarget* testTarget, PythonTestRunner::JobInfo::Id jobId) const
    {
        const auto runArtifact = GenerateTargetRunArtifactFilePath(testTarget, m_artifactDir);
        const Command args = {
            AZStd::string::format(
                "\"%s\" " // 1. Python binary
                "-m pytest -s " 
                "\"%s\" " // 2. Python test script
                "--build-directory \"%s\" " // 3. Build directory path
                "--junitxml=\"%s\"", // 4. JUnit output file

                m_pythonBinary.c_str(), // // 1. Python binary
                testTarget->GetScriptPath().c_str(), // 2. Python test script
                m_buildDir.c_str(), // 3. Build directory path
                runArtifact.c_str()) // 4. JUnit output 
        };

        return JobInfo(jobId, args, JobData(runArtifact, m_artifactDir));
    }
} // namespace TestImpact
