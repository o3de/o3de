/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TestImpactFramework/Python/TestImpactPythonConfiguration.h>

namespace TestImpact
{
    //! Parses the python-specific configuration data (in JSON format) and returns the constructed runtime configuration.
    PythonRuntimeConfig PythonRuntimeConfigurationFactory(const AZStd::string& configurationData);
} // namespace TestImpact
