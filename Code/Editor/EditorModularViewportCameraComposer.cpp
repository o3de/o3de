/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EditorModularViewportCameraComposer.h>

#include <AtomToolsFramework/Viewport/ModularViewportCameraControllerRequestBus.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzFramework/Render/IntersectorInterface.h>
#include <AzToolsFramework/Viewport/ViewportMessages.h>
#include <AzToolsFramework/ViewportSelection/EditorTransformComponentSelectionRequestBus.h>
#include <EditorViewportSettings.h>

namespace SandboxEditor
{
    static AzFramework::TranslateCameraInputChannelIds BuildTranslateCameraInputChannelIds()
    {
        AzFramework::TranslateCameraInputChannelIds translateCameraInputChannelIds;
        translateCameraInputChannelIds.m_leftChannelId = SandboxEditor::CameraTranslateLeftChannelId();
        translateCameraInputChannelIds.m_rightChannelId = SandboxEditor::CameraTranslateRightChannelId();
        translateCameraInputChannelIds.m_forwardChannelId = SandboxEditor::CameraTranslateForwardChannelId();
        translateCameraInputChannelIds.m_backwardChannelId = SandboxEditor::CameraTranslateBackwardChannelId();
        translateCameraInputChannelIds.m_upChannelId = SandboxEditor::CameraTranslateUpChannelId();
        translateCameraInputChannelIds.m_downChannelId = SandboxEditor::CameraTranslateDownChannelId();
        translateCameraInputChannelIds.m_boostChannelId = SandboxEditor::CameraTranslateBoostChannelId();

        return translateCameraInputChannelIds;
    }

    EditorModularViewportCameraComposer::EditorModularViewportCameraComposer(const AzFramework::ViewportId viewportId)
        : m_viewportId(viewportId)
    {
        EditorModularViewportCameraComposerNotificationBus::Handler::BusConnect(viewportId);
        Camera::EditorCameraNotificationBus::Handler::BusConnect();
    }

    EditorModularViewportCameraComposer::~EditorModularViewportCameraComposer()
    {
        Camera::EditorCameraNotificationBus::Handler::BusDisconnect();
        EditorModularViewportCameraComposerNotificationBus::Handler::BusDisconnect();
    }

    AZStd::shared_ptr<AtomToolsFramework::ModularViewportCameraController> EditorModularViewportCameraComposer::
        CreateModularViewportCameraController()
    {
        SetupCameras();

        auto controller = AZStd::make_shared<AtomToolsFramework::ModularViewportCameraController>();

        controller->SetCameraViewportContextBuilderCallback(
            [viewportId = m_viewportId](AZStd::unique_ptr<AtomToolsFramework::ModularCameraViewportContext>& cameraViewportContext)
            {
                cameraViewportContext = AZStd::make_unique<AtomToolsFramework::ModularCameraViewportContextImpl>(viewportId);
            });

        controller->SetCameraPriorityBuilderCallback(
            [](AtomToolsFramework::CameraControllerPriorityFn& cameraControllerPriorityFn)
            {
                cameraControllerPriorityFn = AtomToolsFramework::DefaultCameraControllerPriority;
            });

        controller->SetCameraPropsBuilderCallback(
            [](AzFramework::CameraProps& cameraProps)
            {
                cameraProps.m_rotateSmoothnessFn = []
                {
                    return SandboxEditor::CameraRotateSmoothness();
                };

                cameraProps.m_translateSmoothnessFn = []
                {
                    return SandboxEditor::CameraTranslateSmoothness();
                };

                cameraProps.m_rotateSmoothingEnabledFn = []
                {
                    return SandboxEditor::CameraRotateSmoothingEnabled();
                };

                cameraProps.m_translateSmoothingEnabledFn = []
                {
                    return SandboxEditor::CameraTranslateSmoothingEnabled();
                };
            });

        controller->SetCameraListBuilderCallback(
            [this](AzFramework::Cameras& cameras)
            {
                cameras.AddCamera(m_firstPersonRotateCamera);
                cameras.AddCamera(m_firstPersonPanCamera);
                cameras.AddCamera(m_firstPersonTranslateCamera);
                cameras.AddCamera(m_firstPersonScrollCamera);
                cameras.AddCamera(m_firstPersonFocusCamera);
                cameras.AddCamera(m_orbitCamera);
            });

        return controller;
    }

