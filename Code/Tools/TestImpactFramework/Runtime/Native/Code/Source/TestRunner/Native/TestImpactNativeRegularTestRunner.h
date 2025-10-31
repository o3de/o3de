/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <TestImpactFramework/TestImpactUtils.h>

#include <Artifact/Factory/TestImpactTestRunSuiteFactory.h>
#include <TestRunner/Common/TestImpactTestRunnerException.h>
#include <TestRunner/Common/TestImpactTestRunner.h>
#include <TestRunner/Common/Job/TestImpactTestRunJobData.h>
#include <TestRunner/Native/Job/TestImpactNativeTestRunJobData.h>

namespace TestImpact
{
    class NativeRegularTestRunJobInfoGenerator;

    class NativeRegularTestRunner
        : public TestRunner<NativeTestRunJobData<TestRunJobData>>
    {
    public:
        using JobInfoGenerator = NativeRegularTestRunJobInfoGenerator;
        using TestRunner<NativeTestRunJobData<TestRunJobData>>::TestRunner;

    protected:
        JobPayloadOutcome PayloadExtractor(const JobInfo& jobData, const JobMeta& jobMeta) override
        {
            try
            {
                return AZ::Success(JobPayload(
                    GTest::TestRunSuitesFactory(ReadFileContents<TestRunnerException>(jobData.GetRunArtifactPath())),
                    jobMeta.m_duration.value()));
            }
            catch (const Exception& e)
            {
                return AZ::Failure(AZStd::string::format("%s\n", e.what()));
            }
        }
    };
    
} // namespace TestImpact
