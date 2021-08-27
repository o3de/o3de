/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <TestImpactFramework/TestImpactClientTestRun.h>

#include <Process/TestImpactProcessInfo.h>

#include <AzCore/std/optional.h>

namespace TestImpact
{
    //! Checks for known test instrumentation error return codes and returns the corresponding client test run result or empty.
    AZStd::optional<Client::TestRunResult> CheckForKnownTestInstrumentErrorCode(ReturnCode returnCode);

    //! Checks for known test runner error return codes and returns the corresponding client test run result or empty.
    AZStd::optional<Client::TestRunResult> CheckForKnownTestRunnerErrorCode(ReturnCode returnCode);

    //! Checks for known test library error return codes and returns the corresponding client test run result or empty.
    AZStd::optional<Client::TestRunResult> CheckForKnownTestLibraryErrorCode(ReturnCode returnCode);

    //! Checks for all known error return codes and returns the corresponding client test run result or empty.
    AZStd::optional<Client::TestRunResult> CheckForAnyKnownErrorCode(ReturnCode returnCode);
} // namespace TestImpact
