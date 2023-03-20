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
    NativeTestEnumerationJobInfoGenerator::NativeTestEnumerationJobInfoGenerator(
        const RepoPath& targetBinaryDir,
        const RepoPath& cacheDir,
        const ArtifactDir& artifactDir,
        const RepoPath& testRunnerBinary)
        : m_targetBinaryDir(targetBinaryDir)
        , m_cacheDir(cacheDir)
        , m_artifactDir(artifactDir)
        , m_testRunnerBinary(testRunnerBinary)
    {
    }


    NativeTestEnumerator::JobInfo NativeTestEnumerationJobInfoGenerator::GenerateJobInfo(
        const NativeTestTarget* testTarget, NativeTestEnumerator::JobInfo::Id jobId) const
    {
        using Cache = NativeTestEnumerator::JobData::Cache;

        const auto enumerationArtifact = GenerateTargetEnumerationArtifactFilePath(testTarget, m_artifactDir.m_enumerationCacheDirectory);
        const Command args = { AZStd::string::format(
            "%s --gtest_list_tests --gtest_output=xml:\"%s\"",
            GenerateLaunchArgument(testTarget, m_targetBinaryDir, m_testRunnerBinary).c_str(), enumerationArtifact.c_str()) };

        return JobInfo(
            jobId, args,
            JobData(enumerationArtifact, Cache{ m_cachePolicy, GenerateTargetEnumerationCacheFilePath(testTarget, m_cacheDir) }));
    }

    void NativeTestEnumerationJobInfoGenerator::SetCachePolicy(NativeTestEnumerator::JobInfo::CachePolicy cachePolicy)
    {
        m_cachePolicy = cachePolicy;
    }

    NativeTestEnumerator::JobInfo::CachePolicy NativeTestEnumerationJobInfoGenerator::GetCachePolicy() const
    {
        return m_cachePolicy;
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
        const auto runArtifact = GenerateTargetRunArtifactFilePath(testTarget, m_artifactDir.m_testRunArtifactDirectory);
        const Command args = { AZStd::string::format(
            "%s --gtest_output=xml:\"%s\"", GenerateLaunchArgument(testTarget, m_targetBinaryDir, m_testRunnerBinary).c_str(),
            runArtifact.c_str()) };

        return JobInfo(jobId, args, JobData(testTarget->GetLaunchMethod(), runArtifact));
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
        const auto coverageArtifact = GenerateTargetCoverageArtifactFilePath(testTarget, m_artifactDir.m_coverageArtifactDirectory);
        const auto runArtifact = GenerateTargetRunArtifactFilePath(testTarget, m_artifactDir.m_testRunArtifactDirectory);
        const Command args = {
            AZStd::string::format(
                "\"%s\" " // 1. Instrumented test runner
                "--coverage_level %s " // 2. Coverage level
                "--export_type cobertura:\"%s\" " // 3. Test coverage artifact path
                "--modules \"%s\" " // 4. Modules path
                "--excluded_modules \"%s\" " // 5. Exclude modules
                "--sources \"%s\" -- " // 6. Sources path
                "%s " // 7. Launch command
                "--gtest_output=xml:\"%s\"", // 8. Result artifact

                m_instrumentBinary.c_str(), // 1. Instrumented test runner
                (m_coverageLevel == CoverageLevel::Line ? "line" : "source"), // 2. Coverage level
                coverageArtifact.c_str(), // 3. Test coverage artifact path
                m_targetBinaryDir.c_str(), // 4. Modules path
                m_testRunnerBinary.c_str(), // 5. Exclude modules
                m_sourceDir.c_str(), // 6. Sources path
                GenerateLaunchArgument(testTarget, m_targetBinaryDir, m_testRunnerBinary).c_str(), // 7. Launch command
                runArtifact.c_str()) // 8. Result artifact
        };

        return JobInfo(jobId, args, JobData(testTarget->GetLaunchMethod(), runArtifact, coverageArtifact));
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
