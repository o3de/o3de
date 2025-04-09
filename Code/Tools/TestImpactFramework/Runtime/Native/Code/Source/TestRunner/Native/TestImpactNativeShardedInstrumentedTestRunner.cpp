/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TestRunner/Common/Job/TestImpactTestJobInfoUtils.h>
#include <TestRunner/Native/TestImpactNativeShardedInstrumentedTestRunner.h>

namespace TestImpact
{
    typename NativeInstrumentedTestRunner::ResultType NativeShardedInstrumentedTestRunner::ConsolidateSubJobs(
        const typename NativeInstrumentedTestRunner::ResultType& result,
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
                const auto& [subTestRun, subTestCoverage] = payload.value();
                const auto shardedTestJobInfo = shardToParentShardedJobMap.at(subJob.GetJobInfo().GetId().m_value);
                const auto parentJobInfoId = shardedTestJobInfo->GetId();
                auto& [testSuites, testCoverage] = consolidatedJobArtifacts[parentJobInfoId.m_value];

                // Accumulate test results
                if (subTestRun.has_value())
                {
                    for (const auto& subTestSuite : subTestRun->GetTestSuites())
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

                // Accumulate test coverage
                for (const auto& subModuleCoverage : subTestCoverage.GetModuleCoverages())
                {
                    auto& moduleCoverage = testCoverage[subModuleCoverage.m_path];
                    if (moduleCoverage.m_path.empty())
                    {
                        moduleCoverage.m_path = subModuleCoverage.m_path;
                        moduleCoverage.m_sources.insert(
                            moduleCoverage.m_sources.end(), subModuleCoverage.m_sources.begin(), subModuleCoverage.m_sources.end());
                    }
                }
            }
            else
            {
                LogSuspectedShardFileRaceCondition(subJob, shardToParentShardedJobMap, completedShardMap);
            }
        }

        AZStd::vector<typename NativeInstrumentedTestRunner::Job> consolidatedJobs;
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

            // Consolidate test coverages
            AZStd::vector<ModuleCoverage> moduleCoverages;
            moduleCoverages.reserve(testCoverage.size());
            for (auto&& [modulePath, moduleCoverage] : testCoverage)
            {
                moduleCoverages.emplace_back(AZStd::move(moduleCoverage));
            }

            auto payload =
                typename NativeInstrumentedTestRunner::JobPayload{ AZStd::move(run), TestCoverage(AZStd::move(moduleCoverages)) };

            if (shardedTestJobInfo->GetJobInfos().size() > 1)
            {
                // Serialize the consolidated run and coverage as artifacts in the canonical run and coverage directories
                WriteFileContents<TestRunnerException>(
                    Cobertura::SerializeTestCoverage(payload.second, m_repoRoot),
                    m_artifactDir.m_coverageArtifactDirectory / RepoPath(shardedTestJobInfo->GetTestTarget()->GetName() + ".xml"));
                if (payload.first.has_value())
                {
                    WriteFileContents<TestRunnerException>(
                        GTest::SerializeTestRun(payload.first.value()),
                        m_artifactDir.m_testRunArtifactDirectory /
                            RepoPath(GenerateFullQualifiedTargetNameStem(shardedTestJobInfo->GetTestTarget()).String() + ".xml"));
                }
            }

            consolidatedJobs.emplace_back(jobData->m_jobInfo, JobMeta{ jobData->m_meta }, AZStd::move(payload));
        }

        return { result.first, consolidatedJobs };
    }
} // namespace TestImpact
