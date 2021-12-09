/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomToolsFramework/Viewport/RenderViewportWidget.h>
#include <Atom/RPI.Public/ViewportContext.h>
#include <Atom/RPI.Public/ViewportContextBus.h>
#include <Atom/RPI.Public/View.h>
#include <AzFramework/Input/Devices/Mouse/InputDeviceMouse.h>
#include <AzFramework/Viewport/ViewportControllerList.h>
#include <AzFramework/Viewport/ViewportScreen.h>
#include <AzToolsFramework/Viewport/ViewportTypes.h>
#include <AzCore/Math/MathUtils.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/Bootstrap/BootstrapRequestBus.h>

#include <QApplication>
#include <QCursor>
#include <QBoxLayout>
#include <QWindow>
#include <QMouseEvent>

namespace AtomToolsFramework
{
    RenderViewportWidget::RenderViewportWidget(QWidget* parent, bool shouldInitializeViewportContext)
        : QWidget(parent)
        , AzFramework::InputChannelEventListener(AzFramework::InputChannelEventListener::GetPriorityDefault())
    {
        if (shouldInitializeViewportContext)
        {
            InitializeViewportContext();
        }

        setUpdatesEnabled(false);
        setFocusPolicy(Qt::FocusPolicy::WheelFocus);
        setMouseTracking(true);
    }

    bool RenderViewportWidget::InitializeViewportContext(AzFramework::ViewportId id)
    {
        if (m_viewportContext != nullptr)
        {
            AZ_Assert(id == AzFramework::InvalidViewportId || m_viewportContext->GetId() == id, "Attempted to reinitialize RenderViewportWidget with a different ID");
            return true;
        }

        auto viewportContextManager = AZ::Interface<AZ::RPI::ViewportContextRequestsInterface>::Get();
        AZ_Assert(viewportContextManager, "Attempted to initialize RenderViewportWidget without ViewportContextManager");

        if (viewportContextManager == nullptr)
        {
            return false;
        }

        // Before we do anything else, we must create a ViewportContext which will give us a ViewportId if we didn't manually specify one.
        AZ::RPI::ViewportContextRequestsInterface::CreationParameters params;
        params.device = AZ::RHI::RHISystemInterface::Get()->GetDevice();
        params.windowHandle = reinterpret_cast<AzFramework::NativeWindowHandle>(winId());
        params.id = id;
        AzFramework::WindowRequestBus::Handler::BusConnect(params.windowHandle);
        m_viewportContext = viewportContextManager->CreateViewportContext(AZ::Name(), params);

        if (m_viewportContext == nullptr)
        {
            return false;
        }

        SetControllerList(AZStd::make_shared<AzFramework::ViewportControllerList>());

        AZ::Name cameraName = AZ::Name(AZStd::string::format("Viewport %i Default Camera", m_viewportContext->GetId()));
        m_defaultCamera = AZ::RPI::View::CreateView(cameraName, AZ::RPI::View::UsageFlags::UsageCamera);
        AZ::Interface<AZ::RPI::ViewportContextRequestsInterface>::Get()->PushView(m_viewportContext->GetName(), m_defaultCamera);

        m_viewportInteractionImpl = AZStd::make_unique<ViewportInteractionImpl>(m_defaultCamera);
        m_viewportInteractionImpl->m_deviceScalingFactorFn = [this] { return aznumeric_cast<float>(devicePixelRatioF()); };
        m_viewportInteractionImpl->m_screenSizeFn = [this] { return AzFramework::ScreenSize(width(), height()); };
        m_viewportInteractionImpl->Connect(id);

        AzToolsFramework::ViewportInteraction::ViewportMouseCursorRequestBus::Handler::BusConnect(GetId());
        AzFramework::InputChannelEventListener::Connect();
        AZ::TickBus::Handler::BusConnect();
        AzFramework::WindowRequestBus::Handler::BusConnect(params.windowHandle);

        m_inputChannelMapper = new AzToolsFramework::QtEventToAzInputMapper(this, id);

        // Forward input events to our controller list.
        QObject::connect(m_inputChannelMapper, &AzToolsFramework::QtEventToAzInputMapper::InputChannelUpdated, this,
            [this](const AzFramework::InputChannel* inputChannel, QEvent* event)
        {
            AzFramework::NativeWindowHandle windowId = reinterpret_cast<AzFramework::NativeWindowHandle>(winId());
            if (m_controllerList->HandleInputChannelEvent(AzFramework::ViewportControllerInputEvent{GetId(), windowId, *inputChannel}))
            {
                // If the controller handled the input event, mark the event as accepted so it doesn't continue to propagate.
                if (event)
                {
                    event->setAccepted(true);
                }
            }
        });

        return true;
    }

    RenderViewportWidget::~RenderViewportWidget()
    {
        AzFramework::WindowRequestBus::Handler::BusDisconnect();
        AZ::TickBus::Handler::BusDisconnect();
        AzFramework::InputChannelEventListener::Disconnect();
        AzToolsFramework::ViewportInteraction::ViewportMouseCursorRequestBus::Handler::BusDisconnect();
        m_viewportInteractionImpl->Disconnect();
    }

