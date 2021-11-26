/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <TestRunner/Common/Enumeration/TestImpactTestEnumerator.h>
#include <TestRunner/Common/Job/TestImpactTestEnumerationJobData.h>


#include <TestImpactFramework/TestImpactUtils.h>
#include <TestRunner/Common/Enumeration/TestImpactTestEnumerationSerializer.h>
#include <Artifact/Factory/TestImpactTestEnumerationSuiteFactory.h>

namespace TestImpact
{
    struct NativeTestEnumerationJobData
        : public TestEnumerationJobData
    {
        using TestEnumerationJobData::TestEnumerationJobData;
    };

    class NativeTestEnumerator
        : public TestEnumerator<NativeTestEnumerationJobData>
    {
    public:
        using TestEnumerator<NativeTestEnumerationJobData>::TestEnumerator;
    };

    template<>
    inline PayloadOutcome<TestEnumeration> PayloadFactory(const JobInfo<NativeTestEnumerationJobData>& jobData, [[maybe_unused]]const JobMeta& jobMeta)
    {
        try
        {
            return AZ::Success(TestEnumeration(
                GTest::TestEnumerationSuitesFactory(ReadFileContents<TestRunnerException>(jobData.GetEnumerationArtifactPath()))));
        } catch (const Exception& e)
        {
            return AZ::Failure(AZStd::string::format("%s\n", e.what()));
        }
    };

    ////! Enumerate a batch of test targets to determine the test suites and fixtures they contain, caching the results where applicable.
    //class NativeTestEnumerator
    //    : public TestJobRunner<TestEnumerationJobData, TestEnumeration>
    //{
    //    using JobRunner = TestJobRunner<TestEnumerationJobData, TestEnumeration>;

    //public:
    //    using JobRunner::JobRunner;

    //    //! Executes the specified test enumeration jobs according to the specified cache and job exception policies.
    //    //! @param jobInfos The enumeration jobs to execute.
    //    //! @param cacheExceptionPolicy The cache exception policy to be used for this run.
    //    //! @param jobExceptionPolicy The enumeration job exception policy to be used for this run.
    //    //! @param enumerationTimeout The maximum duration an enumeration may be in-flight for before being forcefully terminated.
    //    //! @param enumeratorTimeout The maximum duration the enumerator may run before forcefully terminating all in-flight enumerations.
    //    //! @param clientCallback The optional client callback to be called whenever an enumeration job changes state.
    //    //! @return The result of the run sequence and the enumeration jobs with their associated test enumeration payloads.
    //    AZStd::pair<ProcessSchedulerResult, AZStd::vector<Job>> Enumerate(
    //        const AZStd::vector<JobInfo>& jobInfos,
    //        AZStd::optional<AZStd::chrono::milliseconds> enumerationTimeout,
    //        AZStd::optional<AZStd::chrono::milliseconds> enumeratorTimeout,
    //        AZStd::optional<ClientJobCallback> clientCallback)
    //    {
    //        AZStd::vector<Job> cachedJobs;
    //        AZStd::vector<JobInfo> jobQueue;

    //        for (auto jobInfo = jobInfos.begin(); jobInfo != jobInfos.end(); ++jobInfo)
    //        {
    //            // If this job has a cache read policy attempt to read the cache
    //            if (jobInfo->GetCache().has_value())
    //            {
    //                if (jobInfo->GetCache()->m_policy == JobData::CachePolicy::Read)
    //                {
    //                    JobMeta meta;
    //                    AZStd::optional<JobPayload> enumeration;

    //                    try
    //                    {
    //                        enumeration =
    //                            JobPayload(DeserializeTestEnumeration(ReadFileContents<TestRunnerException>(jobInfo->GetCache()->m_file)));
    //                    } catch (const TestRunnerException& e)
    //                    {
    //                        AZ_Printf("Enumerate", AZStd::string::format("Enumeration cache error: %s\n", e.what()).c_str());
    //                        DeleteFile(jobInfo->GetCache()->m_file);
    //                    }

    //                    // Even though cached jobs don't get executed we still give the client the opportunity to handle the job state
    //                    // change in order to make the caching process transparent to the client
    //                    if (enumeration.has_value())
    //                    {
    //                        // Cache read successfully, this job will not be placed in the job queue
    //                        cachedJobs.emplace_back(Job(*jobInfo, AZStd::move(meta), AZStd::move(enumeration)));

