/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Public/View.h>
#include <Atom/RPI.Public/ViewportContext.h>
#include <AtomToolsFramework/Viewport/ModularViewportCameraController.h>
#include <AzCore/Math/MatrixUtils.h>
#include <AzFramework/Viewport/CameraInput.h>
#include <AzFramework/Viewport/ViewportBus.h>
#include <AzFramework/Viewport/ViewportControllerList.h>
#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/EMStudioManager.h>

#include <EMStudio/AnimViewportRenderer.h>
#include <EMStudio/AnimViewportSettings.h>
#include <EMStudio/AnimViewportWidget.h>
#include <EMStudio/AtomRenderPlugin.h>

namespace EMStudio
{
    AnimViewportWidget::AnimViewportWidget(AtomRenderPlugin* parentPlugin)
        : AtomToolsFramework::RenderViewportWidget(parentPlugin->GetInnerWidget())
        , m_plugin(parentPlugin)
        , m_renderOverlay(m_plugin->GetInnerWidget())
    {
        setObjectName(QString::fromUtf8("AtomViewportWidget"));
        QSizePolicy qSize(QSizePolicy::Preferred, QSizePolicy::Preferred);
        qSize.setHorizontalStretch(0);
        qSize.setVerticalStretch(0);
        qSize.setHeightForWidth(sizePolicy().hasHeightForWidth());
        setSizePolicy(qSize);
        setAutoFillBackground(false);
        setStyleSheet(QString::fromUtf8(""));

        m_renderer = AZStd::make_unique<AnimViewportRenderer>(GetViewportContext(), m_plugin->GetRenderOptions());
        SetScene(m_renderer->GetFrameworkScene(), false);

        SetupCameras();
        SetupCameraController();
        Reinit();

        AnimViewportRequestBus::Handler::BusConnect();
        ViewportPluginRequestBus::Handler::BusConnect();

        m_renderOverlay.setVisible(true);
        m_renderOverlay.setUpdatesEnabled(false);
        m_renderOverlay.setMouseTracking(true);
        m_renderOverlay.setObjectName("renderOverlay");
        m_renderOverlay.setContentsMargins(0, 0, 0, 0);
        m_renderOverlay.winId(); // Force the render overlay to create a backing native window
        m_renderOverlay.lower();

        // get debug display interface for the viewport
        AzFramework::DebugDisplayRequestBus::BusPtr debugDisplayBus;
        AzFramework::DebugDisplayRequestBus::Bind(debugDisplayBus, GetViewportId());
        AZ_Assert(debugDisplayBus, "Invalid DebugDisplayRequestBus.");

        m_debugDisplay = AzFramework::DebugDisplayRequestBus::FindFirstHandler(debugDisplayBus);

        m_viewportUiManager.InitializeViewportUi(this, &m_renderOverlay);
        m_viewportUiManager.ConnectViewportUiBus(GetViewportId());

        AZ::RPI::SceneNotificationBus::Handler::BusConnect(m_renderer->GetRenderSceneId());
    }

    AnimViewportWidget::~AnimViewportWidget()
    {
        m_debugDisplay = nullptr;
        AZ::RPI::SceneNotificationBus::Handler::BusDisconnect();
        m_viewportUiManager.DisconnectViewportUiBus();
        ViewportPluginRequestBus::Handler::BusDisconnect();
        AnimViewportRequestBus::Handler::BusDisconnect();
    }

    void AnimViewportWidget::Reinit(bool resetCamera)
    {
        m_renderer->Reinit();
        m_renderer->UpdateActorRenderFlag(m_plugin->GetRenderOptions()->GetRenderFlags());

        if (resetCamera)
        {
            UpdateCameraViewMode(RenderOptions::CameraViewMode::DEFAULT);
        }
    }

