/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Artifact/Dynamic/TestImpactTestRunSuite.h>
#include <TestEngine/TestImpactTestSuiteContainer.h>

namespace TestImpact
{
    //! Representation of a given test target's test run results.
    class TestRun
        : public TestSuiteContainer<TestRunSuite>
    {
        using TestSuiteContainer = TestSuiteContainer<TestRunSuite>;

    public:
        TestRun(const TestRun&);
        TestRun(TestRun&&) noexcept;
        TestRun(const AZStd::vector<TestRunSuite>& testSuites, AZStd::chrono::milliseconds duration);
        TestRun(AZStd::vector<TestRunSuite>&& testSuites, AZStd::chrono::milliseconds duration) noexcept;

        TestRun& operator=(const TestRun&);
        TestRun& operator=(TestRun&&) noexcept;

        //! Returns the total number of tests that were run.
        size_t GetNumRuns() const;

        //! Returns the total number of tests that were not run.
        size_t GetNumNotRuns() const;

        //! Returns the total number of tests that were run and passed.
        size_t GetNumPasses() const;

        //! Returns the total number of tests that were run and failed.
        size_t GetNumFailures() const;

        //! Returns the duration of the job that was executed to yield this run data.
        AZStd::chrono::milliseconds GetDuration() const;

    private:
        void CalculateTestMetrics();

        size_t m_numRuns = 0;
        size_t m_numNotRuns = 0;
        size_t m_numPasses = 0;
        size_t m_numFailures = 0;
        AZStd::chrono::milliseconds m_duration = AZStd::chrono::milliseconds{0};
    };
} // namespace TestImpact
