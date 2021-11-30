/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TestEngine/Native/TestImpactNativeTestTargetExtension.h>
#include <TestEngine/Native/Job/TestImpactNativeTestJobInfoUtils.h>
#include <TestEngine/Native/Job/TestImpactNativeTestEnumerationJobInfoGenerator.h>

namespace TestImpact
{
    NativeTestEnumerationJobInfoGenerator::NativeTestEnumerationJobInfoGenerator(
        const RepoPath& targetBinaryDir,
        const RepoPath& cacheDir,
        const RepoPath& artifactDir,
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
        using Command = NativeTestEnumerator::Command;
        using JobInfo = NativeTestEnumerator::JobInfo;
        using JobData = NativeTestEnumerator::JobData;
        using Cache = NativeTestEnumerator::JobData::Cache;

        const auto enumerationArtifact = GenerateTargetEnumerationArtifactFilePath(testTarget, m_artifactDir);
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
}
