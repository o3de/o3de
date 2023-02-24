/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Settings/SettingsRegistry.h>
#include <AzToolsFramework/Prefab/PrefabEditorPreferences.h>

namespace AzToolsFramework::Prefab
{
    static constexpr AZStd::string_view OutlinerOverrideManagement = "/O3DE/Preferences/Prefabs/EnableOutlinerOverrideManagement";
    static constexpr AZStd::string_view InspectorOverrideManagementKey = "/O3DE/Preferences/Prefabs/EnableInspectorOverrideManagement";
    static constexpr AZStd::string_view HotReloadToggleKey = "/O3DE/Preferences/Prefabs/EnableHotReloading";

    bool IsHotReloadingEnabled()
    {
        bool isHotReloadingEnabled = false;

        if (auto* registry = AZ::SettingsRegistry::Get())
        {
            registry->Get(isHotReloadingEnabled, HotReloadToggleKey);
        }

        return isHotReloadingEnabled;
    }

    bool IsOutlinerOverrideManagementEnabled()
    {
        bool isOutlinerOverrideManagementEnabled = false;
        if (auto* registry = AZ::SettingsRegistry::Get())
        {
            registry->Get(isOutlinerOverrideManagementEnabled, OutlinerOverrideManagement);
        }

        return isOutlinerOverrideManagementEnabled;
    }

    bool IsInspectorOverrideManagementEnabled()
    {
        bool isInspectorOverrideManagementEnabled = false;

        if (auto* registry = AZ::SettingsRegistry::Get())
        {
            registry->Get(isInspectorOverrideManagementEnabled, InspectorOverrideManagementKey);
        }

        return isInspectorOverrideManagementEnabled;
    }

} // namespace AzToolsFramework::Prefab
