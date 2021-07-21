/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EditorViewportSettings.h>

#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Settings/SettingsRegistry.h>
#include <AzCore/std/string/string_view.h>

namespace SandboxEditor
{
    constexpr AZStd::string_view GridSnappingSetting = "/Amazon/Preferences/Editor/GridSnapping";
    constexpr AZStd::string_view GridSizeSetting = "/Amazon/Preferences/Editor/GridSize";
    constexpr AZStd::string_view AngleSnappingSetting = "/Amazon/Preferences/Editor/AngleSnapping";
    constexpr AZStd::string_view AngleSizeSetting = "/Amazon/Preferences/Editor/AngleSize";
    constexpr AZStd::string_view ShowGridSetting = "/Amazon/Preferences/Editor/ShowGrid";

    bool GridSnappingEnabled()
    {
        bool enabled = false;
        if (auto* registry = AZ::SettingsRegistry::Get())
        {
            registry->Get(enabled, GridSnappingSetting);
        }
        return enabled;
    }

    float GridSnappingSize()
    {
        double gridSize = 0.1;
        if (auto* registry = AZ::SettingsRegistry::Get())
        {
            registry->Get(gridSize, GridSizeSetting);
        }
        return aznumeric_cast<float>(gridSize);
    }

    bool AngleSnappingEnabled()
    {
        bool enabled = false;
        if (auto* registry = AZ::SettingsRegistry::Get())
        {
            registry->Get(enabled, AngleSnappingSetting);
        }
        return enabled;
    }

    float AngleSnappingSize()
    {
        double angleSize = 5.0;
        if (auto* registry = AZ::SettingsRegistry::Get())
        {
            registry->Get(angleSize, AngleSizeSetting);
        }
        return aznumeric_cast<float>(angleSize);
    }

    bool ShowingGrid()
    {
        bool enabled = false;
        if (auto* registry = AZ::SettingsRegistry::Get())
        {
            registry->Get(enabled, ShowGridSetting);
        }
        return enabled;
    }

    void SetGridSnapping(const bool enabled)
    {
        if (auto* registry = AZ::SettingsRegistry::Get())
        {
            registry->Set(GridSnappingSetting, enabled);
        }
    }

    void SetGridSnappingSize(const float size)
    {
        if (auto* registry = AZ::SettingsRegistry::Get())
        {
            registry->Set(GridSizeSetting, size);
        }
    }

    void SetAngleSnapping(const bool enabled)
    {
        if (auto* registry = AZ::SettingsRegistry::Get())
        {
            registry->Set(AngleSnappingSetting, enabled);
        }
    }

    void SetAngleSnappingSize(const float size)
    {
        if (auto* registry = AZ::SettingsRegistry::Get())
        {
            registry->Set(AngleSizeSetting, size);
        }
    }

    void SetShowingGrid(const bool showing)
    {
        if (auto* registry = AZ::SettingsRegistry::Get())
        {
            registry->Set(ShowGridSetting, showing);
        }
    }
} // namespace SandboxEditor
