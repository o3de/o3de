/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/Viewport/ViewportSettings.h>

namespace AzToolsFramework
{
    constexpr AZStd::string_view FlipManipulatorAxesTowardsViewSetting = "/Amazon/Preferences/Editor/Manipulator/FlipManipulatorAxesTowardsView";
    constexpr AZStd::string_view LinearManipulatorAxisLengthSetting = "/Amazon/Preferences/Editor/Manipulator/LinearManipulatorAxisLength";
    constexpr AZStd::string_view PlanarManipulatorAxisLengthSetting = "/Amazon/Preferences/Editor/Manipulator/PlanarManipulatorAxisLength";
    constexpr AZStd::string_view SurfaceManipulatorRadiusSetting = "/Amazon/Preferences/Editor/Manipulator/SurfaceManipulatorRadius";
    constexpr AZStd::string_view SurfaceManipulatorOpacitySetting = "/Amazon/Preferences/Editor/Manipulator/SurfaceManipulatorOpacity";
    constexpr AZStd::string_view LinearManipulatorConeLengthSetting = "/Amazon/Preferences/Editor/Manipulator/LinearManipulatorConeLength";
    constexpr AZStd::string_view LinearManipulatorConeRadiusSetting = "/Amazon/Preferences/Editor/Manipulator/LinearManipulatorConeRadius";
    constexpr AZStd::string_view ScaleManipulatorBoxHalfExtentSetting = "/Amazon/Preferences/Editor/Manipulator/ScaleManipulatorBoxHalfExtent";
    constexpr AZStd::string_view RotationManipulatorRadiusSetting = "/Amazon/Preferences/Editor/Manipulator/RotationManipulatorRadius";
    constexpr AZStd::string_view ManipulatorViewBaseScaleSetting = "/Amazon/Preferences/Editor/Manipulator/ViewBaseScale";

    bool FlipManipulatorAxesTowardsView()
    {
        return GetRegistry(FlipManipulatorAxesTowardsViewSetting, true);
    }

    void SetFlipManipulatorAxesTowardsView(const bool enabled)
    {
        SetRegistry(FlipManipulatorAxesTowardsViewSetting, enabled);
    }

    float LinearManipulatorAxisLength()
    {
        return aznumeric_cast<float>(GetRegistry(LinearManipulatorAxisLengthSetting, 2.0));
    }

    void SetLinearManipulatorAxisLength(const float length)
    {
        SetRegistry(LinearManipulatorAxisLengthSetting, length);
    }

    float PlanarManipulatorAxisLength()
    {
        return aznumeric_cast<float>(GetRegistry(PlanarManipulatorAxisLengthSetting, 0.6));
    }

    void SetPlanarManipulatorAxisLength(const float length)
    {
        SetRegistry(PlanarManipulatorAxisLengthSetting, length);
    }

    float SurfaceManipulatorRadius()
    {
        return aznumeric_cast<float>(GetRegistry(SurfaceManipulatorRadiusSetting, 0.1));
    }

    void SetSurfaceManipulatorRadius(const float radius)
    {
        SetRegistry(SurfaceManipulatorRadiusSetting, radius);
    }

    float SurfaceManipulatorOpacity()
    {
        return aznumeric_cast<float>(GetRegistry(SurfaceManipulatorOpacitySetting, 0.75));
    }

    void SetSurfaceManipulatorOpacity(const float opacity)
    {
        SetRegistry(SurfaceManipulatorOpacitySetting, opacity);
    }

    float LinearManipulatorConeLength()
    {
        return aznumeric_cast<float>(GetRegistry(LinearManipulatorConeLengthSetting, 0.28));
    }

    void SetLinearManipulatorConeLength(const float length)
    {
        SetRegistry(LinearManipulatorConeLengthSetting, length);
    }

    float LinearManipulatorConeRadius()
    {
        return aznumeric_cast<float>(GetRegistry(LinearManipulatorConeRadiusSetting, 0.1));
    }

    void SetLinearManipulatorConeRadius(const float radius)
    {
        SetRegistry(LinearManipulatorConeRadiusSetting, radius);
    }

    float ScaleManipulatorBoxHalfExtent()
    {
        return aznumeric_cast<float>(GetRegistry(ScaleManipulatorBoxHalfExtentSetting, 0.1));
    }

    void SetScaleManipulatorBoxHalfExtent(const float size)
    {
        SetRegistry(ScaleManipulatorBoxHalfExtentSetting, size);
    }

    float RotationManipulatorRadius()
    {
        return aznumeric_cast<float>(GetRegistry(RotationManipulatorRadiusSetting, 2.0));
    }

    void SetRotationManipulatorRadius(const float radius)
    {
        SetRegistry(RotationManipulatorRadiusSetting, radius);
    }

    float ManipulatorViewBaseScale()
    {
        return aznumeric_cast<float>(GetRegistry(ManipulatorViewBaseScaleSetting, 1.0));
    }

    void SetManipulatorViewBaseScale(const float scale)
    {
        SetRegistry(ManipulatorViewBaseScaleSetting, scale);
    }
} // namespace AzToolsFramework
