/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <QGraphicsDropShadowEffect>
#include <QGraphicsScene>
#include <QResizeEvent>
#include <QVBoxLayout>
#include <QWheelEvent>

#include <GraphCanvas/Widgets/GraphCanvasGraphicsView/GraphCanvasGraphicsView.h>
#include <GraphCanvas/Widgets/MiniMapGraphicsView/MiniMapGraphicsView.h>

namespace GraphCanvas
{
    MiniMapGraphicsView::MiniMapGraphicsView(const AZ::Crc32& graphCanvasEditorNotificationBusId,
        bool isStandAlone,
        const AZ::EntityId sceneId,
        QWidget* parent)
        : QGraphicsView(parent)
    {
        setInteractive(false);
        setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

        QObject::connect(&m_miniMapDragUpdateTimer,
            &QTimer::timeout,
            [ this ]()
            {
                // Mainview -> Magnifier
                m_updateSceneContentNeeded = true;
                m_ApplyMainViewToMagnifierNeeded = true;
            });
        m_miniMapDragUpdateTimer.setInterval(MINIMAP_UPDATE_TIMER_DELAY_IN_MILLISECONDS);

        if (isStandAlone)
        {
            AssetEditorNotificationBus::Handler::BusConnect(graphCanvasEditorNotificationBusId);
        }
        else
        {
            // Add a drop shadow for more contrast.
            QGraphicsDropShadowEffect* effect = new QGraphicsDropShadowEffect;
            effect->setBlurRadius(50);
            effect->setXOffset(0);
            effect->setYOffset(0);
            effect->setColor(QColor(0, 0, 0, 127));
            setGraphicsEffect(effect);

            setFixedSize(200, 200);
            SetScene(sceneId);
        }
    }

    MiniMapGraphicsView::~MiniMapGraphicsView()
    {
        AssetEditorNotificationBus::Handler::BusDisconnect();
        ViewNotificationBus::Handler::BusDisconnect();
    }

    // GraphCanvasEditorNotificationBus
    void MiniMapGraphicsView::OnActiveGraphChanged(const AZ::EntityId& sceneId)
    {
        SceneNotificationBus::Handler::BusDisconnect();
        ViewNotificationBus::Handler::BusDisconnect();

        SetScene(sceneId);
    }

    void MiniMapGraphicsView::ApplyMainViewToMagnifier()
    {
        if (!m_mainViewId.IsValid())
        {
            // There's no scene.
            return;
        }

        if (m_blockNotificationsFromMainView)
        {
            // This happens, for example, when OnViewScrolled()
            // is triggered from our own call to centerOn().
            return;
        }

        UpdateMiniMapZoom();
        UpdateMagnifierBoxFrame();
    }

    void MiniMapGraphicsView::ApplyMagnifierToMainView(QMouseEvent* ev)
    {
        if (!m_mainViewId.IsValid())
        {
            // There's no scene.
            return;
        }

        if ((ev->buttons() != Qt::LeftButton) ||
            (ev->modifiers() != Qt::NoModifier))
        {
            return;
        }

        // IMPORTANT: Using QSignalBlocker with m_mainView here
        // DOESN'T prevent OnViewScrolled from being triggered.
        // Therefore we HAVE to filter-out redundant notifications
        // manually.
        m_blockNotificationsFromMainView = true;
        ViewRequestBus::Event(m_mainViewId, &ViewRequests::CenterOn, mapToScene(ev->pos()));
        m_blockNotificationsFromMainView = false;

        UpdateMagnifierBoxFrame();
        QTimer::singleShot(0, this, SLOT(repaint()));
    }

    // ViewNotificationBus
    void MiniMapGraphicsView::OnViewResized(QResizeEvent* /*event*/)
    {
        // The main view has been resized.
        // Mainview -> Magnifier
        m_ApplyMainViewToMagnifierNeeded = true;
    }

    // ViewNotificationBus
    void MiniMapGraphicsView::OnViewScrolled()
    {
        // The main view has scrolled.
        // Mainview -> Magnifier
        m_ApplyMainViewToMagnifierNeeded = true;
    }

