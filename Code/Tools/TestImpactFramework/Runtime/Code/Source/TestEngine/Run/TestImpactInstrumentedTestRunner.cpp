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

#include <Artifact/Factory/TestImpactModuleCoverageFactory.h>
#include <Artifact/Factory/TestImpactTestRunSuiteFactory.h>
#include <TestEngine/Run/TestImpactInstrumentedTestRunner.h>
#include <TestEngine/Run/TestImpactTestRunException.h>
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
        AZStd::chrono::milliseconds duration,
        InstrumentedTestRunner::CoverageExceptionPolicy coverageExceptionPolicy)
    {
        TestRun run(GTest::TestRunSuitesFactory(ReadFileContents<TestRunException>(runFile)), duration);
        AZStd::vector<ModuleCoverage> moduleCoverages = Cobertura::ModuleCoveragesFactory(ReadFileContents<TestRunException>(coverageFile));
        if (moduleCoverages.empty())
        {
            AZ_TestImpact_Eval(
                !IsFlagSet(coverageExceptionPolicy, Bitwise::CoverageExceptionPolicy::OnEmptyCoverage), TestRunException,
                AZStd::string::format("No coverage data generated for '%s'", coverageFile.c_str()));
        }

        TestCoverage coverage(AZStd::move(moduleCoverages));
        return {AZStd::move(run), AZStd::move(coverage)};
    }

    InstrumentedTestRunner::InstrumentedTestRunner(size_t maxConcurrentRuns)
        : JobRunner(maxConcurrentRuns)
    {
    }

    AZStd::pair<ProcessSchedulerResult, AZStd::vector<InstrumentedTestRunner::Job>> InstrumentedTestRunner::RunInstrumentedTests(
        const AZStd::vector<JobInfo>& jobInfos,
        CoverageExceptionPolicy coverageExceptionPolicy,
        JobExceptionPolicy jobExceptionPolicy,
        AZStd::optional<AZStd::chrono::milliseconds> runTimeout,
        AZStd::optional<AZStd::chrono::milliseconds> runnerTimeout,
        AZStd::optional<ClientJobCallback> clientCallback)
    {
        const auto payloadGenerator = [this, coverageExceptionPolicy](const JobDataMap& jobDataMap)
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
                            meta.m_duration.value(),
                            coverageExceptionPolicy);
                    }
                    catch (const Exception& e)
                    {
                        AZ_Warning("RunInstrumentedTests", false, e.what());
                        runs[jobId] = AZStd::nullopt;

                        if (coverageExceptionPolicy == CoverageExceptionPolicy::OnEmptyCoverage)
                        {
                            break;
                        }
                    }
                }
            }

            return runs;
        };

        return ExecuteJobs(
            jobInfos,
            jobExceptionPolicy,
            payloadGenerator,
            StdOutputRouting::None,
            StdErrorRouting::None,
            runTimeout,
            runnerTimeout,
            clientCallback,
            AZStd::nullopt);
    }
} // namespace TestImpact
