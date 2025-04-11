/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Console/IConsole.h>
#include <AzCore/Settings/SettingsRegistry.h>
#include <AzToolsFramework/Prefab/PrefabEditorPreferences.h>

AZ_CVAR(
    bool,
    ed_enableInspectorOverrideManagement,
    true,
    nullptr,
    AZ::ConsoleFunctorFlags::DontReplicate | AZ::ConsoleFunctorFlags::DontDuplicate,
    "If set, enables experimental prefab override support in the Entity Inspector");

AZ_CVAR(
    bool,
    ed_enableOutlinerOverrideManagement,
    true,
    nullptr,
    AZ::ConsoleFunctorFlags::DontReplicate | AZ::ConsoleFunctorFlags::DontDuplicate,
    "If set, enables experimental prefab override support in the Outliner");

namespace AzToolsFramework::Prefab
{
    const AZStd::string_view HotReloadToggleKey = "/O3DE/Preferences/Prefabs/EnableHotReloading";

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
        bool isOutlinerOverrideManagementEnabled = true;
        if (auto* console = AZ::Interface<AZ::IConsole>::Get(); console != nullptr)
        {
            console->GetCvarValue("ed_enableOutlinerOverrideManagement", isOutlinerOverrideManagementEnabled);
        }
        return isOutlinerOverrideManagementEnabled;
    }

    bool IsInspectorOverrideManagementEnabled()
    {
        bool isInspectorOverrideManagementEnabled = true;
        if (auto* console = AZ::Interface<AZ::IConsole>::Get(); console != nullptr)
        {
            console->GetCvarValue("ed_enableInspectorOverrideManagement", isInspectorOverrideManagementEnabled);
        }
        return isInspectorOverrideManagementEnabled;
    }

} // namespace AzToolsFramework::Prefab
