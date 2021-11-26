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
#include <TestRunner/Common/Run/TestImpactTestRunner.h>
#include <TestRunner/Common/Job/TestImpactTestRunJobData.h>
#include <TestRunner/Native/Job/TestImpactNativeTestRunJobData.h>

namespace TestImpact
{
    class NativeRegularTestRunner
        : public TestRunner<NativeTestRunJobData<TestRunJobData>>
    {
    public:
        using TestRunner<NativeTestRunJobData<TestRunJobData>>::TestRunner;
    };

    template<>
    inline NativeRegularTestRunner::JobPayloadOutcome PayloadFactory(const NativeRegularTestRunner::JobInfo& jobData, const JobMeta& jobMeta)
    {
        try
        {
            return AZ::Success(NativeRegularTestRunner::JobPayload(
                GTest::TestRunSuitesFactory(ReadFileContents<TestRunnerException>(jobData.GetRunArtifactPath())),
                jobMeta.m_duration.value()));
        }
        catch (const Exception& e)
        {
            return AZ::Failure(AZStd::string::format("%s\n", e.what()));
        }
    };
} // namespace TestImpact
