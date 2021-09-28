/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Public/ViewportContext.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/ViewportContextBus.h>
#include <Atom/RPI.Public/ViewportContextManager.h>
#include <Atom/RPI.Public/View.h>

namespace AZ
{
    namespace RPI
    {
        ViewportContext::ViewportContext(ViewportContextManager* manager, AzFramework::ViewportId id, const AZ::Name& name, RHI::Device& device, AzFramework::NativeWindowHandle nativeWindow, ScenePtr renderScene)
            : m_rootScene(nullptr)
            , m_id(id)
            , m_windowContext(AZStd::make_shared<WindowContext>())
            , m_manager(manager)
            , m_name(name)
            , m_viewportSize(1, 1)
        {
            m_windowContext->Initialize(device, nativeWindow);
            AzFramework::WindowRequestBus::Event(nativeWindow, [this](AzFramework::WindowRequestBus::Events* window)
            {
                m_viewportSize = window->GetClientAreaSize();
                m_viewportDpiScaleFactor = window->GetDpiScaleFactor();
                m_vsyncInterval = window->GetSyncInterval();
                m_refreshRate = window->GetDisplayRefreshRate();
            });
            AzFramework::WindowNotificationBus::Handler::BusConnect(nativeWindow);
            AzFramework::ViewportRequestBus::Handler::BusConnect(id);

            m_onProjectionMatrixChangedHandler = ViewportContext::MatrixChangedEvent::Handler([this](const AZ::Matrix4x4& matrix)
            {
                m_projectionMatrixChangedEvent.Signal(matrix);
            });

            m_onViewMatrixChangedHandler = ViewportContext::MatrixChangedEvent::Handler([this](const AZ::Matrix4x4& matrix)
            {
                m_viewMatrixChangedEvent.Signal(matrix);
            });

            m_prepareFrameHandler = RenderPipeline::FrameNotificationEvent::Handler(
                [this]()
                {
                    ViewportContextNotificationBus::Event(GetName(), &ViewportContextNotificationBus::Events::OnRenderTick);
                    ViewportContextIdNotificationBus::Event(GetId(), &ViewportContextIdNotificationBus::Events::OnRenderTick);
                });

            m_endFrameHandler = RenderPipeline::FrameNotificationEvent::Handler(
                [this]()
                {
                    ViewportContextNotificationBus::Event(GetName(), &ViewportContextNotificationBus::Events::OnFrameEnd);
                    ViewportContextIdNotificationBus::Event(GetId(), &ViewportContextIdNotificationBus::Events::OnFrameEnd);
                });

            SetRenderScene(renderScene);
        }

        ViewportContext::~ViewportContext()
        {
            m_aboutToBeDestroyedEvent.Signal(m_id);

            AzFramework::WindowNotificationBus::Handler::BusDisconnect();
            AzFramework::ViewportRequestBus::Handler::BusDisconnect();

            if (m_currentPipeline)
            {
                m_currentPipeline->RemoveFromRenderTick();
                m_currentPipeline->RemoveFromScene();
            }
            m_manager->UnregisterViewportContext(m_id);
            m_windowContext->Shutdown();
        }

        AzFramework::ViewportId ViewportContext::GetId() const
        {
            return m_id;
        }

        AzFramework::NativeWindowHandle ViewportContext::GetWindowHandle() const
        {
            return m_windowContext->GetWindowHandle();
        }

        WindowContextSharedPtr ViewportContext::GetWindowContext()
        {
            return m_windowContext;
        }

        ScenePtr ViewportContext::GetRenderScene()
        {
            return m_rootScene;
        }

        void ViewportContext::SetRenderScene(ScenePtr scene)
        {
            if (m_rootScene != scene)
            {
                if (m_rootScene)
                {
                    SceneNotificationBus::Handler::BusDisconnect(m_rootScene->GetId());
                }
                else
                {
                    // If the scene was empty, we should save the default view from this scene as default view for the context.
                    auto renderPipeline = scene->FindRenderPipelineForWindow(m_windowContext->GetWindowHandle());
                    if (renderPipeline)
                    {
                        if (AZ::RPI::ViewPtr pipelineView = renderPipeline->GetDefaultView(); pipelineView)
                        {
                            SetDefaultView(pipelineView);
                        }
                    }
                }

                m_rootScene = scene;
                if (m_rootScene)
                {
                    SceneNotificationBus::Handler::BusConnect(m_rootScene->GetId());
                }
                ResetCurrentPipeline();
                UpdatePipelineView();
                UpdatePipelineRefreshRate();
            }

            m_sceneChangedEvent.Signal(scene);
        }