    void AnimViewportWidget::SetupCameras()
    {
        const auto translateCameraInputChannelIds = EMStudio::ViewportUtil::TranslateCameraInputChannelIds();

        m_lookRotateCamera = AZStd::make_shared<AzFramework::RotateCameraInput>(EMStudio::ViewportUtil::RotateCameraInputChannelId());
        m_lookTranslateCamera = AZStd::make_shared<AzFramework::TranslateCameraInput>(
            translateCameraInputChannelIds, AzFramework::LookTranslation, AzFramework::TranslatePivotLook);
        m_lookTranslateCamera->m_translateSpeedFn = []
        {
            return 3.0f;
        };
        m_lookScrollTranslationCamera = AZStd::make_shared<AzFramework::LookScrollTranslationCameraInput>();
        m_lookPanCamera = AZStd::make_shared<AzFramework::PanCameraInput>(
            EMStudio::ViewportUtil::PanCameraInputChannelId(), AzFramework::LookPan, AzFramework::TranslatePivotLook);

        m_orbitCamera = AZStd::make_shared<AzFramework::OrbitCameraInput>(EMStudio::ViewportUtil::OrbitCameraInputChannelId());
        m_orbitCamera->SetPivotFn(
            [this]([[maybe_unused]] const AZ::Vector3& position, [[maybe_unused]] const AZ::Vector3& direction)
            {
                return m_renderer->GetCharacterCenter();
            });
        m_orbitCamera->SetActivationEndedFn(
            [viewportId = GetViewportId()]
            {
                // when the orbit camera ends, ensure that the internal camera returns to a look state
                // (internal offset value for camera is zero)
                AtomToolsFramework::ModularViewportCameraControllerRequestBus::Event(
                    viewportId, &AtomToolsFramework::ModularViewportCameraControllerRequestBus::Events::LookFromOrbit);
            });

        m_orbitTranslateCamera = AZStd::make_shared<AzFramework::TranslateCameraInput>(
            translateCameraInputChannelIds, AzFramework::LookTranslation, AzFramework::TranslatePivotLook);
        m_orbitRotateCamera = AZStd::make_shared<AzFramework::RotateCameraInput>(EMStudio::ViewportUtil::OrbitLookCameraInputChannelId());
        m_orbitScrollDollyCamera = AZStd::make_shared<AzFramework::OrbitScrollDollyCameraInput>();
        m_orbitPanCamera = AZStd::make_shared<AzFramework::PanCameraInput>(
            EMStudio::ViewportUtil::PanCameraInputChannelId(), AzFramework::LookPan, AzFramework::TranslateOffsetOrbit);
        m_orbitMotionDollyCamera =
            AZStd::make_shared<AzFramework::OrbitMotionDollyCameraInput>(EMStudio::ViewportUtil::OrbitDollyCameraInputChannelId());

        m_orbitCamera->m_orbitCameras.AddCamera(m_orbitRotateCamera);
        m_orbitCamera->m_orbitCameras.AddCamera(m_orbitScrollDollyCamera);
        m_orbitCamera->m_orbitCameras.AddCamera(m_orbitTranslateCamera);
        m_orbitCamera->m_orbitCameras.AddCamera(m_orbitMotionDollyCamera);
        m_orbitCamera->m_orbitCameras.AddCamera(m_orbitPanCamera);

        m_followRotateCamera = AZStd::make_shared<AzFramework::RotateCameraInput>(EMStudio::ViewportUtil::OrbitLookCameraInputChannelId());
        m_followScrollDollyCamera = AZStd::make_shared<AzFramework::OrbitScrollDollyCameraInput>();
        m_followScrollMotionCamera =
            AZStd::make_shared<AzFramework::OrbitMotionDollyCameraInput>(EMStudio::ViewportUtil::OrbitDollyCameraInputChannelId());
    }

    void AnimViewportWidget::SetupCameraController()
    {
        auto controller = AZStd::make_shared<AtomToolsFramework::ModularViewportCameraController>();
        controller->SetCameraViewportContextBuilderCallback(
            [viewportId =
                 GetViewportContext()->GetId()](AZStd::unique_ptr<AtomToolsFramework::ModularCameraViewportContext>& cameraViewportContext)
            {
                cameraViewportContext = AZStd::make_unique<AtomToolsFramework::ModularCameraViewportContextImpl>(viewportId);
            });

        controller->SetCameraPriorityBuilderCallback(
            [](AtomToolsFramework::CameraControllerPriorityFn& cameraControllerPriorityFn)
            {
                cameraControllerPriorityFn = AtomToolsFramework::DefaultCameraControllerPriority;
            });

        controller->SetCameraPropsBuilderCallback(
            [](AzFramework::CameraProps& cameraProps)
            {
                cameraProps.m_rotateSmoothnessFn = []
                {
                    return EMStudio::ViewportUtil::CameraRotateSmoothness();
                };

                cameraProps.m_translateSmoothnessFn = []
                {
                    return EMStudio::ViewportUtil::CameraTranslateSmoothness();
                };

                cameraProps.m_rotateSmoothingEnabledFn = []
                {
                    return EMStudio::ViewportUtil::CameraRotateSmoothingEnabled();
                };

                cameraProps.m_translateSmoothingEnabledFn = []
                {
                    return EMStudio::ViewportUtil::CameraTranslateSmoothingEnabled();
                };
            });

        controller->SetCameraListBuilderCallback(
            [this](AzFramework::Cameras& cameras)
            {
                cameras.AddCamera(m_lookRotateCamera);
                cameras.AddCamera(m_lookTranslateCamera);
                cameras.AddCamera(m_lookScrollTranslationCamera);
                cameras.AddCamera(m_lookPanCamera);
                cameras.AddCamera(m_orbitCamera);
            });

        GetControllerList()->Add(controller);
    }

