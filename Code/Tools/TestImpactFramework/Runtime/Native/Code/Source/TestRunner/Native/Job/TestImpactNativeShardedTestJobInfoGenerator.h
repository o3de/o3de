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
    //! Job info for all shards of a given test target.
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

        //! Returns the id of the job info (the first shard is considered the "parent" id of all shards).
        Id GetId() const
        {
            return m_jobInfos.front().GetId();
        }

        //! Returns the test target that is sharded.
        const NativeTestTarget* GetTestTarget() const
        {
            return m_testTarget;
        }

        // Returns the job infos of each shard for the test target.
        const JobInfos& GetJobInfos() const
        {
            return m_jobInfos;
        }

    private:
        const NativeTestTarget* m_testTarget = nullptr; //!< The sharded test target.
        JobInfos m_jobInfos; //! The job infos for each shard of the test target.
    };

    //! Type alias for the instrumented test runner.
    using ShardedInstrumentedTestJobInfo = ShardedTestJobInfo<NativeInstrumentedTestRunner>;

    //! Type alias for the regular test runner.
    using ShardedRegularTestJobInfo = ShardedTestJobInfo<NativeRegularTestRunner>;

    //! Helper pair for a test target and its enumeration (if any).
    using TestTargetAndEnumeration = AZStd::pair<const NativeTestTarget*, AZStd::optional<TestEnumeration>>;

    //! Base class for the regular and instrumented sharded job info generators.
    template<typename TestJobRunner>
    class NativeShardedTestRunJobInfoGeneratorBase
    {
    public:
        using JobInfoGenerator = typename TestJobRunner::JobInfoGenerator;

        NativeShardedTestRunJobInfoGeneratorBase(
            const JobInfoGenerator& jobInfoGenerator,
            size_t maxConcurrency,
            const RepoPath& sourceDir,
            const RepoPath& targetBinaryDir,
            const NativeShardedArtifactDir& artifactDir,
            const RepoPath& testRunnerBinary);

        virtual ~NativeShardedTestRunJobInfoGeneratorBase() = default;

        //! Generates the sharded job info for a given test target based on its enumerated tests.
        //! @note If no enumeration is provided, the standard job info generator for the underlying test runner will be used.
        ShardedTestJobInfo<TestJobRunner> GenerateJobInfo(
            const TestTargetAndEnumeration& testTargetAndEnumeration,
            typename TestJobRunner::JobInfo::Id startingId);

        //! Generates the sharded job infos for a set of test target based on their enumerated tests.
        AZStd::vector<ShardedTestJobInfo<TestJobRunner>> GenerateJobInfos(
            const AZStd::vector<TestTargetAndEnumeration>& testTargetsAndEnumerations);

    protected:
        //! The interleaved tests for a given set of shards.
        using ShardedTestsList = AZStd::vector<AZStd::vector<AZStd::string>>;

        //! The test framework specific string to filter the tests for a given shard.
        using ShardedTestsFilter = AZStd::vector<AZStd::string>;

        //! The specialized (instrumented or regular test runner) job generator.
        virtual ShardedTestJobInfo<TestJobRunner> GenerateJobInfoImpl(
            const TestTargetAndEnumeration& testTargetAndEnumeration, typename TestJobRunner::JobInfo::Id startingId) const = 0;

        //! Generates the sharded test list according to the test target's sharding configuration.
        ShardedTestsList GenerateShardedTestsList(const TestTargetAndEnumeration& testTargetAndEnumeration) const;

        //! Converts the raw shard test lists to the test framework specific filters.
        ShardedTestsFilter TestListsToTestFilters(const ShardedTestsList& shardedTestList) const;

        //! Generates a sharded run artifact file path for a given test target's shard.
        RepoPath GenerateShardedTargetRunArtifactFilePath(const NativeTestTarget* testTarget, size_t shardNumber) const;

        //! Generates a sharded AzTestRunner additional arguments file path for a given test target's shard.
        RepoPath GenerateShardedAdditionalArgsFilePath(const NativeTestTarget* testTarget, size_t shardNumber) const;

        //! Generates the launch command for a given test target's shard.
        AZStd::string GenerateShardedLaunchCommand(
            const NativeTestTarget* testTarget, const RepoPath& shardAdditionalArgsFile) const;

        const JobInfoGenerator* m_jobInfoGenerator = nullptr; //!< The standard job info generator to use if a target cannot be sharded.
        size_t m_maxConcurrency; //!< The maximum number of shards to generate for a test target.
        RepoPath m_sourceDir; //!< Path to the repository.
        RepoPath m_targetBinaryDir; //!< Path to the test target binaries.
        NativeShardedArtifactDir m_artifactDir; //!< Path to the sharded artifact directories.
        RepoPath m_testRunnerBinary; //!< Path to the test runner binary.

    private:
        //! Interleaves the enumerated tests across the shards.
        ShardedTestsList ShardTestInterleaved(const TestTargetAndEnumeration& testTargetAndEnumeration) const;

        //! Interleaves the enumerated fixtures across the shards.
        ShardedTestsList ShardFixtureInterleaved(const TestTargetAndEnumeration& testTargetAndEnumeration) const;
    };

    //! Job info generator for the instrumented sharded test runner.
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
        //! Generates a sharded coverage artifact file path for a given test target's shard.
        RepoPath GenerateShardedTargetCoverageArtifactFilePath(const NativeTestTarget* testTarget, size_t shardNumber) const;

        RepoPath m_instrumentBinary; //!< Path to the coverage instrumentation binary.
        CoverageLevel m_coverageLevel; //!< Coverage level to use for instrumentation.
    };

    //! Job info generator for the regular sharded test runner.
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
        // If the target can shard and has more than one test, and the max concurrency is greater than 1, we will shard
        const auto [testTarget, testEnumeration] = testTargetAndEnumeration;
        const bool testTargetCanShard = testEnumeration.has_value() &&
            ((testTarget->GetShardingConfiguration() == ShardingConfiguration::TestInterleaved
                && testEnumeration->GetNumEnabledTests() > 1) ||
                (testTarget->GetShardingConfiguration() == ShardingConfiguration::FixtureInterleaved
                    && testEnumeration->GetNumEnabledTestSuites() > 1));

        if (testTarget->CanShard()
            && m_maxConcurrency > 1
            && testTargetCanShard)
        {
            std::printf(R"(Splitting test run into shards with sharding configuration "%s", enabled test count = "%zu")"
                R"(, enabled test suite/fixture count = "%zu" for target "%s")" "\n",
                testTarget->GetShardingConfiguration() == ShardingConfiguration::TestInterleaved
                ? "Test Interleaved" : "Fixture Interleaved",
                testEnumeration->GetNumEnabledTests(),
                testEnumeration->GetNumEnabledTestSuites(),
                testTarget->GetName().c_str());
            return GenerateJobInfoImpl(testTargetAndEnumeration, startingId);
        }
        else
        {
            // Target cannot be sharded, use the standard job info generator.
            return { testTarget, { m_jobInfoGenerator->GenerateJobInfo(testTarget, startingId) } };
        }
    }

    template<typename TestJobRunner>
    typename NativeShardedTestRunJobInfoGeneratorBase<TestJobRunner>::ShardedTestsList NativeShardedTestRunJobInfoGeneratorBase<
        TestJobRunner>::GenerateShardedTestsList(const TestTargetAndEnumeration& testTargetAndEnumeration) const
    {
        if (testTargetAndEnumeration.first->GetShardingConfiguration() == ShardingConfiguration::TestInterleaved)
        {
            return ShardTestInterleaved(testTargetAndEnumeration);
        }
        if (testTargetAndEnumeration.first->GetShardingConfiguration() == ShardingConfiguration::FixtureInterleaved)
        {
            return ShardFixtureInterleaved(testTargetAndEnumeration);
        }
        else
        {
            // This shouldn't be reachable but leave here to signpost where additional sharding configuration options should go
            AZ_Error(
                "NativeShardedTestRunJobInfoGeneratorBase",
                false,
                AZStd::string::format(
                    "Unexpected sharding configuration for test target '%s'", testTargetAndEnumeration.first->GetName().c_str()).c_str());

            return ShardedTestsList{};
        }
    }

    template<typename TestJobRunner>
    typename NativeShardedTestRunJobInfoGeneratorBase<TestJobRunner>::ShardedTestsList NativeShardedTestRunJobInfoGeneratorBase<
        TestJobRunner>::ShardTestInterleaved(const TestTargetAndEnumeration& testTargetAndEnumeration) const
    {
        const auto [testTarget, testEnumeration] = testTargetAndEnumeration;
        const auto numTests = testEnumeration->GetNumEnabledTests();
        const auto numShards = std::min(m_maxConcurrency, numTests);
        if (numShards == 0)
        {
            // If there are no shards, there is no work to be done
            return {};
        }
        ShardedTestsList shardTestList(numShards);

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
    typename NativeShardedTestRunJobInfoGeneratorBase<TestJobRunner>::ShardedTestsList NativeShardedTestRunJobInfoGeneratorBase<
        TestJobRunner>::ShardFixtureInterleaved(const TestTargetAndEnumeration& testTargetAndEnumeration) const
    {
        const auto [testTarget, testEnumeration] = testTargetAndEnumeration;
        const auto numFixtures = testEnumeration->GetNumEnabledTestSuites();
        const auto numShards = std::min(m_maxConcurrency, numFixtures);
        if (numShards == 0)
        {
            // If there are no shards, there is no work to be done
            return {};
        }
        ShardedTestsList shardTestList(numShards);

        size_t fixtureIndex = 0;
        for (const auto fixture : testEnumeration->GetTestSuites())
        {
            if (fixture.m_enabled)
            {
                const auto numEnabledTests = std::count_if(
                    fixture.m_tests.begin(),
                    fixture.m_tests.end(),
                    [](const auto& test)
                    {
                        return test.m_enabled;
                    });

                if (numEnabledTests)
                {
                    shardTestList[fixtureIndex++ % numShards].emplace_back(AZStd::string::format("%s.*", fixture.m_name.c_str()));
                }
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
                // The trailing colon added by the last test is still a valid GTest filter
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

            // Increment the job id by the number of sharded job infos generated for the most recently added test target so that the next
            // job id used is contiguous in seqeunce
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
    //! Less function for TestTargetAndEnumeration types for use in maps and sets.
    template<>
    struct less<TestImpact::TestTargetAndEnumeration>
    {
        bool operator()(const TestImpact::TestTargetAndEnumeration& lhs, const TestImpact::TestTargetAndEnumeration& rhs) const
        {
            return reinterpret_cast<size_t>(lhs.first) < reinterpret_cast<size_t>(rhs.first);
        }
    };

    //! Hash function for TestTargetAndEnumeration types for use in unordered maps and sets.
    template<>
    struct hash<TestImpact::TestTargetAndEnumeration>
    {
        size_t operator()(const TestImpact::TestTargetAndEnumeration& testTargetAndEnumeration) const noexcept
        {
            return reinterpret_cast<size_t>(testTargetAndEnumeration.first);
        }
    };
} // namespace AZStd
