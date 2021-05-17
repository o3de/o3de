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
    TestEngineInstrumentedRun::TestEngineInstrumentedRun(TestEngineJob&& testJob, AZStd::optional<TestRun>&& testRun, AZStd::optional<TestCoverage>&& testCoverage, TestResult testResult)
        : TestEngineRegularRun(AZStd::move(testJob), AZStd::move(testRun), testResult)
        , m_testCoverage(AZStd::move(testCoverage))
    {
    }

    const AZStd::optional<TestCoverage>& TestEngineInstrumentedRun::GetTestCoverge() const
    {
        return m_testCoverage;
    }
} // namespace TestImpact
