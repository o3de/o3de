/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <TestRunner/Common/Run/TestImpactTestCoverageSerializer.h>
#include <TestRunner/Common/Run/TestImpactTestRunSerializer.h>
#include <TestRunner/Native/Job/TestImpactNativeShardedTestJobInfoGenerator.h>
#include <TestRunner/Native/Shard/TestImpactNativeShardedTestJob.h>

#include <AzCore/std/numeric.h>

namespace TestImpact
{
    //! Base class for all sharded test runners.
    template<typename TestRunnerType>
    class NativeShardedTestRunnerBase
    {
    public:
        //! Public-facing `JobInfo` for interoperability with the various helper functions and event handlers.
        using JobInfo = typename TestRunnerType::JobInfo;

        //! The actual collection of `JobInfo`s condumed by this runner.
        using JobInfos = AZStd::vector<ShardedTestJobInfo<TestRunnerType>>;

        using JobId = typename JobInfo::Id;
        using Job = typename TestRunnerType::Job;

        //! Constructs the sharded test system to wrap around the specified test runner.
        NativeShardedTestRunnerBase(TestRunnerType& testRunner, const RepoPath& repoRoot, const ArtifactDir& artifactDir);
        virtual ~NativeShardedTestRunnerBase() = default;

        //! Wrapper around the test runner's `RunTests` method to present the sharded test running interface to the user.
        [[nodiscard]] AZStd::pair<ProcessSchedulerResult, AZStd::vector<Job>> RunTests(
            const JobInfos& shardedJobInfos,
            StdOutputRouting stdOutRouting,
            StdErrorRouting stdErrRouting,
            AZStd::optional<AZStd::chrono::milliseconds> runTimeout,
            AZStd::optional<AZStd::chrono::milliseconds> runnerTimeout);

        //! Bus for native sharded test system notifications.
        class Notifications
            : public AZ::EBusTraits
        {
        public:
            // EBusTraits overrides ...
            static constexpr AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
            static constexpr AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;

            //! Callback for sharded job completion/failure.
            //! @param jobInfo The job information associated with this job.
            //! @param meta The meta-data about the job run.
            //! @param std The standard output and standard error of the process running the job.
            virtual ProcessCallbackResult OnJobComplete(
                [[maybe_unused]] const JobInfo& jobInfo,
                [[maybe_unused]] const JobMeta& meta,
                [[maybe_unused]] const StdContent& std)
            {
                return ProcessCallbackResult::Continue;
            }

            //! Callback for sharded sub-job completion/failure.
            //! @param subJobCount The number of sub-jobs that make up this job.
            //! @param jobId The id of the sharded job.
            //! @param subJobInfo The job information associated with this sharded sub-job.
            //! @param subJobMeta The meta-data about the sharded sub-job run.
            //! @param subJobStd The standard output and standard error of the process running the sharded sub-job.
            virtual ProcessCallbackResult OnShardedJobComplete(
                [[maybe_unused]] JobId jobId,
                [[maybe_unused]] size_t subJobCount,
                [[maybe_unused]] const JobInfo& subJobInfo,
                [[maybe_unused]] const JobMeta& subJobMeta,
                [[maybe_unused]] const StdContent& subJobStd)
            {
                return ProcessCallbackResult::Continue;
            }
        };

        using NotificationBus = AZ::EBus<Notifications>;

    protected:
        //! Map of sharded sub jobs to the parent sharded test job info.
        using ShardToParentShardedJobMap = AZStd::unordered_map<typename JobId::IdType, const ShardedTestJobInfo<TestRunnerType>*>;

        //! Map of sharded test job infos to their sharded test jobs.
        using CompletedShardMap = AZStd::unordered_map<const ShardedTestJobInfo<TestRunnerType>*, ShardedTestJob<TestRunnerType>>;

        //! Helper function to aid in debugging flakey test targets that may contain file race conditions when sharded.
        //! @note It's actually impossible to definitively say at this level if a sharded sub job failed due to a race condition
        //! but provide some helpful output to aid in debugging in any case. The idea behind this function is to print out the last
        //! chunk of standard output produced by the offending sub job as if the output suddenly terminates, it means the shard 
        //! crashed (possible due to file race conditions with other shards) but if the output ends gracefully (i.e. with a test
        //! framework summary of the test run) then it failed to produce the requisite test run artifacts for unrelated reasons.
        static void LogSuspectedShardFileRaceCondition(
            const Job& subJob, const ShardToParentShardedJobMap& shardToParentShardedJobMap, const CompletedShardMap& completedShardMap);

