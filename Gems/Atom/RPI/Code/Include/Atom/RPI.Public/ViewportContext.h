/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/Viewport/ViewportBus.h>
#include <AzFramework/Windowing/WindowBus.h>
#include <Atom/RPI.Public/WindowContext.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/SceneBus.h>

namespace AZ
{
    namespace RPI
    {
        class ViewportContextManager;

        //! ViewportContext wraps a native window and represents a minimal viewport
        //! in which a scene is rendered on-screen.
        //! ViewportContexts are registered on creation to allow consumers to listen to notifications
        //! and manage the view stack for a given viewport.
        class ViewportContext
            : public SceneNotificationBus::Handler
            , public AzFramework::WindowNotificationBus::Handler
            , public AzFramework::ViewportRequestBus::Handler
        {
        public:
            AZ_CLASS_ALLOCATOR(ViewportContext, AZ::SystemAllocator, 0);

            //! Used by ViewportContextManager, use AZ::Interface<ViewportContextRequestsInterface>::Get()->CreateViewportContext to create a viewport
            //! context from outside of the ViewportContextManager.
            ViewportContext(ViewportContextManager* manager, AzFramework::ViewportId id, const AZ::Name& name, RHI::Device& device, AzFramework::NativeWindowHandle nativeWindow, ScenePtr renderScene);
            ~ViewportContext();

            //! Gets an opaque ID that can be used to uniquely identify this ViewportContext.
            AzFramework::ViewportId GetId() const;
            //! Convenience method, gets the window handle associated with this viewport's window manager.
            AzFramework::NativeWindowHandle GetWindowHandle() const;
            //! Gets the window context associated with this viewport.
            WindowContextSharedPtr GetWindowContext();
            //! Gets the root scene (if any) associated with this viewport.
            ScenePtr GetRenderScene();
            //! Gets the current render pipeline associated with our WindowContext, if there is one.
            RenderPipelinePtr GetCurrentPipeline();
            //! Sets the root scene associated with this viewport.
            //! This does not provide a default render pipeline, one must be provided to enable rendering.
            void SetRenderScene(ScenePtr scene);
            //! Runs one simulation and render tick and renders a frame to this viewport's window.
            //! @note This is likely to be replaced by a tick management system in the RPI.
            void RenderTick();

            //! Gets the current name of this ViewportContext.
            //! This name is used to tie this ViewportContext to its View stack, and ViewportContexts may be
            //! renamed via AZ::RPI::ViewportContextRequests::Get()->RenameViewportContext(...).
            AZ::Name GetName() const;

            //! Gets the default view associated with this ViewportContext.
            //! Alternatively, use  AZ::RPI::ViewportContextRequests::Get()->GetCurrentView().
            ViewPtr GetDefaultView();
            ConstViewPtr GetDefaultView() const;

            //! Gets the current size of the viewport.
            //! This value is cached and updated on-demand, so may be efficiently queried.
            AzFramework::WindowSize GetViewportSize() const;

            //! Gets the screen DPI scaling factor.
            //! This value is cached and updated on-demand, so may be efficiently queried.
            //! \see AzFramework::WindowRequests::GetDpiScaleFactor
            float GetDpiScalingFactor() const;

            // SceneNotificationBus interface overrides...
            //! Ensures our default view remains set when our scene's render pipelines are modified.
            void OnRenderPipelineAdded(RenderPipelinePtr pipeline) override;
            //! Ensures our default view remains set when our scene's render pipelines are modified.
            void OnRenderPipelineRemoved(RenderPipeline* pipeline) override;
            //! OnBeginPrepareRender is forwarded to our RenderTick notification to allow subscribers to do rendering.
            void OnBeginPrepareRender() override;

            // WindowNotificationBus interface overrides...
            //! Used to fire a notification when our window resizes.
            void OnWindowResized(uint32_t width, uint32_t height) override;
            //! Used to fire a notification when our window DPI changes.
            void OnDpiScaleFactorChanged(float dpiScaleFactor) override;

            using SizeChangedEvent = AZ::Event<AzFramework::WindowSize>;
            //! Notifies consumers when the viewport size has changed.
            //! Alternatively, connect to ViewportContextNotificationsBus and listen to ViewportContextNotifications::OnViewportSizeChanged.
            void ConnectSizeChangedHandler(SizeChangedEvent::Handler& handler);

            using ScalarChangedEvent = AZ::Event<float>;
            //! Notifies consumers when the viewport DPI scaling ratio has changed.
            //! Alternatively, connect to ViewportContextNotificationsBus and listen to ViewportContextNotifications::OnViewportDpiScalingChanged.
            void ConnectDpiScalingFactorChangedHandler(ScalarChangedEvent::Handler& handler);

            //! Notifies consumers when the view matrix has changed.
            void ConnectViewMatrixChangedHandler(MatrixChangedEvent::Handler& handler);
            //! Notifies consumers when the projection matrix has changed.
            void ConnectProjectionMatrixChangedHandler(MatrixChangedEvent::Handler& handler);

            using SceneChangedEvent = AZ::Event<ScenePtr>;
            //! Notifies consumers when the render scene has changed.
            void ConnectSceneChangedHandler(SceneChangedEvent::Handler& handler);

            using PipelineChangedEvent = AZ::Event<RenderPipelinePtr>;
            //! Notifies consumers when the current pipeline associated with our window has changed.
            void ConnectCurrentPipelineChangedHandler(PipelineChangedEvent::Handler& handler);

            using ViewChangedEvent = AZ::Event<ViewPtr>;
            //! Notifies consumers when the default view has changed.
            void ConnectDefaultViewChangedHandler(ViewChangedEvent::Handler& handler);

            using ViewportIdEvent = AZ::Event<AzFramework::ViewportId>;
            //! Notifies consumers when this ViewportContext is about to be destroyed.
            void ConnectAboutToBeDestroyedHandler(ViewportIdEvent::Handler& handler);

            // ViewportRequestBus interface overrides...
            const AZ::Matrix4x4& GetCameraViewMatrix() const override;
            AZ::Matrix3x4 GetCameraViewMatrixAsMatrix3x4() const override;
            void SetCameraViewMatrix(const AZ::Matrix4x4& matrix) override;
            const AZ::Matrix4x4& GetCameraProjectionMatrix() const override;
            void SetCameraProjectionMatrix(const AZ::Matrix4x4& matrix) override;
            AZ::Transform GetCameraTransform() const override;
            void SetCameraTransform(const AZ::Transform& transform) override;

        private:
            // Used by the manager to set the current default camera.
            void SetDefaultView(ViewPtr view);
            // Ensures our render pipeline's default camera matches ours.
            void UpdatePipelineView();

            ScenePtr m_rootScene;
            WindowContextSharedPtr m_windowContext;
            ViewPtr m_defaultView;
            AzFramework::WindowSize m_viewportSize;
            float m_viewportDpiScaleFactor = 1.0f;

            SizeChangedEvent m_sizeChangedEvent;
            ScalarChangedEvent m_dpiScalingFactorChangedEvent;
            MatrixChangedEvent m_viewMatrixChangedEvent;
            MatrixChangedEvent::Handler m_onViewMatrixChangedHandler;
            MatrixChangedEvent m_projectionMatrixChangedEvent;
            MatrixChangedEvent::Handler m_onProjectionMatrixChangedHandler;
            SceneChangedEvent m_sceneChangedEvent;
            PipelineChangedEvent m_currentPipelineChangedEvent;
            ViewChangedEvent m_defaultViewChangedEvent;
            ViewportIdEvent m_aboutToBeDestroyedEvent;

            ViewportContextManager* m_manager;
            RenderPipelinePtr m_currentPipeline;
            Name m_name;
            AzFramework::ViewportId m_id;

            friend class ViewportContextManager;
        };
    } // namespace RPI
} // namespace AZ