    void RenderViewportWidget::LockRenderTargetSize(uint32_t width, uint32_t height)
    {
        setFixedSize(aznumeric_cast<int>(width), aznumeric_cast<int>(height));
    }

    void RenderViewportWidget::UnlockRenderTargetSize()
    {
        setSizePolicy(QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Preferred);
    }

    AZ::RPI::ViewportContextPtr RenderViewportWidget::GetViewportContext()
    {
        return m_viewportContext;
    }

    AZ::RPI::ConstViewportContextPtr RenderViewportWidget::GetViewportContext() const
    {
        return m_viewportContext;
    }

    void RenderViewportWidget::SetScene(const AZStd::shared_ptr<AzFramework::Scene>& scene, bool useDefaultRenderPipeline)
    {
        if (scene == nullptr)
        {
            m_viewportContext->SetRenderScene(nullptr);
            return;
        }

        // Check if the scene already has an atom scene attached. In this case we don't need to create a new atom scene.
        if (auto existingScene = scene->FindSubsystem<AZ::RPI::ScenePtr>())
        {
            m_viewportContext->SetRenderScene(*existingScene);

            // If we have a render pipeline, use it and ensure an AuxGeom feature processor is installed.
            // Otherwise, fall through and ensure a render pipeline is installed for this scene.
            if (m_viewportContext->GetCurrentPipeline())
            {
                if (auto auxGeomFP = existingScene->get()->GetFeatureProcessor<AZ::RPI::AuxGeomFeatureProcessorInterface>())
                {
                    m_auxGeom = auxGeomFP->GetOrCreateDrawQueueForView(m_defaultCamera.get());
                }
                return;
            }
        }

        AZ::RPI::ScenePtr atomScene;
        auto initializeScene = [&](AZ::Render::Bootstrap::Request* bootstrapRequests)
        {
            atomScene = bootstrapRequests->GetOrCreateAtomSceneFromAzScene(scene.get());
            if (useDefaultRenderPipeline)
            {
                // atomScene may already have a default render pipeline installed.
                // If so, this will be a no-op.
                bootstrapRequests->EnsureDefaultRenderPipelineInstalledForScene(atomScene, m_viewportContext);
            }
        };
        AZ::Render::Bootstrap::RequestBus::Broadcast(initializeScene);
        m_viewportContext->SetRenderScene(atomScene);
        if (auto auxGeomFP = atomScene->GetFeatureProcessor<AZ::RPI::AuxGeomFeatureProcessorInterface>())
        {
            m_auxGeom = auxGeomFP->GetOrCreateDrawQueueForView(m_defaultCamera.get());
        }
    }

    AZ::RPI::ViewPtr RenderViewportWidget::GetDefaultCamera()
    {
        return m_defaultCamera;
    }

    AZ::RPI::ConstViewPtr RenderViewportWidget::GetDefaultCamera() const
    {
        return m_defaultCamera;
    }

    bool RenderViewportWidget::OnInputChannelEventFiltered(const AzFramework::InputChannel& inputChannel)
    {
        if (!hasFocus())
        {
            return false;
        }

        // Only forward channels that aren't covered by our Qt -> AZ event mapper
        if (!m_inputChannelMapper || m_inputChannelMapper->HandlesInputEvent(inputChannel))
        {
            return false;
        }

        bool shouldConsumeEvent = true;

        AzFramework::NativeWindowHandle windowId = reinterpret_cast<AzFramework::NativeWindowHandle>(winId());
        const bool eventHandled = m_controllerList->HandleInputChannelEvent({GetId(), windowId, inputChannel});

        // If our controllers handled the event and it's one we can safely consume (i.e. it's not an Ended event that other viewports might need), consume it.
        return eventHandled && shouldConsumeEvent;
    }

    void RenderViewportWidget::OnTick([[maybe_unused]]float deltaTime, AZ::ScriptTimePoint time)
    {
        m_time = time;
        m_controllerList->UpdateViewport({GetId(), AzFramework::FloatSeconds(deltaTime), m_time});
    }

    bool RenderViewportWidget::event(QEvent* event)
    {
        switch (event->type()) 
        {
            case QEvent::Resize:
                SendWindowResizeEvent();
                break;

            default:
                break;
        }
        return QWidget::event(event);
    }

    void RenderViewportWidget::enterEvent([[maybe_unused]] QEvent* event)
    {
        m_mouseOver = true;
    }

    void RenderViewportWidget::leaveEvent([[maybe_unused]] QEvent* event)
    {
        m_mouseOver = false;
    }

    void RenderViewportWidget::SendWindowResizeEvent()
    {
        // Scale the size by the DPI of the platform to
        // get the proper size in pixels.
        const auto pixelRatio = devicePixelRatioF();
        const QSize uiWindowSize = size();
        const QSize windowSize = uiWindowSize * pixelRatio;

        const AzFramework::NativeWindowHandle windowId = reinterpret_cast<AzFramework::NativeWindowHandle>(winId());
        AzFramework::WindowNotificationBus::Event(
            windowId, &AzFramework::WindowNotifications::OnWindowResized, windowSize.width(), windowSize.height());
    }

