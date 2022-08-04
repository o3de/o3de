/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TestEngine/Common/Job/TestImpactTestJobInfoUtils.h>
#include <TestEngine/Python/Job/TestImpactPythonTestJobInfoGenerator.h>
#include <TestEngine/Python/Job/TestImpactPythonTestJobInfoUtils.h>

#include <AzCore/StringFunc/StringFunc.h>

namespace TestImpact
{
    PythonTestRunJobInfoGenerator::PythonTestRunJobInfoGenerator(
        const RepoPath& repoDir, const RepoPath& pythonBinary, const RepoPath& buildDir, const ArtifactDir& artifactDir)
        : m_repoDir(repoDir)
        , m_pythonBinary(pythonBinary)
        , m_buildDir(buildDir)
        , m_artifactDir(artifactDir)
    {
    }

    AZStd::string CompileParentFolderName(const PythonTestTarget* testTarget)
    {
        // Compile a unique folder name based on the aprent script path
        auto parentfolder = testTarget->GetScriptPath().String();
        AZ::StringFunc::Replace(parentfolder, '/', '_');
        AZ::StringFunc::Replace(parentfolder, '\\', '_');
        AZ::StringFunc::Replace(parentfolder, '.', '_');
        return parentfolder;
    }

    PythonTestRunner::JobInfo PythonTestRunJobInfoGenerator::GenerateJobInfo(
        const PythonTestTarget* testTarget, PythonTestRunner::JobInfo::Id jobId) const
    {
        const auto parentFolderName = RepoPath(CompileParentFolderName(testTarget));
        const auto runArtifact = GenerateTargetRunArtifactFilePath(testTarget, m_artifactDir.m_testRunArtifactDirectory);
        //const Command args = {
        //    AZStd::string::format(
        //        "\"%s\" " // 1. Python binary
        //        "-m pytest -s " 
        //        "\"%s\" " // 2. Python test script
        //        "--build-directory \"%s\" " // 3. Build directory path
        //        "--junitxml=\"%s\"", // 4. JUnit output file
        //
        //        m_pythonBinary.c_str(), // // 1. Python binary
        //        GenerateTestScriptPath(testTarget, m_repoDir).c_str(), // 2. Python test script
        //        m_buildDir.c_str(), // 3. Build directory path
        //        runArtifact.c_str()) // 4. JUnit output 
        //};

        const Command args = { testTarget->GetCommand() };

        return JobInfo(jobId, args, JobData(runArtifact, m_artifactDir.m_coverageArtifactDirectory / parentFolderName));
    }
} // namespace TestImpact
