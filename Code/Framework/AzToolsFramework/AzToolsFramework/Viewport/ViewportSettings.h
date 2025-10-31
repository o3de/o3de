/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/AzToolsFrameworkAPI.h>

#include <AzToolsFramework/Viewport/ViewportTypes.h>

namespace AzToolsFramework
{
    inline constexpr float DefaultManipulatorViewBaseScale = 1.0f;
    inline constexpr float MinManipulatorViewBaseScale = 0.25f;
    inline constexpr float MaxManipulatorViewBaseScale = 2.0f;
    inline constexpr ViewportInteraction::KeyboardModifier DefaultSymmetricalEditingModifier = ViewportInteraction::KeyboardModifier::Shift;

    AZTF_API bool FlipManipulatorAxesTowardsView();
    AZTF_API void SetFlipManipulatorAxesTowardsView(bool enabled);

    AZTF_API float LinearManipulatorAxisLength();
    AZTF_API void SetLinearManipulatorAxisLength(float length);

    AZTF_API float PlanarManipulatorAxisLength();
    AZTF_API void SetPlanarManipulatorAxisLength(float length);

    AZTF_API float SurfaceManipulatorRadius();
    AZTF_API void SetSurfaceManipulatorRadius(float radius);

    AZTF_API float SurfaceManipulatorOpacity();
    AZTF_API void SetSurfaceManipulatorOpacity(float opacity);

    AZTF_API float LinearManipulatorConeLength();
    AZTF_API void SetLinearManipulatorConeLength(float length);

    AZTF_API float LinearManipulatorConeRadius();
    AZTF_API void SetLinearManipulatorConeRadius(float radius);

    AZTF_API float ScaleManipulatorBoxHalfExtent();
    AZTF_API void SetScaleManipulatorBoxHalfExtent(float halfExtent);

    AZTF_API float RotationManipulatorRadius();
    AZTF_API void SetRotationManipulatorRadius(float radius);

    AZTF_API float ManipulatorViewBaseScale();
    AZTF_API void SetManipulatorViewBaseScale(float scale);

    AZTF_API bool IconsVisible();
    AZTF_API void SetIconsVisible(bool visible);

    AZTF_API bool HelpersVisible();
    AZTF_API void SetHelpersVisible(bool visible);

    AZTF_API bool OnlyShowHelpersForSelectedEntities();
    AZTF_API void SetOnlyShowHelpersForSelectedEntities(bool visible);

    AZTF_API bool ComponentSwitcherEnabled();

    AZTF_API bool PrefabEditModeEffectEnabled();
    AZTF_API void SetPrefabEditModeEffectEnabled(bool enabled);

} // namespace AzToolsFramework