        float ViewportContext::GetFpsLimit() const
        {
            return m_fpsLimit;
        }

        void ViewportContext::SetFpsLimit(float fpsLimit)
        {
            m_fpsLimit = fpsLimit;
            UpdatePipelineRefreshRate();
        }

        float ViewportContext::GetTargetFrameRate() const
        {
            float targetFrameRate = GetFpsLimit();
            const AZ::u32 vsyncInterval = GetVsyncInterval();
            if (vsyncInterval != 0)
            {
                const float vsyncFrameRate = static_cast<float>(GetRefreshRate()) / static_cast<float>(vsyncInterval);
                if (targetFrameRate == 0.f || vsyncFrameRate < targetFrameRate)
                {
                    targetFrameRate = vsyncFrameRate;
                }
            }
            return targetFrameRate;
        }

        AZ::Name ViewportContext::GetName() const
        {
            return m_name;
        }

        ViewPtr ViewportContext::GetDefaultView()
        {
            return m_defaultView;
        }

        ConstViewPtr ViewportContext::GetDefaultView() const
        {
            return m_defaultView;
        }

        AzFramework::WindowSize ViewportContext::GetViewportSize() const
        {
            return m_viewportSize;
        }

        float ViewportContext::GetDpiScalingFactor() const
        {
            return m_viewportDpiScaleFactor;
        }

        uint32_t ViewportContext::GetVsyncInterval() const
        {
            return m_vsyncInterval;
        }

        uint32_t ViewportContext::GetRefreshRate() const
        {
            return m_refreshRate;
        }

        void ViewportContext::ConnectSizeChangedHandler(SizeChangedEvent::Handler& handler)
        {
            handler.Connect(m_sizeChangedEvent);
        }

        void ViewportContext::ConnectDpiScalingFactorChangedHandler(ScalarChangedEvent::Handler& handler)
        {
            handler.Connect(m_dpiScalingFactorChangedEvent);
        }

        void ViewportContext::ConnectVsyncIntervalChangedHandler(UintChangedEvent::Handler& handler)
        {
            handler.Connect(m_vsyncIntervalChangedEvent);
        }

        void ViewportContext::ConnectRefreshRateChangedHandler(UintChangedEvent::Handler& handler)
        {
            handler.Connect(m_refreshRateChangedEvent);
        }

        void ViewportContext::ConnectViewMatrixChangedHandler(MatrixChangedEvent::Handler& handler)
        {
            handler.Connect(m_viewMatrixChangedEvent);
        }

        void ViewportContext::ConnectProjectionMatrixChangedHandler(MatrixChangedEvent::Handler& handler)
        {
            handler.Connect(m_projectionMatrixChangedEvent);
        }

        void ViewportContext::ConnectSceneChangedHandler(SceneChangedEvent::Handler& handler)
        {
            handler.Connect(m_sceneChangedEvent);
        }

        void ViewportContext::ConnectCurrentPipelineChangedHandler(PipelineChangedEvent::Handler& handler)
        {
            handler.Connect(m_currentPipelineChangedEvent);
        }

        void ViewportContext::ConnectDefaultViewChangedHandler(ViewChangedEvent::Handler& handler)
        {
            handler.Connect(m_defaultViewChangedEvent);
        }

        void ViewportContext::ConnectAboutToBeDestroyedHandler(ViewportIdEvent::Handler& handler)
        {
            handler.Connect(m_aboutToBeDestroyedEvent);
        }

        const AZ::Matrix4x4& ViewportContext::GetCameraViewMatrix() const
        {
            return GetDefaultView()->GetWorldToViewMatrix();
        }

        void ViewportContext::SetCameraViewMatrix(const AZ::Matrix4x4& matrix)
        {
            GetDefaultView()->SetWorldToViewMatrix(matrix);
            m_viewMatrixChangedEvent.Signal(matrix);
        }

        const AZ::Matrix4x4& ViewportContext::GetCameraProjectionMatrix() const
        {
            return GetDefaultView()->GetViewToClipMatrix();
        }

        void ViewportContext::SetCameraProjectionMatrix(const AZ::Matrix4x4& matrix)
        {
            GetDefaultView()->SetViewToClipMatrix(matrix);
        }

        AZ::Transform ViewportContext::GetCameraTransform() const
        {
            return GetDefaultView()->GetCameraTransform();
        }

        void ViewportContext::SetCameraTransform(const AZ::Transform& transform)
        {
            const auto view = GetDefaultView();
            view->SetCameraTransform(AZ::Matrix3x4::CreateFromTransform(transform.GetOrthogonalized()));
            m_viewMatrixChangedEvent.Signal(view->GetWorldToViewMatrix());
        }

