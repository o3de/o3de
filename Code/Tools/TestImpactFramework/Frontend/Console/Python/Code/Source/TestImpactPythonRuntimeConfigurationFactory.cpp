/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TestImpactPythonRuntimeConfigurationFactory.h>
#include <TestImpactRuntimeConfigurationFactory.h>

namespace TestImpact
{
    PythonRuntimeConfig PythonRuntimeConfigurationFactory(const AZStd::string& configurationData)
    {
        PythonRuntimeConfig runtimeConfig;
        runtimeConfig.m_commonConfig = RuntimeConfigurationFactory(configurationData);

        return runtimeConfig;
    }
} // namespace TestImpact
