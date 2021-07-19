/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once
#include <AzCore/std/optional.h>

namespace AZ
{
    namespace RHI
    {
        enum class ValidationMode
        {
            Disabled,
            // Print warnings and errors
            Enabled,
            // Print all warnings, errors and info messages
            Verbose,
            // Enable GPU-based validation
            GPU
        };

        // Read the RHI validation mode from the command line arguments.
        // If not present in the arguments, return nullopt.
        AZStd::optional<ValidationMode> ReadValidationModeFromCommandArgs();
    }
}
