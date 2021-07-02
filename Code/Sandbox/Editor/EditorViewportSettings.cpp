/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EditorViewportSettings.h>

#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Settings/SettingsRegistry.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
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
    constexpr AZStd::string_view CameraTranslateForwardKeySetting = "/Amazon/Preferences/Editor/Camera/CameraTranslateForwardKey";
    constexpr AZStd::string_view CameraTranslateBackwardKeySetting = "/Amazon/Preferences/Editor/Camera/CameraTranslateBackwardKey";
    constexpr AZStd::string_view CameraTranslateLeftKeySetting = "/Amazon/Preferences/Editor/Camera/CameraTranslateLeftKey";
    constexpr AZStd::string_view CameraTranslateRightKeySetting = "/Amazon/Preferences/Editor/Camera/CameraTranslateRightKey";
    constexpr AZStd::string_view CameraTranslateUpKeySetting = "/Amazon/Preferences/Editor/Camera/CameraTranslateUpKey";
    constexpr AZStd::string_view CameraTranslateDownKeySetting = "/Amazon/Preferences/Editor/Camera/CameraTranslateUpDownKey";

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

    struct EditorViewportSettingsCallbacksImpl : public EditorViewportSettingsCallbacks
    {
        EditorViewportSettingsCallbacksImpl()
        {
            if (auto* registry = AZ::SettingsRegistry::Get())
            {
                using AZ::SettingsRegistryMergeUtils::IsPathAncestorDescendantOrEqual;

                m_notifyEventHandler = registry->RegisterNotifier(
                    [this](const AZStd::string_view path, [[maybe_unused]] const AZ::SettingsRegistryInterface::Type type)
                    {
                        if (IsPathAncestorDescendantOrEqual(GridSnappingSetting, path))
                        {
                            m_gridSnappingChanged.Signal(GridSnappingEnabled());
                        }
                    });
            }
        }

        void SetGridSnappingChangedEvent(GridSnappingChangedEvent::Handler& handler) override
        {
            handler.Connect(m_gridSnappingChanged);
        }

        GridSnappingChangedEvent m_gridSnappingChanged;
        AZ::SettingsRegistryInterface::NotifyEventHandler m_notifyEventHandler;
    };

    AZStd::unique_ptr<EditorViewportSettingsCallbacks> CreateEditorViewportSettingsCallbacks()
    {
        return AZStd::make_unique<EditorViewportSettingsCallbacksImpl>();
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

    AzFramework::InputChannelId CameraTranslateForwardKey()
    {
        return AzFramework::InputChannelId(
            GetRegistry(CameraTranslateForwardKeySetting, AZStd::string("keyboard_key_alphanumeric_W")).c_str());
    }

    void SetCameraTranslateForwardKey(AZStd::string_view cameraTranslateForwardKey)
    {
        SetRegistry(CameraTranslateForwardKeySetting, cameraTranslateForwardKey);
    }

    AzFramework::InputChannelId CameraTranslateBackwardKey()
    {
        return AzFramework::InputChannelId(
            GetRegistry(CameraTranslateBackwardKeySetting, AZStd::string("keyboard_key_alphanumeric_S")).c_str());
    }

    void SetCameraTranslateBackwardKey(AZStd::string_view cameraTranslateBackwardKey)
    {
        SetRegistry(CameraTranslateBackwardKeySetting, cameraTranslateBackwardKey);
    }

    AzFramework::InputChannelId CameraTranslateLeftKey()
    {
        return AzFramework::InputChannelId(
            GetRegistry(CameraTranslateLeftKeySetting, AZStd::string("keyboard_key_alphanumeric_A")).c_str());
    }

    void SetCameraTranslateLeftKey(AZStd::string_view cameraTranslateLeftKey)
    {
        SetRegistry(CameraTranslateLeftKeySetting, cameraTranslateLeftKey);
    }

    AzFramework::InputChannelId CameraTranslateRightKey()
    {
        return AzFramework::InputChannelId(
            GetRegistry(CameraTranslateRightKeySetting, AZStd::string("keyboard_key_alphanumeric_D")).c_str());
    }

    void SetCameraTranslateRightKey(AZStd::string_view cameraTranslateRightKey)
    {
        SetRegistry(CameraTranslateRightKeySetting, cameraTranslateRightKey);
    }

    AzFramework::InputChannelId CameraTranslateUpKey()
    {
        return AzFramework::InputChannelId(
            GetRegistry(CameraTranslateUpKeySetting, AZStd::string("keyboard_key_alphanumeric_E")).c_str());
    }

    void SetCameraTranslateUpKey(AZStd::string_view cameraTranslateUpKey)
    {
        SetRegistry(CameraTranslateUpKeySetting, cameraTranslateUpKey);
    }

    AzFramework::InputChannelId CameraTranslateDownKey()
    {
        return AzFramework::InputChannelId(
            GetRegistry(CameraTranslateDownKeySetting, AZStd::string("keyboard_key_alphanumeric_Q")).c_str());
    }

    void SetCameraTranslateDownKey(AZStd::string_view cameraTranslateDownKey)
    {
        SetRegistry(CameraTranslateDownKeySetting, cameraTranslateDownKey);
    }
} // namespace SandboxEditor