    void EditorModularViewportCameraComposer::SetupCameras()
    {
        const auto hideCursor = [viewportId = m_viewportId]
        {
            if (SandboxEditor::CameraCaptureCursorForLook())
            {
                AzToolsFramework::ViewportInteraction::ViewportMouseCursorRequestBus::Event(
                    viewportId, &AzToolsFramework::ViewportInteraction::ViewportMouseCursorRequestBus::Events::BeginCursorCapture);
            }
        };

        const auto showCursor = [viewportId = m_viewportId]
        {
            if (SandboxEditor::CameraCaptureCursorForLook())
            {
                AzToolsFramework::ViewportInteraction::ViewportMouseCursorRequestBus::Event(
                    viewportId, &AzToolsFramework::ViewportInteraction::ViewportMouseCursorRequestBus::Events::EndCursorCapture);
            }
        };

        m_firstPersonRotateCamera = AZStd::make_shared<AzFramework::RotateCameraInput>(SandboxEditor::CameraFreeLookChannelId());

        m_firstPersonRotateCamera->m_rotateSpeedFn = []
        {
            return SandboxEditor::CameraRotateSpeed();
        };

        // default behavior is to hide the cursor but this can be disabled (useful for remote desktop)
        // note: See CaptureCursorLook in the Settings Registry
        m_firstPersonRotateCamera->SetActivationBeganFn(hideCursor);
        m_firstPersonRotateCamera->SetActivationEndedFn(showCursor);

        m_firstPersonPanCamera = AZStd::make_shared<AzFramework::PanCameraInput>(
            SandboxEditor::CameraFreePanChannelId(), AzFramework::LookPan, AzFramework::TranslatePivotLook);

        m_firstPersonPanCamera->m_panSpeedFn = []
        {
            return SandboxEditor::CameraPanSpeed();
        };

        m_firstPersonPanCamera->m_invertPanXFn = []
        {
            return SandboxEditor::CameraPanInvertedX();
        };

        m_firstPersonPanCamera->m_invertPanYFn = []
        {
            return SandboxEditor::CameraPanInvertedY();
        };

        const auto translateCameraInputChannelIds = BuildTranslateCameraInputChannelIds();

        m_firstPersonTranslateCamera = AZStd::make_shared<AzFramework::TranslateCameraInput>(
            translateCameraInputChannelIds, AzFramework::LookTranslation, AzFramework::TranslatePivotLook);

        m_firstPersonTranslateCamera->m_translateSpeedFn = []
        {
            return SandboxEditor::CameraTranslateSpeed();
        };

        m_firstPersonTranslateCamera->m_boostMultiplierFn = []
        {
            return SandboxEditor::CameraBoostMultiplier();
        };

        m_firstPersonScrollCamera = AZStd::make_shared<AzFramework::LookScrollTranslationCameraInput>();

        m_firstPersonScrollCamera->m_scrollSpeedFn = []
        {
            return SandboxEditor::CameraScrollSpeed();
        };

        const auto pivotFn = []
        {
            // use the manipulator transform as the pivot point
            AZStd::optional<AZ::Transform> entityPivot;
            AzToolsFramework::EditorTransformComponentSelectionRequestBus::EventResult(
                entityPivot, AzToolsFramework::GetEntityContextId(),
                &AzToolsFramework::EditorTransformComponentSelectionRequestBus::Events::GetManipulatorTransform);

            if (entityPivot.has_value())
            {
                return entityPivot->GetTranslation();
            }

            // otherwise just use the identity
            return AZ::Vector3::CreateZero();
        };

        m_firstPersonFocusCamera =
            AZStd::make_shared<AzFramework::FocusCameraInput>(SandboxEditor::CameraFocusChannelId(), AzFramework::FocusLook);

        m_firstPersonFocusCamera->SetPivotFn(pivotFn);

        m_orbitCamera = AZStd::make_shared<AzFramework::OrbitCameraInput>(SandboxEditor::CameraOrbitChannelId());

        m_orbitCamera->SetPivotFn(
            [pivotFn]([[maybe_unused]] const AZ::Vector3& position, [[maybe_unused]] const AZ::Vector3& direction)
            {
                return pivotFn();
            });

        m_orbitRotateCamera = AZStd::make_shared<AzFramework::RotateCameraInput>(SandboxEditor::CameraOrbitLookChannelId());

        m_orbitRotateCamera->m_rotateSpeedFn = []
        {
            return SandboxEditor::CameraRotateSpeed();
        };

        m_orbitRotateCamera->m_invertYawFn = []
        {
            return SandboxEditor::CameraOrbitYawRotationInverted();
        };

        m_orbitTranslateCamera = AZStd::make_shared<AzFramework::TranslateCameraInput>(
            translateCameraInputChannelIds, AzFramework::LookTranslation, AzFramework::TranslateOffsetOrbit);

        m_orbitTranslateCamera->m_translateSpeedFn = []
        {
            return SandboxEditor::CameraTranslateSpeed();
        };

        m_orbitTranslateCamera->m_boostMultiplierFn = []
        {
            return SandboxEditor::CameraBoostMultiplier();
        };

        m_orbitDollyScrollCamera = AZStd::make_shared<AzFramework::OrbitDollyScrollCameraInput>();

        m_orbitDollyScrollCamera->m_scrollSpeedFn = []
        {
            return SandboxEditor::CameraScrollSpeed();
        };

        m_orbitDollyMoveCamera = AZStd::make_shared<AzFramework::OrbitDollyMotionCameraInput>(SandboxEditor::CameraOrbitDollyChannelId());

        m_orbitDollyMoveCamera->m_motionSpeedFn = []
        {
            return SandboxEditor::CameraDollyMotionSpeed();
        };

        m_orbitPanCamera = AZStd::make_shared<AzFramework::PanCameraInput>(
            SandboxEditor::CameraOrbitPanChannelId(), AzFramework::LookPan, AzFramework::TranslateOffsetOrbit);

        m_orbitPanCamera->m_panSpeedFn = []
        {
            return SandboxEditor::CameraPanSpeed();
        };

        m_orbitPanCamera->m_invertPanXFn = []
        {
            return SandboxEditor::CameraPanInvertedX();
        };

        m_orbitPanCamera->m_invertPanYFn = []
        {
            return SandboxEditor::CameraPanInvertedY();
        };

        m_orbitFocusCamera =
            AZStd::make_shared<AzFramework::FocusCameraInput>(SandboxEditor::CameraFocusChannelId(), AzFramework::FocusOrbit);

        m_orbitFocusCamera->SetPivotFn(pivotFn);

        m_orbitCamera->m_orbitCameras.AddCamera(m_orbitRotateCamera);
        m_orbitCamera->m_orbitCameras.AddCamera(m_orbitTranslateCamera);
        m_orbitCamera->m_orbitCameras.AddCamera(m_orbitDollyScrollCamera);
        m_orbitCamera->m_orbitCameras.AddCamera(m_orbitDollyMoveCamera);
        m_orbitCamera->m_orbitCameras.AddCamera(m_orbitPanCamera);
        m_orbitCamera->m_orbitCameras.AddCamera(m_orbitFocusCamera);
    }

