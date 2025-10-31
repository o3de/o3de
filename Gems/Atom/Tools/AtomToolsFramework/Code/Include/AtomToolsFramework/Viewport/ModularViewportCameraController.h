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
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzFramework/Viewport/CameraInput.h>
#include <AzFramework/Viewport/MultiViewportController.h>
#include <AzToolsFramework/Viewport/ViewportMessages.h>

namespace AtomToolsFramework
{
    class ModularViewportCameraControllerInstance;

    //! A reduced ViewportContext interface for use by the ModularViewportCameraController.
    //! @note This extra indirection is used to facilitate testing the ModularViewportCameraController.
    class ModularCameraViewportContext
    {
    public:
        virtual ~ModularCameraViewportContext() = default;

        virtual AZ::Transform GetCameraTransform() const = 0;
        virtual void SetCameraTransform(const AZ::Transform& transform) = 0;
        virtual void ConnectViewMatrixChangedHandler(AZ::RPI::MatrixChangedEvent::Handler& handler) = 0;
    };

    //! A function object to represent returning a camera controller priority.
    using CameraControllerPriorityFn =
        AZStd::function<AzFramework::ViewportControllerPriority(const AzFramework::CameraSystem& cameraSystem)>;
    using CameraViewportContextFn = AZStd::function<AZStd::unique_ptr<ModularCameraViewportContext>(AzFramework::ViewportId)>;

    //! The default behavior for what priority the camera controller should respond to events at.
    //! @note This can change based on the state of the camera controller/system.
    AzFramework::ViewportControllerPriority DefaultCameraControllerPriority(const AzFramework::CameraSystem& cameraSystem);

    //! Builder class to create and configure a ModularViewportCameraControllerInstance.
    class ModularViewportCameraController
        : public AzFramework::MultiViewportController<
              ModularViewportCameraControllerInstance,
              AzFramework::ViewportControllerPriority::DispatchToAllPriorities>
    {
    public:
        friend ModularViewportCameraControllerInstance;

        using CameraListBuilder = AZStd::function<void(AzFramework::Cameras&)>;
        using CameraPropsBuilder = AZStd::function<void(AzFramework::CameraProps&)>;
        using CameraPriorityBuilder = AZStd::function<void(CameraControllerPriorityFn&)>;
        using CameraViewportContextBuilder = AZStd::function<void(AZStd::unique_ptr<ModularCameraViewportContext>&)>;

        //! Sets the camera list builder callback used to populate new ModularViewportCameraControllerInstances.
        void SetCameraListBuilderCallback(const CameraListBuilder& builder);
        //! Sets the camera props builder callback used to populate new ModularViewportCameraControllerInstances.
        void SetCameraPropsBuilderCallback(const CameraPropsBuilder& builder);
        //! Sets the camera controller priority builder callback used to populate new ModularViewportCameraControllerInstances.
        void SetCameraPriorityBuilderCallback(const CameraPriorityBuilder& builder);
        //! Sets the camera controller viewport context builder callback to populate new ModularViewportCameraControllerInstances.
        void SetCameraViewportContextBuilderCallback(const CameraViewportContextBuilder& builder);

    private:
        //! Sets up a camera list based on this controller's CameraListBuilderCallback.
        void SetupCameras(AzFramework::Cameras& cameras);
        //! Sets up properties shared across all cameras.
        void SetupCameraProperties(AzFramework::CameraProps& cameraProps);
        //! Sets up how the camera controller should decide at what priority level to respond to.
        void SetupCameraControllerPriority(CameraControllerPriorityFn& cameraPriorityFn);
        //! Sets up what viewport context should be used by the camera controller.
        void SetupCameraControllerViewportContext(AZStd::unique_ptr<ModularCameraViewportContext>& cameraViewportContext);

        //! Builder to generate a list of CameraInputs to run in the ModularViewportCameraControllerInstance.
        CameraListBuilder m_cameraListBuilder;
        //! Builder to define custom camera properties to use for things such as rotate and translate interpolation.
        CameraPropsBuilder m_cameraPropsBuilder;
        //! Builder to define what priority level the camera controller should respond to events at.
        CameraPriorityBuilder m_cameraControllerPriorityBuilder;
        //! Builder to define what viewport context interface the camera controller should use.
        CameraViewportContextBuilder m_cameraViewportContextBuilder;
    };

    //! The production modular camera viewport context backed by an AZ::RPI::ViewportContextPtr.
    //! @note This is instantiated during normal runtime use.
    class ModularCameraViewportContextImpl : public ModularCameraViewportContext
    {
    public:
        explicit ModularCameraViewportContextImpl(AzFramework::ViewportId viewportId);

        // ModularCameraViewportContext overrides ...
        AZ::Transform GetCameraTransform() const override;
        void SetCameraTransform(const AZ::Transform& transform) override;
        void ConnectViewMatrixChangedHandler(AZ::RPI::MatrixChangedEvent::Handler& handler) override;

    private:
        AzFramework::ViewportId m_viewportId;
    };

