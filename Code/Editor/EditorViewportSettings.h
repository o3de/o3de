/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <SandboxAPI.h>

#include <AzCore/Settings/SettingsRegistry.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzFramework/Input/Channels/InputChannelId.h>

namespace SandboxEditor
{
    using AngleSnappingChangedEvent = AZ::Event<bool>;
    using CameraSpeedScaleChangedEvent = AZ::Event<float>;
    using GridShowingChangedEvent = AZ::Event<bool>;
    using GridSnappingChangedEvent = AZ::Event<bool>;
    using PerspectiveChangedEvent = AZ::Event<float>;
    using NearFarPlaneChangedEvent = AZ::Event<float>;

    //! Set callbacks to listen for editor settings change events.
    class EditorViewportSettingsCallbacks
    {
    public:
        virtual ~EditorViewportSettingsCallbacks() = default;

        virtual void SetAngleSnappingChangedEvent(AngleSnappingChangedEvent::Handler& handler) = 0;
        virtual void SetCameraSpeedScaleChangedEvent(CameraSpeedScaleChangedEvent::Handler& handler) = 0;
        virtual void SetGridShowingChangedEvent(GridShowingChangedEvent::Handler& handler) = 0;
        virtual void SetGridSnappingChangedEvent(GridSnappingChangedEvent::Handler& handler) = 0;
        virtual void SetFarPlaneDistanceChangedEvent(NearFarPlaneChangedEvent::Handler& handler) = 0;
        virtual void SetPerspectiveChangedEvent(PerspectiveChangedEvent::Handler& handler) = 0;
        virtual void SetNearPlaneDistanceChangedEvent(NearFarPlaneChangedEvent::Handler& handler) = 0;
    };

    //! Create an instance of EditorViewportSettingsCallbacks
    //! Note: EditorViewportSettingsCallbacks is implemented in EditorViewportSettings.cpp - a change
    //! event will fire when a value in the settings registry (editorpreferences.setreg) is modified.
    SANDBOX_API AZStd::unique_ptr<EditorViewportSettingsCallbacks> CreateEditorViewportSettingsCallbacks();

    SANDBOX_API AZ::u64 MaxItemsShownInAssetBrowserSearch();
    SANDBOX_API void SetMaxItemsShownInAssetBrowserSearch(AZ::u64 numberOfItemsShown);

    SANDBOX_API bool GridSnappingEnabled();
    SANDBOX_API void SetGridSnapping(bool enabled);

    SANDBOX_API float GridSnappingSize();
    SANDBOX_API void SetGridSnappingSize(float size);

    SANDBOX_API bool AngleSnappingEnabled();
    SANDBOX_API void SetAngleSnapping(bool enabled);

    SANDBOX_API float AngleSnappingSize();
    SANDBOX_API void SetAngleSnappingSize(float size);

    SANDBOX_API bool ShowingGrid();
    SANDBOX_API void SetShowingGrid(bool showing);

    SANDBOX_API bool StickySelectEnabled();
    SANDBOX_API void SetStickySelectEnabled(bool enabled);

    SANDBOX_API float ManipulatorLineBoundWidth();
    SANDBOX_API void SetManipulatorLineBoundWidth(float lineBoundWidth);

    SANDBOX_API float ManipulatorCircleBoundWidth();
    SANDBOX_API void SetManipulatorCircleBoundWidth(float circleBoundWidth);

    SANDBOX_API float CameraSpeedScale();
    SANDBOX_API void SetCameraSpeedScale(float speedScale);

    SANDBOX_API float CameraTranslateSpeed();
    SANDBOX_API float CameraTranslateSpeedScaled();
    SANDBOX_API void SetCameraTranslateSpeed(float speed);

    SANDBOX_API float CameraBoostMultiplier();
    SANDBOX_API void SetCameraBoostMultiplier(float multiplier);

    SANDBOX_API float CameraRotateSpeed();
    SANDBOX_API void SetCameraRotateSpeed(float speed);

    SANDBOX_API float CameraScrollSpeed();
    SANDBOX_API float CameraScrollSpeedScaled();
    SANDBOX_API void SetCameraScrollSpeed(float speed);

