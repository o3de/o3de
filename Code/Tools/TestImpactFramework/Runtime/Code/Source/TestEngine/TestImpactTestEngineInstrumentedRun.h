/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <TestEngine/TestImpactTestEngineRegularRun.h>
#include <TestEngine/Run/TestImpactTestCoverage.h>

namespace TestImpact
{
    //! Represents the generated test run and coverage data for an instrumented regular test engine run.
    class TestEngineInstrumentedRun
        : public TestEngineRegularRun
    {
    public:
        TestEngineInstrumentedRun(TestEngineJob&& testJob, AZStd::optional<AZStd::pair<TestRun, TestCoverage>>&& testRunAndCoverage);

        //! Returns the test coverage payload for this job (if any).
        const AZStd::optional<TestCoverage>& GetTestCoverge() const;

    private:
        AZStd::optional<TestCoverage> m_testCoverage;
    };
} // namespace TestImpact