    void AnimViewportWidget::UpdateCameraViewMode(RenderOptions::CameraViewMode mode)
    {
        // Set the camera view mode.
        const AZ::Vector3 targetPosition = m_renderer->GetCharacterCenter();
        AZ::Vector3 cameraPosition;
        switch (mode)
        {
        case RenderOptions::CameraViewMode::FRONT:
            cameraPosition.Set(targetPosition.GetX(), targetPosition.GetY() + CameraDistance, targetPosition.GetZ());
            break;
        case RenderOptions::CameraViewMode::BACK:
            cameraPosition.Set(targetPosition.GetX(), targetPosition.GetY() - CameraDistance, targetPosition.GetZ());
            break;
        case RenderOptions::CameraViewMode::TOP:
            cameraPosition.Set(targetPosition.GetX(), targetPosition.GetY(), CameraDistance + targetPosition.GetZ());
            break;
        case RenderOptions::CameraViewMode::BOTTOM:
            cameraPosition.Set(targetPosition.GetX(), targetPosition.GetY(), -CameraDistance + targetPosition.GetZ());
            break;
        case RenderOptions::CameraViewMode::LEFT:
            cameraPosition.Set(targetPosition.GetX() - CameraDistance, targetPosition.GetY(), targetPosition.GetZ());
            break;
        case RenderOptions::CameraViewMode::RIGHT:
            cameraPosition.Set(targetPosition.GetX() + CameraDistance, targetPosition.GetY(), targetPosition.GetZ());
            break;
        case RenderOptions::CameraViewMode::DEFAULT:
            // The default view mode is looking from the top left of the character.
            const AZ::Vector3 displacement = AZ::Vector3(-1.0f, 1.0f, 1.0f).GetNormalized() * CameraDistance;
            cameraPosition = targetPosition + displacement;
            break;
        }

        GetViewportContext()->SetCameraTransform(AZ::Transform::CreateLookAt(cameraPosition, targetPosition));

        // only if we're in follow mode should we set the pivot to the target position
        // (when not following, the pivot will be the camera position until alt is pressed)
        if (m_plugin->GetRenderOptions()->GetCameraFollowUp())
        {
            AtomToolsFramework::ModularViewportCameraControllerRequestBus::Event(
                GetViewportId(),
                &AtomToolsFramework::ModularViewportCameraControllerRequestBus::Events::SetCameraPivotDetachedImmediate,
                targetPosition);
        }
    }

