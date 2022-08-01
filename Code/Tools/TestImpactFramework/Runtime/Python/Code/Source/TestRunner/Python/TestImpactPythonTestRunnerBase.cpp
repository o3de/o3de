/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <TestImpactFramework/TestImpactUtils.h>

#include <TestRunner/Python/TestImpactPythonTestRunnerBase.h>

namespace TestImpact
{
    PythonTestRunnerBase::PythonTestRunnerBase(const RepoPath& artifactDir)
        : TestRunnerWithCoverage(1)
        , m_artifactDir(artifactDir)
    {
    }

    typename PythonTestRunnerBase::PayloadMap PythonTestRunnerBase::PayloadMapProducer(
        [[maybe_unused]] const typename PythonTestRunnerBase::JobDataMap& jobDataMap)
    {
        PayloadMap runs;



        return runs;
    }
} // namespace TestImpact
