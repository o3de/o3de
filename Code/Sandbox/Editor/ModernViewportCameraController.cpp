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
#include <AzCore/Console/IConsole.h>
#include <AzCore/Interface/Interface.h>
#include <AzFramework/Viewport/CameraInput.h>
#include <AzFramework/Viewport/ScreenGeometry.h>
#include <AzFramework/Windowing/WindowBus.h>
#include <AzToolsFramework/Viewport/ViewportMessages.h>

AZ_CVAR(bool, ed_newCameraSystemDebug, false, nullptr, AZ::ConsoleFunctorFlags::Null, "Enable debug drawing for the new camera system");

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

    struct ModernViewportCameraControllerInstance::Impl
    {
        AzFramework::Camera m_camera;
        AzFramework::Camera m_targetCamera;
        AzFramework::SmoothProps m_smoothProps;
        AzFramework::CameraSystem m_cameraSystem;
    };

    ModernViewportCameraControllerInstance::ModernViewportCameraControllerInstance(const AzFramework::ViewportId viewportId)
        : MultiViewportControllerInstanceInterface(viewportId)
        , m_impl(AZStd::make_unique<Impl>())
    {
        // LYN-2315 TODO - move setup out of constructor, pass cameras in
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

        m_impl->m_cameraSystem.m_cameras.AddCamera(firstPersonRotateCamera);
        m_impl->m_cameraSystem.m_cameras.AddCamera(firstPersonPanCamera);
        m_impl->m_cameraSystem.m_cameras.AddCamera(firstPersonTranslateCamera);
        m_impl->m_cameraSystem.m_cameras.AddCamera(firstPersonWheelCamera);
        m_impl->m_cameraSystem.m_cameras.AddCamera(orbitCamera);

        if (const auto viewportContext = RetrieveViewportContext(viewportId))
        {
            // set position but not orientation
            m_impl->m_targetCamera.m_lookAt = viewportContext->GetCameraTransform().GetTranslation();

            // LYN-2315 TODO https://www.geometrictools.com/Documentation/EulerAngles.pdf

            m_impl->m_camera = m_impl->m_targetCamera;
        }

        AzFramework::ViewportDebugDisplayEventBus::Handler::BusConnect(AzToolsFramework::GetEntityContextId());
    }

    ModernViewportCameraControllerInstance::~ModernViewportCameraControllerInstance()
    {
        AzFramework::ViewportDebugDisplayEventBus::Handler::BusDisconnect();
    }

    bool ModernViewportCameraControllerInstance::HandleInputChannelEvent(const AzFramework::ViewportControllerInputEvent& event)
    {
        AzFramework::WindowSize windowSize;
        AzFramework::WindowRequestBus::EventResult(
            windowSize, event.m_windowHandle, &AzFramework::WindowRequestBus::Events::GetClientAreaSize);
        return m_impl->m_cameraSystem.HandleEvents(AzFramework::BuildInputEvent(event.m_inputChannel, windowSize));
    }

    void ModernViewportCameraControllerInstance::UpdateViewport(const AzFramework::ViewportControllerUpdateEvent& event)
    {
        if (auto viewportContext = RetrieveViewportContext(GetViewportId()))
        {
            m_impl->m_targetCamera = m_impl->m_cameraSystem.StepCamera(m_impl->m_targetCamera, event.m_deltaTime.count());
            m_impl->m_camera =
                AzFramework::SmoothCamera(m_impl->m_camera, m_impl->m_targetCamera, m_impl->m_smoothProps, event.m_deltaTime.count());

            viewportContext->SetCameraTransform(m_impl->m_camera.Transform());
        }
    }

    void ModernViewportCameraControllerInstance::DisplayViewport(
        [[maybe_unused]] const AzFramework::ViewportInfo& viewportInfo, AzFramework::DebugDisplayRequests& debugDisplay)
    {
        if (ed_newCameraSystemDebug)
        {
            debugDisplay.SetColor(AZ::Colors::White);
            debugDisplay.DrawWireSphere(m_impl->m_targetCamera.m_lookAt, 0.5f);
        }
    }
} // namespace SandboxEditor
