/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <TestRunner/Common/TestImpactErrorCodeChecker.h>

namespace TestImpact
{
    //! Returns the error code handler for native instrumentation error codes.
    ErrorCodeHandler GetNativeInstrumentationErrorCodeHandler();

    //! Returns the error code handler for native test runner error codes.
    ErrorCodeHandler GetNativeTestRunnerErrorCodeHandler();

    //! Returns the error code handler for native test library error codes.
    ErrorCodeHandler GetNativeTestLibraryErrorCodeHandler();
} // namespace TestImpact
