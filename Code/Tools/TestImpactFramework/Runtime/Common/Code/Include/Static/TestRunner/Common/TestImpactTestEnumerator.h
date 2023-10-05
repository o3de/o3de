/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <TestImpactFramework/TestImpactUtils.h>

#include <Artifact/TestImpactArtifactException.h>
#include <TestRunner/Common/TestImpactTestRunnerException.h>
#include <TestRunner/Common/Enumeration/TestImpactTestEnumeration.h>
#include <TestRunner/Common/Job/TestImpactTestEnumerationJobData.h>
#include <TestRunner/Common/Job/TestImpactTestJobRunner.h>
#include <TestRunner/Common/Enumeration/TestImpactTestEnumerationSerializer.h>

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
        using NotificationBus = typename TestJobRunner::NotificationBus;

        //! Executes the specified test enumeration jobs according to the specified cache and job exception policies.
        //! @param jobInfos The enumeration jobs to execute.
        //! @param stdOutRouting The standard output routing to be specified for all jobs.
        //! @param stdErrRouting The standard error routing to be specified for all jobs.
        //! @param enumerationTimeout The maximum duration an enumeration may be in-flight for before being forcefully terminated.
        //! @param enumeratorTimeout The maximum duration the enumerator may run before forcefully terminating all in-flight enumerations.
        //! @return The result of the run sequence and the enumeration jobs with their associated test enumeration payloads.
        [[nodiscard]] AZStd::pair<ProcessSchedulerResult, AZStd::vector<typename TestJobRunner::Job>> Enumerate(
            const typename TestJobRunner::JobInfos& jobInfos,
            StdOutputRouting stdOutRouting,
            StdErrorRouting stdErrRouting,
            AZStd::optional<AZStd::chrono::milliseconds> enumerationTimeout,
            AZStd::optional<AZStd::chrono::milliseconds> enumeratorTimeout);

    protected:
        //! Default implementation of payload producer for test enumerators.
        virtual typename TestJobRunner::PayloadMap PayloadMapProducer(const typename TestJobRunner::JobDataMap& jobDataMap);

        //! Extracts the payload outcome for a given job payload.
        virtual typename TestJobRunner::JobPayloadOutcome
        PayloadExtractor(const typename TestJobRunner::JobInfo& jobData, const JobMeta& jobMeta) = 0;
    };

    template<typename AdditionalInfo>
    AZStd::pair<ProcessSchedulerResult, AZStd::vector<typename TestEnumerator<AdditionalInfo>::TestJobRunner::Job>> TestEnumerator<
        AdditionalInfo>::
        Enumerate(
        const typename TestJobRunner::JobInfos& jobInfos,
        StdOutputRouting stdOutRouting,
        StdErrorRouting stdErrRouting,
        AZStd::optional<AZStd::chrono::milliseconds> enumerationTimeout,
        AZStd::optional<AZStd::chrono::milliseconds> enumeratorTimeout)
    {
        AZStd::vector<typename TestJobRunner::Job> cachedAndUnenumerableJobs;
        typename TestJobRunner::JobInfos jobQueue;

        for (auto jobInfo = jobInfos.begin(); jobInfo != jobInfos.end(); ++jobInfo)
        {
            // If this job doesn't support enumeration, add an empty enumeration
            if (jobInfo->GetCommand().m_args.empty())
            {
                // Test target cannot enumerate, this job will not be placed in the job queue
                const JobMeta JobMeta;
                cachedAndUnenumerableJobs.emplace_back(typename TestJobRunner::Job(*jobInfo, {}, AZStd::nullopt));

                AZ::EBusAggregateResults<ProcessCallbackResult> results;
                NotificationBus::BroadcastResult(results, &NotificationBus::Events::OnJobComplete, *jobInfo, JobMeta, StdContent{});
                if (GetAggregateProcessCallbackResult(results) == ProcessCallbackResult::Abort)
                {
                    // Client chose to abort so we will copy over the existing cache enumerations and fill the rest with blanks
                    AZStd::vector<typename TestJobRunner::Job> jobs(cachedAndUnenumerableJobs);
                    for (auto emptyJobInfo = ++jobInfo; emptyJobInfo != jobInfos.end(); ++emptyJobInfo)
                    {
                        jobs.emplace_back(typename TestJobRunner::Job(*emptyJobInfo, {}, AZStd::nullopt));
                    }

                    return { ProcessSchedulerResult::UserAborted, jobs };
                }
            }
            // If this job has a cache read policy attempt to read the cache
            else if (jobInfo->GetCache().has_value())
            {
                if (jobInfo->GetCache()->m_policy == TestEnumerationJobData::CachePolicy::Read)
                {
                    JobMeta meta;
                    AZStd::optional<TestEnumeration> enumeration;

                    try
                    {
                        enumeration =
                            TestEnumeration(DeserializeTestEnumeration(ReadFileContents<TestRunnerException>(jobInfo->GetCache()->m_file)));
                    } catch (const TestRunnerException& e)
                    {
                        AZ_Printf("Enumerate", AZStd::string::format("Enumeration cache error: %s\n", e.what()).c_str());
                        DeleteFile(jobInfo->GetCache()->m_file);
                    }

                    // Even though cached jobs don't get executed we still give the client the opportunity to handle the job state
                    // change in order to make the caching process transparent to the client
                    if (enumeration.has_value())
                    {
                        // Cache read successfully, this job will not be placed in the job queue
                        cachedAndUnenumerableJobs.emplace_back(
                            typename TestJobRunner::Job(*jobInfo, AZStd::move(meta), AZStd::move(enumeration)));

                        AZ::EBusAggregateResults<ProcessCallbackResult> results;
                        NotificationBus::BroadcastResult(results, &NotificationBus::Events::OnJobComplete, *jobInfo, meta, StdContent{});
                        if (GetAggregateProcessCallbackResult(results) == ProcessCallbackResult::Abort)
                        {
                            // Client chose to abort so we will copy over the existing cache enumerations and fill the rest with blanks
                            AZStd::vector<typename TestJobRunner::Job> jobs(cachedAndUnenumerableJobs);
                            for (auto emptyJobInfo = ++jobInfo; emptyJobInfo != jobInfos.end(); ++emptyJobInfo)
                            {
                                jobs.emplace_back(typename TestJobRunner::Job(*emptyJobInfo, {}, AZStd::nullopt));
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

        const auto payloadGenerator = [this](const typename TestJobRunner::JobDataMap& jobDataMap)
        {
            return PayloadMapProducer(jobDataMap);
        };

        // Generate the enumeration results for the jobs that weren't cached
        auto [result, jobs] = this->m_jobRunner.Execute(
            jobQueue,
            payloadGenerator,
            stdOutRouting,
            stdErrRouting,
            enumerationTimeout,
            enumeratorTimeout);

        // We need to add the cached and unenumerable jobs to the completed job list even though they technically weren't executed
        for (auto&& job : cachedAndUnenumerableJobs)
        {
            jobs.emplace_back(AZStd::move(job));
        }

        return { result, jobs };
    }

    template<typename AdditionalInfo>
    typename TestJobRunner<AdditionalInfo, TestEnumeration>::PayloadMap TestEnumerator<AdditionalInfo>::PayloadMapProducer(
        const typename TestJobRunner::JobDataMap& jobDataMap)
    {
        typename TestJobRunner::PayloadMap enumerations;
        for (const auto& [jobId, jobData] : jobDataMap)
        {
            const auto& [meta, jobInfo] = jobData;
            if (meta.m_result == JobResult::ExecutedWithSuccess)
            {
                if (auto outcome = PayloadExtractor(*jobInfo, meta); outcome.IsSuccess())
                {
                    const auto& enumeration = (enumerations[jobId] = AZStd::move(outcome.TakeValue()));

                    // Write out the enumeration to a cache file if we have a cache write policy for this job
                    if (jobInfo->GetCache().has_value() && jobInfo->GetCache()->m_policy == TestEnumerationJobData::CachePolicy::Write)
                    {
                        WriteFileContents<TestRunnerException>(SerializeTestEnumeration(enumeration.value()), jobInfo->GetCache()->m_file);
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
    }
} // namespace TestImpact
