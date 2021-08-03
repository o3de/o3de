/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
    class ModularViewportCameraControllerInstance;

    //! Builder class to create and configure a ModularViewportCameraControllerInstance.
    class ModularViewportCameraController
        : public AzFramework::MultiViewportController<
              ModularViewportCameraControllerInstance,
              AzFramework::ViewportControllerPriority::DispatchToAllPriorities>
    {
    public:
        using CameraListBuilder = AZStd::function<void(AzFramework::Cameras&)>;
        using CameraPropsBuilder = AZStd::function<void(AzFramework::CameraProps&)>;

        //! Sets the camera list builder callback used to populate new ModularViewportCameraControllerInstances
        void SetCameraListBuilderCallback(const CameraListBuilder& builder);
        //! Sets the camera props builder callback used to populate new ModularViewportCameraControllerInstances
        void SetCameraPropsBuilderCallback(const CameraPropsBuilder& builder);
        //! Sets up a camera list based on this controller's CameraListBuilderCallback
        void SetupCameras(AzFramework::Cameras& cameras);
        //! Sets up properties shared across all cameras
        void SetupCameraProperies(AzFramework::CameraProps& cameraProps);

    private:
        //! Builder to generate a list of CameraInputs to run in the ModularViewportCameraControllerInstance.
        CameraListBuilder m_cameraListBuilder;
        CameraPropsBuilder m_cameraPropsBuilder; //!< Builder to define custom camera properties to use for things such as rotate and
                                                 //!< translate interpolation.
    };

    //! A customizable camera controller that can be configured to run a varying set of CameraInput instances.
    //! The controller can also be animated from its current transform to a new translation and orientation.
    class ModularViewportCameraControllerInstance final
        : public AzFramework::MultiViewportControllerInstanceInterface<ModularViewportCameraController>
        , public ModularViewportCameraControllerRequestBus::Handler
        , private AzFramework::ViewportDebugDisplayEventBus::Handler
    {
    public:
        explicit ModularViewportCameraControllerInstance(AzFramework::ViewportId viewportId, ModularViewportCameraController* controller);
        ~ModularViewportCameraControllerInstance() override;

        // MultiViewportControllerInstanceInterface overrides ...
        bool HandleInputChannelEvent(const AzFramework::ViewportControllerInputEvent& event) override;
        void UpdateViewport(const AzFramework::ViewportControllerUpdateEvent& event) override;

        // ModularViewportCameraControllerRequestBus overrides ...
        void InterpolateToTransform(const AZ::Transform& worldFromLocal, float lookAtDistance) override;
        AZStd::optional<AZ::Vector3> LookAtAfterInterpolation() const override;

    private:
        // AzFramework::ViewportDebugDisplayEventBus overrides ...
        void DisplayViewport(const AzFramework::ViewportInfo& viewportInfo, AzFramework::DebugDisplayRequests& debugDisplay) override;

        //! The current mode the camera controller is in.
        enum class CameraMode
        {
            Control, //!< The camera is being driven by user input.
            Animation //!< The camera is being animated (interpolated) from one transform to another.
        };

        //! Encapsulates an animation (interpolation) between two transforms.
        struct CameraAnimation
        {
            //! The transform of the camera at the start of the animation.
            AZ::Transform m_transformStart = AZ::Transform::CreateIdentity();
            AZ::Transform m_transformEnd = AZ::Transform::CreateIdentity(); //!< The transform of the camera at the end of the animation.
            float m_time = 0.0f; //!< The interpolation amount between the start and end transforms (in the range 0.0 - 1.0).
        };

        AzFramework::Camera m_camera; //!< The current camera state (pitch/yaw/position/look-distance).
        AzFramework::Camera m_targetCamera; //!< The target (next) camera state that m_camera is catching up to.
        AzFramework::CameraSystem m_cameraSystem; //!< The camera system responsible for managing all CameraInputs.
        AzFramework::CameraProps m_cameraProps; //!< Camera properties to control rotate and translate smoothness.

        CameraAnimation m_cameraAnimation; //!< Camera animation state (used during CameraMode::Animation).
        CameraMode m_cameraMode = CameraMode::Control; //!< The current mode the camera is operating in.
        AZStd::optional<AZ::Vector3> m_lookAtAfterInterpolation; //!< The look at point after an interpolation has finished.
                                                                 //!< Will be cleared when the view changes (camera looks away).
        //! Flag to prevent circular updates of the camera transform (while the viewport transform is being updated internally).
        bool m_updatingTransformInternally = false;
        //! Listen for camera view changes outside of the camera controller.
        AZ::RPI::ViewportContext::MatrixChangedEvent::Handler m_cameraViewMatrixChangeHandler;
    };
} // namespace AtomToolsFramework
