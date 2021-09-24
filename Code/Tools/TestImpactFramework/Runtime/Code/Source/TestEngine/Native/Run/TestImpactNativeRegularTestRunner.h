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
#include <TestEngine/TestImpactTestEngineException.h>
#include <TestEngine/Common/Run/TestImpactRegularTestRunner.h>
#include <TestEngine/Common/Job/TestImpactRegularTestRunJobData.h>

namespace TestImpact
{
    struct NativeTestRunJobData
        : public TestRunJobData
    {
        using TestRunJobData::TestRunJobData;
    };

    class NativeRegularTestRunner
        : public RegularTestRunner<NativeTestRunJobData>
    {
    public:
        using RegularTestRunner<NativeTestRunJobData>::RegularTestRunner;
    };

    template<>
    inline PayloadOutcome<TestRun> PayloadFactory(const JobInfo<NativeTestRunJobData>& jobData, const JobMeta& jobMeta)
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
