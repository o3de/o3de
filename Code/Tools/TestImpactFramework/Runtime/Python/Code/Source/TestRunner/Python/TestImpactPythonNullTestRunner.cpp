/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <TestRunner/Python/TestImpactPythonNullTestRunner.h>

namespace TestImpact
{
    AZStd::pair<ProcessSchedulerResult, AZStd::vector<PythonNullTestRunner::TestJobRunner::Job>> PythonNullTestRunner::RunTests(
        [[maybe_unused]] const AZStd::vector<TestJobRunner::JobInfo>& jobInfos,
        [[maybe_unused]] StdOutputRouting stdOutRouting,
        [[maybe_unused]] StdErrorRouting stdErrRouting,
        [[maybe_unused]] AZStd::optional<AZStd::chrono::milliseconds> runTimeout,
        [[maybe_unused]] AZStd::optional<AZStd::chrono::milliseconds> runnerTimeout,
        [[maybe_unused]] AZStd::optional<TestJobRunner::JobCallback> clientCallback,
        [[maybe_unused]] AZStd::optional<TestJobRunner::StdContentCallback> stdContentCallback)
    {
        AZStd::vector<Job> jobs;
        jobs.reserve(jobInfos.size());

        for (auto& jobInfo : jobInfos)
        {
            if (auto outcome = PayloadExtractor(jobInfo, {}); outcome.IsSuccess())
            {
                JobMeta meta;
                meta.m_result = JobResult::ExecutedWithSuccess;
                Job job(jobInfo, JobMeta{}, outcome.TakeValue());
                jobs.push_back(job);
            }
            else
            {
                JobMeta meta;
                meta.m_result = JobResult::FailedToExecute;
                Job job(jobInfo, JobMeta{}, {});
                jobs.push_back(job);
            }
        }

        return { ProcessSchedulerResult::Graceful, jobs };
    }

    PythonNullTestRunner::JobPayloadOutcome PythonNullTestRunner::PayloadExtractor(
        [[maybe_unused]] const JobInfo& jobData, [[maybe_unused]] const JobMeta& jobMeta)
    {
        return AZ::Failure(AZStd::string("Not implemented"));
    }
} // namespace TestImpact