    void AnimViewportWidget::UpdateCameraFollowUp(bool followUp)
    {
        const auto lookAndOrbitCameras =
            AZStd::vector<AZStd::shared_ptr<AzFramework::CameraInput>>{ m_lookRotateCamera, m_lookTranslateCamera,
                                                                        m_lookScrollTranslationCamera, m_lookPanCamera, m_orbitCamera };

        const auto followCameras =
            AZStd::vector<AZStd::shared_ptr<AzFramework::CameraInput>>{ m_followRotateCamera, m_followScrollDollyCamera,
                                                                        m_followScrollMotionCamera };
        if (followUp)
        {
            AtomToolsFramework::ModularViewportCameraControllerRequestBus::Event(
                GetViewportId(), &AtomToolsFramework::ModularViewportCameraControllerRequestBus::Events::RemoveCameras,
                lookAndOrbitCameras);

            AtomToolsFramework::ModularViewportCameraControllerRequestBus::Event(
                GetViewportId(), &AtomToolsFramework::ModularViewportCameraControllerRequestBus::Events::AddCameras, followCameras);

            AtomToolsFramework::ModularViewportCameraControllerRequestBus::Event(
                GetViewportId(),
                &AtomToolsFramework::ModularViewportCameraControllerRequestBus::Events::SetCameraPivotAttachedImmediate,
                m_renderer->GetCharacterCenter());

            AtomToolsFramework::ModularViewportCameraControllerRequestBus::Event(
                GetViewportId(), &AtomToolsFramework::ModularViewportCameraControllerRequestBus::Events::SetCameraOffsetImmediate,
                AZ::Vector3::CreateAxisY(-CameraDistance));
        }
        else
        {
            AtomToolsFramework::ModularViewportCameraControllerRequestBus::Event(
                GetViewportId(), &AtomToolsFramework::ModularViewportCameraControllerRequestBus::Events::RemoveCameras, followCameras);

            AtomToolsFramework::ModularViewportCameraControllerRequestBus::Event(
                GetViewportId(), &AtomToolsFramework::ModularViewportCameraControllerRequestBus::Events::AddCameras, lookAndOrbitCameras);

            AtomToolsFramework::ModularViewportCameraControllerRequestBus::Event(
                GetViewportId(), &AtomToolsFramework::ModularViewportCameraControllerRequestBus::Events::SetCameraOffsetImmediate,
                AZ::Vector3::CreateZero());

            AtomToolsFramework::ModularViewportCameraControllerRequestBus::Event(
                GetViewportId(), &AtomToolsFramework::ModularViewportCameraControllerRequestBus::Events::SetCameraPivotAttachedImmediate,
                GetViewportContext()->GetCameraTransform().GetTranslation());
        }
    }

    void AnimViewportWidget::OnTick(float deltaTime, AZ::ScriptTimePoint time)
    {
        RenderViewportWidget::OnTick(deltaTime, time);
        CalculateCameraProjection();
        RenderCustomPluginData();
        FollowCharacter();
        m_viewportUiManager.Update();
    }

    void AnimViewportWidget::CalculateCameraProjection()
    {
        auto viewportContext = GetViewportContext();
        auto windowSize = viewportContext->GetViewportSize();
        // Prevent division by zero
        const float height = AZStd::max<float>(aznumeric_cast<float>(windowSize.m_height), 1.0f);
        const float aspectRatio = aznumeric_cast<float>(windowSize.m_width) / height;

        const RenderOptions* renderOptions = m_plugin->GetRenderOptions();
        AZ::Matrix4x4 viewToClipMatrix;
        AZ::MakePerspectiveFovMatrixRH(
            viewToClipMatrix, AZ::DegToRad(renderOptions->GetFOV()), aspectRatio, renderOptions->GetNearClipPlaneDistance(),
            renderOptions->GetFarClipPlaneDistance(), true);

        viewportContext->GetDefaultView()->SetViewToClipMatrix(viewToClipMatrix);
    }

    void AnimViewportWidget::RenderCustomPluginData()
    {
        const EMotionFX::ActorRenderFlagsNamespace::ActorRenderFlags renderFlags = m_plugin->GetRenderOptions()->GetRenderFlags();

        for (EMStudioPlugin* plugin : GetPluginManager()->GetActivePlugins())
        {
            plugin->Render(renderFlags);
        }

        for (const AZStd::unique_ptr<PersistentPlugin>& plugin : GetPluginManager()->GetPersistentPlugins())
        {
            plugin->Render(renderFlags);
        }
    }

    void AnimViewportWidget::FollowCharacter()
    {
        if (m_plugin->GetRenderOptions()->GetCameraFollowUp())
        {
            AtomToolsFramework::ModularViewportCameraControllerRequestBus::Event(
                GetViewportId(), &AtomToolsFramework::ModularViewportCameraControllerRequestBus::Events::SetCameraPivotAttached,
                m_renderer->GetCharacterCenter());

            m_renderer->UpdateGroundplane();
        }
    }

    void AnimViewportWidget::UpdateRenderFlags(EMotionFX::ActorRenderFlags renderFlags)
    {
        m_renderer->UpdateActorRenderFlag(renderFlags);
        m_plugin->UpdatePickingRenderFlags(renderFlags);
    }

    AZ::s32 AnimViewportWidget::GetViewportId() const
    {
        return GetViewportContext()->GetId();
    }

    void AnimViewportWidget::mousePressEvent(QMouseEvent* event)
    {
        m_pixelsSinceClick = 0;
        m_prevMousePoint = event->globalPos();
    }

