/*
 * All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 * its licensors.
 *
 * For complete copyright and license terms please see the LICENSE at the root of this
 * distribution (the "License"). All use of this software is governed by the License,
 * or, if provided, by the license below or the license accompanying this file. Do not
 * remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 */

#include "ModernViewportCameraController.h"

#include <Atom/RPI.Public/ViewportContext.h>
#include <Atom/RPI.Public/ViewportContextBus.h>
#include <AzCore/Interface/Interface.h>
#include <AzFramework/Viewport/ScreenGeometry.h>
#include <AzFramework/Windowing/WindowBus.h>

namespace SandboxEditor
{
    static AZ::RPI::ViewportContextPtr RetrieveViewportContext(const AzFramework::ViewportId viewportId)
    {
        auto viewportContextManager = AZ::Interface<AZ::RPI::ViewportContextRequestsInterface>::Get();
        if (!viewportContextManager)
        {
            return nullptr;
        }

        auto viewportContext = viewportContextManager->GetViewportContextById(viewportId);
        if (!viewportContext)
        {
            return nullptr;
        }

        return viewportContext;
    }

    AzFramework::Cameras ModernViewportCameraController::GetCameras() const
    {
        AzFramework::Cameras cameras;
        auto firstPersonRotateCamera = AZStd::make_shared<AzFramework::RotateCameraInput>(AzFramework::InputDeviceMouse::Button::Right);
        auto firstPersonPanCamera = AZStd::make_shared<AzFramework::PanCameraInput>(AzFramework::LookPan);
        auto firstPersonTranslateCamera = AZStd::make_shared<AzFramework::TranslateCameraInput>(AzFramework::LookTranslation);
        auto firstPersonWheelCamera = AZStd::make_shared<AzFramework::ScrollTranslationCameraInput>();

        auto orbitCamera = AZStd::make_shared<AzFramework::OrbitCameraInput>();
        auto orbitRotateCamera = AZStd::make_shared<AzFramework::RotateCameraInput>(AzFramework::InputDeviceMouse::Button::Left);
        auto orbitTranslateCamera = AZStd::make_shared<AzFramework::TranslateCameraInput>(AzFramework::OrbitTranslation);
        auto orbitDollyWheelCamera = AZStd::make_shared<AzFramework::OrbitDollyScrollCameraInput>();
        auto orbitDollyMoveCamera = AZStd::make_shared<AzFramework::OrbitDollyCursorMoveCameraInput>();
        auto orbitPanCamera = AZStd::make_shared<AzFramework::PanCameraInput>(AzFramework::OrbitPan);
        orbitCamera->m_orbitCameras.AddCamera(orbitRotateCamera);
        orbitCamera->m_orbitCameras.AddCamera(orbitTranslateCamera);
        orbitCamera->m_orbitCameras.AddCamera(orbitDollyWheelCamera);
        orbitCamera->m_orbitCameras.AddCamera(orbitDollyMoveCamera);
        orbitCamera->m_orbitCameras.AddCamera(orbitPanCamera);

        cameras.AddCamera(firstPersonRotateCamera);
        cameras.AddCamera(firstPersonPanCamera);
        cameras.AddCamera(firstPersonTranslateCamera);
        cameras.AddCamera(firstPersonWheelCamera);
        cameras.AddCamera(orbitCamera);
        return AZStd::move(cameras);
    }

    ModernViewportCameraControllerInstance::ModernViewportCameraControllerInstance(const AzFramework::ViewportId viewportId, ModernViewportCameraController* controller)
        : MultiViewportControllerInstanceInterface<ModernViewportCameraController>(viewportId, controller)
    {
        // LYN-2315 TODO - move setup out of constructor, pass cameras in
        m_cameraSystem.m_cameras = controller->GetCameras();

        if (const auto viewportContext = RetrieveViewportContext(viewportId))
        {
            // set position but not orientation
            m_targetCamera.m_lookAt = viewportContext->GetCameraTransform().GetTranslation();

            // LYN-2315 TODO https://www.geometrictools.com/Documentation/EulerAngles.pdf

            m_camera = m_targetCamera;
        }
    }

    bool ModernViewportCameraControllerInstance::HandleInputChannelEvent(const AzFramework::ViewportControllerInputEvent& event)
    {
        AzFramework::WindowSize windowSize;
        AzFramework::WindowRequestBus::EventResult(
            windowSize, event.m_windowHandle, &AzFramework::WindowRequestBus::Events::GetClientAreaSize);
        m_cameraSystem.HandleEvents(AzFramework::BuildInputEvent(event.m_inputChannel, windowSize));
        return true; // consume event
    }

    void ModernViewportCameraControllerInstance::UpdateViewport(const AzFramework::ViewportControllerUpdateEvent& event)
    {
        if (auto viewportContext = RetrieveViewportContext(GetViewportId()))
        {
            m_targetCamera = m_cameraSystem.StepCamera(m_targetCamera, event.m_deltaTime.count());
            m_camera = AzFramework::SmoothCamera(m_camera, m_targetCamera, m_smoothProps, event.m_deltaTime.count());

            viewportContext->SetCameraTransform(m_camera.Transform());
        }
    }
} // namespace SandboxEditor
