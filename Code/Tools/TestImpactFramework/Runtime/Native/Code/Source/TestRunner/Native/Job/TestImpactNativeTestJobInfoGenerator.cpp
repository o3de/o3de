/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TestEngine/Common/TestImpactTestEngineException.h>
#include <TestEngine/Native/TestImpactNativeTestTargetExtension.h>
#include <TestRunner/Common/Job/TestImpactTestJobInfoUtils.h>
#include <TestRunner/Native/Job/TestImpactNativeTestJobInfoGenerator.h>
#include <TestRunner/Native/Job/TestImpactNativeTestJobInfoUtils.h>

namespace TestImpact
{
    NativeTestEnumerator::JobInfo NativeTestEnumerationJobInfoGenerator::GenerateJobInfoImpl(
        const NativeTestTarget* testTarget, NativeTestEnumerator::JobInfo::Id jobId) const
    {
        const auto enumerationArtifact = GenerateTargetEnumerationArtifactFilePath(testTarget, m_artifactDir.m_enumerationCacheDirectory);
        const auto launchArgument = GenerateLaunchArgument(testTarget, m_targetBinaryDir, m_testRunnerBinary);
        const auto command = GenerateTestEnumeratorJobInfoCommand(launchArgument, enumerationArtifact);

        return JobInfo(
            jobId,
            command,
            JobData(
                enumerationArtifact,
                Cache{ GetCachePolicy(), GenerateTargetEnumerationCacheFilePath(testTarget, m_artifactDir.m_enumerationCacheDirectory) }));
    }

    NativeRegularTestRunJobInfoGenerator::NativeRegularTestRunJobInfoGenerator(
        const RepoPath& sourceDir, const RepoPath& targetBinaryDir, const ArtifactDir& artifactDir, const RepoPath& testRunnerBinary)
        : m_sourceDir(sourceDir)
        , m_targetBinaryDir(targetBinaryDir)
        , m_artifactDir(artifactDir)
        , m_testRunnerBinary(testRunnerBinary)
    {
    }

    NativeRegularTestRunner::JobInfo NativeRegularTestRunJobInfoGenerator::GenerateJobInfo(
        const NativeTestTarget* testTarget, NativeRegularTestRunner::JobInfo::Id jobId) const
    {
        const auto launchArgument = GenerateLaunchArgument(testTarget, m_targetBinaryDir, m_testRunnerBinary);
        const auto runArtifact = GenerateTargetRunArtifactFilePath(testTarget, m_artifactDir.m_testRunArtifactDirectory);
        const auto command = GenerateRegularTestJobInfoCommand(launchArgument, runArtifact);
        return JobInfo(jobId, command, JobData(testTarget->GetLaunchMethod(), runArtifact));
    }

    NativeInstrumentedTestRunJobInfoGenerator::NativeInstrumentedTestRunJobInfoGenerator(
        const RepoPath& sourceDir,
        const RepoPath& targetBinaryDir,
        const ArtifactDir& artifactDir,
        const RepoPath& testRunnerBinary,
        const RepoPath& instrumentBinary,
        CoverageLevel coverageLevel)
        : m_sourceDir(sourceDir)
        , m_targetBinaryDir(targetBinaryDir)
        , m_artifactDir(artifactDir)
        , m_testRunnerBinary(testRunnerBinary)
        , m_instrumentBinary(instrumentBinary)
        , m_coverageLevel(coverageLevel)
    {
    }

    NativeInstrumentedTestRunner::JobInfo NativeInstrumentedTestRunJobInfoGenerator::GenerateJobInfo(
        const NativeTestTarget* testTarget, NativeInstrumentedTestRunner::JobInfo::Id jobId) const
    {
        const auto launchArgument = GenerateLaunchArgument(testTarget, m_targetBinaryDir, m_testRunnerBinary);
        const auto runArtifact = GenerateTargetRunArtifactFilePath(testTarget, m_artifactDir.m_testRunArtifactDirectory);
        const auto coverageArtifact = GenerateTargetCoverageArtifactFilePath(testTarget, m_artifactDir.m_coverageArtifactDirectory);
        const auto command = GenerateInstrumentedTestJobInfoCommand(
            m_instrumentBinary,
            coverageArtifact,
            m_coverageLevel,
            m_targetBinaryDir,
            m_testRunnerBinary,
            m_sourceDir,
            GenerateRegularTestJobInfoCommand(launchArgument, runArtifact));

        return JobInfo(jobId, command, JobData(testTarget->GetLaunchMethod(), runArtifact, coverageArtifact));
    }

    void NativeInstrumentedTestRunJobInfoGenerator::SetCoverageLevel(CoverageLevel coverageLevel)
    {
        AZ_TestImpact_Eval(coverageLevel == CoverageLevel::Source, TestEngineException, "Only source level coverage is supported in this version.");
        m_coverageLevel = coverageLevel;
    }

    CoverageLevel NativeInstrumentedTestRunJobInfoGenerator::GetCoverageLevel() const
    {
        return m_coverageLevel;
    }
}
