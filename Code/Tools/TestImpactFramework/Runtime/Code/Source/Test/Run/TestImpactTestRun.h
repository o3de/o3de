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

#include <Artifact/Dynamic/TestImpactTestRunSuite.h>
#include <Test/TestImpactTestSuiteContainer.h>

namespace TestImpact
{
    //! Representation of a given test target's test run results.
    class TestRun
        : public TestSuiteContainer<TestRunSuite>
    {
        using TestSuiteContainer = TestSuiteContainer<TestRunSuite>;

    public:
        TestRun(AZStd::vector<TestRunSuite>&& testSuites, AZStd::chrono::milliseconds duration);

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
        size_t m_numRuns = 0;
        size_t m_numNotRuns = 0;
        size_t m_numPasses = 0;
        size_t m_numFailures = 0;
        AZStd::chrono::milliseconds m_duration = AZStd::chrono::milliseconds{0};
    };
} // namespace TestImpact
