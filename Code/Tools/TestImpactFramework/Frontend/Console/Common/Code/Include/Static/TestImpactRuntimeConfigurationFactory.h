/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TestImpactFramework/TestImpactConfiguration.h>

#include <AzCore/JSON/document.h>

namespace TestImpact
{

    //!
    ExcludedTargets ParseTargetExcludeList(const rapidjson::Value::ConstArray& testExcludes);

    //!
    WorkspaceConfig::Temp ParseTempWorkspaceConfig(const rapidjson::Value& tempWorkspace);

    //!
    WorkspaceConfig::Active ParseActiveWorkspaceConfig(const rapidjson::Value& activeWorkspace);

    //!
    WorkspaceConfig ParseWorkspaceConfig(const rapidjson::Value& workspace);

    //! Parses the common configuration data (in JSON format) and returns the constructed runtime configuration.
    RuntimeConfig RuntimeConfigurationFactory(const AZStd::string& configurationData);
} // namespace TestImpact