    //                        if (m_clientJobCallback.has_value() && (*m_clientJobCallback)(*jobInfo, meta) == ProcessCallbackResult::Abort)
    //                        {
    //                            // Client chose to abort so we will copy over the existing cache enumerations and fill the rest with blanks
    //                            AZStd::vector<Job> jobs(cachedJobs);
    //                            for (auto emptyJobInfo = ++jobInfo; emptyJobInfo != jobInfos.end(); ++emptyJobInfo)
    //                            {
    //                                jobs.emplace_back(Job(*emptyJobInfo, {}, AZStd::nullopt));
    //                            }

    //                            return { ProcessSchedulerResult::UserAborted, jobs };
    //                        }
    //                    }
    //                    else
    //                    {
    //                        // The cache read failed and exception policy for cache read failures is not to throw so instead place this
    //                        // job in the job queue
    //                        jobQueue.emplace_back(*jobInfo);
    //                    }
    //                }
    //                else
    //                {
    //                    // This job has no cache read policy so delete the cache and place in job queue
    //                    DeleteFile(jobInfo->GetCache()->m_file);
    //                    jobQueue.emplace_back(*jobInfo);
    //                }
    //            }
    //            else
    //            {
    //                // This job has no cache read policy so delete the cache and place in job queue
    //                DeleteFile(jobInfo->GetCache()->m_file);
    //                jobQueue.emplace_back(*jobInfo);
    //            }
    //        }

    //        /* As per comment on PR51, this suggestion will be explored once the test coverage code for this subsystem is revisited
    //        bool aborted = false;
    //        for (const auto& jobInfo : jobInfos)
    //        {
    //            if (!jobInfo->GetCache().has_value() ||
    //                 jobInfo->GetCache()->m_policy != JobData::CachePolicy::Read)
    //            {
    //                DeleteFile(jobInfo->GetCache()->m_file);
    //                jobQueue.emplace_back(*jobInfo);
    //                continue;
    //            }

    //            ... // try catch part
    //            if (!enumeration.has_value())
    //            {
    //                jobQueue.emplace_back(*jobInfo);
    //                continue;
    //            }

    //            // Cache read successfully, this job will not be placed in the job queue
    //            cachedJobs.emplace_back(Job(*jobInfo, AZStd::move(meta), AZStd::move(enumeration)));

    //            if (m_clientJobCallback.has_value() && (*m_clientJobCallback)(*jobInfo, meta) == ProcessCallbackResult::Abort)
    //            {
    //               aborted = true; // catch the index too
    //               break;
    //            }
    //        }

    //        if (aborted)
    //        {
    //            // do the abortion part

    //            return { ProcessSchedulerResult::UserAborted, jobs };
    //        }
    //        */

    //        const auto payloadGenerator = [](const JobDataMap& jobDataMap)
    //        {
    //            PayloadMap<Job> enumerations;
    //            for (const auto& [jobId, jobData] : jobDataMap)
    //            {
    //                const auto& [meta, jobInfo] = jobData;
    //                if (meta.m_result == JobResult::ExecutedWithSuccess)
    //                {
    //                    try
    //                    {
    //                        const auto& enumeration =
    //                            (enumerations[jobId] = TestEnumeration(GTest::TestEnumerationSuitesFactory(
    //                                 ReadFileContents<TestRunnerException>(jobInfo->GetEnumerationArtifactPath()))));

    //                        // Write out the enumeration to a cache file if we have a cache write policy for this job
    //                        if (jobInfo->GetCache().has_value() && jobInfo->GetCache()->m_policy == JobData::CachePolicy::Write)
    //                        {
    //                            WriteFileContents<TestRunnerException>(
    //                                SerializeTestEnumeration(enumeration.value()), jobInfo->GetCache()->m_file);
    //                        }
    //                    } catch ([[maybe_unused]] const Exception& e)
    //                    {
    //                        AZ_Warning("Enumerate", false, e.what());
    //                        enumerations[jobId] = AZStd::nullopt;
    //                    }
    //                }
    //            }

    //            return enumerations;
    //        };

    //        // Generate the enumeration results for the jobs that weren't cached
    //        auto [result, jobs] = ExecuteJobs(
    //            jobQueue, payloadGenerator, StdOutputRouting::None, StdErrorRouting::None, enumerationTimeout, enumeratorTimeout,
    //            clientCallback, AZStd::nullopt);

    //        // We need to add the cached jobs to the completed job list even though they technically weren't executed
    //        for (auto&& job : cachedJobs)
    //        {
    //            jobs.emplace_back(AZStd::move(job));
    //        }

    //        return { result, jobs };
    //    }
    //};
} // namespace TestImpact
