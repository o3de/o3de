/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <TestEngine/TestImpactTestEngineInstrumentedRun.h>

namespace TestImpact
{
    namespace
    {
        AZStd::optional<TestRun> ReleaseTestRun(AZStd::optional<AZStd::pair<AZStd::optional<TestRun>, TestCoverage>>& testRunAndCoverage)
        {
            if (testRunAndCoverage.has_value() && testRunAndCoverage->first.has_value())
            {
                return AZStd::move(testRunAndCoverage.value().first);
            }

            return AZStd::nullopt;
        }

        AZStd::optional<TestCoverage> ReleaseTestCoverage(
            AZStd::optional<AZStd::pair<AZStd::optional<TestRun>, TestCoverage>>& testRunAndCoverage)
        {
            if (testRunAndCoverage.has_value())
            {
                return AZStd::move(testRunAndCoverage.value().second);
            }

            return AZStd::nullopt;
        }
    }

    TestEngineInstrumentedRun::TestEngineInstrumentedRun(
        TestEngineJob&& testJob, AZStd::optional<AZStd::pair<AZStd::optional<TestRun>, TestCoverage>>&& testRunAndCoverage)
        : TestEngineRegularRun(AZStd::move(testJob), ReleaseTestRun(testRunAndCoverage))
        , m_testCoverage(ReleaseTestCoverage(testRunAndCoverage))
    {
    }

    const AZStd::optional<TestCoverage>& TestEngineInstrumentedRun::GetTestCoverge() const
    {
        return m_testCoverage;
    }
} // namespace TestImpact