    SANDBOX_API float CameraDollyMotionSpeed();
    SANDBOX_API float CameraDollyMotionSpeedScaled();
    SANDBOX_API void SetCameraDollyMotionSpeed(float speed);

    SANDBOX_API bool CameraOrbitYawRotationInverted();
    SANDBOX_API void SetCameraOrbitYawRotationInverted(bool inverted);

    SANDBOX_API bool CameraPanInvertedX();
    SANDBOX_API void SetCameraPanInvertedX(bool inverted);

    SANDBOX_API bool CameraPanInvertedY();
    SANDBOX_API void SetCameraPanInvertedY(bool inverted);

    SANDBOX_API float CameraPanSpeed();
    SANDBOX_API float CameraPanSpeedScaled();
    SANDBOX_API void SetCameraPanSpeed(float speed);

    SANDBOX_API float CameraRotateSmoothness();
    SANDBOX_API void SetCameraRotateSmoothness(float smoothness);

    SANDBOX_API float CameraTranslateSmoothness();
    SANDBOX_API void SetCameraTranslateSmoothness(float smoothness);

    SANDBOX_API bool CameraRotateSmoothingEnabled();
    SANDBOX_API void SetCameraRotateSmoothingEnabled(bool enabled);

    SANDBOX_API bool CameraTranslateSmoothingEnabled();
    SANDBOX_API void SetCameraTranslateSmoothingEnabled(bool enabled);

    SANDBOX_API bool CameraCaptureCursorForLook();
    SANDBOX_API void SetCameraCaptureCursorForLook(bool capture);

    SANDBOX_API AZ::Vector3 CameraDefaultEditorPosition();
    SANDBOX_API void SetCameraDefaultEditorPosition(const AZ::Vector3& position);

    //! @return pitch/yaw value in x/y Vector2 component in degrees.
    SANDBOX_API AZ::Vector2 CameraDefaultEditorOrientation();
    //! @param pitchYaw pitch/yaw value in x/y Vector2 component in degrees.
    SANDBOX_API void SetCameraDefaultEditorOrientation(const AZ::Vector2& pitchYaw);

    SANDBOX_API float CameraDefaultOrbitDistance();
    SANDBOX_API void SetCameraDefaultOrbitDistance(float distance);

    SANDBOX_API bool CameraGoToPositionInstantlyEnabled();
    SANDBOX_API void SetCameraGoToPositionInstantlyEnabled(bool instant);

    SANDBOX_API float CameraGoToPositionDuration();
    SANDBOX_API void SetCameraGoToPositionDuration(float duration); 

    SANDBOX_API AzFramework::InputChannelId CameraTranslateForwardChannelId();
    SANDBOX_API void SetCameraTranslateForwardChannelId(AZStd::string_view cameraTranslateForwardId);

    SANDBOX_API AzFramework::InputChannelId CameraTranslateBackwardChannelId();
    SANDBOX_API void SetCameraTranslateBackwardChannelId(AZStd::string_view cameraTranslateBackwardId);

    SANDBOX_API AzFramework::InputChannelId CameraTranslateLeftChannelId();
    SANDBOX_API void SetCameraTranslateLeftChannelId(AZStd::string_view cameraTranslateLeftId);

    SANDBOX_API AzFramework::InputChannelId CameraTranslateRightChannelId();
    SANDBOX_API void SetCameraTranslateRightChannelId(AZStd::string_view cameraTranslateRightId);

    SANDBOX_API AzFramework::InputChannelId CameraTranslateUpChannelId();
    SANDBOX_API void SetCameraTranslateUpChannelId(AZStd::string_view cameraTranslateUpId);

    SANDBOX_API AzFramework::InputChannelId CameraTranslateDownChannelId();
    SANDBOX_API void SetCameraTranslateDownChannelId(AZStd::string_view cameraTranslateDownId);

    SANDBOX_API AzFramework::InputChannelId CameraTranslateBoostChannelId();
    SANDBOX_API void SetCameraTranslateBoostChannelId(AZStd::string_view cameraTranslateBoostId);

    SANDBOX_API AzFramework::InputChannelId CameraOrbitChannelId();
    SANDBOX_API void SetCameraOrbitChannelId(AZStd::string_view cameraOrbitId);