        //! Consolidates the sharded sub job artifacts into one set of artifacts per test target.
        [[nodiscard]] virtual typename TestRunnerType::ResultType ConsolidateSubJobs(
            const typename TestRunnerType::ResultType& result,
            const ShardToParentShardedJobMap& shardToParentShardedJobMap,
            const CompletedShardMap& completedShardMap) = 0;

        RepoPath m_repoRoot; //!< Path to repo root.
        ArtifactDir m_artifactDir; //!< Path to repo artifact directory.

    private:
        //! Handler for test job runner notifications.
        class TestJobRunnerNotificationHandler;

        TestRunnerType* m_testRunner = nullptr; //!< Pointer to the unerlying (non-sharded) test runner used by this sharded runner.
    };

    template<typename TestRunnerType>
    class NativeShardedTestRunnerBase<TestRunnerType>::TestJobRunnerNotificationHandler
        : private TestRunnerType::NotificationBus::Handler
    {
    public:
        using JobInfo = typename TestRunnerType::JobInfo;

        TestJobRunnerNotificationHandler(ShardToParentShardedJobMap& shardToParentShardedJobMap, CompletedShardMap& completedShardMap);
        virtual ~TestJobRunnerNotificationHandler();

    private:
        // NotificationsBus overrides ...
        ProcessCallbackResult OnJobComplete(
            const typename TestRunnerType::Job::Info& jobInfo, const JobMeta& meta, const StdContent& std) override;

        ShardToParentShardedJobMap* m_shardToParentShardedJobMap = nullptr;
        CompletedShardMap* m_completedShardMap = nullptr;
    };

    template<typename TestRunnerType>
    NativeShardedTestRunnerBase<TestRunnerType>::TestJobRunnerNotificationHandler::TestJobRunnerNotificationHandler(
        ShardToParentShardedJobMap& shardToParentShardedJobMap, CompletedShardMap& completedShardMap)
        : m_shardToParentShardedJobMap(&shardToParentShardedJobMap)
        , m_completedShardMap(&completedShardMap)
    {
        TestRunnerType::NotificationBus::Handler::BusConnect();
    }

    template<typename TestRunnerType>
    NativeShardedTestRunnerBase<TestRunnerType>::TestJobRunnerNotificationHandler::~TestJobRunnerNotificationHandler()
    {
        TestRunnerType::NotificationBus::Handler::BusDisconnect();
    }

    template<typename TestRunnerType>
    ProcessCallbackResult NativeShardedTestRunnerBase<TestRunnerType>::TestJobRunnerNotificationHandler::OnJobComplete(
        const typename TestRunnerType::Job::Info& jobInfo, const JobMeta& meta, const StdContent& std)
    {
        const auto& shardedJobInfo = m_shardToParentShardedJobMap->at(jobInfo.GetId().m_value);
        auto& shardedTestJob = m_completedShardMap->at(shardedJobInfo);
        
        {
            AZ::EBusAggregateResults<ProcessCallbackResult> results;
            NotificationBus::BroadcastResult(
                results,
                &NotificationBus::Events::OnShardedJobComplete,
                shardedJobInfo->GetJobInfos().begin()->GetId(),
                shardedJobInfo->GetJobInfos().size(),
                jobInfo,
                meta,
                std);

            const auto result = GetAggregateProcessCallbackResult(results);
            if (result == ProcessCallbackResult::Abort)
            {
                return result;
            }
        }

        shardedTestJob.RegisterCompletedSubJob(jobInfo, meta, std);

        if (shardedTestJob.IsComplete())
        {
            auto& consolidatedJobData = *shardedTestJob.GetConsolidatedJobData();
            AZ::EBusAggregateResults<ProcessCallbackResult> results;
            NotificationBus::BroadcastResult(
                results,
                &NotificationBus::Events::OnJobComplete,
                consolidatedJobData.m_jobInfo,
                consolidatedJobData.m_meta,
                consolidatedJobData.m_std);
            return GetAggregateProcessCallbackResult(results);
        }

        return ProcessCallbackResult::Continue;
    }

    template<typename TestRunnerType>
    NativeShardedTestRunnerBase<TestRunnerType>::NativeShardedTestRunnerBase(
        TestRunnerType& testRunner, const RepoPath& repoRoot, const ArtifactDir& artifactDir)
        : m_testRunner(&testRunner)
        , m_repoRoot(repoRoot)
        , m_artifactDir(artifactDir)
    {
    }

