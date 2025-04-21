/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once
#include <AzCore/std/optional.h>

namespace AZ::RHI
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

    // Read the RHI validation mode considering configurations,
    // cvars, command line options and registry settings.
    ValidationMode ReadValidationMode();
}
