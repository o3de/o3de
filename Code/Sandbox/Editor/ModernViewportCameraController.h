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

#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzFramework/Viewport/CameraInput.h>
#include <AzFramework/Viewport/MultiViewportController.h>

namespace SandboxEditor
{
    class ModernViewportCameraControllerInstance final : public AzFramework::MultiViewportControllerInstanceInterface,
                                                         private AzFramework::ViewportDebugDisplayEventBus::Handler,
                                                         private AzFramework::ModernViewportCameraControllerRequestBus::Handler
    {
    public:
        explicit ModernViewportCameraControllerInstance(AzFramework::ViewportId viewportId);
        ~ModernViewportCameraControllerInstance();

        // MultiViewportControllerInstanceInterface overrides ...
        bool HandleInputChannelEvent(const AzFramework::ViewportControllerInputEvent& event) override;
        void UpdateViewport(const AzFramework::ViewportControllerUpdateEvent& event) override;

        // AzFramework::ViewportDebugDisplayEventBus overrides ...
        void DisplayViewport(const AzFramework::ViewportInfo& viewportInfo, AzFramework::DebugDisplayRequests& debugDisplay) override;

        // ModernViewportCameraControllerRequestBus overrides ...
        void SetTargetCameraTransform(const AZ::Transform& transform) override;

    private:
        AzFramework::Camera m_camera;
        AzFramework::Camera m_targetCamera;
        AzFramework::SmoothProps m_smoothProps;
        AzFramework::CameraSystem m_cameraSystem;
    };

    using ModernViewportCameraController = AzFramework::MultiViewportController<ModernViewportCameraControllerInstance>;
} // namespace SandboxEditor
