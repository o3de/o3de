/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RPI.Public/ViewportContext.h>
#include <AtomToolsFramework/Viewport/ModularViewportCameraControllerRequestBus.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzFramework/Viewport/CameraInput.h>
#include <AzFramework/Viewport/MultiViewportController.h>

namespace AtomToolsFramework
{
    class ModernViewportCameraControllerInstance;
    class ModularViewportCameraController
        : public AzFramework::MultiViewportController<
              ModernViewportCameraControllerInstance, AzFramework::ViewportControllerPriority::DispatchToAllPriorities>
    {
    public:
        using CameraListBuilder = AZStd::function<void(AzFramework::Cameras&)>;
        using CameraPropsBuilder = AZStd::function<void(AzFramework::CameraProps&)>;

        //! Sets the camera list builder callback used to populate new ModernViewportCameraControllerInstances
        void SetCameraListBuilderCallback(const CameraListBuilder& builder);
        //! Sets the camera props builder callback used to populate new ModernViewportCameraControllerInstances
        void SetCameraPropsBuilderCallback(const CameraPropsBuilder& builder);
        //! Sets up a camera list based on this controller's CameraListBuilderCallback
        void SetupCameras(AzFramework::Cameras& cameras);
        //! Sets up properties shared across all cameras
        void SetupCameraProperies(AzFramework::CameraProps& cameraProps);

    private:
        CameraListBuilder m_cameraListBuilder;
        CameraPropsBuilder m_cameraPropsBuilder;
    };

    class ModernViewportCameraControllerInstance final
        : public AzFramework::MultiViewportControllerInstanceInterface<ModularViewportCameraController>,
          public ModularViewportCameraControllerRequestBus::Handler,
          private AzFramework::ViewportDebugDisplayEventBus::Handler
    {
    public:
        explicit ModernViewportCameraControllerInstance(AzFramework::ViewportId viewportId, ModularViewportCameraController* controller);
        ~ModernViewportCameraControllerInstance() override;

        // MultiViewportControllerInstanceInterface overrides ...
        bool HandleInputChannelEvent(const AzFramework::ViewportControllerInputEvent& event) override;
        void UpdateViewport(const AzFramework::ViewportControllerUpdateEvent& event) override;

        // ModularViewportCameraControllerRequestBus overrides ...
        void InterpolateToTransform(const AZ::Transform& worldFromLocal, float lookAtDistance) override;
        AZStd::optional<AZ::Vector3> LookAtAfterInterpolation() const override;

    private:
        // AzFramework::ViewportDebugDisplayEventBus overrides ...
        void DisplayViewport(const AzFramework::ViewportInfo& viewportInfo, AzFramework::DebugDisplayRequests& debugDisplay) override;

        enum class CameraMode
        {
            Control,
            Animation
        };

        AzFramework::Camera m_camera;
        AzFramework::Camera m_targetCamera;
        AzFramework::CameraSystem m_cameraSystem;
        AzFramework::CameraProps m_cameraProps;

        AZ::Transform m_transformStart = AZ::Transform::CreateIdentity();
        AZ::Transform m_transformEnd = AZ::Transform::CreateIdentity();
        float m_animationT = 0.0f;
        CameraMode m_cameraMode = CameraMode::Control;
        AZStd::optional<AZ::Vector3> m_lookAtAfterInterpolation; //!< The look at point after an interpolation has finished.
                                                                 //!< Will be cleared when the view changes (camera looks away).
        bool m_updatingTransform = false;

        AZ::RPI::ViewportContext::MatrixChangedEvent::Handler m_cameraViewMatrixChangeHandler;
    };
} // namespace AtomToolsFramework
