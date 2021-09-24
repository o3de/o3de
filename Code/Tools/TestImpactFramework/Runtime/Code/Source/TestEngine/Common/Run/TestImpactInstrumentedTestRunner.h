/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <TestEngine/Common/Run/TestImpactTestCoverage.h>
#include <TestEngine/Common/Run/TestImpactTestRun.h>
#include <TestEngine/Common/Run/TestImpactTestRunner.h>

namespace TestImpact
{
    template<typename AdditionalInfo>
    class InstrumentedTestRunner
        : public TestRunner<AdditionalInfo, AZStd::pair<AZStd::optional<TestRun>, TestCoverage>>
    {
    public:
        using TestRunner =  TestRunner<AdditionalInfo, AZStd::pair<AZStd::optional<TestRun>, TestCoverage>>;
        using TestRunner::TestRunner;
    };

} // namespace TestImpact
