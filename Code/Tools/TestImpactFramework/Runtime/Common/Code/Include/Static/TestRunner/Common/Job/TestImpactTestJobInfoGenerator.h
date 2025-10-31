/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/vector.h>

namespace TestImpact
{
    //! Helper class for generating test job infos.
    template<typename TestJobRunner, typename TestTarget>
    class TestJobInfoGeneratorBase
    {
    public:
        using TestJobRunenrType = TestJobRunner;
        using TestTargetType = TestTarget;
        using Command = typename TestJobRunner::Command;
        using JobInfo = typename TestJobRunner::JobInfo;
        using JobData = typename TestJobRunner::JobData;

        virtual ~TestJobInfoGeneratorBase() = default;

        //! Generates the information for a test job.
        //! @param testTarget The test target to generate the job information for.
        //! @param jobId The id to assign for this job.
        virtual typename TestJobRunner::JobInfo GenerateJobInfo(
            const TestTarget* testTarget, typename TestJobRunner::JobInfo::Id jobId) const = 0;

        //! Generates the information for the batch of test enumeration jobs.
        typename TestJobRunner::JobInfos GenerateJobInfos(const AZStd::vector<const TestTarget*>& testTargets) const;
    };

    //! Helper class for generating test enumeration job infos.
    template<typename TestJobRunner, typename TestTarget>
    class TestEnumerationJobInfoGeneratorBase
        : public TestJobInfoGeneratorBase<TestJobRunner, TestTarget>
    {
    public:
        using TestJobRunenrType = TestJobRunner;
        using TestTargetType = TestTarget;
        using Command = typename TestJobRunner::Command;
        using JobInfo = typename TestJobRunner::JobInfo;
        using JobData = typename TestJobRunner::JobData;
        using CachePolicy = typename JobInfo::CachePolicy;
        using Cache = typename JobData::Cache;

        //! Configures the test job info generator with the necessary path information for launching test targets.
        //! @param targetBinaryDir Path to where the test target binaries are found.
        //! @param artifactDir Path to the transient directory where test artifacts are produced.
        //! @param testRunnerBinary Path to the binary responsible for launching test targets that have the TestRunner launch method.
        TestEnumerationJobInfoGeneratorBase(
            const RepoPath& targetBinaryDir, const ArtifactDir& artifactDir, const RepoPath& testRunnerBinary);

        virtual ~TestEnumerationJobInfoGeneratorBase() = default;

        //! Sets the cache policy to be used by this generator.
        //! @param cachePolicy The cache policy to use for job generation.
        void SetCachePolicy(CachePolicy cachePolicy);

        //! Gets the cache policy used by this generator.
        CachePolicy GetCachePolicy() const;

        // TestJobInfoGeneratorBase overrides ...
        typename TestJobRunner::JobInfo GenerateJobInfo(
            const TestTarget* testTarget, typename TestJobRunner::JobInfo::Id jobId) const override;

    protected:
        //! Generates the information for a test enumeration job.
        //! @param testTarget The test target to generate the job information for.
        //! @param jobId The id to assign for this job.
        virtual typename TestJobRunner::JobInfo GenerateJobInfoImpl(
            const TestTarget* testTarget, typename TestJobRunner::JobInfo::Id jobId) const = 0;

        RepoPath m_targetBinaryDir;
        ArtifactDir m_artifactDir;
        RepoPath m_testRunnerBinary;

    private:
        CachePolicy m_cachePolicy = CachePolicy::Write;
    };

    template<typename TestJobRunner, typename TestTarget>
    typename TestJobRunner::JobInfos TestJobInfoGeneratorBase<TestJobRunner, TestTarget>::GenerateJobInfos(
        const AZStd::vector<const TestTarget*>& testTargets) const
    {
        typename TestJobRunner::JobInfos jobInfos;
        jobInfos.reserve(testTargets.size());
        for (size_t jobId = 0; jobId < testTargets.size(); jobId++)
        {
            jobInfos.push_back(GenerateJobInfo(testTargets[jobId], { jobId }));
        }

        return jobInfos;
    }

    template<typename TestJobRunner, typename TestTarget>
    TestEnumerationJobInfoGeneratorBase<TestJobRunner, TestTarget>::TestEnumerationJobInfoGeneratorBase(
        const RepoPath& targetBinaryDir, const ArtifactDir& artifactDir, const RepoPath& testRunnerBinary)
        : m_targetBinaryDir(targetBinaryDir)
        , m_artifactDir(artifactDir)
        , m_testRunnerBinary(testRunnerBinary)
    {
    }

    template<typename TestJobRunner, typename TestTarget>
    typename TestJobRunner::JobInfo TestEnumerationJobInfoGeneratorBase<TestJobRunner, TestTarget>::GenerateJobInfo(
        const TestTarget* testTarget, typename TestJobRunner::JobInfo::Id jobId) const
    {
        if (testTarget->CanEnumerate())
        {
            return GenerateJobInfoImpl(testTarget, jobId);
        }

        // If enumeration is not supported, we cannot shard the tests, so add an empty job info instead
        return JobInfo(
            jobId,
            Command{ "" },
            JobData(
                "",
                Cache{ m_cachePolicy, GenerateTargetEnumerationCacheFilePath(testTarget, m_artifactDir.m_enumerationCacheDirectory) }));
    }

    template<typename TestJobRunner, typename TestTarget>
    void TestEnumerationJobInfoGeneratorBase<TestJobRunner, TestTarget>::SetCachePolicy(CachePolicy cachePolicy)
    {
        m_cachePolicy = cachePolicy;
    }

    template<typename TestJobRunner, typename TestTarget>
    auto TestEnumerationJobInfoGeneratorBase<TestJobRunner, TestTarget>::GetCachePolicy() const -> CachePolicy
    {
        return m_cachePolicy;
    }
} // namespace TestImpact
