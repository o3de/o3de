/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <TestImpactFramework/TestImpactRuntime.h>
#include <TestImpactFramework/TestImpactUtils.h>

#include <Artifact/TestImpactArtifactException.h>
#include <TestEngine/TestImpactTestEngineException.h>
#include <TestRunner/Common/Enumeration/TestImpactTestEnumeration.h>
#include <TestRunner/Common/Job/TestImpactTestEnumerationJobData.h>
#include <TestRunner/Common/Job/TestImpactTestJobRunner.h>

namespace TestImpact
{
    //! Enumerate a batch of test targets to determine the test suites and fixtures they contain, caching the results where applicable.
    template<typename AdditionalInfo>
    class TestEnumerator
        : public TestJobRunner<AdditionalInfo, TestEnumeration>
    {
    public:
        using TestJobRunner = TestJobRunner<AdditionalInfo, TestEnumeration>;
        using TestJobRunner::TestJobRunner;

        //! Executes the specified test enumeration jobs according to the specified cache and job exception policies.
        //! @param jobInfos The enumeration jobs to execute.
        //! @param stdOutRouting The standard output routing to be specified for all jobs.
        //! @param stdErrRouting The standard error routing to be specified for all jobs.
        //! @param cacheExceptionPolicy The cache exception policy to be used for this run.
        //! @param jobExceptionPolicy The enumeration job exception policy to be used for this run.
        //! @param enumerationTimeout The maximum duration an enumeration may be in-flight for before being forcefully terminated.
        //! @param enumeratorTimeout The maximum duration the enumerator may run before forcefully terminating all in-flight enumerations.
        //! @param clientCallback The optional client callback to be called whenever an enumeration job changes state.
        //! @param stdContentCallback 
        //! @return The result of the run sequence and the enumeration jobs with their associated test enumeration payloads.
        AZStd::pair<ProcessSchedulerResult, AZStd::vector<typename TestJobRunner::Job>> Enumerate(
            const AZStd::vector<typename TestJobRunner::JobInfo>& jobInfos,
            StdOutputRouting stdOutRouting,
            StdErrorRouting stdErrRouting,
            AZStd::optional<AZStd::chrono::milliseconds> enumerationTimeout,
            AZStd::optional<AZStd::chrono::milliseconds> enumeratorTimeout,
            AZStd::optional<typename TestJobRunner::JobCallback> clientCallback,
            AZStd::optional<typename TestJobRunner::StdContentCallback> stdContentCallback)
        {
            AZStd::vector<typename TestJobRunner::Job> cachedJobs;
            AZStd::vector<typename TestJobRunner::JobInfo> jobQueue;

            for (auto jobInfo = jobInfos.begin(); jobInfo != jobInfos.end(); ++jobInfo)
            {
                // If this job has a cache read policy attempt to read the cache
                if (jobInfo->GetCache().has_value())
                {
                    if (jobInfo->GetCache()->m_policy == TestEnumerationJobData::CachePolicy::Read)
                    {
                        JobMeta meta;
                        AZStd::optional<TestEnumeration> enumeration;

                        try
                        {
                            enumeration = TestEnumeration(
                                DeserializeTestEnumeration(ReadFileContents<TestEngineException>(jobInfo->GetCache()->m_file)));
                        } catch (const TestEngineException& e)
                        {
                            AZ_Printf("Enumerate", AZStd::string::format("Enumeration cache error: %s\n", e.what()).c_str());
                            DeleteFile(jobInfo->GetCache()->m_file);
                        }

                        // Even though cached jobs don't get executed we still give the client the opportunity to handle the job state
                        // change in order to make the caching process transparent to the client
                        if (enumeration.has_value())
                        {
                            // Cache read successfully, this job will not be placed in the job queue
                            cachedJobs.emplace_back(Job(*jobInfo, AZStd::move(meta), AZStd::move(enumeration)));

                            if (clientCallback.has_value() && (*clientCallback)(*jobInfo, meta) == ProcessCallbackResult::Abort)
                            {
                                // Client chose to abort so we will copy over the existing cache enumerations and fill the rest with blanks
                                AZStd::vector<Job> jobs(cachedJobs);
                                for (auto emptyJobInfo = ++jobInfo; emptyJobInfo != jobInfos.end(); ++emptyJobInfo)
                                {
                                    jobs.emplace_back(Job(*emptyJobInfo, {}, AZStd::nullopt));
                                }

                                return { ProcessSchedulerResult::UserAborted, jobs };
                            }
                        }
                        else
                        {
                            // The cache read failed and exception policy for cache read failures is not to throw so instead place this
                            // job in the job queue
                            jobQueue.emplace_back(*jobInfo);
                        }
                    }
                    else
                    {
                        // This job has no cache read policy so delete the cache and place in job queue
                        DeleteFile(jobInfo->GetCache()->m_file);
                        jobQueue.emplace_back(*jobInfo);
                    }
                }
                else
                {
                    // This job has no cache read policy so delete the cache and place in job queue
                    DeleteFile(jobInfo->GetCache()->m_file);
                    jobQueue.emplace_back(*jobInfo);
                }
            }

            /* As per comment on PR51, this suggestion will be explored once the test coverage code for this subsystem is revisited
            bool aborted = false;
            for (const auto& jobInfo : jobInfos)
            {
                if (!jobInfo->GetCache().has_value() ||
                     jobInfo->GetCache()->m_policy != JobData::CachePolicy::Read)
                {
                    DeleteFile(jobInfo->GetCache()->m_file);
                    jobQueue.emplace_back(*jobInfo);
                    continue;
                }

                ... // try catch part
                if (!enumeration.has_value())
                {
                    jobQueue.emplace_back(*jobInfo);
                    continue;
                }

                // Cache read successfully, this job will not be placed in the job queue
                cachedJobs.emplace_back(Job(*jobInfo, AZStd::move(meta), AZStd::move(enumeration)));

                if (m_clientJobCallback.has_value() && (*m_clientJobCallback)(*jobInfo, meta) == ProcessCallbackResult::Abort)
                {
                   aborted = true; // catch the index too
                   break;
                }
            }

            if (aborted)
            {
                // do the abortion part

                return { ProcessSchedulerResult::UserAborted, jobs };
            }
            */

            const auto payloadGenerator = [](const typename TestJobRunner::JobDataMap& jobDataMap)
            {
                typename TestJobRunner::PayloadMap enumerations;
                for (const auto& [jobId, jobData] : jobDataMap)
                {
                    const auto& [meta, jobInfo] = jobData;
                    if (meta.m_result == JobResult::ExecutedWithSuccess)
                    {
                        if (auto outcome = PayloadFactory<AdditionalInfo, TestEnumeration>(*jobInfo, meta);
                            outcome.IsSuccess())
                        {
                            const auto& enumeration = (enumerations[jobId] = AZStd::move(outcome.TakeValue()));
                            
                            // Write out the enumeration to a cache file if we have a cache write policy for this job
                            if (jobInfo->GetCache().has_value() &&
                                jobInfo->GetCache()->m_policy == TestEnumerationJobData::CachePolicy::Write)
                            {
                                WriteFileContents<TestEngineException>(
                                    SerializeTestEnumeration(enumeration.value()), jobInfo->GetCache()->m_file);
                            }
                        }
                        else
                        {
                            AZ_Warning("Enumerate", false, outcome.GetError().c_str());
                            enumerations[jobId] = AZStd::nullopt;
                        }
                    }
                }

                return enumerations;
            };

            // Generate the enumeration results for the jobs that weren't cached
            auto [result, jobs] = this->m_jobRunner.Execute(
                jobQueue,
                payloadGenerator,
                stdOutRouting,
                stdErrRouting,
                enumerationTimeout,
                enumeratorTimeout,
                clientCallback,
                stdContentCallback);

            // We need to add the cached jobs to the completed job list even though they technically weren't executed
            for (auto&& job : cachedJobs)
            {
                jobs.emplace_back(AZStd::move(job));
            }

            return { result, jobs };
        }
    };
} // namespace TestImpact
