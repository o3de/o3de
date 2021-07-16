/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <TestImpactFramework/TestImpactRuntime.h>

#include <Artifact/TestImpactArtifactException.h>
#include <TestEngine/Enumeration/TestImpactTestEnumeration.h>
#include <TestEngine/JobRunner/TestImpactTestJobRunner.h>

namespace TestImpact
{
    //! Per-job data for test enumerations.
    class TestEnumerationJobData
    {
    public:
        //! Policy for how a test enumeration will be written/read from the a previous cache instead of enumerated from the test target.
        enum class CachePolicy
        {
            Read, //!< Do read from a cache file but do not overwrite any existing cache file.
            Write //!< Do not read from a cache file but instead overwrite any existing cache file.
        };

        //! Cache configuration for a given test enumeration command.
        struct Cache
        {
            CachePolicy m_policy;
            RepoPath m_file;
        };

        TestEnumerationJobData(const RepoPath& enumerationArtifact, AZStd::optional<Cache>&& cache);

        //! Returns the path to the enumeration artifact produced by the test target.
        const RepoPath& GetEnumerationArtifactPath() const;

        //! Returns the cache details for this job.
        const AZStd::optional<Cache>& GetCache() const;

    private:
        RepoPath m_enumerationArtifact; //!< Path to enumeration artifact to be processed.
        AZStd::optional<Cache> m_cache = AZStd::nullopt; //!< No caching takes place if cache is empty.
    };

    //! Enumerate a batch of test targets to determine the test suites and fixtures they contain, caching the results where applicable.
    class TestEnumerator
        : public TestJobRunner<TestEnumerationJobData, TestEnumeration>
    {
        using JobRunner = TestJobRunner<TestEnumerationJobData, TestEnumeration>;

    public:
        //! Constructs a test enumerator with the specified parameters common to all enumeration job runs of this enumerator.
        //! @param maxConcurrentEnumerations The maximum number of enumerations to be in flight at any given time.
        explicit TestEnumerator(size_t maxConcurrentEnumerations);

        //! Executes the specified test enumeration jobs according to the specified cache and job exception policies.
        //! @param jobInfos The enumeration jobs to execute.
        //! @param cacheExceptionPolicy The cache exception policy to be used for this run.
        //! @param jobExceptionPolicy The enumeration job exception policy to be used for this run.
        //! @param enumerationTimeout The maximum duration an enumeration may be in-flight for before being forcefully terminated.
        //! @param enumeratorTimeout The maximum duration the enumerator may run before forcefully terminating all in-flight enumerations.
        //! @param clientCallback The optional client callback to be called whenever an enumeration job changes state.
        //! @return The result of the run sequence and the enumeration jobs with their associated test enumeration payloads.
        AZStd::pair<ProcessSchedulerResult, AZStd::vector<Job>> Enumerate(
            const AZStd::vector<JobInfo>& jobInfos,
            AZStd::optional<AZStd::chrono::milliseconds> enumerationTimeout,
            AZStd::optional<AZStd::chrono::milliseconds> enumeratorTimeout,
            AZStd::optional<ClientJobCallback> clientCallback);
    };
} // namespace TestImpact
