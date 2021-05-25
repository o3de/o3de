/*
 * All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 * its licensors.
 *
 * For complete copyright and license terms please see the LICENSE at the root of this
 * distribution (the "License"). All use of this software is governed by the License,
 * or, if provided, by the license below or the license accompanying this file. Do not
 * remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
