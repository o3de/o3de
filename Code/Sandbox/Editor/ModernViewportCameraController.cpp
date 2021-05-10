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
#include <AzFramework/Input/Devices/Keyboard/InputDeviceKeyboard.h>
#include <AzFramework/Input/Devices/Mouse/InputDeviceMouse.h>
#include <AzFramework/Viewport/ScreenGeometry.h>
#include <AzFramework/Windowing/WindowBus.h>
#include <AzToolsFramework/Viewport/ViewportMessages.h>

namespace SandboxEditor
{
    static void DrawPreviewAxis(AzFramework::DebugDisplayRequests& display, const AZ::Transform& transform, const float axisLength)
    {
        display.SetColor(AZ::Colors::Red);
        display.DrawLine(transform.GetTranslation(), transform.GetTranslation() + transform.GetBasisX().GetNormalizedSafe() * axisLength);
        display.SetColor(AZ::Colors::Green);
        display.DrawLine(transform.GetTranslation(), transform.GetTranslation() + transform.GetBasisY().GetNormalizedSafe() * axisLength);
        display.SetColor(AZ::Colors::Blue);
        display.DrawLine(transform.GetTranslation(), transform.GetTranslation() + transform.GetBasisZ().GetNormalizedSafe() * axisLength);
    }

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

    void ModernViewportCameraController::SetCameraListBuilderCallback(const CameraListBuilder& builder)
    {
        m_cameraListBuilder = builder;
    }

    void ModernViewportCameraController::SetupCameras(AzFramework::Cameras& cameras)
    {
        if (m_cameraListBuilder)
        {
            m_cameraListBuilder(cameras);
        }
    }

    ModernViewportCameraControllerInstance::ModernViewportCameraControllerInstance(const AzFramework::ViewportId viewportId, ModernViewportCameraController* controller)
        : MultiViewportControllerInstanceInterface<ModernViewportCameraController>(viewportId, controller)
    {
        controller->SetupCameras(m_cameraSystem.m_cameras);

        if (auto viewportContext = RetrieveViewportContext(GetViewportId()))
        {
            auto handleCameraChange = [this](const AZ::Matrix4x4& matrix) {
                UpdateCameraFromTransform(
                    m_targetCamera,
                    AZ::Transform::CreateFromMatrix3x3AndTranslation(AZ::Matrix3x3::CreateFromMatrix4x4(matrix), matrix.GetTranslation()));
            };

            m_cameraViewMatrixChangeHandler = AZ::RPI::ViewportContext::MatrixChangedEvent::Handler(handleCameraChange);

            viewportContext->ConnectViewMatrixChangedHandler(m_cameraViewMatrixChangeHandler);
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

        if (m_cameraMode == CameraMode::Control)
        {
            if (AzFramework::InputDeviceKeyboard::IsKeyboardDevice(event.m_inputChannel.GetInputDevice().GetInputDeviceId()))
            {
                if (event.m_inputChannel.GetInputChannelId() == AzFramework::InputDeviceKeyboard::Key::AlphanumericR)
                {
                    m_transformEnd = m_camera.Transform();

                    return true;
                }
                else if (event.m_inputChannel.GetInputChannelId() == AzFramework::InputDeviceKeyboard::Key::AlphanumericP)
                {
                    m_animationT = 0.0f;
                    m_cameraMode = CameraMode::Animation;
                    m_transformStart = m_camera.Transform();

                    return true;
                }
            }
        }

        return m_cameraSystem.HandleEvents(AzFramework::BuildInputEvent(event.m_inputChannel, windowSize));
    }

    void ModernViewportCameraControllerInstance::UpdateViewport(const AzFramework::ViewportControllerUpdateEvent& event)
    {
        if (auto viewportContext = RetrieveViewportContext(GetViewportId()))
        {
            if (m_cameraMode == CameraMode::Control)
            {
                m_targetCamera = m_cameraSystem.StepCamera(m_targetCamera, event.m_deltaTime.count());
                m_camera = AzFramework::SmoothCamera(m_camera, m_targetCamera, event.m_deltaTime.count());

                viewportContext->SetCameraTransform(m_camera.Transform());
            }
            else if (m_cameraMode == CameraMode::Animation)
            {
                const auto smootherStepFn = [](const float t) { return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f); };
                const float transitionT = smootherStepFn(m_animationT);

                const AZ::Transform current = AZ::Transform::CreateFromQuaternionAndTranslation(
                    m_transformStart.GetRotation().Slerp(m_transformEnd.GetRotation(), transitionT),
                    m_transformStart.GetTranslation().Lerp(m_transformEnd.GetTranslation(), transitionT));

                const AZ::Vector3 eulerAngles = AzFramework::EulerAngles(AZ::Matrix3x3::CreateFromTransform(current));
                m_camera.m_pitch = eulerAngles.GetX();
                m_camera.m_yaw = eulerAngles.GetZ();
                m_camera.m_lookAt = current.GetTranslation();
                m_targetCamera = m_camera;

                if (m_animationT >= 1.0f)
                {
                    m_cameraMode = CameraMode::Control;
                }

                m_animationT = AZ::GetClamp(m_animationT + event.m_deltaTime.count(), 0.0f, 1.0f);

                viewportContext->SetCameraTransform(current);
            }
        }
    }

    void ModernViewportCameraControllerInstance::DisplayViewport(
        [[maybe_unused]] const AzFramework::ViewportInfo& viewportInfo, AzFramework::DebugDisplayRequests& debugDisplay)
    {
        if (const float alpha = AZStd::min(-m_camera.m_lookDist / 5.0f, 1.0f); alpha > AZ::Constants::FloatEpsilon)
        {
            debugDisplay.SetColor(1.0f, 1.0f, 1.0f, alpha);
            debugDisplay.DrawWireSphere(m_camera.m_lookAt, 0.5f);
        }

        DrawPreviewAxis(debugDisplay, m_transformEnd, 2.0f);
    }
} // namespace SandboxEditor
