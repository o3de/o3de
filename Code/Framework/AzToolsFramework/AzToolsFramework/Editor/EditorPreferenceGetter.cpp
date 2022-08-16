/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/Editor/EditorPreferenceGetter.h>

#include <AzCore/Settings/SettingsRegistry.h>

namespace AzToolsFramework
{
    static constexpr AZStd::string_view EnablePrefabOverridesUxRegistryKey = "/O3DE/Preferences/Prefabs/EnableOverridesUx";

    bool IsPrefabOverridesUxEnabled()
    {
        bool prefabOverridesUxEnabled = false;
        if (auto* registry = AZ::SettingsRegistry::Get())
        {
            registry->Get(prefabOverridesUxEnabled, EnablePrefabOverridesUxRegistryKey);
        }

        return prefabOverridesUxEnabled;
    }

} // namespace AzToolsFramework
