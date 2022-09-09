/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <TestImpactCommandLineOptions.h>

namespace TestImpact
{
    //! Representation of the command line options specific to the python runtime supplied to the console frontend application.
    class PythonCommandLineOptions
        : public CommandLineOptions
    {
        using CommandLineOptions::CommandLineOptions;
    };
} // namespace TestImpact
