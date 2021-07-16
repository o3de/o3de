/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TestEngine/TestImpactTestEngineJobFailure.h>

namespace TestImpact
{
    // Known error codes for test runner and test library
    namespace ErrorCodes
    {
        namespace GTest
        {
            static constexpr ReturnCode Unsuccessful = 1;
        }

        namespace AZTestRunner
        {
            static constexpr ReturnCode InvalidArgs = 101;
            static constexpr ReturnCode FailedToFindTargetBinary = 102;
            static constexpr ReturnCode SymbolNotFound = 103;
            static constexpr ReturnCode ModuleSkipped = 104;
        }
    }

    AZStd::optional<Client::TestRunResult> CheckForKnownTestRunnerErrorCode(int returnCode)
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

    AZStd::optional<Client::TestRunResult> CheckForKnownTestLibraryErrorCode(int returnCode)
    {
        if (returnCode == ErrorCodes::GTest::Unsuccessful)
        {
            return Client::TestRunResult::TestFailures;
        }

        return AZStd::nullopt;
    }

    AZStd::optional<Client::TestRunResult> CheckForAnyKnownErrorCode(ReturnCode returnCode)
    {
        if (const auto result = CheckForKnownTestInstrumentErrorCode(returnCode);
            result != AZStd::nullopt)
        {
            return result.value();
        }

        if (const auto result = CheckForKnownTestRunnerErrorCode(returnCode);
            result != AZStd::nullopt)
        {
            return result.value();
        }

        if (const auto result = CheckForKnownTestLibraryErrorCode(returnCode);
            result != AZStd::nullopt)
        {
            return result.value();
        }

        return AZStd::nullopt;
    }
} // namespace TestImpact