    void EditorModularViewportCameraComposer::OnEditorModularViewportCameraComposerSettingsChanged()
    {
        const auto translateCameraInputChannelIds = BuildTranslateCameraInputChannelIds();
        m_firstPersonTranslateCamera->SetTranslateCameraInputChannelIds(translateCameraInputChannelIds);
        m_firstPersonPanCamera->SetPanInputChannelId(SandboxEditor::CameraFreePanChannelId());
        m_firstPersonRotateCamera->SetRotateInputChannelId(SandboxEditor::CameraFreeLookChannelId());
        m_firstPersonFocusCamera->SetFocusInputChannelId(SandboxEditor::CameraFocusChannelId());

        m_orbitCamera->SetOrbitInputChannelId(SandboxEditor::CameraOrbitChannelId());
        m_orbitTranslateCamera->SetTranslateCameraInputChannelIds(translateCameraInputChannelIds);
        m_orbitPanCamera->SetPanInputChannelId(SandboxEditor::CameraOrbitPanChannelId());
        m_orbitRotateCamera->SetRotateInputChannelId(SandboxEditor::CameraOrbitLookChannelId());
        m_orbitDollyMoveCamera->SetDollyInputChannelId(SandboxEditor::CameraOrbitDollyChannelId());
        m_orbitFocusCamera->SetFocusInputChannelId(SandboxEditor::CameraFocusChannelId());
    }

    void EditorModularViewportCameraComposer::OnViewportViewEntityChanged(const AZ::EntityId& viewEntityId)
    {
        if (viewEntityId.IsValid())
        {
            AZ::Transform worldFromLocal = AZ::Transform::CreateIdentity();
            AZ::TransformBus::EventResult(worldFromLocal, viewEntityId, &AZ::TransformBus::Events::GetWorldTM);

            AtomToolsFramework::ModularViewportCameraControllerRequestBus::Event(
                m_viewportId, &AtomToolsFramework::ModularViewportCameraControllerRequestBus::Events::SetReferenceFrame, worldFromLocal);
        }
        else
        {
            AtomToolsFramework::ModularViewportCameraControllerRequestBus::Event(
                m_viewportId, &AtomToolsFramework::ModularViewportCameraControllerRequestBus::Events::ClearReferenceFrame);
        }
    }
} // namespace SandboxEditor
