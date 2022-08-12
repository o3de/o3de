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
    //! Checks the Python runtime error code to determine the result of the test run.
    AZStd::optional<Client::TestRunResult> CheckPythonErrorCode(ReturnCode returnCode);

    //! Checks the PyTest framework error code to determine the result of the test run.
    AZStd::optional<Client::TestRunResult> CheckPyTestErrorCode(ReturnCode returnCode);
} // namespace TestImpact