    // ViewNotificationBus
    void MiniMapGraphicsView::OnViewCenteredOnArea()
    {
        // The main view has been centered on an area using CenterOnArea()
        // Mainview -> Magnifier
        m_ApplyMainViewToMagnifierNeeded = true;
    }

    // SceneNotificationBus
    void MiniMapGraphicsView::OnNodeAdded(const AZ::EntityId& /*nodeId*/, bool)
    {
        // Mainview -> Magnifier
        m_updateSceneContentNeeded = true;
        m_ApplyMainViewToMagnifierNeeded = true;
    }

    // SceneNotificationBus
    void MiniMapGraphicsView::OnNodeRemoved(const AZ::EntityId& /*nodeId*/)
    {
        // Mainview -> Magnifier
        m_updateSceneContentNeeded = true;
        m_ApplyMainViewToMagnifierNeeded = true;
    }

    // SceneNotificationBus
    void MiniMapGraphicsView::OnSceneMemberDragBegin()
    {
        if (m_miniMapDragUpdateTimer.isActive())
        {
            // IMPORTANT: We MUST verify if the timer is active because
            // QTimer::start() will stop and restart an active timer.
            return;
        }

        m_miniMapDragUpdateTimer.start();
    }

    // SceneNotificationBus
    void MiniMapGraphicsView::OnSceneMemberDragComplete()
    {
        m_miniMapDragUpdateTimer.stop();
    }

    void MiniMapGraphicsView::SetScene(const AZ::EntityId& sceneId)
    {
        m_completeSceneContentInSceneCoordinates = QRect();
        m_magnifierBoxInWindowCoordinates = QRect();
        m_sceneId = sceneId;
        m_mainViewId.SetInvalid();
        setScene(nullptr);

        if (m_sceneId.IsValid())
        {
            SceneNotificationBus::Handler::BusConnect(m_sceneId);

            SceneRequestBus::EventResult(m_mainViewId, m_sceneId, &SceneRequests::GetViewId);
            ViewNotificationBus::Handler::BusConnect(m_mainViewId);

            m_updateSceneContentNeeded = true;

            QGraphicsScene* graphicsScene = nullptr;
            SceneRequestBus::EventResult(graphicsScene, m_sceneId, &SceneRequests::AsQGraphicsScene);
            if (graphicsScene)
            {
                setScene(graphicsScene);
            }
        }

        // Mainview -> Magnifier
        m_ApplyMainViewToMagnifierNeeded = true;
    }

    QSize MiniMapGraphicsView::minimumSizeHint() const
    {
        return QSize(0, 0);
    }

    void MiniMapGraphicsView::resizeEvent(QResizeEvent* ev)
    {
        // Mainview -> Magnifier
        m_ApplyMainViewToMagnifierNeeded = true;

        QGraphicsView::resizeEvent(ev);
    }

    void MiniMapGraphicsView::wheelEvent(QWheelEvent* ev)
    {
        ViewRequestBus::Event(m_mainViewId, &ViewRequests::WheelEvent, ev);
    }

    void MiniMapGraphicsView::mousePressEvent(QMouseEvent* ev)
    {
        // Mainview <- Magnifier
        ApplyMagnifierToMainView(ev);
        // Mainview -> Magnifier
        m_ApplyMainViewToMagnifierNeeded = true;

        QGraphicsView::mousePressEvent(ev);
    }

    void MiniMapGraphicsView::mouseMoveEvent(QMouseEvent* ev)
    {
        // Mainview <- Magnifier
        ApplyMagnifierToMainView(ev);
        // Mainview -> Magnifier
        m_ApplyMainViewToMagnifierNeeded = true;

        QGraphicsView::mouseMoveEvent(ev);
    }

