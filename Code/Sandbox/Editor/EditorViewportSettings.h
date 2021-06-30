/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <SandboxAPI.h>

#include <AzCore/Settings/SettingsRegistry.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

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

    //! Return if the new editor camera system is enabled or not.
    //! @note This is implemented in EditorViewportWidget.cpp
    SANDBOX_API bool UsingNewCameraSystem();
} // namespace SandboxEditor
