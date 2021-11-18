/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <TestEngine/Common/Run/TestImpactTestRunnerBase.h>
#include <TestEngine/Common/Run/TestImpactTestRun.h>

namespace TestImpact
{
    template<typename JobData>
    class TestRunner
        : public TestRunnerBase<JobData, TestRun>
    {
    public:
        using TestRunnerBase = TestRunnerBase<JobData, TestRun>;
        using TestRunnerBase::TestRunnerBase;
    };

} // namespace TestImpact
