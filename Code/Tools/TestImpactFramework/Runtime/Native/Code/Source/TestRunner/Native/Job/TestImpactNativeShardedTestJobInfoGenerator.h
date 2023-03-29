/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <TestImpactFramework/Native/TestImpactNativeConfiguration.h>

#include <Target/Native/TestImpactNativeTestTarget.h>
#include <TestEngine/Common/Enumeration/TestImpactTestEngineEnumeration.h>
#include <TestRunner/Common/Job/TestImpactTestJobInfoUtils.h>
#include <TestRunner/Native/TestImpactNativeRegularTestRunner.h>
#include <TestRunner/Native/TestImpactNativeInstrumentedTestRunner.h>
#include <TestRunner/Native/TestImpactNativeRegularTestRunner.h>
#include <TestRunner/Native/Job/TestImpactNativeTestJobInfoGenerator.h>
#include <TestRunner/Native/Job/TestImpactNativeTestJobInfoUtils.h>

namespace TestImpact
{
    //!
    template<typename TestJobRunner>
    class ShardedTestJobInfo
    {
    public:
        using Id = typename TestJobRunner::JobInfo::Id;
        using IdType = typename TestJobRunner::JobInfo::IdType;
        using JobInfos = typename TestJobRunner::JobInfos;

        ShardedTestJobInfo(const NativeTestTarget* testTarget, JobInfos&& jobInfos)
            : m_testTarget(testTarget)
            , m_jobInfos(AZStd::move(jobInfos))
        {
            AZ_TestImpact_Eval(!m_jobInfos.empty(), TestRunnerException, "Attepted to instantiate a sharded test job info no sub job infos");
        }

        //!
        Id GetId() const
        {
            return m_jobInfos.front().GetId();
        }

        const NativeTestTarget* GetTestTarget() const
        {
            return m_testTarget;
        }

        const JobInfos& GetJobInfos() const
        {
            return m_jobInfos;
        }

    private:
        const NativeTestTarget* m_testTarget = nullptr;
        JobInfos m_jobInfos;
    };

    //!
    using ShardedInstrumentedTestJobInfo = ShardedTestJobInfo<NativeInstrumentedTestRunner>;

    //!
    using ShardedRegularTestJobInfo = ShardedTestJobInfo<NativeRegularTestRunner>;

    using TestTargetAndEnumeration = AZStd::pair<const NativeTestTarget*, AZStd::optional<TestEnumeration>>;

    //!
    template<typename TestJobRunner>
    class NativeShardedTestRunJobInfoGeneratorBase
    {
    public:
        //!
        using JobInfoGenerator = typename TestJobRunner::JobInfoGenerator;

        NativeShardedTestRunJobInfoGeneratorBase(
            const JobInfoGenerator& jobInfoGenerator,
            size_t maxConcurrency,
            const RepoPath& sourceDir,
            const RepoPath& targetBinaryDir,
            const NativeShardedArtifactDir& artifactDir,
            const RepoPath& testRunnerBinary);

        virtual ~NativeShardedTestRunJobInfoGeneratorBase() = default;

        //!
        ShardedTestJobInfo<TestJobRunner> GenerateJobInfo(
            const TestTargetAndEnumeration& testTargetAndEnumeration,
            typename TestJobRunner::JobInfo::Id startingId);

        AZStd::vector<ShardedTestJobInfo<TestJobRunner>> GenerateJobInfos(
            const AZStd::vector<TestTargetAndEnumeration>& testTargetsAndEnumerations);

    protected:
        //!
        using ShardedTestsList = AZStd::vector<AZStd::vector<AZStd::string>>;

        //!
        using ShardedTestsFilter = AZStd::vector<AZStd::string>;

        //!
        virtual ShardedTestJobInfo<TestJobRunner> GenerateJobInfoImpl(
            const TestTargetAndEnumeration& testTargetAndEnumeration, typename TestJobRunner::JobInfo::Id startingId) const = 0;

        //!
        ShardedTestsList ShardTestInterleaved(const TestTargetAndEnumeration& testTargetAndEnumeration) const;

        //!
        ShardedTestsFilter TestListsToTestFilters(const ShardedTestsList& shardedTestList) const;

        //!
        RepoPath GenerateShardedTargetRunArtifactFilePath(const NativeTestTarget* testTarget, size_t shardNumber) const;

        //!
        RepoPath GenerateShardedAdditionalArgsFilePath(const NativeTestTarget* testTarget, size_t shardNumber) const;