    void MiniMapGraphicsView::mouseReleaseEvent(QMouseEvent* ev)
    {
        // Mainview <- Magnifier
        ApplyMagnifierToMainView(ev);
        // Mainview -> Magnifier
        m_ApplyMainViewToMagnifierNeeded = true;

        // IMPORTANT: We DON'T want to invoke QGraphicsView::mouseReleaseEvent(ev)
        // here because it would invoke SceneNotifications::OnMouseReleased.
        // That's how we distinguish between clicks in the minimap, and clicks in
        // the main view.
    }

    void MiniMapGraphicsView::paintEvent(QPaintEvent* ev)
    {
        if (!m_mainViewId.IsValid())
        {
            // There's no scene.
            return;
        }

        if (m_ApplyMainViewToMagnifierNeeded)
        {
            ApplyMainViewToMagnifier();
            m_ApplyMainViewToMagnifierNeeded = false;
        }
        if (m_updateSceneContentNeeded)
        {
            UpdateCompleteSceneContentInSceneCoordinates();
            m_updateSceneContentNeeded = false;
        }

        QGraphicsView::paintEvent(ev);

        QPainter painter(viewport());

        QPen pen;
        pen.setColor(QColor(255, 255, 0, 255)); // Opaque yellow.
        pen.setWidth(MINIMAP_MAGNIFIER_BOX_THICKNESS); // Box thickness.
        painter.setPen(pen);

        painter.drawRect(aznumeric_cast<int>(m_magnifierBoxInWindowCoordinates.x()),
            aznumeric_cast<int>(m_magnifierBoxInWindowCoordinates.y()),
            aznumeric_cast<int>(m_magnifierBoxInWindowCoordinates.width()),
            aznumeric_cast<int>(m_magnifierBoxInWindowCoordinates.height()));
    }

    void MiniMapGraphicsView::UpdateCompleteSceneContentInSceneCoordinates()
    {
        ViewRequestBus::EventResult(m_completeSceneContentInSceneCoordinates, m_mainViewId, &ViewRequests::GetCompleteArea);
    }

    void MiniMapGraphicsView::UpdateMiniMapZoom()
    {
        if (!m_mainViewId.IsValid())
        {
            // There's no scene.
            return;
        }

        fitInView(m_completeSceneContentInSceneCoordinates, Qt::KeepAspectRatio);

        // Clamp the scaling factor.
        qreal scale = transform().m11();
        if (scale > ZOOM_MAX)
        {
            scale = ZOOM_MAX;
        }

        QTransform m;
        m.scale(scale, scale);
        setTransform(m);
    }

    void MiniMapGraphicsView::UpdateMagnifierBoxFrame()
    {
        if (!m_mainViewId.IsValid())
        {
            // There's no scene.
            return;
        }

        QRectF viewableAreaInSceneCoordinates;
        ViewRequestBus::EventResult(viewableAreaInSceneCoordinates, m_mainViewId, &ViewRequests::GetViewableAreaInSceneCoordinates);

        m_magnifierBoxInWindowCoordinates = mapFromScene(viewableAreaInSceneCoordinates).boundingRect();

        // Keep the box inset.
        m_magnifierBoxInWindowCoordinates.setWidth(m_magnifierBoxInWindowCoordinates.width() - (qreal)MINIMAP_MAGNIFIER_BOX_THICKNESS);
        m_magnifierBoxInWindowCoordinates.setHeight(m_magnifierBoxInWindowCoordinates.height() - (qreal)MINIMAP_MAGNIFIER_BOX_THICKNESS);
    }

    MiniMapDockWidget::MiniMapDockWidget(const AZ::Crc32& graphCanvasEditorNotificationBusId, QWidget* parent)
        : AzQtComponents::StyledDockWidget(parent)
    {
        setWindowTitle("MiniMap");

        QWidget* host = new QWidget();
        QVBoxLayout* layout = new QVBoxLayout();
        layout->setContentsMargins(0, 0, 0, 0);
        layout->addWidget(aznew MiniMapGraphicsView(graphCanvasEditorNotificationBusId));
        host->setLayout(layout);
        setWidget(host);
    }
}
