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
    TestRun::TestRun(const TestRun& other)
        : TestSuiteContainer(other)
    {
        CalculateTestMetrics();
    }

    TestRun::TestRun(TestRun&& other) noexcept
        : TestSuiteContainer(AZStd::move(other))
        , m_numRuns(other.m_numRuns)
        , m_numNotRuns(other.m_numNotRuns)
        , m_numPasses(other.m_numPasses)
        , m_numFailures(other.m_numFailures)
        , m_duration(other.m_duration)
    {
        other.~TestRun();
    }

    TestRun::TestRun(AZStd::vector<TestRunSuite>&& testSuites, AZStd::chrono::milliseconds duration) noexcept
        : TestSuiteContainer(AZStd::move(testSuites))
        , m_duration(duration)
    {
        CalculateTestMetrics();
    }

    TestRun::TestRun(const AZStd::vector<TestRunSuite>& testSuites, AZStd::chrono::milliseconds duration)
        : TestSuiteContainer(testSuites)
        , m_duration(duration)
    {
        CalculateTestMetrics();
    }

    TestRun::~TestRun()
    {
        m_numRuns = 0;
        m_numNotRuns = 0;
        m_numPasses = 0;
        m_numFailures = 0;
        m_duration = AZStd::chrono::milliseconds{ 0 };
    }

    TestRun& TestRun::operator=(TestRun&& other) noexcept
    {
        if (this != &other)
        {
            this->~TestRun();
            new(this)TestRun(AZStd::move(other));
            other.~TestRun();
        }

        return *this;
    }

    TestRun& TestRun::operator=(const TestRun& other)
    {
        if (this != &other)
        {
            this->~TestRun();
            new(this)TestRun(other);
        }

        return *this;
    }

    void TestRun::CalculateTestMetrics()
    {
        m_numRuns = 0;
        m_numNotRuns = 0;
        m_numPasses = 0;
        m_numFailures = 0;

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
