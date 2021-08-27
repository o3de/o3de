/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EditorModularViewportCameraComposer.h>

#include <AtomToolsFramework/Viewport/ModularViewportCameraControllerRequestBus.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzFramework/Render/IntersectorInterface.h>
#include <AzToolsFramework/Viewport/ViewportMessages.h>
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
    }

    EditorModularViewportCameraComposer::~EditorModularViewportCameraComposer()
    {
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

        m_firstPersonPanCamera =
            AZStd::make_shared<AzFramework::PanCameraInput>(SandboxEditor::CameraFreePanChannelId(), AzFramework::LookPan);

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

        m_firstPersonTranslateCamera =
            AZStd::make_shared<AzFramework::TranslateCameraInput>(AzFramework::LookTranslation, translateCameraInputChannelIds);

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

        m_orbitCamera = AZStd::make_shared<AzFramework::OrbitCameraInput>(SandboxEditor::CameraOrbitChannelId());

        m_orbitCamera->SetLookAtFn(
            [viewportId = m_viewportId](const AZ::Vector3& position, const AZ::Vector3& direction) -> AZStd::optional<AZ::Vector3>
            {
                AZStd::optional<AZ::Vector3> lookAtAfterInterpolation;
                AtomToolsFramework::ModularViewportCameraControllerRequestBus::EventResult(
                    lookAtAfterInterpolation, viewportId,
                    &AtomToolsFramework::ModularViewportCameraControllerRequestBus::Events::LookAtAfterInterpolation);

                // initially attempt to use the last set look at point after an interpolation has finished
                if (lookAtAfterInterpolation.has_value())
                {
                    return *lookAtAfterInterpolation;
                }

                const float RayDistance = 1000.0f;
                AzFramework::RenderGeometry::RayRequest ray;
                ray.m_startWorldPosition = position;
                ray.m_endWorldPosition = position + direction * RayDistance;
                ray.m_onlyVisible = true;

                AzFramework::RenderGeometry::RayResult renderGeometryIntersectionResult;
                AzFramework::RenderGeometry::IntersectorBus::EventResult(
                    renderGeometryIntersectionResult, AzToolsFramework::GetEntityContextId(),
                    &AzFramework::RenderGeometry::IntersectorBus::Events::RayIntersect, ray);

                // attempt a ray intersection with any visible mesh and return the intersection position if successful
                if (renderGeometryIntersectionResult)
                {
                    return renderGeometryIntersectionResult.m_worldPosition;
                }

                // if there is no selection or no intersection, fallback to default camera orbit behavior (ground plane
                // intersection)
                return {};
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

        m_orbitTranslateCamera =
            AZStd::make_shared<AzFramework::TranslateCameraInput>(AzFramework::OrbitTranslation, translateCameraInputChannelIds);

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

        m_orbitDollyMoveCamera =
            AZStd::make_shared<AzFramework::OrbitDollyCursorMoveCameraInput>(SandboxEditor::CameraOrbitDollyChannelId());

        m_orbitDollyMoveCamera->m_cursorSpeedFn = []
        {
            return SandboxEditor::CameraDollyMotionSpeed();
        };

        m_orbitPanCamera = AZStd::make_shared<AzFramework::PanCameraInput>(SandboxEditor::CameraOrbitPanChannelId(), AzFramework::OrbitPan);

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

        m_orbitCamera->m_orbitCameras.AddCamera(m_orbitRotateCamera);
        m_orbitCamera->m_orbitCameras.AddCamera(m_orbitTranslateCamera);
        m_orbitCamera->m_orbitCameras.AddCamera(m_orbitDollyScrollCamera);
        m_orbitCamera->m_orbitCameras.AddCamera(m_orbitDollyMoveCamera);
        m_orbitCamera->m_orbitCameras.AddCamera(m_orbitPanCamera);
    }

    void EditorModularViewportCameraComposer::OnEditorModularViewportCameraComposerSettingsChanged()
    {
        const auto translateCameraInputChannelIds = BuildTranslateCameraInputChannelIds();
        m_firstPersonTranslateCamera->SetTranslateCameraInputChannelIds(translateCameraInputChannelIds);
        m_orbitTranslateCamera->SetTranslateCameraInputChannelIds(translateCameraInputChannelIds);

        m_firstPersonPanCamera->SetPanInputChannelId(SandboxEditor::CameraFreePanChannelId());
        m_orbitPanCamera->SetPanInputChannelId(SandboxEditor::CameraOrbitPanChannelId());
        m_firstPersonRotateCamera->SetRotateInputChannelId(SandboxEditor::CameraFreeLookChannelId());
        m_orbitRotateCamera->SetRotateInputChannelId(SandboxEditor::CameraOrbitLookChannelId());
        m_orbitCamera->SetOrbitInputChannelId(SandboxEditor::CameraOrbitChannelId());
        m_orbitDollyMoveCamera->SetDollyInputChannelId(SandboxEditor::CameraOrbitDollyChannelId());
    }
} // namespace SandboxEditor