        //!
        AZStd::string GenerateShardedLaunchCommand(
            const NativeTestTarget* testTarget, const RepoPath& shardAdditionalArgsFile) const;

        const JobInfoGenerator* m_jobInfoGenerator = nullptr;
        size_t m_maxConcurrency;
        RepoPath m_sourceDir;
        RepoPath m_targetBinaryDir;
        NativeShardedArtifactDir m_artifactDir;
        RepoPath m_testRunnerBinary;
    };

    //!
    class NativeShardedInstrumentedTestRunJobInfoGenerator
        : public NativeShardedTestRunJobInfoGeneratorBase<NativeInstrumentedTestRunner>
    {
    public:
        NativeShardedInstrumentedTestRunJobInfoGenerator(
            const JobInfoGenerator& jobInfoGenerator,
            size_t maxConcurrency,
            const RepoPath& sourceDir,
            const RepoPath& targetBinaryDir,
            const NativeShardedArtifactDir& artifactDir,
            const RepoPath& testRunnerBinary,
            const RepoPath& instrumentBinary,
            CoverageLevel coverageLevel = CoverageLevel::Source);

    protected:
        // NativeShardedTestRunJobInfoGeneratorBase overrides ...
        ShardedInstrumentedTestJobInfo GenerateJobInfoImpl(
            const TestTargetAndEnumeration& testTargetAndEnumeration,
            typename NativeInstrumentedTestRunner::JobInfo::Id startingId) const override;

    private:
        //!
        RepoPath GenerateShardedTargetCoverageArtifactFilePath(const NativeTestTarget* testTarget, size_t shardNumber) const;

        RepoPath m_cacheDir;
        RepoPath m_instrumentBinary;
        CoverageLevel m_coverageLevel;
    };

    //!
    class NativeShardedRegularTestRunJobInfoGenerator
        :  public NativeShardedTestRunJobInfoGeneratorBase<NativeRegularTestRunner>
    {
    public:
        using NativeShardedTestRunJobInfoGeneratorBase<NativeRegularTestRunner>::NativeShardedTestRunJobInfoGeneratorBase;

    protected:
        // NativeShardedTestRunJobInfoGeneratorBase overrides ...
        ShardedRegularTestJobInfo GenerateJobInfoImpl(
            const TestTargetAndEnumeration& testTargetAndEnumeration,
            typename NativeRegularTestRunner::JobInfo::Id startingId) const override;
    };

    template<typename TestJobRunner>
    NativeShardedTestRunJobInfoGeneratorBase<TestJobRunner>::NativeShardedTestRunJobInfoGeneratorBase(
        const JobInfoGenerator& jobInfoGenerator,
        size_t maxConcurrency,
        const RepoPath& sourceDir,
        const RepoPath& targetBinaryDir,
        const NativeShardedArtifactDir& artifactDir,
        const RepoPath& testRunnerBinary)
        : m_jobInfoGenerator(&jobInfoGenerator)
        , m_maxConcurrency(maxConcurrency)
        , m_sourceDir(sourceDir)
        , m_targetBinaryDir(targetBinaryDir)
        , m_artifactDir(artifactDir)
        , m_testRunnerBinary(testRunnerBinary)
    {
        AZ_TestImpact_Eval(maxConcurrency != 0, TestRunnerException, "Max Number of concurrent processes in flight cannot be 0");
    }

    template<typename TestJobRunner>
    ShardedTestJobInfo<TestJobRunner> NativeShardedTestRunJobInfoGeneratorBase<TestJobRunner>::GenerateJobInfo(
        const TestTargetAndEnumeration& testTargetAndEnumeration, typename TestJobRunner::JobInfo::Id startingId)
    {
        if (const auto [testTarget, testEnumeration] = testTargetAndEnumeration;
            m_maxConcurrency > 1 && testEnumeration.has_value() && testEnumeration->GetNumEnabledTests() > 1)
        {
            return GenerateJobInfoImpl(testTargetAndEnumeration, startingId);
        }
        else
        {
            return { testTarget, { m_jobInfoGenerator->GenerateJobInfo(testTarget, startingId) } };
        }
    }

