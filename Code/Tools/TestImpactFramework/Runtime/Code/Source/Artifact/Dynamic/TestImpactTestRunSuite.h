/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Artifact/Dynamic/TestImpactTestSuite.h>

#include <AzCore/std/chrono/chrono.h>
#include <AzCore/std/optional.h>

namespace TestImpact
{
    //! Result of a test that was ran.
    enum class TestRunResult : bool
    {
        Failed, //! The given test failed.
        Passed //! The given test passed.
    };

    //! Status of test as to whether or not it was ran.
    enum class TestRunStatus : bool
    {
        NotRun, //!< The test was not run (typically because the test run was aborted by the client or runner before the test could run).
        Run //!< The test was run (see TestRunResult for the result of this test).
    };

    //! Test case for test run artifacts.
    struct TestRunCase
        : public TestCase
    {
        AZStd::optional<TestRunResult> m_result;
        AZStd::chrono::milliseconds m_duration = AZStd::chrono::milliseconds{0}; //! Duration this test took to run.
        TestRunStatus m_status = TestRunStatus::NotRun;
    };

    //! Test suite for test run artifacts.
    struct TestRunSuite
        : public TestSuite<TestRunCase>
    {
        AZStd::chrono::milliseconds m_duration = AZStd::chrono::milliseconds{0}; //!< Duration this test suite took to run all of its tests.
    };
} // namespace TestImpact
