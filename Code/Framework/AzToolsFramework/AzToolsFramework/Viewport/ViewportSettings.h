/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Settings/SettingsRegistry.h>

namespace AzToolsFramework
{
    template<typename T>
    void SetRegistry(const AZStd::string_view setting, T&& value)
    {
        if (auto* registry = AZ::SettingsRegistry::Get())
        {
            registry->Set(setting, AZStd::forward<T>(value));
        }
    }

    template<typename T>
    AZStd::remove_cvref_t<T> GetRegistry(const AZStd::string_view setting, T&& defaultValue)
    {
        AZStd::remove_cvref_t<T> value = AZStd::forward<T>(defaultValue);
        if (const auto* registry = AZ::SettingsRegistry::Get())
        {
            T potentialValue;
            if (registry->Get(potentialValue, setting))
            {
                value = AZStd::move(potentialValue);
            }
        }

        return value;
    }

    inline void ClearRegistry(const AZStd::string_view setting)
    {
        if (auto* registry = AZ::SettingsRegistry::Get())
        {
            registry->Remove(setting);
        }
    }

    inline constexpr float DefaultManipulatorViewBaseScale = 1.0f;
    inline constexpr float MinManipulatorViewBaseScale = 0.25f;
    inline constexpr float MaxManipulatorViewBaseScale = 2.0f;

    bool FlipManipulatorAxesTowardsView();
    void SetFlipManipulatorAxesTowardsView(bool enabled);

    float LinearManipulatorAxisLength();
    void SetLinearManipulatorAxisLength(float length);

    float PlanarManipulatorAxisLength();
    void SetPlanarManipulatorAxisLength(float length);

    float SurfaceManipulatorRadius();
    void SetSurfaceManipulatorRadius(float radius);

    float SurfaceManipulatorOpacity();
    void SetSurfaceManipulatorOpacity(float opacity);

    float LinearManipulatorConeLength();
    void SetLinearManipulatorConeLength(float length);

    float LinearManipulatorConeRadius();
    void SetLinearManipulatorConeRadius(float radius);

    float ScaleManipulatorBoxHalfExtent();
    void SetScaleManipulatorBoxHalfExtent(float halfExtent);

    float RotationManipulatorRadius();
    void SetRotationManipulatorRadius(float radius);

    float ManipulatorViewBaseScale();
    void SetManipulatorViewBaseScale(float scale);

    bool IconsVisible();
    void SetIconsVisible(bool visible);

    bool HelpersVisible();
    void SetHelpersVisible(bool visible);

    bool OnlyShowHelpersForSelectedEntities();
    void SetOnlyShowHelpersForSelectedEntities(bool visible);

    bool ComponentSwitcherEnabled();

    bool PrefabEditModeEffectEnabled();
    void SetPrefabEditModeEffectEnabled(bool enabled);

} // namespace AzToolsFramework
