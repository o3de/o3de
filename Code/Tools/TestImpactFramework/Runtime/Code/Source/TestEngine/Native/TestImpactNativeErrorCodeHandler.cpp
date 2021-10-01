/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TestEngine/Native/TestImpactNativeErrorCodeHandler.h>

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

    ErrorCodeHandler GetNativeTestRunnerErrorCodeHandler()
    {
        return [](ReturnCode returnCode) -> AZStd::optional<Client::TestRunResult>
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
        };
    }

    ErrorCodeHandler GetNativeTestLibraryErrorCodeHandler()
    {
        return [](ReturnCode returnCode) -> AZStd::optional<Client::TestRunResult>
        {
            if (returnCode == ErrorCodes::GTest::Unsuccessful)
            {
                return Client::TestRunResult::TestFailures;
            }

            return AZStd::nullopt;
        };
    }
} // namespace TestImpact
