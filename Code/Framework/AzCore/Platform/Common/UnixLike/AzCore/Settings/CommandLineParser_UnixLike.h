/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/base.h>

namespace AZ::Settings::Platform
{
    //! A class to ensure command-line parameters are in utf-8 encoding.
    //! On non-Windows platforms, it is assumed that utf-8 is used by default, so no further processing is required.
    class AZCORE_API CommandLineConverter
    {
    public:
        CommandLineConverter(int& argc, char**& argv);
    };
} // namespace AZ::Settings::Platform