    template<typename TestJobRunner>
    typename NativeShardedTestRunJobInfoGeneratorBase<TestJobRunner>::ShardedTestsList NativeShardedTestRunJobInfoGeneratorBase<
        TestJobRunner>::ShardTestInterleaved(const TestTargetAndEnumeration& testTargetAndEnumeration) const
    {
        const auto [testTarget, testEnumeration] = testTargetAndEnumeration;
        const auto numTests = testEnumeration->GetNumEnabledTests();
        const auto numShards = std::min(m_maxConcurrency, numTests);
        ShardedTestsList shardTestList(numShards);
        const auto testsPerShard = numTests / numShards;

        size_t testIndex = 0;
        for (const auto fixture : testEnumeration->GetTestSuites())
        {
            if (!fixture.m_enabled)
            {
                continue;
            }

            for (const auto test : fixture.m_tests)
            {
                if (!test.m_enabled)
                {
                    continue;
                }

                shardTestList[testIndex++ % numShards].emplace_back(
                    AZStd::string::format("%s.%s", fixture.m_name.c_str(), test.m_name.c_str()));
            }
        }

        return shardTestList;
    }

    template<typename TestJobRunner>
    auto NativeShardedTestRunJobInfoGeneratorBase<TestJobRunner>::TestListsToTestFilters(const ShardedTestsList& shardedTestList) const
        -> ShardedTestsFilter
    {
        ShardedTestsFilter shardedTestFilter;
        shardedTestFilter.reserve(shardedTestList.size());

        for (const auto& shardTests : shardedTestList)
        {
            AZStd::string testFilter = "--gtest_filter=";
            for (const auto& test : shardTests)
            {
                // The trailing colon added by the last test is still a valid gtest filter
                testFilter += AZStd::string::format("%s:", test.c_str());
            }

            shardedTestFilter.emplace_back(testFilter);
        }

        return shardedTestFilter;
    }

    template<typename TestJobRunner>
    auto NativeShardedTestRunJobInfoGeneratorBase<TestJobRunner>::GenerateJobInfos(
        const AZStd::vector<TestTargetAndEnumeration>& testTargetsAndEnumerations)
        -> AZStd::vector<ShardedTestJobInfo<TestJobRunner>>
    {
        AZStd::vector<ShardedTestJobInfo<TestJobRunner>> jobInfos;
        for (size_t testTargetIndex = 0, jobId = 0; testTargetIndex < testTargetsAndEnumerations.size(); testTargetIndex++)
        {
            jobInfos.push_back(GenerateJobInfo(testTargetsAndEnumerations[testTargetIndex], { jobId }));
            jobId += jobInfos.back().GetJobInfos().size();
        }

        return jobInfos;
    }

    template<typename TestJobRunner>
    RepoPath NativeShardedTestRunJobInfoGeneratorBase<TestJobRunner>::GenerateShardedTargetRunArtifactFilePath(
        const NativeTestTarget* testTarget, const size_t shardNumber) const
    {
        auto artifactFilePath = GenerateTargetRunArtifactFilePath(testTarget, m_artifactDir.m_shardedTestRunArtifactDirectory);
        return artifactFilePath.ReplaceExtension(AZStd::string::format("%zu%s", shardNumber, artifactFilePath.Extension().String().c_str()).c_str());
    }

    template<typename TestJobRunner>
    AZStd::string NativeShardedTestRunJobInfoGeneratorBase<TestJobRunner>::GenerateShardedLaunchCommand(
        const NativeTestTarget* testTarget, const RepoPath& shardAdditionalArgsFile) const
    {
        return AZStd::string::format(
            "%s --args_from_file \"%s\"",
            GenerateLaunchArgument(testTarget, m_targetBinaryDir, m_testRunnerBinary).c_str(),
            shardAdditionalArgsFile.c_str());
    }

    template<typename TestJobRunner>
    RepoPath NativeShardedTestRunJobInfoGeneratorBase<TestJobRunner>::GenerateShardedAdditionalArgsFilePath(
        const NativeTestTarget* testTarget, size_t shardNumber) const
    {
        return AZStd::string::format(
            "%s.%zu.args", (m_artifactDir.m_shardedTestRunArtifactDirectory / RepoPath(testTarget->GetName())).c_str(), shardNumber);
    }
} // namespace TestImpact

namespace AZStd
{
    //!
    template<>
    struct less<TestImpact::TestTargetAndEnumeration>
    {
        bool operator()(const TestImpact::TestTargetAndEnumeration& lhs, const TestImpact::TestTargetAndEnumeration& rhs) const
        {
            return reinterpret_cast<size_t>(lhs.first) < reinterpret_cast<size_t>(rhs.first);
        }
    };

    //!
    template<>
    struct hash<TestImpact::TestTargetAndEnumeration>
    {
        size_t operator()(const TestImpact::TestTargetAndEnumeration& testTargetAndEnumeration) const noexcept
        {
            return reinterpret_cast<size_t>(testTargetAndEnumeration.first);
        }
    };
} // namespace AZStd
