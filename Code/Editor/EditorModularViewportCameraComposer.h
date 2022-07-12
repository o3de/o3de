/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AtomToolsFramework/Viewport/ModularViewportCameraController.h>
#include <AzCore/Component/TickBus.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
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
        , private AzFramework::ViewportDebugDisplayEventBus::Handler
        , private AZ::TickBus::Handler
    {
    public:
        SANDBOX_API explicit EditorModularViewportCameraComposer(AzFramework::ViewportId viewportId);
        SANDBOX_API ~EditorModularViewportCameraComposer();

        //! Build a ModularViewportCameraController from the associated camera inputs.
        SANDBOX_API AZStd::shared_ptr<AtomToolsFramework::ModularViewportCameraController> CreateModularViewportCameraController();

    private:
        //! Display style/state for the pivot
        enum class PivotDisplayState
        {
            Hidden, //!< Orbit camera inactive.
            Faded, //!< Orbit camera active but not rotating.
            Full //!< Orbit camera active and rotating.
        };

        // AzFramework::ViewportDebugDisplayEventBus overrides ...
        void DisplayViewport(const AzFramework::ViewportInfo& viewportInfo, AzFramework::DebugDisplayRequests& debugDisplay) override;

        // AZ::TickBus overrides ...
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

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
        AZStd::shared_ptr<AzFramework::OrbitScrollDollyCameraInput> m_orbitScrollDollyCamera;
        AZStd::shared_ptr<AzFramework::OrbitMotionDollyCameraInput> m_orbitMotionDollyCamera;
        AZStd::shared_ptr<AzFramework::PanCameraInput> m_orbitPanCamera;
        AZStd::shared_ptr<AzFramework::FocusCameraInput> m_orbitFocusCamera;

        AzFramework::ViewportId m_viewportId;

        AZStd::optional<AZ::Vector3> m_pivot; //!< The picked pivot to orbit about in the viewport.
        float m_orbitOpacity = 0.0f; //!< The picked pivot opacity (to fade in and out).
        PivotDisplayState m_pivotDisplayState; //!< The state of the pivot for the orbit camera.
    };
} // namespace SandboxEditor
