/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/string/string.h>
#include <AzCore/std/chrono/chrono.h>

#pragma once

namespace TestImpact
{
    namespace Client
    {
        //! Result of a test run.
        enum class TestRunResult
        {
            NotRun, //!< The test run was not executed due to the test sequence terminating prematurely.
            FailedToExecute, //!< The test run failed to execute either due to the target binary missing or incorrect arguments.
            Timeout, //!< The test run timed out whilst in flight before being able to complete its run.
            TestFailures, //!< The test run completed its run but there were failing tests.
            AllTestsPass //!< The test run completed its run and all tests passed.
        };

        class TestRun
        {
        public:
            TestRun(const AZStd::string& name, TestRunResult result, AZStd::chrono::milliseconds duration);
            const AZStd::string& GetTargetName() const;
            TestRunResult GetResult() const;
            AZStd::chrono::milliseconds GetDuration() const;

        private:
            AZStd::string m_targetName;
            TestRunResult m_result;
            AZStd::chrono::milliseconds m_duration;
        };
    } // namespace Client
} // namespace TestImpact
