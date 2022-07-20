/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <TestEngine/Common/Run/TestImpactTestEngineRegularRun.h>
#include <TestRunner/Common/Run/TestImpactTestCoverage.h>

namespace TestImpact
{
    //! Represents the generated test run and coverage data for an instrumented regular test engine run.
    template<typename TestTarget, typename Coverage>
    class TestEngineInstrumentedRun
        : public TestEngineRegularRun<TestTarget>
    {
    public:
        TestEngineInstrumentedRun(
            TestEngineJob<TestTarget>&& testJob, AZStd::optional<AZStd::pair<AZStd::optional<TestRun>, Coverage>>&& testRunAndCoverage);

        //! Returns the test coverage payload for this job (if any).
        const AZStd::optional<Coverage>& GetCoverge() const;

    private:
        //!
        static AZStd::optional<TestRun> ReleaseTestRun(
            AZStd::optional<AZStd::pair<AZStd::optional<TestRun>, Coverage>>& testRunAndCoverage);

        //!
        static AZStd::optional<Coverage> ReleaseCoverage(
            AZStd::optional<AZStd::pair<AZStd::optional<TestRun>, Coverage>>& testRunAndCoverage);

        AZStd::optional<Coverage> m_coverage;
    };

    template<typename TestTarget, typename Coverage>
    TestEngineInstrumentedRun<TestTarget, Coverage>::TestEngineInstrumentedRun(
        TestEngineJob<TestTarget>&& testJob, AZStd::optional<AZStd::pair<AZStd::optional<TestRun>, Coverage>>&& testRunAndCoverage)
        : TestEngineRegularRun<TestTarget>(AZStd::move(testJob), ReleaseTestRun(testRunAndCoverage))
        , m_coverage(ReleaseCoverage(testRunAndCoverage))
    {
    }

    template<typename TestTarget, typename Coverage>
    const AZStd::optional<Coverage>& TestEngineInstrumentedRun<TestTarget, Coverage>::GetCoverge() const
    {
        return m_coverage;
    }

    template<typename TestTarget, typename Coverage>
    AZStd::optional<TestRun> TestEngineInstrumentedRun<TestTarget, Coverage>::ReleaseTestRun(
        AZStd::optional<AZStd::pair<AZStd::optional<TestRun>, Coverage>>& testRunAndCoverage)
    {
        if (testRunAndCoverage.has_value() && testRunAndCoverage->first.has_value())
        {
            return AZStd::move(testRunAndCoverage.value().first);
        }

        return AZStd::nullopt;
    }

    template<typename TestTarget, typename Coverage>
    AZStd::optional<Coverage> TestEngineInstrumentedRun<TestTarget, Coverage>::ReleaseCoverage(
        AZStd::optional<AZStd::pair<AZStd::optional<TestRun>, Coverage>>& testRunAndCoverage)
    {
        if (testRunAndCoverage.has_value())
        {
            return AZStd::move(testRunAndCoverage.value().second);
        }

        return AZStd::nullopt;
    }
} // namespace TestImpact
