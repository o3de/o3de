/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TestRunner/Native/TestImpactNativeErrorCodeChecker.h>

namespace TestImpact
{
    // Known error codes for test instrumentation
    namespace ErrorCodes
    {
        namespace OpenCppCoverage
        {
            static constexpr ReturnCode InvalidArgs = 0x9F8C8E5C;
        }
    }

    AZStd::optional<Client::TestRunResult> CheckNativeInstrumentationErrorCode(ReturnCode returnCode)
    {
        if (returnCode == ErrorCodes::OpenCppCoverage::InvalidArgs)
        {
            return Client::TestRunResult::FailedToExecute;
        }

        return AZStd::nullopt;
    }
} // namespace TestImpact
