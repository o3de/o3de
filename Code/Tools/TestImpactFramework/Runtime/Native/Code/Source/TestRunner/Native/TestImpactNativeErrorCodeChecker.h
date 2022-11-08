/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Process/TestImpactProcessInfo.h>
#include <TestImpactFramework/TestImpactClientTestRun.h>

#include <AzCore/std/optional.h>

namespace TestImpact
{
    // Known error codes for test runner and test library
    namespace ErrorCodes
    {
        namespace GTest
        {
            inline constexpr ReturnCode Unsuccessful = 1;
        } // namespace GTest

        namespace AZTestRunner
        {
            inline constexpr ReturnCode InvalidArgs = 101;
            inline constexpr ReturnCode FailedToFindTargetBinary = 102;
            inline constexpr ReturnCode SymbolNotFound = 103;
            inline constexpr ReturnCode ModuleSkipped = 104;
        } // namespace AZTestRunner
    } // namespace ErrorCodes

    //! Returns the error code for native instrumentation error codes.
    AZStd::optional<Client::TestRunResult> CheckNativeInstrumentationErrorCode(ReturnCode returnCode);

    //! Returns the error code for native test runner error codes.
    AZStd::optional<Client::TestRunResult> CheckNativeTestRunnerErrorCode(ReturnCode returnCode);

    //! Returns the error code for native test library error codes.
    AZStd::optional<Client::TestRunResult> CheckNativeTestLibraryErrorCode(ReturnCode returnCode);

    //! Returns the error code for native stand alone (executable) test targets.
    AZStd::optional<Client::TestRunResult> CheckStandAloneError(ReturnCode returnCode);
} // namespace TestImpact
