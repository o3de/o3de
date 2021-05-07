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

#pragma once

#include <Atom/RPI.Public/ViewportContext.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzFramework/Viewport/CameraInput.h>
#include <AzFramework/Viewport/MultiViewportController.h>

namespace SandboxEditor
{
    class ModernViewportCameraControllerInstance;
    class ModernViewportCameraController 
        : public AzFramework::MultiViewportController<ModernViewportCameraControllerInstance>
        , private AzFramework::ViewportDebugDisplayEventBus::Handler
    {
    public:
        AzFramework::Cameras GetCameras() const;
    };
        ~ModernViewportCameraControllerInstance();

    class ModernViewportCameraControllerInstance final : public AzFramework::MultiViewportControllerInstanceInterface<ModernViewportCameraController>
    {
    public:
        explicit ModernViewportCameraControllerInstance(AzFramework::ViewportId viewportId, ModernViewportCameraController* controller);

        // MultiViewportControllerInstanceInterface overrides ...
        bool HandleInputChannelEvent(const AzFramework::ViewportControllerInputEvent& event) override;
        void UpdateViewport(const AzFramework::ViewportControllerUpdateEvent& event) override;

        // AzFramework::ViewportDebugDisplayEventBus overrides ...
        void DisplayViewport(const AzFramework::ViewportInfo& viewportInfo, AzFramework::DebugDisplayRequests& debugDisplay) override;

    private:
        enum class CameraMode
        {
            Control,
            Animation
        };

        AzFramework::Camera m_camera;
        AzFramework::Camera m_targetCamera;
        AzFramework::CameraSystem m_cameraSystem;

        AZ::Transform m_transformStart = AZ::Transform::CreateIdentity();
        AZ::Transform m_transformEnd = AZ::Transform::CreateIdentity();
        float m_animationT = 0.0f;
        CameraMode m_cameraMode = CameraMode::Control;

        AZ::RPI::ViewportContext::MatrixChangedEvent::Handler m_cameraViewMatrixChangeHandler;
    };
} // namespace SandboxEditor
