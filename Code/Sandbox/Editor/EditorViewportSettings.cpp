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
    constexpr AZStd::string_view CameraTranslateSpeedSetting = "/Amazon/Preferences/Editor/Camera/TranslateSpeed";
    constexpr AZStd::string_view CameraBoostMultiplierSetting = "/Amazon/Preferences/Editor/Camera/BoostMultiplier";
    constexpr AZStd::string_view CameraRotateSpeedSetting = "/Amazon/Preferences/Editor/Camera/RotateSpeed";
    constexpr AZStd::string_view CameraScrollSpeedSetting = "/Amazon/Preferences/Editor/Camera/DollyScrollSpeed";
    constexpr AZStd::string_view CameraDollyMotionSpeedSetting = "/Amazon/Preferences/Editor/Camera/DollyMotionSpeed";
    constexpr AZStd::string_view CameraOrbitYawRotationInvertedSetting = "/Amazon/Preferences/Editor/Camera/YawRotationInverted";
    constexpr AZStd::string_view CameraPanInvertedXSetting = "/Amazon/Preferences/Editor/Camera/PanInvertedX";
    constexpr AZStd::string_view CameraPanInvertedYSetting = "/Amazon/Preferences/Editor/Camera/PanInvertedY";
    constexpr AZStd::string_view CameraPanSpeedSetting = "/Amazon/Preferences/Editor/Camera/PanSpeed";
    constexpr AZStd::string_view CameraRotateSmoothnessSetting = "/Amazon/Preferences/Editor/Camera/RotateSmoothness";
    constexpr AZStd::string_view CameraTranslateSmoothnessSetting = "/Amazon/Preferences/Editor/Camera/TranslateSmoothness";

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
        if (auto* registry = AZ::SettingsRegistry::Get())
        {
            registry->Get(value, setting);
        }

        return value;
    }

    bool GridSnappingEnabled()
    {
        return GetRegistry(GridSnappingSetting, false);
    }

    void SetGridSnapping(const bool enabled)
    {
        SetRegistry(GridSnappingSetting, enabled);
    }

    float GridSnappingSize()
    {
        return aznumeric_cast<float>(GetRegistry(GridSizeSetting, 0.1));
    }

    void SetGridSnappingSize(const float size)
    {
        SetRegistry(GridSizeSetting, size);
    }

    bool AngleSnappingEnabled()
    {
        return GetRegistry(AngleSnappingSetting, false);
    }

    void SetAngleSnapping(const bool enabled)
    {
        SetRegistry(AngleSnappingSetting, enabled);
    }

    float AngleSnappingSize()
    {
        return aznumeric_cast<float>(GetRegistry(AngleSizeSetting, 5.0));
    }

    void SetAngleSnappingSize(const float size)
    {
        SetRegistry(AngleSizeSetting, size);
    }

    bool ShowingGrid()
    {
        return GetRegistry(ShowGridSetting, false);
    }

    void SetShowingGrid(const bool showing)
    {
        SetRegistry(ShowGridSetting, showing);
    }

    float CameraTranslateSpeed()
    {
        return aznumeric_cast<float>(GetRegistry(CameraTranslateSpeedSetting, 10.0));
    }

    void SetCameraTranslateSpeed(const float speed)
    {
        SetRegistry(CameraTranslateSpeedSetting, speed);
    }

    float CameraBoostMultiplier()
    {
        return aznumeric_cast<float>(GetRegistry(CameraBoostMultiplierSetting, 3.0));
    }

    void SetCameraBoostMultiplier(const float multiplier)
    {
        SetRegistry(CameraBoostMultiplierSetting, multiplier);
    }

    float CameraRotateSpeed()
    {
        return aznumeric_cast<float>(GetRegistry(CameraRotateSpeedSetting, 0.005));
    }

    void SetCameraRotateSpeed(const float speed)
    {
        SetRegistry(CameraRotateSpeedSetting, speed);
    }

    float CameraScrollSpeed()
    {
        return aznumeric_cast<float>(GetRegistry(CameraScrollSpeedSetting, 0.02));
    }

    void SetCameraScrollSpeed(const float speed)
    {
        SetRegistry(CameraScrollSpeedSetting, speed);
    }

    float CameraDollyMotionSpeed()
    {
        return aznumeric_cast<float>(GetRegistry(CameraDollyMotionSpeedSetting, 0.01));
    }

    void SetCameraDollyMotionSpeed(const float speed)
    {
        SetRegistry(CameraDollyMotionSpeedSetting, speed);
    }

    bool CameraOrbitYawRotationInverted()
    {
        return GetRegistry(CameraOrbitYawRotationInvertedSetting, false);
    }

    void SetCameraOrbitYawRotationInverted(const bool inverted)
    {
        SetRegistry(CameraOrbitYawRotationInvertedSetting, inverted);
    }

    bool CameraPanInvertedX()
    {
        return GetRegistry(CameraPanInvertedXSetting, true);
    }

    void SetCameraPanInvertedX(const bool inverted)
    {
        SetRegistry(CameraPanInvertedXSetting, inverted);
    }

    bool CameraPanInvertedY()
    {
        return GetRegistry(CameraPanInvertedYSetting, true);
    }

    void SetCameraPanInvertedY(const bool inverted)
    {
        SetRegistry(CameraPanInvertedYSetting, inverted);
    }

    float CameraPanSpeed()
    {
        return aznumeric_cast<float>(GetRegistry(CameraPanSpeedSetting, 0.01));
    }

    void SetCameraPanSpeed(float speed)
    {
        SetRegistry(CameraPanSpeedSetting, speed);
    }

    float CameraRotateSmoothness()
    {
        return aznumeric_cast<float>(GetRegistry(CameraRotateSmoothnessSetting, 5.0));
    }

    void SetCameraRotateSmoothness(const float smoothness)
    {
        SetRegistry(CameraRotateSmoothnessSetting, smoothness);
    }

    float CameraTranslateSmoothness()
    {
        return aznumeric_cast<float>(GetRegistry(CameraTranslateSmoothnessSetting, 5.0));
    }

    void SetCameraTranslateSmoothness(const float smoothness)
    {
        SetRegistry(CameraTranslateSmoothnessSetting, smoothness);
    }
} // namespace SandboxEditor
