/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TestEngine/Native/TestImpactNativeErrorCodeChecker.h>

namespace TestImpact
{
    // Known error codes for test runner and test library
    namespace ErrorCodes
    {
        namespace GTest
        {
            static constexpr ReturnCode Unsuccessful = 1;
        } // namespace GTest

        namespace AZTestRunner
        {
            static constexpr ReturnCode InvalidArgs = 101;
            static constexpr ReturnCode FailedToFindTargetBinary = 102;
            static constexpr ReturnCode SymbolNotFound = 103;
            static constexpr ReturnCode ModuleSkipped = 104;
        } // namespace AZTestRunner
    } // namespace ErrorCodes

    AZStd::optional<Client::TestRunResult> CheckNativeTestRunnerErrorCode(ReturnCode returnCode)
    {
        switch (returnCode)
        {
        // We will consider test targets that technically execute but their launcher or unit test library return a know error
        // code that pertains to incorrect argument usage as test targets that failed to execute
        case ErrorCodes::AZTestRunner::InvalidArgs:
        case ErrorCodes::AZTestRunner::FailedToFindTargetBinary:
        case ErrorCodes::AZTestRunner::ModuleSkipped:
        case ErrorCodes::AZTestRunner::SymbolNotFound:
            return Client::TestRunResult::FailedToExecute;
        default:
            return AZStd::nullopt;
        }
    }

    AZStd::optional<Client::TestRunResult> CheckNativeTestLibraryErrorCode(ReturnCode returnCode)
    {
        if (returnCode == ErrorCodes::GTest::Unsuccessful)
        {
            return Client::TestRunResult::TestFailures;
        }

        return AZStd::nullopt;
    }

    //!
    AZStd::optional<Client::TestRunResult> CheckStandAloneError(ReturnCode returnCode)
    {
        if (returnCode == 0)
        {
            return AZStd::nullopt;
        }
        else
        {
            return Client::TestRunResult::FailedToExecute;
        }
    }
} // namespace TestImpact