    //! A customizable camera controller that can be configured to run a varying set of CameraInput instances.
    //! The controller can also be animated from its current transform to a new translation and orientation.
    class ModularViewportCameraControllerInstance final
        : public AzFramework::MultiViewportControllerInstanceInterface<ModularViewportCameraController>
        , public ModularViewportCameraControllerRequestBus::Handler
        , public AzToolsFramework::ViewportInteraction::ViewportInteractionNotificationBus::Handler
    {
    public:
        explicit ModularViewportCameraControllerInstance(AzFramework::ViewportId viewportId, ModularViewportCameraController* controller);
        ~ModularViewportCameraControllerInstance() override;

        // MultiViewportControllerInstanceInterface overrides ...
        bool HandleInputChannelEvent(const AzFramework::ViewportControllerInputEvent& event) override;
        void UpdateViewport(const AzFramework::ViewportControllerUpdateEvent& event) override;

        // ModularViewportCameraControllerRequestBus overrides ...
        bool InterpolateToTransform(const AZ::Transform& worldFromLocal, float duration) override;
        bool IsInterpolating() const override;
        void StartTrackingTransform(const AZ::Transform& worldFromLocal) override;
        void StopTrackingTransform() override;
        bool IsTrackingTransform() const override;
        void SetCameraPivotAttached(const AZ::Vector3& pivot) override;
        void SetCameraPivotAttachedImmediate(const AZ::Vector3& pivot) override;
        void SetCameraPivotDetached(const AZ::Vector3& pivot) override;
        void SetCameraPivotDetachedImmediate(const AZ::Vector3& pivot) override;
        void SetCameraOffset(const AZ::Vector3& offset) override;
        void SetCameraOffsetImmediate(const AZ::Vector3& offset) override;
        void LookFromOrbit() override;
        bool AddCameras(const AZStd::vector<AZStd::shared_ptr<AzFramework::CameraInput>>& cameraInputs) override;
        bool RemoveCameras(const AZStd::vector<AZStd::shared_ptr<AzFramework::CameraInput>>& cameraInputs) override;
        void ResetCameras() override;

    private:
        // ViewportInteractionNotificationBus overrides ...
        void OnViewportFocusOut() override;

        //! Combine the current camera transform with any potential roll from the tracked
        //! transform (this is usually zero).
        AZ::Transform CombinedCameraTransform() const;

        //! Reconnect the current view matrix change handler after the viewport context view group has changed.
        //! @note: This happens after switching to track a different camera/viewport transform.
        void ReconnectViewMatrixChangeHandler();

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
            AZ::Transform m_transformStart;
            AZ::Transform m_transformEnd; //!< The transform of the camera at the end of the animation.
            float m_time = 0.0f; //!< The interpolation amount between the start and end transforms (in the range 0.0 - 1.0).
            float m_duration = 1.0f; //!< The length of the animation (how long it takes to complete) in seconds.
        };

        AzFramework::Camera m_camera; //!< The current camera state (pitch/yaw/position/look-distance).
        AzFramework::Camera m_targetCamera; //!< The target (next) camera state that m_camera is catching up to.
        AZStd::optional<AzFramework::Camera> m_storedCamera; //!< A potentially stored camera for when a transform is being tracked.
        AzFramework::CameraSystem m_cameraSystem; //!< The camera system responsible for managing all CameraInputs.
        AzFramework::CameraProps m_cameraProps; //!< Camera properties to control rotate and translate smoothness.
        CameraControllerPriorityFn m_priorityFn; //!< Controls at what priority the camera controller should respond to events.

        AZStd::optional<CameraAnimation> m_cameraAnimation; //!< Camera animation state (used during CameraMode::Animation).
        CameraMode m_cameraMode = CameraMode::Control; //!< The current mode the camera is operating in.
        float m_roll = 0.0f; //!< The current amount of roll to be applied to the camera.
        float m_targetRoll = 0.0f; //!< The target amount of roll to be applied to the camera (current will move towards this).
        //! Listen for camera view changes outside of the camera controller.
        AZ::RPI::MatrixChangedEvent::Handler m_cameraViewMatrixChangeHandler;
        //! The current instance of the modular camera viewport context.
        AZStd::unique_ptr<ModularCameraViewportContext> m_modularCameraViewportContext;
        //! Flag to prevent circular updates of the camera transform (while the viewport transform is being updated internally).
        bool m_updatingTransformInternally = false;
    };

    //! Placeholder implementation for ModularCameraViewportContext (useful for verifying the interface).
    class PlaceholderModularCameraViewportContextImpl : public AtomToolsFramework::ModularCameraViewportContext
    {
    public:
        AZ::Transform GetCameraTransform() const override;
        void SetCameraTransform(const AZ::Transform& transform) override;
        void ConnectViewMatrixChangedHandler(AZ::RPI::MatrixChangedEvent::Handler& handler) override;

    private:
        AZ::Transform m_cameraTransform = AZ::Transform::CreateIdentity();
        AZ::RPI::MatrixChangedEvent m_viewMatrixChangedEvent;
    };
} // namespace AtomToolsFramework
