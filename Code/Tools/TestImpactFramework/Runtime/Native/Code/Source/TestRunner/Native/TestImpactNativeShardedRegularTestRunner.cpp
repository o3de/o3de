/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TestRunner/Common/Job/TestImpactTestJobInfoUtils.h>
#include <TestRunner/Native/TestImpactNativeShardedRegularTestRunner.h>

namespace TestImpact
{
    typename NativeRegularTestRunner::ResultType NativeShardedRegularTestRunner::ConsolidateSubJobs(
        const typename NativeRegularTestRunner::ResultType& result,
        const ShardToParentShardedJobMap& shardToParentShardedJobMap,
        const CompletedShardMap& completedShardMap)
    {
        const auto& [returnCode, subJobs] = result;
        AZStd::unordered_map<
            typename JobId::IdType,
            std::pair<AZStd::unordered_map<AZStd::string, TestRunSuite>, AZStd::unordered_map<RepoPath, ModuleCoverage>>>
            consolidatedJobArtifacts;

        for (const auto& subJob : subJobs)
        {
            if (const auto payload = subJob.GetPayload();
                payload.has_value())
            {
                const auto& subTestRun = payload.value();
                const auto shardedTestJobInfo = shardToParentShardedJobMap.at(subJob.GetJobInfo().GetId().m_value);
                const auto parentJobInfoId = shardedTestJobInfo->GetId();
                auto& [testSuites, testCoverage] = consolidatedJobArtifacts[parentJobInfoId.m_value];

                // Accumulate test results
                for (const auto& subTestSuite : subTestRun.GetTestSuites())
                {
                    auto& testSuite = testSuites[subTestSuite.m_name];
                    if (testSuite.m_name.empty())
                    {
                        testSuite.m_name = subTestSuite.m_name;
                    }

                    testSuite.m_enabled = subTestSuite.m_enabled;
                    testSuite.m_duration += subTestSuite.m_duration;
                    testSuite.m_tests.insert(testSuite.m_tests.end(), subTestSuite.m_tests.begin(), subTestSuite.m_tests.end());
                }
            }
            else
            {
                LogSuspectedShardFileRaceCondition(subJob, shardToParentShardedJobMap, completedShardMap);
            }
        }

        AZStd::vector<typename NativeRegularTestRunner::Job> consolidatedJobs;
        consolidatedJobs.reserve(consolidatedJobArtifacts.size());

        for (auto&& [jobId, artifacts] : consolidatedJobArtifacts)
        {
            auto&& [testSuites, testCoverage] = artifacts;
            const auto shardedTestJobInfo = shardToParentShardedJobMap.at(jobId);
            const auto& shardedTestJob = completedShardMap.at(shardedTestJobInfo);
            const auto& jobData = shardedTestJob.GetConsolidatedJobData();

            // Consolidate test runs
            AZStd::optional<TestRun> run;
            if (testSuites.size())
            {
                AZStd::vector<TestRunSuite> suites;
                suites.reserve(testSuites.size());
                for (auto&& [suiteName, suite] : testSuites)
                {
                    suites.emplace_back(AZStd::move(suite));
                }

                if (jobData.has_value())
                {
                    run = TestRun(AZStd::move(suites), jobData->m_meta.m_duration.value_or(AZStd::chrono::milliseconds{ 0 }));
                }
            }

            // Serialize the consolidated run as and artifact in the canonical run directory
            if (run.has_value() && shardedTestJobInfo->GetJobInfos().size() > 1)
            {
                WriteFileContents<TestRunnerException>(
                    GTest::SerializeTestRun(run.value()),
                    m_artifactDir.m_testRunArtifactDirectory /
                        RepoPath(GenerateFullQualifiedTargetNameStem(shardedTestJobInfo->GetTestTarget()).String() + ".xml"));
            }

            consolidatedJobs.emplace_back(jobData->m_jobInfo, JobMeta{ jobData->m_meta }, AZStd::move(run));
        }

        return { result.first, consolidatedJobs };
    }
} // namespace TestImpact
