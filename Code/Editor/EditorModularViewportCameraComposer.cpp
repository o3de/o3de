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
                cameras.AddCamera(m_pivotCamera);
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
            SandboxEditor::CameraFreePanChannelId(), AzFramework::LookPan, AzFramework::TranslatePivot);

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
            translateCameraInputChannelIds, AzFramework::LookTranslation, AzFramework::TranslatePivot);

        m_firstPersonTranslateCamera->m_translateSpeedFn = []
        {
            return SandboxEditor::CameraTranslateSpeed();
        };

        m_firstPersonTranslateCamera->m_boostMultiplierFn = []
        {
            return SandboxEditor::CameraBoostMultiplier();
        };

        m_firstPersonScrollCamera = AZStd::make_shared<AzFramework::ScrollTranslationCameraInput>();

        m_firstPersonScrollCamera->m_scrollSpeedFn = []
        {
            return SandboxEditor::CameraScrollSpeed();
        };

        m_pivotCamera = AZStd::make_shared<AzFramework::PivotCameraInput>(SandboxEditor::CameraPivotChannelId());

        m_pivotCamera->SetPivotFn(
            []([[maybe_unused]] const AZ::Vector3& position, [[maybe_unused]] const AZ::Vector3& direction)
            {
                // use the manipulator transform as the pivot point
                AZStd::optional<AZ::Transform> entityPivot;
                AzToolsFramework::EditorTransformComponentSelectionRequestBus::EventResult(
                    entityPivot, AzToolsFramework::GetEntityContextId(),
                    &AzToolsFramework::EditorTransformComponentSelectionRequestBus::Events::GetManipulatorTransform);

                // otherwise just use the identity
                return entityPivot.value_or(AZ::Transform::CreateIdentity()).GetTranslation();
            });

        m_pivotRotateCamera = AZStd::make_shared<AzFramework::RotateCameraInput>(SandboxEditor::CameraPivotLookChannelId());

        m_pivotRotateCamera->m_rotateSpeedFn = []
        {
            return SandboxEditor::CameraRotateSpeed();
        };

        m_pivotRotateCamera->m_invertYawFn = []
        {
            return SandboxEditor::CameraPivotYawRotationInverted();
        };

        m_pivotTranslateCamera = AZStd::make_shared<AzFramework::TranslateCameraInput>(
            translateCameraInputChannelIds, AzFramework::LookTranslation, AzFramework::TranslateOffset);

        m_pivotTranslateCamera->m_translateSpeedFn = []
        {
            return SandboxEditor::CameraTranslateSpeed();
        };

        m_pivotTranslateCamera->m_boostMultiplierFn = []
        {
            return SandboxEditor::CameraBoostMultiplier();
        };

        m_pivotDollyScrollCamera = AZStd::make_shared<AzFramework::PivotDollyScrollCameraInput>();

        m_pivotDollyScrollCamera->m_scrollSpeedFn = []
        {
            return SandboxEditor::CameraScrollSpeed();
        };

        m_pivotDollyMoveCamera = AZStd::make_shared<AzFramework::PivotDollyMotionCameraInput>(SandboxEditor::CameraPivotDollyChannelId());

        m_pivotDollyMoveCamera->m_motionSpeedFn = []
        {
            return SandboxEditor::CameraDollyMotionSpeed();
        };

        m_pivotPanCamera = AZStd::make_shared<AzFramework::PanCameraInput>(
            SandboxEditor::CameraPivotPanChannelId(), AzFramework::LookPan, AzFramework::TranslateOffset);

        m_pivotPanCamera->m_panSpeedFn = []
        {
            return SandboxEditor::CameraPanSpeed();
        };

        m_pivotPanCamera->m_invertPanXFn = []
        {
            return SandboxEditor::CameraPanInvertedX();
        };

        m_pivotPanCamera->m_invertPanYFn = []
        {
            return SandboxEditor::CameraPanInvertedY();
        };

        m_pivotCamera->m_pivotCameras.AddCamera(m_pivotRotateCamera);
        m_pivotCamera->m_pivotCameras.AddCamera(m_pivotTranslateCamera);
        m_pivotCamera->m_pivotCameras.AddCamera(m_pivotDollyScrollCamera);
        m_pivotCamera->m_pivotCameras.AddCamera(m_pivotDollyMoveCamera);
        m_pivotCamera->m_pivotCameras.AddCamera(m_pivotPanCamera);
    }

    void EditorModularViewportCameraComposer::OnEditorModularViewportCameraComposerSettingsChanged()
    {
        const auto translateCameraInputChannelIds = BuildTranslateCameraInputChannelIds();
        m_firstPersonTranslateCamera->SetTranslateCameraInputChannelIds(translateCameraInputChannelIds);
        m_firstPersonPanCamera->SetPanInputChannelId(SandboxEditor::CameraFreePanChannelId());
        m_firstPersonRotateCamera->SetRotateInputChannelId(SandboxEditor::CameraFreeLookChannelId());

        m_pivotCamera->SetPivotInputChannelId(SandboxEditor::CameraPivotChannelId());
        m_pivotTranslateCamera->SetTranslateCameraInputChannelIds(translateCameraInputChannelIds);
        m_pivotPanCamera->SetPanInputChannelId(SandboxEditor::CameraPivotPanChannelId());
        m_pivotRotateCamera->SetRotateInputChannelId(SandboxEditor::CameraPivotLookChannelId());
        m_pivotDollyMoveCamera->SetDollyInputChannelId(SandboxEditor::CameraPivotDollyChannelId());
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