    SANDBOX_API AzFramework::InputChannelId CameraFreeLookChannelId();
    SANDBOX_API void SetCameraFreeLookChannelId(AZStd::string_view cameraFreeLookId);

    SANDBOX_API AzFramework::InputChannelId CameraFreePanChannelId();
    SANDBOX_API void SetCameraFreePanChannelId(AZStd::string_view cameraFreePanId);

    SANDBOX_API AzFramework::InputChannelId CameraOrbitLookChannelId();
    SANDBOX_API void SetCameraOrbitLookChannelId(AZStd::string_view cameraOrbitLookId);

    SANDBOX_API AzFramework::InputChannelId CameraOrbitDollyChannelId();
    SANDBOX_API void SetCameraOrbitDollyChannelId(AZStd::string_view cameraOrbitDollyId);

    SANDBOX_API AzFramework::InputChannelId CameraOrbitPanChannelId();
    SANDBOX_API void SetCameraOrbitPanChannelId(AZStd::string_view cameraOrbitPanId);

    SANDBOX_API AzFramework::InputChannelId CameraFocusChannelId();
    SANDBOX_API void SetCameraFocusChannelId(AZStd::string_view cameraFocusId);

    SANDBOX_API float CameraDefaultNearPlaneDistance();
    SANDBOX_API void SetCameraDefaultNearPlaneDistance(float distance);

    SANDBOX_API float CameraDefaultFarPlaneDistance();
    SANDBOX_API void SetCameraDefaultFarPlaneDistance(float distance);

    SANDBOX_API float CameraDefaultFovRadians();
    SANDBOX_API void SetCameraDefaultFovRadians(float fovRadians);

    SANDBOX_API float CameraDefaultFovDegrees();
    SANDBOX_API void SetCameraDefaultFovDegrees(float fovDegrees);

    SANDBOX_API void ResetCameraSpeedScale();
    SANDBOX_API void ResetCameraTranslateSpeed();
    SANDBOX_API void ResetCameraRotateSpeed();
    SANDBOX_API void ResetCameraBoostMultiplier();
    SANDBOX_API void ResetCameraScrollSpeed();
    SANDBOX_API void ResetCameraDollyMotionSpeed();
    SANDBOX_API void ResetCameraPanSpeed();
    SANDBOX_API void ResetCameraRotateSmoothness();
    SANDBOX_API void ResetCameraRotateSmoothingEnabled();
    SANDBOX_API void ResetCameraTranslateSmoothness();
    SANDBOX_API void ResetCameraTranslateSmoothingEnabled();
    SANDBOX_API void ResetCameraCaptureCursorForLook();
    SANDBOX_API void ResetCameraOrbitYawRotationInverted();
    SANDBOX_API void ResetCameraPanInvertedX();
    SANDBOX_API void ResetCameraPanInvertedY();
    SANDBOX_API void ResetCameraDefaultEditorPosition();
    SANDBOX_API void ResetCameraDefaultOrbitDistance();
    SANDBOX_API void ResetCameraDefaultEditorOrientation();
    SANDBOX_API void ResetCameraGoToPositionInstantlyEnabled();
    SANDBOX_API void ResetCameraGoToPositionDuration();

    SANDBOX_API void ResetCameraTranslateForwardChannelId();
    SANDBOX_API void ResetCameraTranslateBackwardChannelId();
    SANDBOX_API void ResetCameraTranslateLeftChannelId();
    SANDBOX_API void ResetCameraTranslateRightChannelId();
    SANDBOX_API void ResetCameraTranslateUpChannelId();
    SANDBOX_API void ResetCameraTranslateDownChannelId();
    SANDBOX_API void ResetCameraTranslateBoostChannelId();
    SANDBOX_API void ResetCameraOrbitChannelId();
    SANDBOX_API void ResetCameraFreeLookChannelId();
    SANDBOX_API void ResetCameraFreePanChannelId();
    SANDBOX_API void ResetCameraOrbitLookChannelId();
    SANDBOX_API void ResetCameraOrbitDollyChannelId();
    SANDBOX_API void ResetCameraOrbitPanChannelId();
    SANDBOX_API void ResetCameraFocusChannelId();
} // namespace SandboxEditor
