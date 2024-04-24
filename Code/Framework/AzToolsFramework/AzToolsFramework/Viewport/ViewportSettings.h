/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/Viewport/ViewportTypes.h>

namespace AzToolsFramework
{
    inline constexpr float DefaultManipulatorViewBaseScale = 1.0f;
    inline constexpr float MinManipulatorViewBaseScale = 0.25f;
    inline constexpr float MaxManipulatorViewBaseScale = 2.0f;
    inline constexpr ViewportInteraction::KeyboardModifier DefaultSymmetricalEditingModifier = ViewportInteraction::KeyboardModifier::Shift;

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
