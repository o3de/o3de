/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <TestImpactFramework/TestImpactUtils.h>

#include <TestRunner/Python/TestImpactPythonRegularTestRunnerBase.h>

#include <AzCore/std/containers/unordered_set.h>

namespace TestImpact
{
    PythonRegularTestRunnerBase::PythonRegularTestRunnerBase()
        : TestRunner(1)
    {
    }

    PythonRegularTestRunnerBase::JobPayloadOutcome PythonRegularTestRunnerBase::PayloadExtractor(
        [[maybe_unused]] const JobInfo& jobData, [[maybe_unused]] const JobMeta& jobMeta)
    {
        AZStd::optional<TestRun> run;
        try
        {
            return AZ::Success(JobPayload(
                JUnit::TestRunSuitesFactory(ReadFileContents<TestRunnerException>(jobData.GetRunArtifactPath())),
                jobMeta.m_duration.value()));
        } catch (const Exception& e)
        {
            // No run result is a failure as all Python tests will be exporting their results to JUnit format
            return AZ::Failure(AZStd::string(e.what()));
        }
    }
} // namespace TestImpact
