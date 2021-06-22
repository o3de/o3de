/*
 * All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 * its licensors.
 *
 * For complete copyright and license terms please see the LICENSE at the root of this
 * distribution (the "License"). All use of this software is governed by the License,
 * or, if provided, by the license below or the license accompanying this file. Do not
 * remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
