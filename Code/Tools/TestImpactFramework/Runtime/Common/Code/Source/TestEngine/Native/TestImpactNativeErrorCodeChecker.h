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
    //! Returns the error code handler for native instrumentation error codes.
    AZStd::optional<Client::TestRunResult> CheckNativeInstrumentationErrorCode(ReturnCode returnCode);

    //! Returns the error code handler for native test runner error codes.
    AZStd::optional<Client::TestRunResult> CheckNativeTestRunnerErrorCode(ReturnCode returnCode);

    //! Returns the error code handler for native test library error codes.
    AZStd::optional<Client::TestRunResult> CheckNativeTestLibraryErrorCode(ReturnCode returnCode);

    //!
    AZStd::optional<Client::TestRunResult> CheckStandAloneError(ReturnCode returnCode);
} // namespace TestImpact
