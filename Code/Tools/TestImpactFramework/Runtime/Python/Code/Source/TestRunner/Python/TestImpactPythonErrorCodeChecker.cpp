/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TestRunner/Python/TestImpactPythonErrorCodeChecker.h>

namespace TestImpact
{
    AZStd::optional<Client::TestRunResult> CheckPythonErrorCode(ReturnCode returnCode)
    {
        switch (returnCode)
        {
        case ErrorCodes::Python::ScriptException:
        case ErrorCodes::Python::InvalidArgs:
            return Client::TestRunResult::FailedToExecute;
        default:
            return AZStd::nullopt;
        }
    }

    AZStd::optional<Client::TestRunResult> CheckPyTestErrorCode(ReturnCode returnCode)
    {
        switch (returnCode)
        {
        case ErrorCodes::PyTest::TestFailures:
            return Client::TestRunResult::TestFailures;
        case ErrorCodes::PyTest::UserInterrupt:
        case ErrorCodes::PyTest::InternalError:
        case ErrorCodes::PyTest::InvalidArgs:
        case ErrorCodes::PyTest::NoTests:
            return Client::TestRunResult::FailedToExecute;
        default:
            return AZStd::nullopt;
        }
    }
} // namespace TestImpact
