/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EditorViewportSettings.h>

#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Math/MathUtils.h>
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
    constexpr AZStd::string_view CameraSpeedScaleSetting = "/Amazon/Preferences/Editor/Camera/SpeedScale";
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
    constexpr AZStd::string_view CameraDefaultStartingPitch = "/Amazon/Preferences/Editor/Camera/DefaultStartingPitch";
    constexpr AZStd::string_view CameraDefaultStartingYaw = "/Amazon/Preferences/Editor/Camera/DefaultStartingYaw";
    constexpr AZStd::string_view CameraNearPlaneDistanceSetting = "/Amazon/Preferences/Editor/Camera/NearPlaneDistance";
    constexpr AZStd::string_view CameraFarPlaneDistanceSetting = "/Amazon/Preferences/Editor/Camera/FarPlaneDistance";
    constexpr AZStd::string_view CameraFovDegreesSetting = "/Amazon/Preferences/Editor/Camera/FovDegrees";

    struct EditorViewportSettingsCallbacksImpl : public EditorViewportSettingsCallbacks
    {
        EditorViewportSettingsCallbacksImpl()
        {
            if (auto* registry = AZ::SettingsRegistry::Get())
            {
                using AZ::SettingsRegistryMergeUtils::IsPathAncestorDescendantOrEqual;

                m_angleSnappingNotifyEventHandler = registry->RegisterNotifier(
                    [this](const AZ::SettingsRegistryInterface::NotifyEventArgs& notifyEventArgs)
                    {
                        if (IsPathAncestorDescendantOrEqual(AngleSnappingSetting, notifyEventArgs.m_jsonKeyPath))
                        {
                            m_angleSnappingChanged.Signal(AngleSnappingEnabled());
                        }
                    });

                m_gridSnappingNotifyEventHandler = registry->RegisterNotifier(
                    [this](const AZ::SettingsRegistryInterface::NotifyEventArgs& notifyEventArgs)
                    {
                        if (IsPathAncestorDescendantOrEqual(GridSnappingSetting, notifyEventArgs.m_jsonKeyPath))
                        {
                            m_gridSnappingChanged.Signal(GridSnappingEnabled());
                        }
                    });

                m_farPlaneDistanceNotifyEventHandler = registry->RegisterNotifier(
                    [this](const AZ::SettingsRegistryInterface::NotifyEventArgs& notifyEventArgs)
                    {
                        if (IsPathAncestorDescendantOrEqual(CameraFarPlaneDistanceSetting, notifyEventArgs.m_jsonKeyPath))
                        {
                            m_farPlaneChanged.Signal(CameraDefaultFarPlaneDistance());
                        }
                    });

                m_nearPlaneDistanceNotifyEventHandler = registry->RegisterNotifier(
                    [this](const AZ::SettingsRegistryInterface::NotifyEventArgs& notifyEventArgs)
                    {
                        if (IsPathAncestorDescendantOrEqual(CameraNearPlaneDistanceSetting, notifyEventArgs.m_jsonKeyPath))
                        {
                            m_nearPlaneChanged.Signal(CameraDefaultNearPlaneDistance());
                        }
                    });

                m_perspectiveNotifyEventHandler = registry->RegisterNotifier(
                    [this](const AZ::SettingsRegistryInterface::NotifyEventArgs& notifyEventArgs)
                    {
                        if (IsPathAncestorDescendantOrEqual(CameraFovDegreesSetting, notifyEventArgs.m_jsonKeyPath))
                        {
                            m_perspectiveChanged.Signal(CameraDefaultFovRadians());
                        }
                    });
            }
        }

        void SetAngleSnappingChangedEvent(AngleSnappingChangedEvent::Handler& handler) override
        {
            handler.Connect(m_angleSnappingChanged);
        }

        void SetGridSnappingChangedEvent(GridSnappingChangedEvent::Handler& handler) override
        {
            handler.Connect(m_gridSnappingChanged);
        }

        void SetFarPlaneDistanceChangedEvent(NearFarPlaneChangedEvent::Handler& handler) override
        {
            handler.Connect(m_farPlaneChanged);
        }

        void SetNearPlaneDistanceChangedEvent(NearFarPlaneChangedEvent::Handler& handler) override
        {
            handler.Connect(m_nearPlaneChanged);
        }

        void SetPerspectiveChangedEvent(PerspectiveChangedEvent::Handler& handler) override
        {
            handler.Connect(m_perspectiveChanged);
        }

        AngleSnappingChangedEvent m_angleSnappingChanged;
        GridSnappingChangedEvent m_gridSnappingChanged;
        PerspectiveChangedEvent m_perspectiveChanged;
        NearFarPlaneChangedEvent m_farPlaneChanged;
        NearFarPlaneChangedEvent m_nearPlaneChanged;
        AZ::SettingsRegistryInterface::NotifyEventHandler m_angleSnappingNotifyEventHandler;
        AZ::SettingsRegistryInterface::NotifyEventHandler m_farPlaneDistanceNotifyEventHandler;
        AZ::SettingsRegistryInterface::NotifyEventHandler m_gridSnappingNotifyEventHandler;
        AZ::SettingsRegistryInterface::NotifyEventHandler m_nearPlaneDistanceNotifyEventHandler;
        AZ::SettingsRegistryInterface::NotifyEventHandler m_perspectiveNotifyEventHandler;
    };

    AZStd::unique_ptr<EditorViewportSettingsCallbacks> CreateEditorViewportSettingsCallbacks()
    {
        return AZStd::make_unique<EditorViewportSettingsCallbacksImpl>();
    }

    AZ::Vector3 CameraDefaultEditorPosition()
    {
        return AZ::Vector3(
            aznumeric_cast<float>(AzToolsFramework::GetRegistry(CameraDefaultStartingPositionX, 0.0)),
            aznumeric_cast<float>(AzToolsFramework::GetRegistry(CameraDefaultStartingPositionY, -10.0)),
            aznumeric_cast<float>(AzToolsFramework::GetRegistry(CameraDefaultStartingPositionZ, 4.0)));
    }

    void SetCameraDefaultEditorPosition(const AZ::Vector3& position)
    {
        AzToolsFramework::SetRegistry(CameraDefaultStartingPositionX, position.GetX());
        AzToolsFramework::SetRegistry(CameraDefaultStartingPositionY, position.GetY());
        AzToolsFramework::SetRegistry(CameraDefaultStartingPositionZ, position.GetZ());
    }

    AZ::Vector2 CameraDefaultEditorOrientation()
    {
        return AZ::Vector2(
            aznumeric_cast<float>(AzToolsFramework::GetRegistry(CameraDefaultStartingPitch, 0.0)),
            aznumeric_cast<float>(AzToolsFramework::GetRegistry(CameraDefaultStartingYaw, 0.0)));
    }

    void SetCameraDefaultEditorOrientation(const AZ::Vector2& pitchYaw)
    {
        AzToolsFramework::SetRegistry(CameraDefaultStartingPitch, pitchYaw.GetX());
        AzToolsFramework::SetRegistry(CameraDefaultStartingYaw, pitchYaw.GetY());
    }

    AZ::u64 MaxItemsShownInAssetBrowserSearch()
    {
        return AzToolsFramework::GetRegistry(AssetBrowserMaxItemsShownInSearchSetting, aznumeric_cast<AZ::u64>(50));
    }

    void SetMaxItemsShownInAssetBrowserSearch(const AZ::u64 numberOfItemsShown)
    {
        AzToolsFramework::SetRegistry(AssetBrowserMaxItemsShownInSearchSetting, numberOfItemsShown);
    }

    bool GridSnappingEnabled()
    {
        return AzToolsFramework::GetRegistry(GridSnappingSetting, false);
    }

    void SetGridSnapping(const bool enabled)
    {
        AzToolsFramework::SetRegistry(GridSnappingSetting, enabled);
    }

    float GridSnappingSize()
    {
        return aznumeric_cast<float>(AzToolsFramework::GetRegistry(GridSizeSetting, 0.1));
    }

    void SetGridSnappingSize(const float size)
    {
        AzToolsFramework::SetRegistry(GridSizeSetting, size);
    }

    bool AngleSnappingEnabled()
    {
        return AzToolsFramework::GetRegistry(AngleSnappingSetting, false);
    }

    void SetAngleSnapping(const bool enabled)
    {
        AzToolsFramework::SetRegistry(AngleSnappingSetting, enabled);
    }

    float AngleSnappingSize()
    {
        return aznumeric_cast<float>(AzToolsFramework::GetRegistry(AngleSizeSetting, 5.0));
    }

    void SetAngleSnappingSize(const float size)
    {
        AzToolsFramework::SetRegistry(AngleSizeSetting, size);
    }

    bool ShowingGrid()
    {
        return AzToolsFramework::GetRegistry(ShowGridSetting, false);
    }

    void SetShowingGrid(const bool showing)
    {
        AzToolsFramework::SetRegistry(ShowGridSetting, showing);
    }

    bool StickySelectEnabled()
    {
        return AzToolsFramework::GetRegistry(StickySelectSetting, false);
    }

    void SetStickySelectEnabled(const bool enabled)
    {
        AzToolsFramework::SetRegistry(StickySelectSetting, enabled);
    }

    float ManipulatorLineBoundWidth()
    {
        return aznumeric_cast<float>(AzToolsFramework::GetRegistry(ManipulatorLineBoundWidthSetting, 0.1));
    }

    void SetManipulatorLineBoundWidth(const float lineBoundWidth)
    {
        AzToolsFramework::SetRegistry(ManipulatorLineBoundWidthSetting, lineBoundWidth);
    }

    float ManipulatorCircleBoundWidth()
    {
        return aznumeric_cast<float>(AzToolsFramework::GetRegistry(ManipulatorCircleBoundWidthSetting, 0.1));
    }

    void SetManipulatorCircleBoundWidth(const float circleBoundWidth)
    {
        AzToolsFramework::SetRegistry(ManipulatorCircleBoundWidthSetting, circleBoundWidth);
    }

    float CameraSpeedScale()
    {
        return aznumeric_cast<float>(AzToolsFramework::GetRegistry(CameraSpeedScaleSetting, 1.0));
    }

    void SetCameraSpeedScale(float speedScale)
    {
        AzToolsFramework::SetRegistry(CameraSpeedScaleSetting, speedScale);
    }

    float CameraTranslateSpeed()
    {
        return aznumeric_cast<float>(AzToolsFramework::GetRegistry(CameraTranslateSpeedSetting, 10.0));
    }

    float CameraTranslateSpeedScaled()
    {
        return CameraTranslateSpeed() * CameraSpeedScale();
    }

    void SetCameraTranslateSpeed(const float speed)
    {
        AzToolsFramework::SetRegistry(CameraTranslateSpeedSetting, speed);
    }

    float CameraBoostMultiplier()
    {
        return aznumeric_cast<float>(AzToolsFramework::GetRegistry(CameraBoostMultiplierSetting, 3.0));
    }

    void SetCameraBoostMultiplier(const float multiplier)
    {
        AzToolsFramework::SetRegistry(CameraBoostMultiplierSetting, multiplier);
    }

    float CameraRotateSpeed()
    {
        return aznumeric_cast<float>(AzToolsFramework::GetRegistry(CameraRotateSpeedSetting, 0.005));
    }

    void SetCameraRotateSpeed(const float speed)
    {
        AzToolsFramework::SetRegistry(CameraRotateSpeedSetting, speed);
    }

    float CameraScrollSpeed()
    {
        return aznumeric_cast<float>(AzToolsFramework::GetRegistry(CameraScrollSpeedSetting, 0.02));
    }

    float CameraScrollSpeedScaled()
    {
        return CameraScrollSpeed() * CameraSpeedScale();
    }

    void SetCameraScrollSpeed(const float speed)
    {
        AzToolsFramework::SetRegistry(CameraScrollSpeedSetting, speed);
    }

    float CameraDollyMotionSpeed()
    {
        return aznumeric_cast<float>(AzToolsFramework::GetRegistry(CameraDollyMotionSpeedSetting, 0.01));
    }

    float CameraDollyMotionSpeedScaled()
    {
        return CameraDollyMotionSpeed() * CameraSpeedScale();
    }

    void SetCameraDollyMotionSpeed(const float speed)
    {
        AzToolsFramework::SetRegistry(CameraDollyMotionSpeedSetting, speed);
    }

    bool CameraOrbitYawRotationInverted()
    {
        return AzToolsFramework::GetRegistry(CameraOrbitYawRotationInvertedSetting, false);
    }

    void SetCameraOrbitYawRotationInverted(const bool inverted)
    {
        AzToolsFramework::SetRegistry(CameraOrbitYawRotationInvertedSetting, inverted);
    }

    bool CameraPanInvertedX()
    {
        return AzToolsFramework::GetRegistry(CameraPanInvertedXSetting, true);
    }

    void SetCameraPanInvertedX(const bool inverted)
    {
        AzToolsFramework::SetRegistry(CameraPanInvertedXSetting, inverted);
    }

    bool CameraPanInvertedY()
    {
        return AzToolsFramework::GetRegistry(CameraPanInvertedYSetting, true);
    }

    void SetCameraPanInvertedY(const bool inverted)
    {
        AzToolsFramework::SetRegistry(CameraPanInvertedYSetting, inverted);
    }

    float CameraPanSpeed()
    {
        return aznumeric_cast<float>(AzToolsFramework::GetRegistry(CameraPanSpeedSetting, 0.01));
    }

    float CameraPanSpeedScaled()
    {
        return CameraPanSpeed() * CameraSpeedScale();
    }

    void SetCameraPanSpeed(float speed)
    {
        AzToolsFramework::SetRegistry(CameraPanSpeedSetting, speed);
    }

    float CameraRotateSmoothness()
    {
        return aznumeric_cast<float>(AzToolsFramework::GetRegistry(CameraRotateSmoothnessSetting, 5.0));
    }

    void SetCameraRotateSmoothness(const float smoothness)
    {
        AzToolsFramework::SetRegistry(CameraRotateSmoothnessSetting, smoothness);
    }

    float CameraTranslateSmoothness()
    {
        return aznumeric_cast<float>(AzToolsFramework::GetRegistry(CameraTranslateSmoothnessSetting, 5.0));
    }

    void SetCameraTranslateSmoothness(const float smoothness)
    {
        AzToolsFramework::SetRegistry(CameraTranslateSmoothnessSetting, smoothness);
    }

    bool CameraRotateSmoothingEnabled()
    {
        return AzToolsFramework::GetRegistry(CameraRotateSmoothingSetting, true);
    }

    void SetCameraRotateSmoothingEnabled(const bool enabled)
    {
        AzToolsFramework::SetRegistry(CameraRotateSmoothingSetting, enabled);
    }

    bool CameraTranslateSmoothingEnabled()
    {
        return AzToolsFramework::GetRegistry(CameraTranslateSmoothingSetting, true);
    }

    void SetCameraTranslateSmoothingEnabled(const bool enabled)
    {
        AzToolsFramework::SetRegistry(CameraTranslateSmoothingSetting, enabled);
    }

    bool CameraCaptureCursorForLook()
    {
        return AzToolsFramework::GetRegistry(CameraCaptureCursorLookSetting, true);
    }

    void SetCameraCaptureCursorForLook(const bool capture)
    {
        AzToolsFramework::SetRegistry(CameraCaptureCursorLookSetting, capture);
    }

    float CameraDefaultOrbitDistance()
    {
        return aznumeric_cast<float>(AzToolsFramework::GetRegistry(CameraDefaultOrbitDistanceSetting, 20.0));
    }

    void SetCameraDefaultOrbitDistance(const float distance)
    {
        AzToolsFramework::SetRegistry(CameraDefaultOrbitDistanceSetting, distance);
    }

    AzFramework::InputChannelId CameraTranslateForwardChannelId()
    {
        return AzFramework::InputChannelId(
            AzToolsFramework::GetRegistry(CameraTranslateForwardIdSetting, AZStd::string("keyboard_key_alphanumeric_W")).c_str());
    }

    void SetCameraTranslateForwardChannelId(AZStd::string_view cameraTranslateForwardId)
    {
        AzToolsFramework::SetRegistry(CameraTranslateForwardIdSetting, cameraTranslateForwardId);
    }

    AzFramework::InputChannelId CameraTranslateBackwardChannelId()
    {
        return AzFramework::InputChannelId(
            AzToolsFramework::GetRegistry(CameraTranslateBackwardIdSetting, AZStd::string("keyboard_key_alphanumeric_S")).c_str());
    }

    void SetCameraTranslateBackwardChannelId(AZStd::string_view cameraTranslateBackwardId)
    {
        AzToolsFramework::SetRegistry(CameraTranslateBackwardIdSetting, cameraTranslateBackwardId);
    }

    AzFramework::InputChannelId CameraTranslateLeftChannelId()
    {
        return AzFramework::InputChannelId(
            AzToolsFramework::GetRegistry(CameraTranslateLeftIdSetting, AZStd::string("keyboard_key_alphanumeric_A")).c_str());
    }

    void SetCameraTranslateLeftChannelId(AZStd::string_view cameraTranslateLeftId)
    {
        AzToolsFramework::SetRegistry(CameraTranslateLeftIdSetting, cameraTranslateLeftId);
    }

    AzFramework::InputChannelId CameraTranslateRightChannelId()
    {
        return AzFramework::InputChannelId(
            AzToolsFramework::GetRegistry(CameraTranslateRightIdSetting, AZStd::string("keyboard_key_alphanumeric_D")).c_str());
    }

    void SetCameraTranslateRightChannelId(AZStd::string_view cameraTranslateRightId)
    {
        AzToolsFramework::SetRegistry(CameraTranslateRightIdSetting, cameraTranslateRightId);
    }

    AzFramework::InputChannelId CameraTranslateUpChannelId()
    {
        return AzFramework::InputChannelId(
            AzToolsFramework::GetRegistry(CameraTranslateUpIdSetting, AZStd::string("keyboard_key_alphanumeric_E")).c_str());
    }

    void SetCameraTranslateUpChannelId(AZStd::string_view cameraTranslateUpId)
    {
        AzToolsFramework::SetRegistry(CameraTranslateUpIdSetting, cameraTranslateUpId);
    }

    AzFramework::InputChannelId CameraTranslateDownChannelId()
    {
        return AzFramework::InputChannelId(
            AzToolsFramework::GetRegistry(CameraTranslateDownIdSetting, AZStd::string("keyboard_key_alphanumeric_Q")).c_str());
    }

    void SetCameraTranslateDownChannelId(AZStd::string_view cameraTranslateDownId)
    {
        AzToolsFramework::SetRegistry(CameraTranslateDownIdSetting, cameraTranslateDownId);
    }

    AzFramework::InputChannelId CameraTranslateBoostChannelId()
    {
        return AzFramework::InputChannelId(
            AzToolsFramework::GetRegistry(CameraTranslateBoostIdSetting, AZStd::string("keyboard_key_modifier_shift_l")).c_str());
    }

    void SetCameraTranslateBoostChannelId(AZStd::string_view cameraTranslateBoostId)
    {
        AzToolsFramework::SetRegistry(CameraTranslateBoostIdSetting, cameraTranslateBoostId);
    }

    AzFramework::InputChannelId CameraOrbitChannelId()
    {
        return AzFramework::InputChannelId(
            AzToolsFramework::GetRegistry(CameraOrbitIdSetting, AZStd::string("keyboard_key_modifier_alt_l")).c_str());
    }

    void SetCameraOrbitChannelId(AZStd::string_view cameraOrbitId)
    {
        AzToolsFramework::SetRegistry(CameraOrbitIdSetting, cameraOrbitId);
    }

    AzFramework::InputChannelId CameraFreeLookChannelId()
    {
        return AzFramework::InputChannelId(
            AzToolsFramework::GetRegistry(CameraFreeLookIdSetting, AZStd::string("mouse_button_right")).c_str());
    }

    void SetCameraFreeLookChannelId(AZStd::string_view cameraFreeLookId)
    {
        AzToolsFramework::SetRegistry(CameraFreeLookIdSetting, cameraFreeLookId);
    }

    AzFramework::InputChannelId CameraFreePanChannelId()
    {
        return AzFramework::InputChannelId(
            AzToolsFramework::GetRegistry(CameraFreePanIdSetting, AZStd::string("mouse_button_middle")).c_str());
    }

    void SetCameraFreePanChannelId(AZStd::string_view cameraFreePanId)
    {
        AzToolsFramework::SetRegistry(CameraFreePanIdSetting, cameraFreePanId);
    }

    AzFramework::InputChannelId CameraOrbitLookChannelId()
    {
        return AzFramework::InputChannelId(
            AzToolsFramework::GetRegistry(CameraOrbitLookIdSetting, AZStd::string("mouse_button_left")).c_str());
    }

    void SetCameraOrbitLookChannelId(AZStd::string_view cameraOrbitLookId)
    {
        AzToolsFramework::SetRegistry(CameraOrbitLookIdSetting, cameraOrbitLookId);
    }

    AzFramework::InputChannelId CameraOrbitDollyChannelId()
    {
        return AzFramework::InputChannelId(
            AzToolsFramework::GetRegistry(CameraOrbitDollyIdSetting, AZStd::string("mouse_button_right")).c_str());
    }

    void SetCameraOrbitDollyChannelId(AZStd::string_view cameraOrbitDollyId)
    {
        AzToolsFramework::SetRegistry(CameraOrbitDollyIdSetting, cameraOrbitDollyId);
    }

    AzFramework::InputChannelId CameraOrbitPanChannelId()
    {
        return AzFramework::InputChannelId(
            AzToolsFramework::GetRegistry(CameraOrbitPanIdSetting, AZStd::string("mouse_button_middle")).c_str());
    }

    void SetCameraOrbitPanChannelId(AZStd::string_view cameraOrbitPanId)
    {
        AzToolsFramework::SetRegistry(CameraOrbitPanIdSetting, cameraOrbitPanId);
    }

    AzFramework::InputChannelId CameraFocusChannelId()
    {
        return AzFramework::InputChannelId(
            AzToolsFramework::GetRegistry(CameraFocusIdSetting, AZStd::string("keyboard_key_alphanumeric_X")).c_str());
    }

    void SetCameraFocusChannelId(AZStd::string_view cameraFocusId)
    {
        AzToolsFramework::SetRegistry(CameraFocusIdSetting, cameraFocusId);
    }

    float CameraDefaultNearPlaneDistance()
    {
        return aznumeric_caster(AzToolsFramework::GetRegistry(CameraNearPlaneDistanceSetting, 0.1));
    }

    void SetCameraDefaultNearPlaneDistance(float distance)
    {
        AzToolsFramework::SetRegistry(CameraNearPlaneDistanceSetting, aznumeric_cast<double>(distance));
    }

    float CameraDefaultFarPlaneDistance()
    {
        return aznumeric_caster(AzToolsFramework::GetRegistry(CameraFarPlaneDistanceSetting, 100.0));
    }

    void SetCameraDefaultFarPlaneDistance(float distance)
    {
        AzToolsFramework::SetRegistry(CameraFarPlaneDistanceSetting, aznumeric_cast<double>(distance));
    }

    float CameraDefaultFovRadians()
    {
        return AZ::DegToRad(CameraDefaultFovDegrees());
    }

    void SetCameraDefaultFovRadians(float fovRadians)
    {
        SetCameraDefaultFovDegrees(AZ::RadToDeg(fovRadians));
    }

    float CameraDefaultFovDegrees()
    {
        return aznumeric_caster(AzToolsFramework::GetRegistry(CameraFovDegreesSetting, aznumeric_cast<double>(60.0)));
    }

    void SetCameraDefaultFovDegrees(float fovDegrees)
    {
        AzToolsFramework::SetRegistry(CameraFovDegreesSetting, aznumeric_cast<double>(fovDegrees));
    }

    void ResetCameraSpeedScale()
    {
        AzToolsFramework::ClearRegistry(CameraSpeedScaleSetting);
    }

    void ResetCameraTranslateSpeed()
    {
        AzToolsFramework::ClearRegistry(CameraTranslateSpeedSetting);
    }

    void ResetCameraRotateSpeed()
    {
        AzToolsFramework::ClearRegistry(CameraRotateSpeedSetting);
    }

    void ResetCameraBoostMultiplier()
    {
        AzToolsFramework::ClearRegistry(CameraBoostMultiplierSetting);
    }

    void ResetCameraScrollSpeed()
    {
        AzToolsFramework::ClearRegistry(CameraScrollSpeedSetting);
    }

    void ResetCameraDollyMotionSpeed()
    {
        AzToolsFramework::ClearRegistry(CameraDollyMotionSpeedSetting);
    }

    void ResetCameraPanSpeed()
    {
        AzToolsFramework::ClearRegistry(CameraPanSpeedSetting);
    }

    void ResetCameraRotateSmoothness()
    {
        AzToolsFramework::ClearRegistry(CameraRotateSmoothnessSetting);
    }

    void ResetCameraRotateSmoothingEnabled()
    {
        AzToolsFramework::ClearRegistry(CameraRotateSmoothingSetting);
    }

    void ResetCameraTranslateSmoothness()
    {
        AzToolsFramework::ClearRegistry(CameraTranslateSmoothnessSetting);
    }

    void ResetCameraTranslateSmoothingEnabled()
    {
        AzToolsFramework::ClearRegistry(CameraTranslateSmoothingSetting);
    }

    void ResetCameraCaptureCursorForLook()
    {
        AzToolsFramework::ClearRegistry(CameraCaptureCursorLookSetting);
    }

    void ResetCameraOrbitYawRotationInverted()
    {
        AzToolsFramework::ClearRegistry(CameraOrbitYawRotationInvertedSetting);
    }

    void ResetCameraPanInvertedX()
    {
        AzToolsFramework::ClearRegistry(CameraPanInvertedXSetting);
    }

    void ResetCameraPanInvertedY()
    {
        AzToolsFramework::ClearRegistry(CameraPanInvertedYSetting);
    }

    void ResetCameraDefaultEditorPosition()
    {
        AzToolsFramework::ClearRegistry(CameraDefaultStartingPositionX);
        AzToolsFramework::ClearRegistry(CameraDefaultStartingPositionY);
        AzToolsFramework::ClearRegistry(CameraDefaultStartingPositionZ);
    }

    void ResetCameraDefaultOrbitDistance()
    {
        AzToolsFramework::ClearRegistry(CameraDefaultOrbitDistanceSetting);
    }

    void ResetCameraDefaultEditorOrientation()
    {
        AzToolsFramework::ClearRegistry(CameraDefaultStartingPitch);
        AzToolsFramework::ClearRegistry(CameraDefaultStartingYaw);
    }

    void ResetCameraTranslateForwardChannelId()
    {
        AzToolsFramework::ClearRegistry(CameraTranslateForwardIdSetting);
    }

    void ResetCameraTranslateBackwardChannelId()
    {
        AzToolsFramework::ClearRegistry(CameraTranslateBackwardIdSetting);
    }

    void ResetCameraTranslateLeftChannelId()
    {
        AzToolsFramework::ClearRegistry(CameraTranslateLeftIdSetting);
    }

    void ResetCameraTranslateRightChannelId()
    {
        AzToolsFramework::ClearRegistry(CameraTranslateRightIdSetting);
    }

    void ResetCameraTranslateUpChannelId()
    {
        AzToolsFramework::ClearRegistry(CameraTranslateUpIdSetting);
    }

    void ResetCameraTranslateDownChannelId()
    {
        AzToolsFramework::ClearRegistry(CameraTranslateDownIdSetting);
    }

    void ResetCameraTranslateBoostChannelId()
    {
        AzToolsFramework::ClearRegistry(CameraTranslateBoostIdSetting);
    }

    void ResetCameraOrbitChannelId()
    {
        AzToolsFramework::ClearRegistry(CameraOrbitIdSetting);
    }

    void ResetCameraFreeLookChannelId()
    {
        AzToolsFramework::ClearRegistry(CameraFreeLookIdSetting);
    }

    void ResetCameraFreePanChannelId()
    {
        AzToolsFramework::ClearRegistry(CameraFreePanIdSetting);
    }

    void ResetCameraOrbitLookChannelId()
    {
        AzToolsFramework::ClearRegistry(CameraOrbitLookIdSetting);
    }

    void ResetCameraOrbitDollyChannelId()
    {
        AzToolsFramework::ClearRegistry(CameraOrbitDollyIdSetting);
    }

    void ResetCameraOrbitPanChannelId()
    {
        AzToolsFramework::ClearRegistry(CameraOrbitPanIdSetting);
    }

    void ResetCameraFocusChannelId()
    {
        AzToolsFramework::ClearRegistry(CameraFocusIdSetting);
    }
} // namespace SandboxEditor