    template<typename TestRunnerType>
    AZStd::pair<ProcessSchedulerResult, AZStd::vector<typename TestRunnerType::Job>> NativeShardedTestRunnerBase<TestRunnerType>::RunTests(
        const JobInfos& shardedJobInfos,
        StdOutputRouting stdOutRouting,
        StdErrorRouting stdErrRouting,
        AZStd::optional<AZStd::chrono::milliseconds> runTimeout,
        AZStd::optional<AZStd::chrono::milliseconds> runnerTimeout)
    {
        ShardToParentShardedJobMap shardToParentShardedJobMap;
        CompletedShardMap completedShardMap;

        // Calculate the total number of shards across all test targets in this run
        const auto totalJobShards = AZStd::accumulate(
            shardedJobInfos.begin(),
            shardedJobInfos.end(),
            size_t{ 0 },
            [](size_t sum, const ShardedTestJobInfo<TestRunnerType>& shardedJobInfo)
            {
                return sum + shardedJobInfo.GetJobInfos().size();
            });

        // Prepare the shard and completed job maps for this run
        AZStd::vector<JobInfo> subJobInfos;
        subJobInfos.reserve(totalJobShards);
        for (const auto& shardedJobInfo : shardedJobInfos)
        {
            completedShardMap.emplace(
                AZStd::piecewise_construct, AZStd::forward_as_tuple(&shardedJobInfo), std::forward_as_tuple(shardedJobInfo));
            const auto& jobInfos = shardedJobInfo.GetJobInfos();
            subJobInfos.insert(subJobInfos.end(), jobInfos.begin(), jobInfos.end());
            for (const auto& jobInfo : jobInfos)
            {
                shardToParentShardedJobMap[jobInfo.GetId().m_value] = &shardedJobInfo;
            }
        }

        // Run each shard as a test run in the underlying standard test runner for this sharded test runenr type
        TestJobRunnerNotificationHandler handler(shardToParentShardedJobMap, completedShardMap);
        const auto result = m_testRunner->RunTests(
            subJobInfos,
            stdOutRouting,
            stdErrRouting,
            runTimeout,
            runnerTimeout);

        // Return the consolidated test run jobs to transparently to the caller
        return ConsolidateSubJobs(result, shardToParentShardedJobMap, completedShardMap);
    }

    template<typename TestRunnerType>
    void NativeShardedTestRunnerBase<TestRunnerType>::LogSuspectedShardFileRaceCondition(
        const Job& subJob, const ShardToParentShardedJobMap& shardToParentShardedJobMap, const CompletedShardMap& completedShardMap)
    {
        const auto jobId = subJob.GetJobInfo().GetId().m_value;
        const auto shardedTestJobInfo = shardToParentShardedJobMap.at(jobId);
        const auto& shardedTestJob = completedShardMap.at(shardedTestJobInfo);
        const auto& shardedSubJobs = shardedTestJob.GetSubJobs();

        // Try and find the offending sharded sub job
        auto jobData = AZStd::find_if(
            shardedSubJobs.begin(),
            shardedSubJobs.end(),
            [&](const typename ShardedTestJob<TestRunnerType>::JobData& jobData)
            {
                return jobData.m_jobInfo.GetId().m_value == jobId;
            });

        // This really should't fail but check anyway
        if (jobData != shardedSubJobs.end())
        {
            [[maybe_unused]] const size_t shardNumber =
                jobData->m_jobInfo.GetId().m_value - shardedTestJobInfo->GetJobInfos().front().GetId().m_value;

            if (jobData->m_std.m_out.has_value())
            {
                // Offending sub job has std output available, print a truncated summary of the last 500 characters of known output
                constexpr size_t numCharsToPrint = 500;
                const size_t subStringLength = AZStd::min(size_t{ numCharsToPrint }, jobData->m_std.m_out->length());
                const auto subString = jobData->m_std.m_out->substr(jobData->m_std.m_out->length() - subStringLength);
                AZ_Warning(
                    "Shard",
                    false,
                    AZStd::string::format(
                        "Possible file race condition detected for test target '%s' on shard '%zu', backtrace of std out for last %zu "
                        "characters (check for properly terminated test log output):\n%s",
                        shardedTestJobInfo->GetTestTarget()->GetName().c_str(),
                        shardNumber,
                        subStringLength,
                        subString.c_str())
                        .c_str());
            }
            else
            {
                // Offending sub job has no std output available, happy hunting!
                AZ_Warning(
                    "Shard",
                    false,
                    AZStd::string::format(
                        "Possible race condition detected for test target '%s' on shard '%zu', backtrace of std out unavailable",
                        shardedTestJobInfo->GetTestTarget()->GetName().c_str(),
                        shardNumber)
                        .c_str());
            }
        }
    }
} // namespace TestImpact
