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
    PythonNullTestRunner::PythonNullTestRunner()
        : TestRunnerWithCoverage(1)
    {
    }

    PythonNullTestRunner::JobPayloadOutcome PythonNullTestRunner::PayloadFactory(
        [[maybe_unused]] const JobInfo& jobData, [[maybe_unused]] const JobMeta& jobMeta)
    {
        // TODO:
        // 

        return AZ::Failure(AZStd::string("Not implemented"));
    }
} // namespace TestImpact
