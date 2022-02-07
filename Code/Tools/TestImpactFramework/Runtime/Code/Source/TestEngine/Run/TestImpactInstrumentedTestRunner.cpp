/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TestImpactFramework/TestImpactUtils.h>

#include <Artifact/Factory/TestImpactModuleCoverageFactory.h>
#include <Artifact/Factory/TestImpactTestRunSuiteFactory.h>
#include <TestEngine/TestImpactTestEngineException.h>
#include <TestEngine/Run/TestImpactInstrumentedTestRunner.h>
#include <TestEngine/Run/TestImpactTestRunSerializer.h>

#include <AzCore/IO/SystemFile.h>

namespace TestImpact
{
    InstrumentedTestRunJobData::InstrumentedTestRunJobData(const RepoPath& resultsArtifact, const RepoPath& coverageArtifact)
        : TestRunJobData(resultsArtifact)
        , m_coverageArtifact(coverageArtifact)
    {
    }

    const RepoPath& InstrumentedTestRunJobData::GetCoverageArtifactPath() const
    {
        return m_coverageArtifact;
    }

    InstrumentedTestRunner::InstrumentedTestRunner(size_t maxConcurrentRuns)
        : JobRunner(maxConcurrentRuns)
    {
    }

    AZStd::pair<ProcessSchedulerResult, AZStd::vector<InstrumentedTestRunner::Job>> InstrumentedTestRunner::RunInstrumentedTests(
        const AZStd::vector<JobInfo>& jobInfos,
        AZStd::optional<AZStd::chrono::milliseconds> runTimeout,
        AZStd::optional<AZStd::chrono::milliseconds> runnerTimeout,
        AZStd::optional<ClientJobCallback> clientCallback)
    {
        const auto payloadGenerator = [](const JobDataMap& jobDataMap)
        {
            PayloadMap<Job> runs;
            for (const auto& [jobId, jobData] : jobDataMap)
            {
                const auto& [meta, jobInfo] = jobData;
                if (meta.m_result == JobResult::ExecutedWithSuccess || meta.m_result == JobResult::ExecutedWithFailure)
                {
                    const auto printException = [](const Exception& e)
                    {
                        AZ_Printf("RunInstrumentedTests", AZStd::string::format("%s\n.", e.what()).c_str());
                    };

                    AZStd::optional<TestRun> run;
                    try
                    {
                        run = TestRun(
                            GTest::TestRunSuitesFactory(ReadFileContents<TestEngineException>(jobInfo->GetRunArtifactPath())),
                            meta.m_duration.value());
                    }
                    catch (const Exception& e)
                    {
                        // No run result is not necessarily a failure (e.g. test targets not using gtest)
                        printException(e);
                    }

                    try
                    {
                        AZStd::vector<ModuleCoverage> moduleCoverages =
                            Cobertura::ModuleCoveragesFactory(ReadFileContents<TestEngineException>(jobInfo->GetCoverageArtifactPath()));
                        TestCoverage coverage(AZStd::move(moduleCoverages));
                        runs[jobId] = { run, AZStd::move(coverage) };
                    }
                    catch (const Exception& e)
                    {
                        printException(e);
                        // No coverage, however, is a failure
                        runs[jobId] = AZStd::nullopt;
                    }
                }
            }

            return runs;
        };

        return ExecuteJobs(
            jobInfos,
            payloadGenerator,
            StdOutputRouting::None,
            StdErrorRouting::None,
            runTimeout,
            runnerTimeout,
            clientCallback,
            AZStd::nullopt);
    }
} // namespace TestImpact
