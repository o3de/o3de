/*
 * All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 * its licensors.
 *
 * For complete copyright and license terms please see the LICENSE at the root of this
 * distribution (the "License"). All use of this software is governed by the License,
 * or, if provided, by the license below or the license accompanying this file. Do not
 * remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 */

#include <TestImpactFramework/TestImpactUtils.h>

#include <Artifact/Factory/TestImpactTestEnumerationSuiteFactory.h>
#include <TestEngine/Enumeration/TestImpactTestEnumerationException.h>
#include <TestEngine/Enumeration/TestImpactTestEnumerationSerializer.h>
#include <TestEngine/Enumeration/TestImpactTestEnumerator.h>
#include <TestEngine/Enumeration/TestImpactTestEnumeration.h>

#include <AzCore/IO/SystemFile.h>

namespace TestImpact
{
    namespace
    {
        void WriteCacheFile(
            const TestEnumeration& enumeration, const AZ::IO::Path& path, Bitwise::CacheExceptionPolicy cacheExceptionPolicy)
        {
            const AZStd::string cacheJSON = SerializeTestEnumeration(enumeration);
            const AZStd::vector<char> cacheBytes(cacheJSON.begin(), cacheJSON.end());
            AZ::IO::SystemFile cacheFile;

            if (!cacheFile.Open(
                    path.c_str(),
                    AZ::IO::SystemFile::SF_OPEN_CREATE | AZ::IO::SystemFile::SF_OPEN_CREATE_PATH | AZ::IO::SystemFile::SF_OPEN_WRITE_ONLY))
            {
                AZ_TestImpact_Eval(
                    !IsFlagSet(cacheExceptionPolicy, Bitwise::CacheExceptionPolicy::OnCacheWriteFailure), TestEnumerationException,
                    "Couldn't open cache file for writing");
                return;
            }

            if (cacheFile.Write(cacheBytes.data(), cacheBytes.size()) == 0)
            {
                AZ_TestImpact_Eval(
                    !IsFlagSet(cacheExceptionPolicy, Bitwise::CacheExceptionPolicy::OnCacheWriteFailure), TestEnumerationException,
                    "Couldn't write cache file data");
                return;
            }
        }

        AZStd::optional<TestEnumeration> ReadCacheFile(const AZ::IO::Path& path, Bitwise::CacheExceptionPolicy cacheExceptionPolicy)
        {
            AZ::IO::SystemFile cacheFile;
            AZStd::string cacheJSON;
            if (!cacheFile.Open(path.c_str(), AZ::IO::SystemFile::SF_OPEN_READ_ONLY))
            {
                AZ_TestImpact_Eval(
                    !IsFlagSet(cacheExceptionPolicy, Bitwise::CacheExceptionPolicy::OnCacheNotExist), TestEnumerationException,
                    "Couldn't locate cache file");
                return AZStd::nullopt;
            }

            const AZ::IO::SystemFile::SizeType length = cacheFile.Length();
            if (length == 0)
            {
                AZ_TestImpact_Eval(
                    !IsFlagSet(cacheExceptionPolicy, Bitwise::CacheExceptionPolicy::OnCacheReadFailure), TestEnumerationException,
                    "Cache file is empty");
                return AZStd::nullopt;
            }

            cacheFile.Seek(0, AZ::IO::SystemFile::SF_SEEK_BEGIN);
            cacheJSON.resize(length);
            if (cacheFile.Read(length, cacheJSON.data()) != length)
            {
                AZ_TestImpact_Eval(
                    !IsFlagSet(cacheExceptionPolicy, Bitwise::CacheExceptionPolicy::OnCacheReadFailure), TestEnumerationException,
                    "Couldn't read cache file");
                return AZStd::nullopt;
            }

            return DeserializeTestEnumeration(cacheJSON);
        }
    } // namespace

    TestEnumeration ParseTestEnumerationFile(const AZ::IO::Path& enumerationFile)
    {
        return TestEnumeration(GTest::TestEnumerationSuitesFactory(ReadFileContents<TestEnumerationException>(enumerationFile)));
    }

