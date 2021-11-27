/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <TestEngine/Common/TestImpactTestEngineRegularRun.h>
#include <TestRunner/Common/Run/TestImpactTestCoverage.h>

namespace TestImpact
{
    //! Represents the generated test run and coverage data for an instrumented regular test engine run.
    template<typename TestTarget>
    class TestEngineInstrumentedRun
        : public TestEngineRegularRun<TestTarget>
    {
    public:
        TestEngineInstrumentedRun(
            TestEngineJob<TestTarget>&& testJob,
            AZStd::optional<AZStd::pair<AZStd::optional<TestRun>, TestCoverage>>&& testRunAndCoverage);

        //! Returns the test coverage payload for this job (if any).
        const AZStd::optional<TestCoverage>& GetTestCoverge() const;

    private:
        //!
        static AZStd::optional<TestRun> ReleaseTestRun(
            AZStd::optional<AZStd::pair<AZStd::optional<TestRun>, TestCoverage>>& testRunAndCoverage);

        //!
        static AZStd::optional<TestCoverage> ReleaseTestCoverage(
            AZStd::optional<AZStd::pair<AZStd::optional<TestRun>, TestCoverage>>& testRunAndCoverage);

        AZStd::optional<TestCoverage> m_testCoverage;
    };

    template<typename TestTarget>
    TestEngineInstrumentedRun<TestTarget>::TestEngineInstrumentedRun(
        TestEngineJob<TestTarget>&& testJob,
        AZStd::optional<AZStd::pair<AZStd::optional<TestRun>, TestCoverage>>&& testRunAndCoverage)
        : TestEngineRegularRun<TestTarget>(AZStd::move(testJob), ReleaseTestRun(testRunAndCoverage))
        , m_testCoverage(ReleaseTestCoverage(testRunAndCoverage))
    {
    }

    template<typename TestTarget>
    const AZStd::optional<TestCoverage>& TestEngineInstrumentedRun<TestTarget>::GetTestCoverge() const
    {
        return m_testCoverage;
    }

    template<typename TestTarget>
    AZStd::optional<TestRun> TestEngineInstrumentedRun<TestTarget>::ReleaseTestRun(
        AZStd::optional<AZStd::pair<AZStd::optional<TestRun>, TestCoverage>>& testRunAndCoverage)
    {
        if (testRunAndCoverage.has_value() && testRunAndCoverage->first.has_value())
        {
            return AZStd::move(testRunAndCoverage.value().first);
        }

        return AZStd::nullopt;
    }

    template<typename TestTarget>
    AZStd::optional<TestCoverage> TestEngineInstrumentedRun<TestTarget>::ReleaseTestCoverage(
        AZStd::optional<AZStd::pair<AZStd::optional<TestRun>, TestCoverage>>& testRunAndCoverage)
    {
        if (testRunAndCoverage.has_value())
        {
            return AZStd::move(testRunAndCoverage.value().second);
        }

        return AZStd::nullopt;
    }
} // namespace TestImpact
