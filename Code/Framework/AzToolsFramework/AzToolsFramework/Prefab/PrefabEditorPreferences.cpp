/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/Prefab/PrefabEditorPreferences.h>
#include <AzCore/Settings/SettingsRegistry.h>

namespace AzToolsFramework::Prefab
{
    static constexpr AZStd::string_view EnablePrefabOverridesUxKey = "/O3DE/Preferences/Prefabs/EnableOverridesUx";
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

    bool IsPrefabOverridesUxEnabled()
    {
        bool prefabOverridesUxEnabled = false;
        if (auto* registry = AZ::SettingsRegistry::Get())
        {
            registry->Get(prefabOverridesUxEnabled, EnablePrefabOverridesUxKey);
        }

        return prefabOverridesUxEnabled;
    }

} // namespace AzToolsFramework::Prefab
