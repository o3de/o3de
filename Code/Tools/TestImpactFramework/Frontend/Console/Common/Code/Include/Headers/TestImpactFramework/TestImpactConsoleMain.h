/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

namespace TestImpact::Console
{
    enum class ReturnCode : int
    {
        Success = 0, //!< The instigated operation(s) returned without error.
        InvalidArgs, //!< The specified command line arguments were incorrect.
        InvalidChangeList, //!< The specified change list could not parsed or was malformed.
        InvalidConfiguration, //!< The runtime configuration is malformed.
        RuntimeError, //!< The runtime encountered an error that it could not recover from.
        UnhandledError, //!< The framework encountered an error that it anticipated but did not handle and could not recover from.
        UnknownError, //!< An error of unknown origin was encountered that the console or runtime could not recover from.
        TestFailure, //!< The test sequence had one or more test failures.
        Timeout //!< The test sequence runtime exceeded the global timeout value.
    };

    //! Entry point for the console front end application.
    [[nodiscard]] ReturnCode Main(int argc, char** argv);
} // namespace TestImpact::Console
