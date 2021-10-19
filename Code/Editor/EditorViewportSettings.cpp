/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
    constexpr AZStd::string_view AssetBrowserMaxItemsShownInSearchSetting = "/Amazon/Preferences/Editor/AssetBrowser/MaxItemsShowInSearch";
    constexpr AZStd::string_view GridSnappingSetting = "/Amazon/Preferences/Editor/GridSnapping";
    constexpr AZStd::string_view GridSizeSetting = "/Amazon/Preferences/Editor/GridSize";
    constexpr AZStd::string_view AngleSnappingSetting = "/Amazon/Preferences/Editor/AngleSnapping";
    constexpr AZStd::string_view AngleSizeSetting = "/Amazon/Preferences/Editor/AngleSize";
    constexpr AZStd::string_view ShowGridSetting = "/Amazon/Preferences/Editor/ShowGrid";
    constexpr AZStd::string_view StickySelectSetting = "/Amazon/Preferences/Editor/StickySelect";
    constexpr AZStd::string_view ManipulatorLineBoundWidthSetting = "/Amazon/Preferences/Editor/Manipulator/LineBoundWidth";
    constexpr AZStd::string_view ManipulatorCircleBoundWidthSetting = "/Amazon/Preferences/Editor/Manipulator/CircleBoundWidth";
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
    constexpr AZStd::string_view CameraTranslateSmoothingSetting = "/Amazon/Preferences/Editor/Camera/TranslateSmoothing";
    constexpr AZStd::string_view CameraRotateSmoothingSetting = "/Amazon/Preferences/Editor/Camera/RotateSmoothing";
    constexpr AZStd::string_view CameraCaptureCursorLookSetting = "/Amazon/Preferences/Editor/Camera/CaptureCursorLook";
    constexpr AZStd::string_view CameraTranslateForwardIdSetting = "/Amazon/Preferences/Editor/Camera/CameraTranslateForwardId";
    constexpr AZStd::string_view CameraTranslateBackwardIdSetting = "/Amazon/Preferences/Editor/Camera/CameraTranslateBackwardId";
    constexpr AZStd::string_view CameraTranslateLeftIdSetting = "/Amazon/Preferences/Editor/Camera/CameraTranslateLeftId";
    constexpr AZStd::string_view CameraTranslateRightIdSetting = "/Amazon/Preferences/Editor/Camera/CameraTranslateRightId";
    constexpr AZStd::string_view CameraTranslateUpIdSetting = "/Amazon/Preferences/Editor/Camera/CameraTranslateUpId";
    constexpr AZStd::string_view CameraTranslateDownIdSetting = "/Amazon/Preferences/Editor/Camera/CameraTranslateUpDownId";
    constexpr AZStd::string_view CameraTranslateBoostIdSetting = "/Amazon/Preferences/Editor/Camera/TranslateBoostId";
    constexpr AZStd::string_view CameraOrbitIdSetting = "/Amazon/Preferences/Editor/Camera/OrbitId";
    constexpr AZStd::string_view CameraFreeLookIdSetting = "/Amazon/Preferences/Editor/Camera/FreeLookId";
    constexpr AZStd::string_view CameraFreePanIdSetting = "/Amazon/Preferences/Editor/Camera/FreePanId";
    constexpr AZStd::string_view CameraOrbitLookIdSetting = "/Amazon/Preferences/Editor/Camera/OrbitLookId";
    constexpr AZStd::string_view CameraOrbitDollyIdSetting = "/Amazon/Preferences/Editor/Camera/OrbitDollyId";
    constexpr AZStd::string_view CameraOrbitPanIdSetting = "/Amazon/Preferences/Editor/Camera/OrbitPanId";
    constexpr AZStd::string_view CameraFocusIdSetting = "/Amazon/Preferences/Editor/Camera/FocusId";
    constexpr AZStd::string_view CameraDefaultStartingPositionX = "/Amazon/Preferences/Editor/Camera/DefaultStartingPosition/x";
    constexpr AZStd::string_view CameraDefaultStartingPositionY = "/Amazon/Preferences/Editor/Camera/DefaultStartingPosition/y";
    constexpr AZStd::string_view CameraDefaultStartingPositionZ = "/Amazon/Preferences/Editor/Camera/DefaultStartingPosition/z";

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

    AZ::Vector3 DefaultEditorCameraPosition()
    {
        float xPosition = aznumeric_cast<float>(GetRegistry(CameraDefaultStartingPositionX, 0.0));
        float yPosition = aznumeric_cast<float>(GetRegistry(CameraDefaultStartingPositionY, -10.0));
        float zPosition = aznumeric_cast<float>(GetRegistry(CameraDefaultStartingPositionZ, 4.0));
        return AZ::Vector3(xPosition, yPosition, zPosition);
    }

    void SetDefaultCameraEditorPosition(const AZ::Vector3 defaultCameraPosition)
    {
        SetRegistry(CameraDefaultStartingPositionX, defaultCameraPosition.GetX());
        SetRegistry(CameraDefaultStartingPositionY, defaultCameraPosition.GetY());
        SetRegistry(CameraDefaultStartingPositionZ, defaultCameraPosition.GetZ());
    }

    AZ::u64 MaxItemsShownInAssetBrowserSearch()
    {
        return GetRegistry(AssetBrowserMaxItemsShownInSearchSetting, aznumeric_cast<AZ::u64>(50));
    }

    void SetMaxItemsShownInAssetBrowserSearch(const AZ::u64 numberOfItemsShown)
    {
        SetRegistry(AssetBrowserMaxItemsShownInSearchSetting, numberOfItemsShown);
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

    bool StickySelectEnabled()
    {
        return GetRegistry(StickySelectSetting, false);
    }

    void SetStickySelectEnabled(const bool enabled)
    {
        SetRegistry(StickySelectSetting, enabled);
    }

    float ManipulatorLineBoundWidth()
    {
        return aznumeric_cast<float>(GetRegistry(ManipulatorLineBoundWidthSetting, 0.1));
    }

    void SetManipulatorLineBoundWidth(const float lineBoundWidth)
    {
        SetRegistry(ManipulatorLineBoundWidthSetting, lineBoundWidth);
    }

    float ManipulatorCircleBoundWidth()
    {
        return aznumeric_cast<float>(GetRegistry(ManipulatorCircleBoundWidthSetting, 0.1));
    }

    void SetManipulatorCircleBoundWidth(const float circleBoundWidth)
    {
        SetRegistry(ManipulatorCircleBoundWidthSetting, circleBoundWidth);
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

    bool CameraRotateSmoothingEnabled()
    {
        return GetRegistry(CameraRotateSmoothingSetting, true);
    }

    void SetCameraRotateSmoothingEnabled(const bool enabled)
    {
        SetRegistry(CameraRotateSmoothingSetting, enabled);
    }

    bool CameraTranslateSmoothingEnabled()
    {
        return GetRegistry(CameraTranslateSmoothingSetting, true);
    }

    void SetCameraTranslateSmoothingEnabled(const bool enabled)
    {
        SetRegistry(CameraTranslateSmoothingSetting, enabled);
    }

    bool CameraCaptureCursorForLook()
    {
        return GetRegistry(CameraCaptureCursorLookSetting, true);
    }

    void SetCameraCaptureCursorForLook(const bool capture)
    {
        SetRegistry(CameraCaptureCursorLookSetting, capture);
    }

    AzFramework::InputChannelId CameraTranslateForwardChannelId()
    {
        return AzFramework::InputChannelId(
            GetRegistry(CameraTranslateForwardIdSetting, AZStd::string("keyboard_key_alphanumeric_W")).c_str());
    }

    void SetCameraTranslateForwardChannelId(AZStd::string_view cameraTranslateForwardId)
    {
        SetRegistry(CameraTranslateForwardIdSetting, cameraTranslateForwardId);
    }

    AzFramework::InputChannelId CameraTranslateBackwardChannelId()
    {
        return AzFramework::InputChannelId(
            GetRegistry(CameraTranslateBackwardIdSetting, AZStd::string("keyboard_key_alphanumeric_S")).c_str());
    }

    void SetCameraTranslateBackwardChannelId(AZStd::string_view cameraTranslateBackwardId)
    {
        SetRegistry(CameraTranslateBackwardIdSetting, cameraTranslateBackwardId);
    }

    AzFramework::InputChannelId CameraTranslateLeftChannelId()
    {
        return AzFramework::InputChannelId(GetRegistry(CameraTranslateLeftIdSetting, AZStd::string("keyboard_key_alphanumeric_A")).c_str());
    }

    void SetCameraTranslateLeftChannelId(AZStd::string_view cameraTranslateLeftId)
    {
        SetRegistry(CameraTranslateLeftIdSetting, cameraTranslateLeftId);
    }

    AzFramework::InputChannelId CameraTranslateRightChannelId()
    {
        return AzFramework::InputChannelId(
            GetRegistry(CameraTranslateRightIdSetting, AZStd::string("keyboard_key_alphanumeric_D")).c_str());
    }

    void SetCameraTranslateRightChannelId(AZStd::string_view cameraTranslateRightId)
    {
        SetRegistry(CameraTranslateRightIdSetting, cameraTranslateRightId);
    }

    AzFramework::InputChannelId CameraTranslateUpChannelId()
    {
        return AzFramework::InputChannelId(GetRegistry(CameraTranslateUpIdSetting, AZStd::string("keyboard_key_alphanumeric_E")).c_str());
    }

    void SetCameraTranslateUpChannelId(AZStd::string_view cameraTranslateUpId)
    {
        SetRegistry(CameraTranslateUpIdSetting, cameraTranslateUpId);
    }

    AzFramework::InputChannelId CameraTranslateDownChannelId()
    {
        return AzFramework::InputChannelId(GetRegistry(CameraTranslateDownIdSetting, AZStd::string("keyboard_key_alphanumeric_Q")).c_str());
    }

    void SetCameraTranslateDownChannelId(AZStd::string_view cameraTranslateDownId)
    {
        SetRegistry(CameraTranslateDownIdSetting, cameraTranslateDownId);
    }

    AzFramework::InputChannelId CameraTranslateBoostChannelId()
    {
        return AzFramework::InputChannelId(
            GetRegistry(CameraTranslateBoostIdSetting, AZStd::string("keyboard_key_modifier_shift_l")).c_str());
    }

    void SetCameraTranslateBoostChannelId(AZStd::string_view cameraTranslateBoostId)
    {
        SetRegistry(CameraTranslateBoostIdSetting, cameraTranslateBoostId);
    }

    AzFramework::InputChannelId CameraOrbitChannelId()
    {
        return AzFramework::InputChannelId(GetRegistry(CameraOrbitIdSetting, AZStd::string("keyboard_key_modifier_alt_l")).c_str());
    }

    void SetCameraOrbitChannelId(AZStd::string_view cameraOrbitId)
    {
        SetRegistry(CameraOrbitIdSetting, cameraOrbitId);
    }

    AzFramework::InputChannelId CameraFreeLookChannelId()
    {
        return AzFramework::InputChannelId(GetRegistry(CameraFreeLookIdSetting, AZStd::string("mouse_button_right")).c_str());
    }

    void SetCameraFreeLookChannelId(AZStd::string_view cameraFreeLookId)
    {
        SetRegistry(CameraFreeLookIdSetting, cameraFreeLookId);
    }

    AzFramework::InputChannelId CameraFreePanChannelId()
    {
        return AzFramework::InputChannelId(GetRegistry(CameraFreePanIdSetting, AZStd::string("mouse_button_middle")).c_str());
    }

    void SetCameraFreePanChannelId(AZStd::string_view cameraFreePanId)
    {
        SetRegistry(CameraFreePanIdSetting, cameraFreePanId);
    }

    AzFramework::InputChannelId CameraOrbitLookChannelId()
    {
        return AzFramework::InputChannelId(GetRegistry(CameraOrbitLookIdSetting, AZStd::string("mouse_button_left")).c_str());
    }

    void SetCameraOrbitLookChannelId(AZStd::string_view cameraOrbitLookId)
    {
        SetRegistry(CameraOrbitLookIdSetting, cameraOrbitLookId);
    }

    AzFramework::InputChannelId CameraOrbitDollyChannelId()
    {
        return AzFramework::InputChannelId(GetRegistry(CameraOrbitDollyIdSetting, AZStd::string("mouse_button_right")).c_str());
    }

    void SetCameraOrbitDollyChannelId(AZStd::string_view cameraOrbitDollyId)
    {
        SetRegistry(CameraOrbitDollyIdSetting, cameraOrbitDollyId);
    }

    AzFramework::InputChannelId CameraOrbitPanChannelId()
    {
        return AzFramework::InputChannelId(GetRegistry(CameraOrbitPanIdSetting, AZStd::string("mouse_button_middle")).c_str());
    }

    void SetCameraOrbitPanChannelId(AZStd::string_view cameraOrbitPanId)
    {
        SetRegistry(CameraOrbitPanIdSetting, cameraOrbitPanId);
    }

    AzFramework::InputChannelId CameraFocusChannelId()
    {
        return AzFramework::InputChannelId(GetRegistry(CameraFocusIdSetting, AZStd::string("keyboard_key_alphanumeric_X")).c_str());
    }

    void SetCameraFocusChannelId(AZStd::string_view cameraFocusId)
    {
        SetRegistry(CameraFocusIdSetting, cameraFocusId);
    }
} // namespace SandboxEditor
