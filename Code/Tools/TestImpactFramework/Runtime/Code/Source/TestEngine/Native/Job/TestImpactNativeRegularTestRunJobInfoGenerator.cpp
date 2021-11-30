/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TestEngine/Native/Job/TestImpactNativeRegularTestRunJobInfoGenerator.h>
#include <TestEngine/Native/Job/TestImpactNativeTestJobInfoUtils.h>
#include <TestEngine/Native/TestImpactNativeTestTargetExtension.h>

namespace TestImpact
{
    NativeRegularTestRunJobInfoGenerator::NativeRegularTestRunJobInfoGenerator(
        const RepoPath& sourceDir,
        const RepoPath& targetBinaryDir,
        const RepoPath& artifactDir,
        const RepoPath& testRunnerBinary)
        : m_sourceDir(sourceDir)
        , m_targetBinaryDir(targetBinaryDir)
        , m_artifactDir(artifactDir)
        , m_testRunnerBinary(testRunnerBinary)
    {
    }

    NativeRegularTestRunner::JobInfo NativeRegularTestRunJobInfoGenerator::GenerateJobInfo(
        const NativeTestTarget* testTarget, NativeRegularTestRunner::JobInfo::Id jobId) const
    {
        using Command = NativeRegularTestRunner::Command;
        using JobInfo = NativeRegularTestRunner::JobInfo;
        using JobData = NativeRegularTestRunner::JobData;

        const auto runArtifact = GenerateTargetRunArtifactFilePath(testTarget, m_artifactDir);
        const Command args =
        {
            AZStd::string::format(
            "%s --gtest_output=xml:\"%s\"", GenerateLaunchArgument(testTarget, m_targetBinaryDir, m_testRunnerBinary).c_str(),
            runArtifact.c_str())
        };
    
        return JobInfo(jobId, args, JobData(testTarget->GetLaunchMethod(), runArtifact));
    }
}
