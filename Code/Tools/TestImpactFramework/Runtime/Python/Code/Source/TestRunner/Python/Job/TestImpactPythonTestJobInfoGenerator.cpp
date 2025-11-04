/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TestRunner/Common/Job/TestImpactTestJobInfoUtils.h>
#include <TestRunner/Python/Job/TestImpactPythonTestJobInfoGenerator.h>

#include <AzCore/StringFunc/StringFunc.h>

namespace TestImpact
{
    PythonInstrumentedTestRunJobInfoGenerator::PythonInstrumentedTestRunJobInfoGenerator(
        const RepoPath& repoDir, const RepoPath& buildDir, const ArtifactDir& artifactDir)
        : m_repoDir(repoDir)
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

    PythonInstrumentedTestRunnerBase::JobInfo PythonInstrumentedTestRunJobInfoGenerator::GenerateJobInfo(
        const PythonTestTarget* testTarget, PythonInstrumentedTestRunnerBase::JobInfo::Id jobId) const
    {
        const auto parentFolderName = RepoPath(CompileParentFolderName(testTarget));
        const auto runArtifact = GenerateTargetRunArtifactFilePath(testTarget, m_artifactDir.m_testRunArtifactDirectory);
        const Command args = { testTarget->GetCommand() };

        return JobInfo(jobId, args, JobData(runArtifact, m_artifactDir.m_coverageArtifactDirectory / parentFolderName));
    }

    PythonRegularTestRunJobInfoGenerator::PythonRegularTestRunJobInfoGenerator(
        const RepoPath& repoDir, const RepoPath& buildDir, const ArtifactDir& artifactDir)
        : m_repoDir(repoDir)
        , m_buildDir(buildDir)
        , m_artifactDir(artifactDir)
    {
    }

    PythonRegularTestRunnerBase::JobInfo PythonRegularTestRunJobInfoGenerator::GenerateJobInfo(
        const PythonTestTarget* testTarget, PythonRegularTestRunnerBase::JobInfo::Id jobId) const
    {
        const auto parentFolderName = RepoPath(CompileParentFolderName(testTarget));
        const auto runArtifact = GenerateTargetRunArtifactFilePath(testTarget, m_artifactDir.m_testRunArtifactDirectory);
        const Command args = { testTarget->GetCommand() };

        return JobInfo(jobId, args, JobData(runArtifact));
    }
} // namespace TestImpact