    AZ::Name RenderViewportWidget::GetCurrentContextName() const
    {
        return m_viewportContext->GetName();
    }

    void RenderViewportWidget::SetCurrentContextName(const AZ::Name& contextName)
    {
        auto viewportContextManager = AZ::Interface<AZ::RPI::ViewportContextRequestsInterface>::Get();
        viewportContextManager->RenameViewportContext(m_viewportContext, contextName);
    }

    AzFramework::ViewportId RenderViewportWidget::GetId() const
    {
        return m_viewportContext->GetId();
    }

    AzFramework::ViewportControllerListPtr RenderViewportWidget::GetControllerList()
    {
        return m_controllerList;
    }

    AzFramework::ConstViewportControllerListPtr RenderViewportWidget::GetControllerList() const
    {
        return m_controllerList;
    }

    void RenderViewportWidget::SetControllerList(AzFramework::ViewportControllerListPtr controllerList)
    {
        if (m_controllerList)
        {
            m_controllerList->UnregisterViewportContext(GetId());
        }
        m_controllerList = controllerList;
        if (m_controllerList)
        {
            m_controllerList->RegisterViewportContext(GetId());
        }
    }

    void RenderViewportWidget::SetInputProcessingEnabled(bool enabled)
    {
        m_inputChannelMapper->SetEnabled(enabled);
        m_controllerList->SetEnabled(enabled);
    }

    AzFramework::CameraState RenderViewportWidget::GetCameraState()
    {
        return m_viewportInteractionImpl->GetCameraState();
    }

    AzFramework::ScreenPoint RenderViewportWidget::ViewportWorldToScreen(const AZ::Vector3& worldPosition)
    {
        return m_viewportInteractionImpl->ViewportWorldToScreen(worldPosition);
    }

    AZ::Vector3 RenderViewportWidget::ViewportScreenToWorld(const AzFramework::ScreenPoint& screenPosition)
    {
        return m_viewportInteractionImpl->ViewportScreenToWorld(screenPosition);
    }

    AzToolsFramework::ViewportInteraction::ProjectedViewportRay RenderViewportWidget::ViewportScreenToWorldRay(
        const AzFramework::ScreenPoint& screenPosition)
    {
        return m_viewportInteractionImpl->ViewportScreenToWorldRay(screenPosition);
    }

    float RenderViewportWidget::DeviceScalingFactor()
    {
        return aznumeric_cast<float>(devicePixelRatioF());
    }

    bool RenderViewportWidget::IsMouseOver() const
    {
        return m_mouseOver;
    }

    void RenderViewportWidget::BeginCursorCapture()
    {
        m_inputChannelMapper->SetCursorCaptureEnabled(true);
    }

    void RenderViewportWidget::EndCursorCapture()
    {
        m_inputChannelMapper->SetCursorCaptureEnabled(false);
    }

    void RenderViewportWidget::SetOverrideCursor(AzToolsFramework::ViewportInteraction::CursorStyleOverride cursorStyleOverride)
    {
        m_inputChannelMapper->SetOverrideCursor(cursorStyleOverride);
    }

    void RenderViewportWidget::ClearOverrideCursor()
    {
        m_inputChannelMapper->ClearOverrideCursor();
    }

    void RenderViewportWidget::SetWindowTitle(const AZStd::string& title)
    {
        setWindowTitle(QString::fromUtf8(title.c_str()));
    }

    AzFramework::WindowSize RenderViewportWidget::GetClientAreaSize() const
    {
        return AzFramework::WindowSize{aznumeric_cast<uint32_t>(width()), aznumeric_cast<uint32_t>(height())};
    }

    void RenderViewportWidget::ResizeClientArea(AzFramework::WindowSize clientAreaSize)
    {
        const QSize targetSize = QSize{aznumeric_cast<int>(clientAreaSize.m_width), aznumeric_cast<int>(clientAreaSize.m_height)};
        resize(targetSize);
    }

    bool RenderViewportWidget::GetFullScreenState() const
    {
        // The RenderViewportWidget does not currently support full screen.
        return false;
    }

    void RenderViewportWidget::SetFullScreenState([[maybe_unused]]bool fullScreenState)
    {
        // The RenderViewportWidget does not currently support full screen.
    }

    bool RenderViewportWidget::CanToggleFullScreenState() const
    {
        // The RenderViewportWidget does not currently support full screen.
        return false;
    }

    void RenderViewportWidget::ToggleFullScreenState()
    {
        // The RenderViewportWidget does not currently support full screen.
    }

    float RenderViewportWidget::GetDpiScaleFactor() const
    {
        return aznumeric_cast<float>(devicePixelRatioF());
    }

    uint32_t RenderViewportWidget::GetDisplayRefreshRate() const
    {
        return 60;
    }

    uint32_t RenderViewportWidget::GetSyncInterval() const
    {
        return 1;
    }

    // Editor ignores requests to change the sync interval
    bool RenderViewportWidget::SetSyncInterval(uint32_t /*ignored*/)
    {
        return false;
    }

} //namespace AtomToolsFramework
