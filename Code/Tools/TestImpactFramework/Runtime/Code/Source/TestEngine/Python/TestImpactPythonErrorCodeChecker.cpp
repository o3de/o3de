/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TestEngine/Python/TestImpactPythonErrorCodeChecker.h>

namespace TestImpact
{
    // Known error codes for Python and PyTest library
    namespace ErrorCodes
    {
        namespace Python
        {
            static constexpr ReturnCode ScriptException = 1;
            static constexpr ReturnCode InvalidArgs = 2;
        } // namespace Python

        namespace PyTest
        {
            static constexpr ReturnCode TestFailures = 1;
            static constexpr ReturnCode UserInterrupt = 2;
            static constexpr ReturnCode InternalError = 3;
            static constexpr ReturnCode InvalidArgs = 4;
            static constexpr ReturnCode NoTests = 5;
        } // namespace PyTest
    } // namespace ErrorCodes

    AZStd::optional<Client::TestRunResult> GetPythonErrorCodeHandler(ReturnCode returnCode)
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

    AZStd::optional<Client::TestRunResult> GetPyTestErrorCodeHandler(ReturnCode returnCode)
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