        void ViewportContext::SetDefaultView(ViewPtr view)
        {
            if (m_defaultView != view)
            {
                m_onProjectionMatrixChangedHandler.Disconnect();
                m_onViewMatrixChangedHandler.Disconnect();

                m_defaultView = view;
                UpdatePipelineView();

                m_defaultViewChangedEvent.Signal(view);
                m_viewMatrixChangedEvent.Signal(view->GetWorldToViewMatrix());
                m_projectionMatrixChangedEvent.Signal(view->GetViewToClipMatrix());

                view->ConnectWorldToViewMatrixChangedHandler(m_onViewMatrixChangedHandler);
                view->ConnectWorldToClipMatrixChangedHandler(m_onProjectionMatrixChangedHandler);
            }
        }

        void ViewportContext::UpdatePipelineView()
        {
            if (!m_defaultView || !m_rootScene)
            {
                return;
            }

            if (!m_currentPipeline)
            {
                m_currentPipeline = m_rootScene ? m_rootScene->FindRenderPipelineForWindow(m_windowContext->GetWindowHandle()) : nullptr;
                m_currentPipelineChangedEvent.Signal(m_currentPipeline);
            }

            if (m_currentPipeline)
            {
                if (!m_prepareFrameHandler.IsConnected())
                {
                    m_currentPipeline->ConnectPrepareFrameHandler(m_prepareFrameHandler);
                    m_currentPipeline->ConnectEndFrameHandler(m_endFrameHandler);
                }
                m_currentPipeline->SetDefaultView(m_defaultView);
            }
        }

        void ViewportContext::UpdatePipelineRefreshRate()
        {
            if (!m_currentPipeline)
            {
                return;
            }

            const float refreshRate = GetTargetFrameRate();
            // If we have a truly unlimited framerate, just render every tick
            if (refreshRate == 0.f)
            {
                m_currentPipeline->AddToRenderTick();
            }
            else
            {
                m_currentPipeline->AddToRenderTickAtInterval(AZStd::chrono::duration<float>(1.f / refreshRate));
            }
        }

        void ViewportContext::ResetCurrentPipeline()
        {
            m_prepareFrameHandler.Disconnect();
            m_endFrameHandler.Disconnect();
            m_currentPipeline.reset();
        }

        RenderPipelinePtr ViewportContext::GetCurrentPipeline()
        {
            return m_currentPipeline;
        }

        void ViewportContext::OnRenderPipelineAdded([[maybe_unused]]RenderPipelinePtr pipeline)
        {
            // If the pipeline is registered to our window, reset our current pipeline and do a lookup
            // Currently, Scene just stores pipelines sequentially in a vector, but we'll attempt to be safe
            // in the event prioritization is added later
            if (pipeline->GetWindowHandle() == m_windowContext->GetWindowHandle())
            {
                ResetCurrentPipeline();
                UpdatePipelineView();
                UpdatePipelineRefreshRate();
            }
        }

        void ViewportContext::OnRenderPipelineRemoved([[maybe_unused]]RenderPipeline* pipeline)
        {
            if (m_currentPipeline.get() == pipeline)
            {
                ResetCurrentPipeline();
                UpdatePipelineView();
                UpdatePipelineRefreshRate();
            }
        }

        void ViewportContext::OnWindowResized(uint32_t width, uint32_t height)
        {
            if (m_viewportSize.m_width != width || m_viewportSize.m_height != height)
            {
                m_viewportSize.m_width = width;
                m_viewportSize.m_height = height;
                m_sizeChangedEvent.Signal(m_viewportSize);
            }
        }

        void ViewportContext::OnRefreshRateChanged(uint32_t refreshRate)
        {
            if (m_refreshRate != refreshRate)
            {
                m_refreshRate = refreshRate;
                m_refreshRateChangedEvent.Signal(m_refreshRate);
                UpdatePipelineRefreshRate();
            }
        }

        void ViewportContext::OnDpiScaleFactorChanged(float dpiScaleFactor)
        {
            m_viewportDpiScaleFactor = dpiScaleFactor;
            m_dpiScalingFactorChangedEvent.Signal(dpiScaleFactor);
        }

        void ViewportContext::OnVsyncIntervalChanged(uint32_t interval)
        {
            if (m_vsyncInterval != interval)
            {
                m_vsyncInterval = interval;
                m_vsyncIntervalChangedEvent.Signal(m_vsyncInterval);
                UpdatePipelineRefreshRate();
            }
        }
    } // namespace RPI
} // namespace AZ
