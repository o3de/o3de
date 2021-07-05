/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TestImpactFramework/TestImpactFileUtils.h>

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

    InstrumentedTestRunner::JobPayload ParseTestRunAndCoverageFiles(
        const RepoPath& runFile,
        const RepoPath& coverageFile,
        AZStd::chrono::milliseconds duration)
    {
        TestRun run(GTest::TestRunSuitesFactory(ReadFileContents<TestEngineException>(runFile)), duration);
        AZStd::vector<ModuleCoverage> moduleCoverages = Cobertura::ModuleCoveragesFactory(ReadFileContents<TestEngineException>(coverageFile));
        TestCoverage coverage(AZStd::move(moduleCoverages));
        return {AZStd::move(run), AZStd::move(coverage)};
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
        const auto payloadGenerator = [this](const JobDataMap& jobDataMap)
        {
            PayloadMap<Job> runs;
            for (const auto& [jobId, jobData] : jobDataMap)
            {
                const auto& [meta, jobInfo] = jobData;
                if (meta.m_result == JobResult::ExecutedWithSuccess || meta.m_result == JobResult::ExecutedWithFailure)
                {
                    try
                    {
                        runs[jobId] = ParseTestRunAndCoverageFiles(
                            jobInfo->GetRunArtifactPath(),
                            jobInfo->GetCoverageArtifactPath(),
                            meta.m_duration.value());
                    }
                    catch (const Exception& e)
                    {
                        AZ_Printf("RunInstrumentedTests", AZStd::string::format("%s\n", e.what()).c_str());
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
