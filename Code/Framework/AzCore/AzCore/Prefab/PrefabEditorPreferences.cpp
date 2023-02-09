/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Console/IConsole.h>
#include <AzCore/Prefab/PrefabEditorPreferences.h>
#include <AzCore/Settings/SettingsRegistry.h>

namespace AZ::Prefab
{
    static constexpr AZStd::string_view EnablePrefabOverridesUxKey = "/O3DE/Preferences/Prefabs/EnableOverridesUx";
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

    bool IsPrefabOverridesUxEnabled()
    {
        bool prefabOverridesUxEnabled = false;
        if (auto* registry = AZ::SettingsRegistry::Get())
        {
            registry->Get(prefabOverridesUxEnabled, EnablePrefabOverridesUxKey);
        }

        return prefabOverridesUxEnabled;
    }

    bool IsInspectorOverrideManagementEnabled()
    {
        bool isInspectorOverrideManagementEnabled = false;
        bool prefabOverridesUxEnabled = false;

        if (auto* registry = AZ::SettingsRegistry::Get())
        {
            registry->Get(isInspectorOverrideManagementEnabled, InspectorOverrideManagementKey);
            registry->Get(prefabOverridesUxEnabled, EnablePrefabOverridesUxKey);
        }

        bool dpeEnabled = false;
        if (auto* console = AZ::Interface<AZ::IConsole>::Get(); console != nullptr)
        {
            console->GetCvarValue("ed_enableDPE", dpeEnabled);
        }

        return (isInspectorOverrideManagementEnabled && prefabOverridesUxEnabled && dpeEnabled);
    }

} // namespace AZ::Prefab
