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
#include <TestEngine/Native/Job/TestImpactNativeRegularTestRunJobData.h>

namespace TestImpact
{
    namespace Native
    {
        class RegularTestRunner
            : public TestImpact::RegularTestRunner<TestRunJobData>
        {
        public:
            using TestImpact::RegularTestRunner<TestRunJobData>::RegularTestRunner;
        };
    }

    template<>
    inline PayloadOutcome<TestRun> PayloadFactory(const JobInfo<Native::TestRunJobData>& jobData, const JobMeta& jobMeta)
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
