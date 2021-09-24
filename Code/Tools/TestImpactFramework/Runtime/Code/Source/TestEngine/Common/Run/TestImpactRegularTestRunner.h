/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <TestEngine/Common/Run/TestImpactTestRunner.h>
#include <TestEngine/Common/Run/TestImpactTestRun.h>

namespace TestImpact
{
    template<typename AdditionalInfo>
    class RegularTestRunner
        : public TestRunner<AdditionalInfo, TestRun>
    {
    public:
        using TestRunner =  TestRunner<AdditionalInfo, TestRun>;
        using TestRunner::TestRunner;
    };

} // namespace TestImpact
