/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/vector.h>

namespace TestImpact
{
    // These are helpers. you don't have to inherit from them

    //!
    template<typename TestJobRunner, typename TestTarget>
    class TestJobInfoGenerator
    {
    public:
        virtual ~TestJobInfoGenerator() = default;

        //! Generates the information for a test enumeration job.
        //! @param testTarget The test target to generate the job information for.
        //! @param jobId The id to assign for this job.
        virtual typename TestJobRunner::JobInfo GenerateJobInfo(
            const TestTarget* testTarget, typename TestJobRunner::JobInfo::Id jobId) const = 0;

        //! Generates the information for the batch of test enumeration jobs.
        AZStd::vector<typename TestJobRunner::JobInfo> GenerateJobInfos(const AZStd::vector<const TestTarget*>& testTargets) const;
    };

    template<typename TestJobRunner, typename TestTarget>
    AZStd::vector<typename TestJobRunner::JobInfo> TestJobInfoGenerator<TestJobRunner, TestTarget>::GenerateJobInfos(
        const AZStd::vector<const TestTarget*>& testTargets) const
    {
        AZStd::vector<typename TestJobRunner::JobInfo> jobInfos;
        jobInfos.reserve(testTargets.size());
        for (size_t jobId = 0; jobId < testTargets.size(); jobId++)
        {
            jobInfos.push_back(GenerateJobInfo(testTargets[jobId], { jobId }));
        }

        return jobInfos;
    }
} // namespace TestImpact
