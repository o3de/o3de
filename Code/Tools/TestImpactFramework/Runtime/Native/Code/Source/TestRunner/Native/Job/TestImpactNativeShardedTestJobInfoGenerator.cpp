/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TestImpactFramework/TestImpactTestSequence.h>

#include <TestRunner/Native/Job/TestImpactNativeShardedTestJobInfoGenerator.h>

#include <AzCore/std/string/string_view.h>

namespace TestImpact
{
    NativeShardedInstrumentedTestRunJobInfoGenerator::NativeShardedInstrumentedTestRunJobInfoGenerator(
        const JobInfoGenerator& jobInfoGenerator,
        size_t maxConcurrency,
        const RepoPath& sourceDir,
        const RepoPath& targetBinaryDir,
        const NativeShardedArtifactDir& artifactDir,
        const RepoPath& testRunnerBinary,
        const RepoPath& instrumentBinary,
        CoverageLevel coverageLevel)
        : NativeShardedTestRunJobInfoGeneratorBase<NativeInstrumentedTestRunner>(
            jobInfoGenerator,
            maxConcurrency,
            sourceDir,
            targetBinaryDir,
            artifactDir,
            testRunnerBinary)
        , m_instrumentBinary(instrumentBinary)
        , m_coverageLevel(coverageLevel)
    {
    }

    RepoPath NativeShardedInstrumentedTestRunJobInfoGenerator::GenerateShardedTargetCoverageArtifactFilePath(
        const NativeTestTarget* testTarget, size_t shardNumber) const
    {
        auto artifactFilePath = GenerateTargetCoverageArtifactFilePath(testTarget, m_artifactDir.m_shardedCoverageArtifactDirectory);
        return artifactFilePath.ReplaceExtension(
            AZStd::string_view(AZStd::string::format("%zu%.*s", shardNumber, AZ_STRING_ARG(artifactFilePath.Extension().Native()))));
    }

    ShardedInstrumentedTestJobInfo NativeShardedInstrumentedTestRunJobInfoGenerator::GenerateJobInfoImpl(
        const TestTargetAndEnumeration& testTargetAndEnumeration,
        typename NativeInstrumentedTestRunner::JobInfo::Id startingId) const
    {
        const auto [testTarget, testEnumeration] = testTargetAndEnumeration;
        const auto testFilters = TestListsToTestFilters(GenerateShardedTestsList(testTargetAndEnumeration));
        typename ShardedInstrumentedTestJobInfo::JobInfos jobInfos;
        jobInfos.reserve(testFilters.size());

        for (size_t i = 0; i < testFilters.size(); i++)
        {
            const auto shardedRunArtifact = GenerateShardedTargetRunArtifactFilePath(testTarget, i);
            const auto shardedCoverageArtifact = GenerateShardedTargetCoverageArtifactFilePath(testTarget, i);
            const RepoPath shardAdditionalArgsFile = GenerateShardedAdditionalArgsFilePath(testTarget, i);
            const auto shardLaunchCommand = GenerateShardedLaunchCommand(testTarget, shardAdditionalArgsFile);
            WriteFileContents<TestRunnerException>(testFilters[i], shardAdditionalArgsFile);

            const auto command = GenerateInstrumentedTestJobInfoCommand(
                m_instrumentBinary,
                shardedCoverageArtifact,
                m_coverageLevel,
                m_targetBinaryDir,
                m_testRunnerBinary,
                m_sourceDir,
                GenerateRegularTestJobInfoCommand(shardLaunchCommand, shardedRunArtifact));

            jobInfos.emplace_back(
                NativeInstrumentedTestRunner::JobInfo::Id{ startingId.m_value + i },
                command,
                NativeInstrumentedTestRunner::JobData(testTarget->GetLaunchMethod(), shardedRunArtifact, shardedCoverageArtifact));
        }

        return ShardedInstrumentedTestJobInfo(testTarget, AZStd::move(jobInfos));
    }

    ShardedRegularTestJobInfo NativeShardedRegularTestRunJobInfoGenerator::GenerateJobInfoImpl(
        const TestTargetAndEnumeration& testTargetAndEnumeration,
        typename NativeRegularTestRunner::JobInfo::Id startingId) const
    {
        const auto [testTarget, testEnumeration] = testTargetAndEnumeration;
        const auto testFilters = TestListsToTestFilters(GenerateShardedTestsList(testTargetAndEnumeration));
        typename ShardedRegularTestJobInfo::JobInfos jobInfos;
        jobInfos.reserve(testFilters.size());

        for (size_t i = 0; i < testFilters.size(); i++)
        {
            const auto shardedRunArtifact = GenerateShardedTargetRunArtifactFilePath(testTarget,  i);
            const RepoPath shardAdditionalArgsFile = GenerateShardedAdditionalArgsFilePath(testTarget, i);
            const auto shardLaunchCommand = GenerateShardedLaunchCommand(testTarget, shardAdditionalArgsFile);
            WriteFileContents<TestRunnerException>(testFilters[i], shardAdditionalArgsFile);
            const auto command = GenerateRegularTestJobInfoCommand(shardLaunchCommand, shardedRunArtifact);

            jobInfos.emplace_back(
                NativeRegularTestRunner::JobInfo::Id{ startingId.m_value + i },
                command,
                NativeRegularTestRunner::JobData(testTarget->GetLaunchMethod(), shardedRunArtifact));
        }

        return ShardedRegularTestJobInfo(testTarget, AZStd::move(jobInfos));
    }
} // namespace TestImpact
