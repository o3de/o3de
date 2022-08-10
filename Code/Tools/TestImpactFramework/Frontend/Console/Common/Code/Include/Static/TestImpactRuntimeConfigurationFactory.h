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

    //! Helper function for parsing test target exclusion lists in config files.
    ExcludedTargets ParseTargetExcludeList(const rapidjson::Value::ConstArray& testExcludes);

    //! Helper function for parsing temporary workspaces in config files.
    ArtifactDir ParseTempWorkspaceConfig(const rapidjson::Value& tempWorkspace);

    //! Helper function for parsing active workspaces in config files.
    WorkspaceConfig::Active ParseActiveWorkspaceConfig(const rapidjson::Value& activeWorkspace);

    //! Helper function for parsing workspaces in config files.
    WorkspaceConfig ParseWorkspaceConfig(const rapidjson::Value& workspace);

    //! Parses the common configuration data (in JSON format) and returns the constructed runtime configuration.
    RuntimeConfig RuntimeConfigurationFactory(const AZStd::string& configurationData);
} // namespace TestImpact
