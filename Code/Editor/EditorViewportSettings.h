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
#include <AzCore/Math/Vector3.h>
#include <AzFramework/Input/Channels/InputChannelId.h>

namespace SandboxEditor
{
    using GridSnappingChangedEvent = AZ::Event<bool>;

    //! Set callbacks to listen for editor settings change events.
    class EditorViewportSettingsCallbacks
    {
    public:
        virtual ~EditorViewportSettingsCallbacks() = default;

        virtual void SetGridSnappingChangedEvent(GridSnappingChangedEvent::Handler& handler) = 0;
    };

    //! Create an instance of EditorViewportSettingsCallbacks
    //! Note: EditorViewportSettingsCallbacks is implemented in EditorViewportSettings.cpp - a change
    //! event will fire when a value in the settings registry (editorpreferences.setreg) is modified.
    SANDBOX_API AZStd::unique_ptr<EditorViewportSettingsCallbacks> CreateEditorViewportSettingsCallbacks();

    SANDBOX_API AZ::Vector3 DefaultEditorCameraPosition();
    SANDBOX_API void SetDefaultCameraEditorPosition(AZ::Vector3 defaultCameraPosition);

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

    SANDBOX_API float CameraTranslateSpeed();
    SANDBOX_API void SetCameraTranslateSpeed(float speed);

    SANDBOX_API float CameraBoostMultiplier();
    SANDBOX_API void SetCameraBoostMultiplier(float multiplier);

    SANDBOX_API float CameraRotateSpeed();
    SANDBOX_API void SetCameraRotateSpeed(float speed);

    SANDBOX_API float CameraScrollSpeed();
    SANDBOX_API void SetCameraScrollSpeed(float speed);

    SANDBOX_API float CameraDollyMotionSpeed();
    SANDBOX_API void SetCameraDollyMotionSpeed(float speed);

    SANDBOX_API bool CameraOrbitYawRotationInverted();
    SANDBOX_API void SetCameraOrbitYawRotationInverted(bool inverted);

    SANDBOX_API bool CameraPanInvertedX();
    SANDBOX_API void SetCameraPanInvertedX(bool inverted);

    SANDBOX_API bool CameraPanInvertedY();
    SANDBOX_API void SetCameraPanInvertedY(bool inverted);

    SANDBOX_API float CameraPanSpeed();
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
} // namespace SandboxEditor
