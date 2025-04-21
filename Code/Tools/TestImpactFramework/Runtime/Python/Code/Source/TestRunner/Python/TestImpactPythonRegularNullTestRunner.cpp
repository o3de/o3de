/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TestRunner/Python/TestImpactPythonErrorCodeChecker.h>
#include <TestRunner/Python/TestImpactPythonRegularNullTestRunner.h>

namespace TestImpact
{
    AZStd::pair<ProcessSchedulerResult, AZStd::vector<PythonRegularNullTestRunner::TestJobRunner::Job>>
    PythonRegularNullTestRunner::RunTests(
        const TestJobRunner::JobInfos& jobInfos,
        [[maybe_unused]] StdOutputRouting stdOutRouting,
        [[maybe_unused]] StdErrorRouting stdErrRouting,
        [[maybe_unused]] AZStd::optional<AZStd::chrono::milliseconds> runTimeout,
        [[maybe_unused]] AZStd::optional<AZStd::chrono::milliseconds> runnerTimeout)
    {
        AZStd::vector<Job> jobs;
        jobs.reserve(jobInfos.size());

        for (auto& jobInfo : jobInfos)
        {
            if (auto outcome = PayloadExtractor(jobInfo, {}); outcome.IsSuccess())
            {
                JobMeta meta;
                meta.m_result = JobResult::ExecutedWithSuccess;
                Job job(jobInfo, AZStd::move(meta), outcome.TakeValue());
                // As these jobs weren't executed, no return code exists, so use the test pass/failure result to set the appropriate error code
                meta.m_returnCode = outcome.GetValue().GetNumFailures() ? ErrorCodes::PyTest::TestFailures : 0;
                jobs.push_back(job);
                NotificationBus::Broadcast(&NotificationBus::Events::OnJobComplete, job.GetJobInfo(), meta, StdContent{});
            }
            else
            {
                JobMeta meta;
                meta.m_result = JobResult::FailedToExecute;
                Job job(jobInfo, AZStd::move(meta), AZStd::nullopt);
                jobs.push_back(job);
                NotificationBus::Broadcast(&NotificationBus::Events::OnJobComplete, job.GetJobInfo(), meta, StdContent{});
            }
        }

        return { ProcessSchedulerResult::Graceful, jobs };
    }
} // namespace TestImpact
