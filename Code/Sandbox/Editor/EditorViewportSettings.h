/*
 * All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 * its licensors.
 *
 * For complete copyright and license terms please see the LICENSE at the root of this
 * distribution (the "License"). All use of this software is governed by the License,
 * or, if provided, by the license below or the license accompanying this file. Do not
 * remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 */

#pragma once

#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Settings/SettingsRegistry.h>
#include <AzCore/std/string/string_view.h>

namespace Editor
{
    inline constexpr const AZStd::string_view GridSnappingSetting = "/Amazon/Preferences/Editor/GridSnapping";
    inline constexpr const AZStd::string_view GridSizeSetting = "/Amazon/Preferences/Editor/GridSize";
    inline constexpr const AZStd::string_view AngleSnappingSetting = "/Amazon/Preferences/Editor/AngleSnapping";
    inline constexpr const AZStd::string_view AngleSizeSetting = "/Amazon/Preferences/Editor/AngleSize";
    inline constexpr const AZStd::string_view ShowGridSetting = "/Amazon/Preferences/Editor/ShowGrid";

    inline bool GridSnappingEnabled()
    {
        bool enabled = false;
        if (auto* registry = AZ::SettingsRegistry::Get())
        {
            registry->Get(enabled, GridSnappingSetting);
        }
        return enabled;
    }

    inline float GridSnappingSize()
    {
        double gridSize = false;
        if (auto* registry = AZ::SettingsRegistry::Get())
        {
            registry->Get(gridSize, GridSizeSetting);
        }
        return aznumeric_cast<float>(gridSize);
    }

    inline bool AngleSnappingEnabled()
    {
        bool enabled = false;
        if (auto* registry = AZ::SettingsRegistry::Get())
        {
            registry->Get(enabled, AngleSnappingSetting);
        }
        return enabled;
    }

    inline float AngleSnappingSize()
    {
        double gridSize = false;
        if (auto* registry = AZ::SettingsRegistry::Get())
        {
            registry->Get(gridSize, AngleSizeSetting);
        }
        return aznumeric_cast<float>(gridSize);
    }

    inline bool ShowingGrid()
    {
        bool enabled = false;
        if (auto* registry = AZ::SettingsRegistry::Get())
        {
            registry->Get(enabled, ShowGridSetting);
        }
        return enabled;
    }

    inline void SetGridSnapping(const bool enabled)
    {
        if (auto* registry = AZ::SettingsRegistry::Get())
        {
            registry->Set(GridSnappingSetting, enabled);
        }
    }

    inline void SetGridSnappingSize(const float size)
    {
        if (auto* registry = AZ::SettingsRegistry::Get())
        {
            registry->Set(GridSizeSetting, size);
        }
    }

    inline void SetAngleSnapping(const bool enabled)
    {
        if (auto* registry = AZ::SettingsRegistry::Get())
        {
            registry->Set(AngleSnappingSetting, enabled);
        }
    }

    inline void SetAngleSnappingSize(const float size)
    {
        if (auto* registry = AZ::SettingsRegistry::Get())
        {
            registry->Set(AngleSizeSetting, size);
        }
    }

    inline void SetShowingGrid(const bool showing)
    {
        if (auto* registry = AZ::SettingsRegistry::Get())
        {
            registry->Set(ShowGridSetting, showing);
        }
    }
} // namespace Editor
