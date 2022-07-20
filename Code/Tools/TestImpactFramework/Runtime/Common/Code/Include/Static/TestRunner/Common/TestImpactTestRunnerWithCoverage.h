/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <TestRunner/Common/Run/TestImpactTestRun.h>
#include <TestRunner/Common/TestImpactTestRunnerBase.h>

namespace TestImpact
{
    template<typename JobData, typename CoverageArtifact>
    class TestRunnerWithCoverage
        : public TestRunnerBase<JobData, AZStd::pair<AZStd::optional<TestRun>, CoverageArtifact>>
    {
    public:
        using TestRunnerBase =  TestRunnerBase<JobData, AZStd::pair<AZStd::optional<TestRun>, CoverageArtifact>>;
        using TestRunnerBase::TestRunnerBase;
    };

} // namespace TestImpact
