/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/Prefab/PrefabEditorPreferences.h>
#include <AzCore/Settings/SettingsRegistry.h>

namespace AzToolsFramework
{
    static constexpr AZStd::string_view HotReloadToggleKey = "/O3DE/Preferences/Prefabs/EnableHotReloading";

    bool IsHotReloadEnabled()
    {
        bool isHotReloadEnabled = true;

        // Retrieve new setting
        if (auto* registry = AZ::SettingsRegistry::Get())
        {
            registry->Get(isHotReloadEnabled, HotReloadToggleKey);
        }

        return isHotReloadEnabled;
    }

} // namespace AzToolsFramework
