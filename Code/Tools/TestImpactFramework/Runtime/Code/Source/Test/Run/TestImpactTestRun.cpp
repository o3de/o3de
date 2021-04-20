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

#include "TestImpactTestRun.h"

namespace TestImpact
{
    TestRun::TestRun(AZStd::vector<TestRunSuite>&& testSuites, AZStd::chrono::milliseconds duration)
        : TestSuiteContainer(AZStd::move(testSuites))
        , m_duration(duration)
    {
        for (const auto& suite : m_testSuites)
        {
            for (const auto& test : suite.m_tests)
            {
                if (test.m_status == TestRunStatus::Run)
                {
                    m_numRuns++;

                    if (test.m_result.value() == TestRunResult::Passed)
                    {
                        m_numPasses++;
                    }
                    else
                    {
                        m_numFailures++;
                    }
                }
                else
                {
                    m_numNotRuns++;
                }
            }
        }
    }

    size_t TestRun::GetNumRuns() const
    {
        return m_numRuns;
    }

    size_t TestRun::GetNumNotRuns() const
    {
        return m_numNotRuns;
    }

    size_t TestRun::GetNumPasses() const
    {
        return m_numPasses;
    }

    size_t TestRun::GetNumFailures() const
    {
        return m_numFailures;
    }

    AZStd::chrono::milliseconds TestRun::GetDuration() const
    {
        return m_duration;
    }
} // namespace TestImpact
