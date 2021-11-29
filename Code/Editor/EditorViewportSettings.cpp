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
#include <AzToolsFramework/Viewport/ViewportSettings.h>

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
    constexpr AZStd::string_view CameraDefaultOrbitDistanceSetting = "/Amazon/Preferences/Editor/Camera/DefaultOrbitDistance";
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

    namespace aztf = AzToolsFramework;

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

    AZ::Vector3 CameraDefaultEditorPosition()
    {
        return AZ::Vector3(
            aznumeric_cast<float>(aztf::GetRegistry(CameraDefaultStartingPositionX, 0.0)),
            aznumeric_cast<float>(aztf::GetRegistry(CameraDefaultStartingPositionY, -10.0)),
            aznumeric_cast<float>(aztf::GetRegistry(CameraDefaultStartingPositionZ, 4.0)));
    }

    void SetCameraDefaultEditorPosition(const AZ::Vector3& defaultCameraPosition)
    {
        aztf::SetRegistry(CameraDefaultStartingPositionX, defaultCameraPosition.GetX());
        aztf::SetRegistry(CameraDefaultStartingPositionY, defaultCameraPosition.GetY());
        aztf::SetRegistry(CameraDefaultStartingPositionZ, defaultCameraPosition.GetZ());
    }

    AZ::u64 MaxItemsShownInAssetBrowserSearch()
    {
        return aztf::GetRegistry(AssetBrowserMaxItemsShownInSearchSetting, aznumeric_cast<AZ::u64>(50));
    }

    void SetMaxItemsShownInAssetBrowserSearch(const AZ::u64 numberOfItemsShown)
    {
        aztf::SetRegistry(AssetBrowserMaxItemsShownInSearchSetting, numberOfItemsShown);
    }

    bool GridSnappingEnabled()
    {
        return aztf::GetRegistry(GridSnappingSetting, false);
    }

    void SetGridSnapping(const bool enabled)
    {
        aztf::SetRegistry(GridSnappingSetting, enabled);
    }

    float GridSnappingSize()
    {
        return aznumeric_cast<float>(aztf::GetRegistry(GridSizeSetting, 0.1));
    }

    void SetGridSnappingSize(const float size)
    {
        aztf::SetRegistry(GridSizeSetting, size);
    }

    bool AngleSnappingEnabled()
    {
        return aztf::GetRegistry(AngleSnappingSetting, false);
    }

    void SetAngleSnapping(const bool enabled)
    {
        aztf::SetRegistry(AngleSnappingSetting, enabled);
    }

    float AngleSnappingSize()
    {
        return aznumeric_cast<float>(aztf::GetRegistry(AngleSizeSetting, 5.0));
    }

    void SetAngleSnappingSize(const float size)
    {
        aztf::SetRegistry(AngleSizeSetting, size);
    }

    bool ShowingGrid()
    {
        return aztf::GetRegistry(ShowGridSetting, false);
    }

    void SetShowingGrid(const bool showing)
    {
        aztf::SetRegistry(ShowGridSetting, showing);
    }

    bool StickySelectEnabled()
    {
        return aztf::GetRegistry(StickySelectSetting, false);
    }

    void SetStickySelectEnabled(const bool enabled)
    {
        aztf::SetRegistry(StickySelectSetting, enabled);
    }

    float ManipulatorLineBoundWidth()
    {
        return aznumeric_cast<float>(aztf::GetRegistry(ManipulatorLineBoundWidthSetting, 0.1));
    }

    void SetManipulatorLineBoundWidth(const float lineBoundWidth)
    {
        aztf::SetRegistry(ManipulatorLineBoundWidthSetting, lineBoundWidth);
    }

    float ManipulatorCircleBoundWidth()
    {
        return aznumeric_cast<float>(aztf::GetRegistry(ManipulatorCircleBoundWidthSetting, 0.1));
    }

    void SetManipulatorCircleBoundWidth(const float circleBoundWidth)
    {
        aztf::SetRegistry(ManipulatorCircleBoundWidthSetting, circleBoundWidth);
    }

    float CameraTranslateSpeed()
    {
        return aznumeric_cast<float>(aztf::GetRegistry(CameraTranslateSpeedSetting, 10.0));
    }

    void SetCameraTranslateSpeed(const float speed)
    {
        aztf::SetRegistry(CameraTranslateSpeedSetting, speed);
    }

    float CameraBoostMultiplier()
    {
        return aznumeric_cast<float>(aztf::GetRegistry(CameraBoostMultiplierSetting, 3.0));
    }

    void SetCameraBoostMultiplier(const float multiplier)
    {
        aztf::SetRegistry(CameraBoostMultiplierSetting, multiplier);
    }

    float CameraRotateSpeed()
    {
        return aznumeric_cast<float>(aztf::GetRegistry(CameraRotateSpeedSetting, 0.005));
    }

    void SetCameraRotateSpeed(const float speed)
    {
        aztf::SetRegistry(CameraRotateSpeedSetting, speed);
    }

    float CameraScrollSpeed()
    {
        return aznumeric_cast<float>(aztf::GetRegistry(CameraScrollSpeedSetting, 0.02));
    }

    void SetCameraScrollSpeed(const float speed)
    {
        aztf::SetRegistry(CameraScrollSpeedSetting, speed);
    }

    float CameraDollyMotionSpeed()
    {
        return aznumeric_cast<float>(aztf::GetRegistry(CameraDollyMotionSpeedSetting, 0.01));
    }

    void SetCameraDollyMotionSpeed(const float speed)
    {
        aztf::SetRegistry(CameraDollyMotionSpeedSetting, speed);
    }

    bool CameraOrbitYawRotationInverted()
    {
        return aztf::GetRegistry(CameraOrbitYawRotationInvertedSetting, false);
    }

    void SetCameraOrbitYawRotationInverted(const bool inverted)
    {
        aztf::SetRegistry(CameraOrbitYawRotationInvertedSetting, inverted);
    }

    bool CameraPanInvertedX()
    {
        return aztf::GetRegistry(CameraPanInvertedXSetting, true);
    }

    void SetCameraPanInvertedX(const bool inverted)
    {
        aztf::SetRegistry(CameraPanInvertedXSetting, inverted);
    }

    bool CameraPanInvertedY()
    {
        return aztf::GetRegistry(CameraPanInvertedYSetting, true);
    }

    void SetCameraPanInvertedY(const bool inverted)
    {
        aztf::SetRegistry(CameraPanInvertedYSetting, inverted);
    }

    float CameraPanSpeed()
    {
        return aznumeric_cast<float>(aztf::GetRegistry(CameraPanSpeedSetting, 0.01));
    }

    void SetCameraPanSpeed(float speed)
    {
        aztf::SetRegistry(CameraPanSpeedSetting, speed);
    }

    float CameraRotateSmoothness()
    {
        return aznumeric_cast<float>(aztf::GetRegistry(CameraRotateSmoothnessSetting, 5.0));
    }

    void SetCameraRotateSmoothness(const float smoothness)
    {
        aztf::SetRegistry(CameraRotateSmoothnessSetting, smoothness);
    }

    float CameraTranslateSmoothness()
    {
        return aznumeric_cast<float>(aztf::GetRegistry(CameraTranslateSmoothnessSetting, 5.0));
    }

    void SetCameraTranslateSmoothness(const float smoothness)
    {
        aztf::SetRegistry(CameraTranslateSmoothnessSetting, smoothness);
    }

    bool CameraRotateSmoothingEnabled()
    {
        return aztf::GetRegistry(CameraRotateSmoothingSetting, true);
    }

    void SetCameraRotateSmoothingEnabled(const bool enabled)
    {
        aztf::SetRegistry(CameraRotateSmoothingSetting, enabled);
    }

    bool CameraTranslateSmoothingEnabled()
    {
        return aztf::GetRegistry(CameraTranslateSmoothingSetting, true);
    }

    void SetCameraTranslateSmoothingEnabled(const bool enabled)
    {
        aztf::SetRegistry(CameraTranslateSmoothingSetting, enabled);
    }

    bool CameraCaptureCursorForLook()
    {
        return aztf::GetRegistry(CameraCaptureCursorLookSetting, true);
    }

    void SetCameraCaptureCursorForLook(const bool capture)
    {
        aztf::SetRegistry(CameraCaptureCursorLookSetting, capture);
    }

    float CameraDefaultOrbitDistance()
    {
        return aznumeric_cast<float>(aztf::GetRegistry(CameraDefaultOrbitDistanceSetting, 20.0));
    }

    void SetCameraDefaultOrbitDistance(const float distance)
    {
        aztf::SetRegistry(CameraDefaultOrbitDistanceSetting, distance);
    }

    AzFramework::InputChannelId CameraTranslateForwardChannelId()
    {
        return AzFramework::InputChannelId(
            aztf::GetRegistry(CameraTranslateForwardIdSetting, AZStd::string("keyboard_key_alphanumeric_W")).c_str());
    }

    void SetCameraTranslateForwardChannelId(AZStd::string_view cameraTranslateForwardId)
    {
        aztf::SetRegistry(CameraTranslateForwardIdSetting, cameraTranslateForwardId);
    }

    AzFramework::InputChannelId CameraTranslateBackwardChannelId()
    {
        return AzFramework::InputChannelId(
            aztf::GetRegistry(CameraTranslateBackwardIdSetting, AZStd::string("keyboard_key_alphanumeric_S")).c_str());
    }

    void SetCameraTranslateBackwardChannelId(AZStd::string_view cameraTranslateBackwardId)
    {
        aztf::SetRegistry(CameraTranslateBackwardIdSetting, cameraTranslateBackwardId);
    }

    AzFramework::InputChannelId CameraTranslateLeftChannelId()
    {
        return AzFramework::InputChannelId(aztf::GetRegistry(CameraTranslateLeftIdSetting, AZStd::string("keyboard_key_alphanumeric_A")).c_str());
    }

    void SetCameraTranslateLeftChannelId(AZStd::string_view cameraTranslateLeftId)
    {
        aztf::SetRegistry(CameraTranslateLeftIdSetting, cameraTranslateLeftId);
    }

    AzFramework::InputChannelId CameraTranslateRightChannelId()
    {
        return AzFramework::InputChannelId(
            aztf::GetRegistry(CameraTranslateRightIdSetting, AZStd::string("keyboard_key_alphanumeric_D")).c_str());
    }

    void SetCameraTranslateRightChannelId(AZStd::string_view cameraTranslateRightId)
    {
        aztf::SetRegistry(CameraTranslateRightIdSetting, cameraTranslateRightId);
    }

    AzFramework::InputChannelId CameraTranslateUpChannelId()
    {
        return AzFramework::InputChannelId(aztf::GetRegistry(CameraTranslateUpIdSetting, AZStd::string("keyboard_key_alphanumeric_E")).c_str());
    }

    void SetCameraTranslateUpChannelId(AZStd::string_view cameraTranslateUpId)
    {
        aztf::SetRegistry(CameraTranslateUpIdSetting, cameraTranslateUpId);
    }

    AzFramework::InputChannelId CameraTranslateDownChannelId()
    {
        return AzFramework::InputChannelId(aztf::GetRegistry(CameraTranslateDownIdSetting, AZStd::string("keyboard_key_alphanumeric_Q")).c_str());
    }

    void SetCameraTranslateDownChannelId(AZStd::string_view cameraTranslateDownId)
    {
        aztf::SetRegistry(CameraTranslateDownIdSetting, cameraTranslateDownId);
    }

    AzFramework::InputChannelId CameraTranslateBoostChannelId()
    {
        return AzFramework::InputChannelId(
            aztf::GetRegistry(CameraTranslateBoostIdSetting, AZStd::string("keyboard_key_modifier_shift_l")).c_str());
    }

    void SetCameraTranslateBoostChannelId(AZStd::string_view cameraTranslateBoostId)
    {
        aztf::SetRegistry(CameraTranslateBoostIdSetting, cameraTranslateBoostId);
    }

    AzFramework::InputChannelId CameraOrbitChannelId()
    {
        return AzFramework::InputChannelId(aztf::GetRegistry(CameraOrbitIdSetting, AZStd::string("keyboard_key_modifier_alt_l")).c_str());
    }

    void SetCameraOrbitChannelId(AZStd::string_view cameraOrbitId)
    {
        aztf::SetRegistry(CameraOrbitIdSetting, cameraOrbitId);
    }

    AzFramework::InputChannelId CameraFreeLookChannelId()
    {
        return AzFramework::InputChannelId(aztf::GetRegistry(CameraFreeLookIdSetting, AZStd::string("mouse_button_right")).c_str());
    }

    void SetCameraFreeLookChannelId(AZStd::string_view cameraFreeLookId)
    {
        aztf::SetRegistry(CameraFreeLookIdSetting, cameraFreeLookId);
    }

    AzFramework::InputChannelId CameraFreePanChannelId()
    {
        return AzFramework::InputChannelId(aztf::GetRegistry(CameraFreePanIdSetting, AZStd::string("mouse_button_middle")).c_str());
    }

    void SetCameraFreePanChannelId(AZStd::string_view cameraFreePanId)
    {
        aztf::SetRegistry(CameraFreePanIdSetting, cameraFreePanId);
    }

    AzFramework::InputChannelId CameraOrbitLookChannelId()
    {
        return AzFramework::InputChannelId(aztf::GetRegistry(CameraOrbitLookIdSetting, AZStd::string("mouse_button_left")).c_str());
    }

    void SetCameraOrbitLookChannelId(AZStd::string_view cameraOrbitLookId)
    {
        aztf::SetRegistry(CameraOrbitLookIdSetting, cameraOrbitLookId);
    }

    AzFramework::InputChannelId CameraOrbitDollyChannelId()
    {
        return AzFramework::InputChannelId(aztf::GetRegistry(CameraOrbitDollyIdSetting, AZStd::string("mouse_button_right")).c_str());
    }

    void SetCameraOrbitDollyChannelId(AZStd::string_view cameraOrbitDollyId)
    {
        aztf::SetRegistry(CameraOrbitDollyIdSetting, cameraOrbitDollyId);
    }

    AzFramework::InputChannelId CameraOrbitPanChannelId()
    {
        return AzFramework::InputChannelId(aztf::GetRegistry(CameraOrbitPanIdSetting, AZStd::string("mouse_button_middle")).c_str());
    }

    void SetCameraOrbitPanChannelId(AZStd::string_view cameraOrbitPanId)
    {
        aztf::SetRegistry(CameraOrbitPanIdSetting, cameraOrbitPanId);
    }

    AzFramework::InputChannelId CameraFocusChannelId()
    {
        return AzFramework::InputChannelId(aztf::GetRegistry(CameraFocusIdSetting, AZStd::string("keyboard_key_alphanumeric_X")).c_str());
    }

    void SetCameraFocusChannelId(AZStd::string_view cameraFocusId)
    {
        aztf::SetRegistry(CameraFocusIdSetting, cameraFocusId);
    }
} // namespace SandboxEditor
