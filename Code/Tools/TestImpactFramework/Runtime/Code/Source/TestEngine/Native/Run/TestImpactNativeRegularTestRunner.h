/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <TestEngine/Common/TestRunner/TestImpactTestRunner.h>
#include <TestEngine/Native/Job/TestImpactNativeRegularTestRunJobData.h>
#include <TestEngine/TestImpactTestEngineException.h>
#include <TestImpactFramework/TestImpactUtils.h>
#include <Artifact/Factory/TestImpactTestRunSuiteFactory.h>
#include <TestEngine/Native/Run/TestImpactNativeTestRun.h>

namespace TestImpact
{
    class RegularTestRunner
        : public TestRunner<TestRunJobData, TestRun>
    {
    public:
        using TestRunner<TestRunJobData, TestRun>::TestRunner;
    };

    template<>
    inline PayloadOutcome<TestRun> PayloadFactory(const JobInfo<TestRunJobData>& jobData, const JobMeta& jobMeta)
    {
        try
        {
            return AZ::Success(TestRun(
                GTest::TestRunSuitesFactory(ReadFileContents<TestEngineException>(jobData.GetRunArtifactPath())),
                jobMeta.m_duration.value()));
        }
        catch (const Exception& e)
        {
            return AZ::Failure(AZStd::string::format("%s\n", e.what()));
        }
    };

} // namespace TestImpact
