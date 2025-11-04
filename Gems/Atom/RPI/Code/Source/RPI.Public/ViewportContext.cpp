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
            AzFramework::WindowRequestBus::EventResult(
                m_viewportSize,
                nativeWindow,
                &AzFramework::WindowRequestBus::Events::GetRenderResolution);
            AzFramework::WindowRequestBus::EventResult(
                m_viewportDpiScaleFactor,
                nativeWindow,
                &AzFramework::WindowRequestBus::Events::GetDpiScaleFactor);
            AzFramework::WindowNotificationBus::Handler::BusConnect(nativeWindow);
            AzFramework::ViewportRequestBus::Handler::BusConnect(id);

            // Clamp the viewport size to a minimum of (1, 1). Otherwise, it's very easy for consumers of this API to miss
            // that they need to guard against (0, 0) when the viewport gets hidden.
            m_viewportSize.m_height = AZStd::max(m_viewportSize.m_height, 1u);
            m_viewportSize.m_width = AZStd::max(m_viewportSize.m_width, 1u);

            m_currentPipelines.resize(MaxViewTypes);
            m_viewChangedEvents.resize(MaxViewTypes);

            m_viewGroup = AZStd::make_shared<ViewGroup>();
            m_viewGroup->Init(ViewGroup::Descriptor{ nullptr, nullptr });
            SetRenderScene(renderScene);
        }

        ViewportContext::~ViewportContext()
        {
            m_aboutToBeDestroyedEvent.Signal(m_id);

            AzFramework::WindowNotificationBus::Handler::BusDisconnect();
            AzFramework::ViewportRequestBus::Handler::BusDisconnect();

            for (const auto& pipeline : m_currentPipelines)
            {
                if (pipeline)
                {
                    pipeline->RemoveFromRenderTick();
                    pipeline->RemoveFromScene();
                }
            }
            m_currentPipelines.clear();
            m_viewChangedEvents.clear();

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
                    for (uint32_t i = 0; i < MaxViewTypes; i++)
                    {
                        auto renderPipeline =
                            scene->FindRenderPipelineForWindow(m_windowContext->GetWindowHandle(), static_cast<ViewType>(i));
                        if (renderPipeline)
                        {
                            if (AZ::RPI::ViewPtr pipelineView = renderPipeline->GetDefaultView(); pipelineView)
                            {
                                m_viewGroup->SetView(pipelineView, static_cast<ViewType>(i));
                            }
                        }
                    }
                }

                m_rootScene = scene;
                if (m_rootScene)
                {
                    SceneNotificationBus::Handler::BusConnect(m_rootScene->GetId());
                }

                for (uint32_t i = 0; i < MaxViewTypes; i++)
                {
                    m_currentPipelines[i].reset();
                    UpdatePipelineView(i);
                }
            }

            m_sceneChangedEvent.Signal(scene);
        }

        // This function (called from BootstrapSystemComponent::OnTick) controls the
        // render pipelines to render or not for the launcher. In the editor the pipelines
        // are controlled from EditorViewportWidget::UpdateScene.
        void ViewportContext::RenderTick()
        {
            auto renderPipelineOnce = [](RenderPipelinePtr& pipeline)
            {
                // add the current pipeline to next render tick if it's not already added.
                if (pipeline && pipeline->GetRenderMode() != RenderPipeline::RenderMode::RenderOnce)
                {
                    pipeline->AddToRenderTickOnce();
                }
            };

            auto stopRenderingPipeline = [](RenderPipelinePtr& pipeline)
            {
                if (pipeline && pipeline->GetRenderMode() != RenderPipeline::RenderMode::NoRender)
                {
                    pipeline->RemoveFromRenderTick();
                }
            };

            if (auto* xrSystem = AZ::RPI::RPISystemInterface::Get()->GetXRSystem())
            {
                // Check wheter to render default pipeline on host or not.
                if (xrSystem->GetRHIXRRenderingInterface()->IsDefaultRenderPipelineNeeded())
                {
                    if (xrSystem->GetRHIXRRenderingInterface()->IsDefaultRenderPipelineEnabledOnHost())
                    {
                        renderPipelineOnce(m_currentPipelines[static_cast<size_t>(AZ::RPI::ViewType::Default)]);
                    }
                    else
                    {
                        stopRenderingPipeline(m_currentPipelines[static_cast<size_t>(AZ::RPI::ViewType::Default)]);
                    }
                }

                // Render XR pipelines
                for (AZ::u32 i = 0; i < xrSystem->GetNumViews(); i++)
                {
                    const AZ::RPI::ViewType viewType = (i == 0) ? AZ::RPI::ViewType::XrLeft : AZ::RPI::ViewType::XrRight;
                    renderPipelineOnce(m_currentPipelines[static_cast<size_t>(viewType)]);
                }
            }
            else
            {
                // Render default pipeline
                renderPipelineOnce(m_currentPipelines[static_cast<size_t>(AZ::RPI::ViewType::Default)]);
            }
        }

        void ViewportContext::OnBeginPrepareRender()
        {
            AZ_PROFILE_FUNCTION(RPI);
            ViewportContextNotificationBus::Event(GetName(), &ViewportContextNotificationBus::Events::OnRenderTick);
            ViewportContextIdNotificationBus::Event(GetId(), &ViewportContextIdNotificationBus::Events::OnRenderTick);
        }

        void ViewportContext::OnEndPrepareRender()
        {
            AZ_PROFILE_FUNCTION(RPI);
            ViewportContextNotificationBus::Event(GetName(), &ViewportContextNotificationBus::Events::WaitForRender);
            ViewportContextIdNotificationBus::Event(GetId(), &ViewportContextIdNotificationBus::Events::WaitForRender);
        }

        AZ::Name ViewportContext::GetName() const
        {
            return m_name;
        }

        ViewGroupPtr ViewportContext::GetViewGroup()
        {
            return m_viewGroup;
        }

        ConstViewGroupPtr ViewportContext::GetViewGroup() const
        {
            return m_viewGroup;
        }

        ViewPtr ViewportContext::GetDefaultView()
        {
            return m_viewGroup->GetView();
        }

        ConstViewPtr ViewportContext::GetDefaultView() const
        {
            return m_viewGroup->GetView();
        }

        ViewPtr ViewportContext::GetStereoscopicView(AZ::RPI::ViewType viewType)
        {
            return m_viewGroup->GetView(viewType);
        }

        ConstViewPtr ViewportContext::GetStereoscopicView(AZ::RPI::ViewType viewType) const
        {
            return m_viewGroup->GetView(viewType);
        }

        AzFramework::WindowSize ViewportContext::GetViewportSize() const
        {
            return m_viewportSize;
        }

        float ViewportContext::GetDpiScalingFactor() const
        {
            return m_viewportDpiScaleFactor;
        }

        void ViewportContext::ConnectSizeChangedHandler(SizeChangedEvent::Handler& handler)
        {
            handler.Connect(m_sizeChangedEvent);
        }

        void ViewportContext::ConnectDpiScalingFactorChangedHandler(ScalarChangedEvent::Handler& handler)
        {
            handler.Connect(m_dpiScalingFactorChangedEvent);
        }

        void ViewportContext::ConnectViewMatrixChangedHandler(MatrixChangedEvent::Handler& handler, ViewType viewType)
        {
            m_viewGroup->ConnectViewMatrixChangedEvent(handler, viewType);
        }

        void ViewportContext::ConnectProjectionMatrixChangedHandler(MatrixChangedEvent::Handler& handler, ViewType viewType)
        {
            m_viewGroup->ConnectProjectionMatrixChangedEvent(handler, viewType);
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
            handler.Connect(m_viewChangedEvents[DefaultViewType]);
        }

        void ViewportContext::ConnectAboutToBeDestroyedHandler(ViewportIdEvent::Handler& handler)
        {
            handler.Connect(m_aboutToBeDestroyedEvent);
        }

        const AZ::Matrix4x4& ViewportContext::GetCameraViewMatrix() const
        {
            return GetDefaultView()->GetWorldToViewMatrix();
        }

        AZ::Matrix3x4 ViewportContext::GetCameraViewMatrixAsMatrix3x4() const
        {
            return GetDefaultView()->GetWorldToViewMatrixAsMatrix3x4();
        }

        void ViewportContext::SetCameraViewMatrix(const AZ::Matrix4x4& matrix)
        {
            GetDefaultView()->SetWorldToViewMatrix(matrix);
            m_viewGroup->SignalViewMatrixChangedEvent(matrix);
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
            m_viewGroup->SignalViewMatrixChangedEvent(view->GetWorldToViewMatrix());
        }

        void ViewportContext::UpdateContextPipelineView(uint32_t viewIndex)
        {
            ViewType viewType = static_cast<ViewType>(viewIndex);
            if (auto view = m_viewGroup->GetView(viewType))
            {
                m_viewGroup->DisconnectProjectionMatrixHandler(viewType);
                m_viewGroup->DisconnectViewMatrixHandler(viewType);

                UpdatePipelineView(viewIndex);

                m_viewChangedEvents[viewIndex].Signal(view);
                m_viewGroup->SignalViewMatrixChangedEvent(view->GetWorldToViewMatrix());
                m_viewGroup->SignalProjectionMatrixChangedEvent(view->GetViewToClipMatrix());

                m_viewGroup->ConnectViewMatrixChangedHandler(viewType);
                m_viewGroup->ConnectProjectionMatrixChangedHandler(viewType);
            }
        }

        void ViewportContext::SetViewGroup(ViewGroupPtr viewGroup)
        {
            m_viewGroup = viewGroup;
            for (uint32_t i = 0; i < MaxViewTypes; i++)
            {
                UpdateContextPipelineView(i);
            }
        }

        void ViewportContext::UpdatePipelineView(uint32_t viewIndex)
        {
            ViewPtr pipelineView = m_viewGroup->GetView(static_cast<ViewType>(viewIndex));
            if (!pipelineView || !m_rootScene)
            {
                return;
            }

            auto& pipeline = m_currentPipelines[viewIndex];

            if (!pipeline)
            {
                pipeline = m_rootScene->FindRenderPipelineForWindow(m_windowContext->GetWindowHandle(), static_cast<ViewType>(viewIndex));
                if (pipeline)
                {
                    m_currentPipelineChangedEvent.Signal(pipeline);
                }
            }

            if (pipeline)
            {
                pipeline->UnregisterView(pipelineView);
                pipeline->SetDefaultView(pipelineView);
            }
        }

        RenderPipelinePtr ViewportContext::GetCurrentPipeline()
        {
            return m_currentPipelines[DefaultViewType];
        }

        void ViewportContext::OnRenderPipelineChanged(RenderPipeline* pipeline,
            SceneNotification::RenderPipelineChangeType changeType)
        {
            if (changeType == RPI::SceneNotification::RenderPipelineChangeType::Added)
            {
                // If the pipeline is registered to our window, reset our current pipeline and do a lookup
                // Currently, Scene just stores pipelines sequentially in a vector, but we'll attempt to be safe
                // in the event prioritization is added later
                if (pipeline->GetWindowHandle() == m_windowContext->GetWindowHandle())
                {
                    uint32_t viewIndex = static_cast<uint32_t>(pipeline->GetViewType());
                    m_currentPipelines[viewIndex].reset();
                    UpdatePipelineView(viewIndex);
                }
            }
            else if (changeType == RPI::SceneNotification::RenderPipelineChangeType::Removed)
            {
                 uint32_t viewIndex = static_cast<uint32_t>(pipeline->GetViewType());
                if (m_currentPipelines[viewIndex].get() == pipeline)
                {
                    m_currentPipelines[viewIndex].reset();
                    UpdatePipelineView(viewIndex);
                }
            }
        }

        void ViewportContext::OnResolutionChanged(uint32_t width, uint32_t height)
        {
            if (m_viewportSize.m_width != width || m_viewportSize.m_height != height)
            {
                // Clamp the viewport size to a minimum of (1, 1).
                m_viewportSize.m_height = AZStd::max(height, 1u);
                m_viewportSize.m_width = AZStd::max(width, 1u);
                m_sizeChangedEvent.Signal(m_viewportSize);
            }
        }

        void ViewportContext::OnDpiScaleFactorChanged(float dpiScaleFactor)
        {
            m_viewportDpiScaleFactor = dpiScaleFactor;
            m_dpiScalingFactorChangedEvent.Signal(dpiScaleFactor);
        }
    } // namespace RPI
} // namespace AZ
