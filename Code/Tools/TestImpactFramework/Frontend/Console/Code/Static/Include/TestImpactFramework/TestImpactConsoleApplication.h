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

namespace TestImpact
{
    namespace Console
    {
        enum class ReturnCode : int
        {
            Success = 0, //!< 	The instigation operation(s) returned without error.
            InvalidArgs, //!< The specified command line arguments were incorrect.
            InvalidUnifiedDiff, //!< The specified unified diff could not be transformed into a valid change list.
            InvalidConfiguration, //!< The runtime configuration is malformed.
            RuntimeError, //!< The runtime encountered an error that it could not recover from.
            UnhandledError, //!< The framework encountered an error that it anticipated but did not handle and could not recover from (this usually means that the framework should be revisited to ensure it handles this error less generically).
            UnknownError, //!< An error of unknown origin was encountered that the console or runtime could not recover from.
            TestFailure, //!< The test sequence had one or more test failures.
            Timeout //!< The test sequence runtime exceeded the global timeout value.
        };

        [[nodiscard]] ReturnCode Main(int argc, char** argv);
    }
}
