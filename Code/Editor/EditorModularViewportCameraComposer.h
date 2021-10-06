/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AtomToolsFramework/Viewport/ModularViewportCameraController.h>
#include <AzFramework/Viewport/CameraInput.h>
#include <AzToolsFramework/API/EditorCameraBus.h>
#include <EditorModularViewportCameraComposerBus.h>
#include <SandboxAPI.h>

namespace SandboxEditor
{
    //! Type responsible for building the editor's modular viewport camera controller.
    class EditorModularViewportCameraComposer
        : private EditorModularViewportCameraComposerNotificationBus::Handler
        , private Camera::EditorCameraNotificationBus::Handler
    {
    public:
        SANDBOX_API explicit EditorModularViewportCameraComposer(AzFramework::ViewportId viewportId);
        SANDBOX_API ~EditorModularViewportCameraComposer();

        //! Build a ModularViewportCameraController from the associated camera inputs.
        SANDBOX_API AZStd::shared_ptr<AtomToolsFramework::ModularViewportCameraController> CreateModularViewportCameraController();

    private:
        //! Setup all internal camera inputs.
        void SetupCameras();

        // EditorModularViewportCameraComposerNotificationBus overrides ...
        void OnEditorModularViewportCameraComposerSettingsChanged() override;

        // EditorCameraNotificationBus overrides ...
        void OnViewportViewEntityChanged(const AZ::EntityId& viewEntityId) override;

        AZStd::shared_ptr<AzFramework::RotateCameraInput> m_firstPersonRotateCamera;
        AZStd::shared_ptr<AzFramework::PanCameraInput> m_firstPersonPanCamera;
        AZStd::shared_ptr<AzFramework::TranslateCameraInput> m_firstPersonTranslateCamera;
        AZStd::shared_ptr<AzFramework::LookScrollTranslationCameraInput> m_firstPersonScrollCamera;
        AZStd::shared_ptr<AzFramework::FocusCameraInput> m_firstPersonFocusCamera;
        AZStd::shared_ptr<AzFramework::OrbitCameraInput> m_orbitCamera;
        AZStd::shared_ptr<AzFramework::RotateCameraInput> m_orbitRotateCamera;
        AZStd::shared_ptr<AzFramework::TranslateCameraInput> m_orbitTranslateCamera;
        AZStd::shared_ptr<AzFramework::OrbitDollyScrollCameraInput> m_orbitDollyScrollCamera;
        AZStd::shared_ptr<AzFramework::OrbitDollyMotionCameraInput> m_orbitDollyMoveCamera;
        AZStd::shared_ptr<AzFramework::PanCameraInput> m_orbitPanCamera;
        AZStd::shared_ptr<AzFramework::FocusCameraInput> m_orbitFocusCamera;

        AzFramework::ViewportId m_viewportId;
    };
} // namespace SandboxEditor
