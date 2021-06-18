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

#include <TestEngine/TestImpactTestEngineInstrumentedRun.h>

namespace TestImpact
{
    namespace
    {
        AZStd::optional<TestRun> ReleaseTestRun(AZStd::optional<AZStd::pair<TestRun, TestCoverage>>& testRunAndCoverage)
        {
            if (testRunAndCoverage.has_value())
            {
                return AZStd::move(testRunAndCoverage.value().first);
            }

            return AZStd::nullopt;
        }

        AZStd::optional<TestCoverage> ReleaseTestCoverage(AZStd::optional<AZStd::pair<TestRun, TestCoverage>>& testRunAndCoverage)
        {
            if (testRunAndCoverage.has_value())
            {
                return AZStd::move(testRunAndCoverage.value().second);
            }

            return AZStd::nullopt;
        }
    }

    TestEngineInstrumentedRun::TestEngineInstrumentedRun(TestEngineJob&& testJob, AZStd::optional<AZStd::pair<TestRun, TestCoverage>>&& testRunAndCoverage)
        : TestEngineRegularRun(AZStd::move(testJob), ReleaseTestRun(testRunAndCoverage))
        , m_testCoverage(ReleaseTestCoverage(testRunAndCoverage))
    {
    }

    const AZStd::optional<TestCoverage>& TestEngineInstrumentedRun::GetTestCoverge() const
    {
        return m_testCoverage;
    }
} // namespace TestImpact
