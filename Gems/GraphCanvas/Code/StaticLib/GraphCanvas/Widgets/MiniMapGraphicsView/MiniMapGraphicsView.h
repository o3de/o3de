/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <QGraphicsView>
#include <QTimer>

#include <AzCore/Component/EntityId.h>
#include <AzQtComponents/Components/StyledDockWidget.h>

#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/ViewBus.h>
#include <GraphCanvas/Editor/AssetEditorBus.h>

class QResizeEvent;
class QWheelEvent;

namespace GraphCanvas
{
    class MiniMapGraphicsView
        : public QGraphicsView
        , public SceneNotificationBus::Handler
        , public AssetEditorNotificationBus::Handler
        , public ViewNotificationBus::Handler
    {
    private:
        //! We probably want this value to always be
        //! the same as GraphCanvasGraphicsView::ZOOM_MAX.
        const qreal ZOOM_MAX = 0.5f;
        const int MINIMAP_UPDATE_TIMER_DELAY_IN_MILLISECONDS = 100;
        const int MINIMAP_MAGNIFIER_BOX_THICKNESS = 4;

    public:
        AZ_TYPE_INFO(MiniMapGraphicsView, "{DF03D03E-2048-43B2-8F01-897098D553F2}");
        AZ_CLASS_ALLOCATOR(MiniMapGraphicsView, AZ::SystemAllocator);

        MiniMapGraphicsView(const AZ::Crc32& graphCanvasEditorNotificationBusId = 0,
            bool isStandAlone = true,
            const AZ::EntityId sceneId = AZ::EntityId(),
            QWidget* parent = nullptr);

        ~MiniMapGraphicsView() override;

        // AssetEditorNotificationBus
        void OnActiveGraphChanged(const AZ::EntityId& sceneId) override;
        ////

        // ViewNotificationBus
        void OnViewResized(QResizeEvent* event) override;
        void OnViewScrolled() override;
        void OnViewCenteredOnArea() override;
        ////

        // SceneNotificationBus
        void OnNodeAdded(const AZ::EntityId& nodeId, bool isPaste) override;
        void OnNodeRemoved(const AZ::EntityId& nodeId) override;
        void OnSceneMemberDragBegin() override;
        void OnSceneMemberDragComplete() override;
        ////

        void ApplyMainViewToMagnifier();
        void ApplyMagnifierToMainView(QMouseEvent* ev);
        void SetScene(const AZ::EntityId& sceneId);

    protected:

        QSize minimumSizeHint() const override;
        void resizeEvent(QResizeEvent* ev) override;
        void wheelEvent(QWheelEvent* ev) override;
        void mousePressEvent(QMouseEvent* ev) override;
        void mouseMoveEvent(QMouseEvent* ev) override;
        void mouseReleaseEvent(QMouseEvent* ev) override;
        void paintEvent(QPaintEvent* ev) override;

    private:

        void UpdateCompleteSceneContentInSceneCoordinates();
        void UpdateMiniMapZoom();
        void UpdateMagnifierBoxFrame();

        QRectF m_completeSceneContentInSceneCoordinates;
        QRectF m_magnifierBoxInWindowCoordinates;

        bool m_blockNotificationsFromMainView = false;
        bool m_updateSceneContentNeeded = false;
        bool m_ApplyMainViewToMagnifierNeeded = false;

        AZ::EntityId m_sceneId;
        ViewId m_mainViewId;
        QTimer m_miniMapDragUpdateTimer;
    };

    class MiniMapDockWidget
        : public AzQtComponents::StyledDockWidget
    {
    public:
        AZ_CLASS_ALLOCATOR(MiniMapDockWidget, AZ::SystemAllocator);

        MiniMapDockWidget(const AZ::Crc32& graphCanvasEditorNotificationBusId, QWidget* parent = nullptr);
    };
}
