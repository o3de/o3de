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
#include <TestEngine/Common/Job/TestImpactTestRunJobData.h>
#include <TestEngine/Common/Run/TestImpactTestRunner.h>
#include <TestEngine/TestImpactTestEngineException.h>

namespace TestImpact
{
    class PythonTestRunJobData
        : public TestRunJobData
    {
        using TestRunJobData::TestRunJobData;
    };

    class PythonTestRunner
        : public TestRunner<PythonTestRunJobData>
    {
    public:
        PythonTestRunner();
    };

    template<>
    inline PayloadOutcome<TestRun> PayloadFactory(
        [[maybe_unused]] const JobInfo<PythonTestRunJobData>& jobData, [[maybe_unused]] const JobMeta& jobMeta)
    {
        try
        {
            return AZ::Failure(AZStd::string("TODO"));
        } catch (const Exception& e)
        {
            return AZ::Failure(AZStd::string::format("%s\n", e.what()));
        }
    };
} // namespace TestImpact
