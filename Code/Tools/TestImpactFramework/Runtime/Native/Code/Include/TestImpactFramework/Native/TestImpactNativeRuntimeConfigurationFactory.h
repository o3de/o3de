/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <TestImpactFramework/Native/TestImpactNativeConfiguration.h>

namespace TestImpact
{
    //! Parses the native-specific configuration data (in JSON format) and returns the constructed runtime configuration.
    NativeRuntimeConfig NativeRuntimeConfigurationFactory(const AZStd::string& configurationData);
} // namespace TestImpact
