/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "LyShineEditorSettings.h"

#include <AzCore/Settings/SettingsRegistry.h>
#include <AzCore/Settings/SettingRegisteryUtility.h>

namespace LyShine
{
    ViewportInteraction::InteractionMode GetInteractionMode()
    {
        return static_cast<ViewportInteraction::InteractionMode>(
            AZ::SettingsRegistryUtils::GetRegistry(AZ::SettingsRegistry::Get(), InteractionModeSetting, aznumeric_cast<AZ::u64>(ViewportInteraction::InteractionMode::SELECTION)));
    }

    void SetInteractionMode(ViewportInteraction::InteractionMode mode)
    {
        AZ::SettingsRegistryUtils::SetRegistry(AZ::SettingsRegistry::Get(),InteractionModeSetting, aznumeric_cast<AZ::u64>(mode));
    }

    ViewportInteraction::CoordinateSystem GetCoordinateSystem()
    {
        return static_cast<ViewportInteraction::CoordinateSystem>(
            AZ::SettingsRegistryUtils::GetRegistry(AZ::SettingsRegistry::Get(), CoordinateSystemSetting, aznumeric_cast<AZ::u64>(ViewportInteraction::CoordinateSystem::LOCAL)));
    }

    void SetCoordinateSystem(ViewportInteraction::CoordinateSystem system)
    {
        AZ::SettingsRegistryUtils::SetRegistry(AZ::SettingsRegistry::Get(),CoordinateSystemSetting, aznumeric_cast<AZ::u64>(system));
    }
}
