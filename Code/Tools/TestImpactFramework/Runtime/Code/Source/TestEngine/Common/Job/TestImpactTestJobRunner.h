/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Process/JobRunner/TestImpactProcessJob.h>
#include <Process/JobRunner/TestImpactProcessJobRunner.h>

#include <AzCore/Outcome/Outcome.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/optional.h>
#include <AzCore/std/string/string.h>

namespace TestImpact
{
    //! Outcome of a payload processed by a test runner payload factory.
    //! @tparam Payload The payload produced by the test runner specialization.
    template<typename Payload>
    using PayloadOutcome = AZ::Outcome<Payload, AZStd::string>;

    //! Default template payload factory (to be specialized by specific test runners).
    //! @tparam AdditionalInfo The additional info class/struct provided to the test runner specialization.
    //! @tparam Payload The The payload produced by the test runner specialization.
    template<typename AdditionalInfo, typename Payload>
    PayloadOutcome<Payload> PayloadFactory(const JobInfo<AdditionalInfo>& jobData, const JobMeta& jobMeta)
    {
        static_assert(false, "Please specify a factory function for the payload and additional info type.");
    };

    //! Base class for test related job runners.
    //! @tparam AdditionalInfo The data structure containing the information additional to the command arguments necessary to execute and
    //! complete a job.
    //! @tparam Payload The output produced by a job.
    template<typename AdditionalInfo, typename Payload>
    class TestJobRunner
    {
        using JobRunner = JobRunner<AdditionalInfo, Payload>;
    public:
        using JobData = typename JobRunner::JobData;
        using JobInfo = typename JobRunner::JobInfo;
        using Command = typename JobRunner::Command;
        using JobPayload = typename JobRunner::JobPayload;;
        using Job = typename JobRunner::Job;
        using PayloadMap = typename JobRunner::PayloadMap;
        using JobDataMap = typename JobRunner::JobDataMap;
        using JobCallback = typename JobRunner::JobCallback;
        using StdContentCallback = typename JobRunner::StdContentCallback;
        using JobPayloadOutcome = PayloadOutcome<JobPayload>;

        //! Constructs the job runner with the specified parameters common to all job runs of this runner.
        //! @param maxConcurrentJobs The maximum number of jobs to be in flight at any given time.
        explicit TestJobRunner(size_t maxConcurrentJobs);

    protected:
        JobRunner m_jobRunner;
    };

    template<typename Data, typename Payload>
    TestJobRunner<Data, Payload>::TestJobRunner(size_t maxConcurrentJobs)
        : m_jobRunner(maxConcurrentJobs)
    {
    }
} // namespace TestImpact
