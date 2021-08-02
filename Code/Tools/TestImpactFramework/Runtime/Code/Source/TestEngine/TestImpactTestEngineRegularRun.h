/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <TestImpactFramework/TestImpactClientTestRun.h>

#include <TestEngine/TestImpactTestEngineJob.h>
#include <TestEngine/Run/TestImpactTestRun.h>

namespace TestImpact
{
    //! Represents the generated test run data for a regular test engine run. 
    class TestEngineRegularRun
        : public TestEngineJob
    {
    public:
        TestEngineRegularRun(TestEngineJob&& testJob, AZStd::optional<TestRun>&& testRun);

        //! Returns the test run payload for this job (if any).
        const AZStd::optional<TestRun>& GetTestRun() const;
    private:
        AZStd::optional<TestRun> m_testRun;
    };
} // namespace TestImpact