    void AnimViewportWidget::mouseMoveEvent(QMouseEvent* event)
    {
        int deltaX = event->globalX() - m_prevMousePoint.x();
        int deltaY = event->globalY() - m_prevMousePoint.y();

        m_pixelsSinceClick += AZStd::abs(deltaX) + AZStd::abs(deltaY);
    }

    void AnimViewportWidget::mouseReleaseEvent(QMouseEvent* event)
    {
        if (event->button() == Qt::RightButton && m_pixelsSinceClick < MinMouseMovePixes)
        {
            OnContextMenuEvent(event);
        }
    }

    void AnimViewportWidget::OnContextMenuEvent(QMouseEvent* event)
    {
        QMenu* menu = new QMenu(this);

        {
            QMenu* cameraMenu = menu->addMenu("Camera Options");
            QAction* frontAction = cameraMenu->addAction("Front");
            QAction* backAction = cameraMenu->addAction("Back");
            QAction* topAction = cameraMenu->addAction("Top");
            QAction* bottomAction = cameraMenu->addAction("Bottom");
            QAction* leftAction = cameraMenu->addAction("Left");
            QAction* rightAction = cameraMenu->addAction("Right");
            cameraMenu->addSeparator();
            QAction* resetCamAction = cameraMenu->addAction("Reset Camera");
            cameraMenu->addSeparator();
            QAction* followAction = cameraMenu->addAction("Follow Character");
            followAction->setCheckable(true);
            followAction->setChecked(m_plugin->GetRenderOptions()->GetCameraFollowUp());
            connect(frontAction, &QAction::triggered, this, [this]()
                {
                    UpdateCameraViewMode(RenderOptions::CameraViewMode::FRONT);
                });
            connect(backAction, &QAction::triggered, this, [this]()
                {
                    UpdateCameraViewMode(RenderOptions::CameraViewMode::BACK);
                });
            connect(topAction, &QAction::triggered, this, [this]()
                {
                    UpdateCameraViewMode(RenderOptions::CameraViewMode::TOP);
                });
            connect(bottomAction, &QAction::triggered, this, [this]()
                {
                    UpdateCameraViewMode(RenderOptions::CameraViewMode::BOTTOM);
                });
            connect(leftAction, &QAction::triggered, this, [this]()
                {
                    UpdateCameraViewMode(RenderOptions::CameraViewMode::LEFT);
                });
            connect(rightAction, &QAction::triggered, this, [this]()
                {
                    UpdateCameraViewMode(RenderOptions::CameraViewMode::RIGHT);
                });
            connect(resetCamAction, &QAction::triggered, this, [this]()
                {
                    UpdateCameraViewMode(RenderOptions::CameraViewMode::DEFAULT);
                });
            connect(followAction, &QAction::triggered, this, [this, followAction]()
                {
                    m_plugin->GetRenderOptions()->SetCameraFollowUp(followAction->isChecked());
                    AnimViewportRequestBus::Broadcast(&AnimViewportRequestBus::Events::UpdateCameraFollowUp, followAction->isChecked());
                });
        }

        if (m_renderer && m_renderer->GetEntityId() != AZ::EntityId())
        {
            QAction* resetAction = menu->addAction("Move Character to Origin");
            connect(resetAction, &QAction::triggered, this, [this]()
                {
                    m_renderer->MoveActorEntitiesToOrigin();
                    UpdateCameraViewMode(RenderOptions::CameraViewMode::DEFAULT);
                });
        }

        if (!menu->isEmpty())
        {
            menu->popup(event->globalPos());
        }
    }

    void AnimViewportWidget::resizeEvent(QResizeEvent* event)
    {
        QWidget::resizeEvent(event);
        m_renderOverlay.setGeometry(geometry());
        m_viewportUiManager.Update();
        CalculateCameraProjection();
    }

    void AnimViewportWidget::OnBeginPrepareRender()
    {
        if (m_debugDisplay)
        {
            for (const auto* entity : m_renderer->GetActorEntities())
            {
                AzFramework::EntityDebugDisplayEventBus::Event(
                    entity->GetId(),
                    &AzFramework::EntityDebugDisplayEvents::DisplayEntityViewport,
                    AzFramework::ViewportInfo{ GetViewportId() },
                    *m_debugDisplay);
            }
        }
    }
} // namespace EMStudio