    TestEnumerationJobData::TestEnumerationJobData(const AZ::IO::Path& enumerationArtifact, AZStd::optional<Cache>&& cache)
        : m_enumerationArtifact(enumerationArtifact)
        , m_cache(AZStd::move(cache))
    {
    }

    const AZ::IO::Path& TestEnumerationJobData::GetEnumerationArtifactPath() const
    {
        return m_enumerationArtifact;
    }

    const AZStd::optional<TestEnumerationJobData::Cache>& TestEnumerationJobData::GetCache() const
    {
        return m_cache;
    }

    TestEnumerator::TestEnumerator(
        AZStd::optional<ClientJobCallback> clientCallback,
        size_t maxConcurrentEnumerations,
        AZStd::optional<AZStd::chrono::milliseconds> enumerationTimeout,
        AZStd::optional<AZStd::chrono::milliseconds> enumeratorTimeout)
        : JobRunner(clientCallback, AZStd::nullopt, StdOutputRouting::None, StdErrorRouting::None, maxConcurrentEnumerations, enumerationTimeout, enumeratorTimeout)
    {
    }

    AZStd::vector<TestEnumerator::Job> TestEnumerator::Enumerate(
        const AZStd::vector<JobInfo>& jobInfos,
        CacheExceptionPolicy cacheExceptionPolicy,
        JobExceptionPolicy jobExceptionPolicy)
    {
        AZStd::vector<Job> cachedJobs;
        AZStd::vector<JobInfo> jobQueue;

        for (const auto& jobInfo : jobInfos)
        {
            // If this job has a cache read policy attempt to read the cache
            if (jobInfo.GetCache().has_value() && jobInfo.GetCache()->m_policy == JobData::CachePolicy::Read)
            {
                JobMeta meta;
                auto enumeration = ReadCacheFile(jobInfo.GetCache()->m_file, cacheExceptionPolicy);

                // Even though cached jobs don't get executed we still give the client the opportunity to handle the job state
                // change in order to make the caching process transparent to the client
                if (enumeration.has_value())
                {
                    if (m_clientJobCallback.has_value())
                    {
                        (*m_clientJobCallback)(jobInfo, meta);
                    }

                    // Cache read successfully, this job will not be placed in the job queue
                    cachedJobs.emplace_back(Job(jobInfo, AZStd::move(meta), AZStd::move(enumeration)));
                }
                else
                {
                    // The cache read failed and exception policy for cache read failures is not to throw so instead place this job in the
                    // job queue
                    jobQueue.emplace_back(jobInfo);
                }
            }
            else
            {
                // This job has no cache read policy so place in job queue
                jobQueue.emplace_back(jobInfo);
            }
        }

        const auto payloadGenerator = [this, cacheExceptionPolicy](const JobDataMap& jobDataMap)
        {
            PayloadMap<Job> enumerations;
            for (const auto& [jobId, jobData] : jobDataMap)
            {
                const auto& [meta, jobInfo] = jobData;
                if (meta.m_result == JobResult::ExecutedWithSuccess)
                {
                    const auto& enumeration = enumerations[jobId] = ParseTestEnumerationFile(jobInfo->GetEnumerationArtifactPath());

                    // Write out the enumeration to a cache file if we have a cache write policy for this job
                    if (jobInfo->GetCache().has_value() && jobInfo->GetCache()->m_policy == JobData::CachePolicy::Write)
                    {
                        WriteCacheFile(enumeration.value(), jobInfo->GetCache()->m_file, cacheExceptionPolicy);
                    }
                }
            }

            return enumerations;
        };

        // Generate the enumeration results for the jobs that weren't cached
        auto jobs = ExecuteJobs(jobQueue, payloadGenerator, jobExceptionPolicy);

        // We need to add the cached jobs to the completed job list even though they technically weren't executed
        for (auto&& job : cachedJobs)
        {
            jobs.emplace_back(AZStd::move(job));
        }

        return jobs;
    }
} // namespace TestImpact
