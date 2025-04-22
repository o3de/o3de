/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/PlatformDef.h>

AZ_PUSH_DISABLE_WARNING(4244 4251 4800, "-Wunknown-warning-option")
#include <QAction>
#include <QApplication>
#include <QKeyEvent>
#include <QGraphicsView>
#include <QGraphicsItem>
#include <QPoint>
#include <QScrollBar>
#include <QScopedValueRollback>
#include <QTimer>
#include <QWheelEvent>
AZ_POP_DISABLE_WARNING

#include <AzCore/Component/TickBus.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/Memory/SystemAllocator.h>

#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/UI/Notifications/ToastNotificationsView.h>

#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/ViewBus.h>
#include <GraphCanvas/Editor/AssetEditorBus.h>

namespace GraphCanvas
{
    //! GraphCanvasGraphicsView
    //! CanvasWidget that should be used to display GraphCanvas scenes.
    //! Will provide a lot of basic UX functionality.
    class GraphCanvasGraphicsView
        : public QGraphicsView
        , protected ViewRequestBus::Handler
        , protected SceneNotificationBus::Handler
        , protected AZ::TickBus::Handler
        , protected AssetEditorSettingsNotificationBus::Handler
        , public AzToolsFramework::EditorEvents::Bus::Handler
    {
    private:
        const int WHEEL_ZOOM = 120;
        const int WHEEL_ZOOM_ANGLE = 15;

        struct FocusQueue
        {
            enum class FocusType
            {
                DisplayArea,
                CenterOnArea
            };

            FocusType m_focusType;
            QRectF m_focusRect;
        };

    public:

        const int IS_EVENT_HANDLER_ONLY = 100;

        AZ_CLASS_ALLOCATOR(GraphCanvasGraphicsView, AZ::SystemAllocator);

        GraphCanvasGraphicsView(QWidget* parent = nullptr, bool registerShortcuts = true);
        ~GraphCanvasGraphicsView();

        const ViewId& GetViewId() const;

        // ViewRequestBus::Handler
        void SetEditorId(const EditorId& editorId) override;
        EditorId GetEditorId() const override;

        void SetScene(const AZ::EntityId& sceneId) override;
        AZ::EntityId GetScene() const override;

        void ClearScene() override;

        AZ::Vector2 GetViewSceneCenter() const override;

        AZ::Vector2 MapToGlobal(const AZ::Vector2& scenePoint) override;
        AZ::Vector2 MapToScene(const AZ::Vector2& view) override;
        AZ::Vector2 MapFromScene(const AZ::Vector2& scene) override;        

        void SetViewParams(const ViewParams& viewParams) override;
        void DisplayArea(const QRectF& viewArea) override;
        void CenterOnArea(const QRectF& viewArea) override;

        void CenterOn(const QPointF& posInSceneCoordinates) override;
        void CenterOnStartOfChain() override;
        void CenterOnEndOfChain() override;
        void CenterOnSelection() override;

        QRectF GetCompleteArea() override;
        void WheelEvent(QWheelEvent* ev) override;
        QRectF GetViewableAreaInSceneCoordinates() override;

        GraphCanvasGraphicsView* AsGraphicsView() override;

        QImage* CreateImageOfGraph() override;
        QImage* CreateImageOfGraphArea(QRectF area) override;

        qreal GetZoomLevel() const override;

        void ScreenshotSelection() override;

        void EnableSelection() override;
        void DisableSelection() override;

        void ShowEntireGraph() override;
        void ZoomIn() override;
        void ZoomOut() override;

        void PanSceneBy(QPointF repositioning, AZStd::chrono::milliseconds duration) override;
        void PanSceneTo(QPointF scenePoint, AZStd::chrono::milliseconds duration) override;

        void RefreshView() override;

        void HideToastNotification(const AzToolsFramework::ToastId& toastId) override;

        AzToolsFramework::ToastId ShowToastNotification(const AzQtComponents::ToastConfiguration& toastConfiguration) override;
        AzToolsFramework::ToastId ShowToastAtCursor(const AzQtComponents::ToastConfiguration& toastConfiguration) override;
        AzToolsFramework::ToastId ShowToastAtPoint(const QPoint& screenPosition, const QPointF& anchorPoint, const AzQtComponents::ToastConfiguration& toastConfiguration) override;

        bool IsShowing() const override;
        ////

        // TickBus
        void OnTick(float tick, AZ::ScriptTimePoint timePoint) override;
        ////

        QRectF GetSelectedArea();
        void SelectAll();        
        void SelectAllRelative(ConnectionType input);        
        void SelectConnectedNodes();
        void ClearSelection();

        // SceneNotificationBus::Handler
        void OnStylesChanged() override;
        void OnNodeIsBeingEdited(bool isEditing) override;
        ////

        // QGraphicsView
        void keyReleaseEvent(QKeyEvent* event) override;
        void keyPressEvent(QKeyEvent* event) override;

        void contextMenuEvent(QContextMenuEvent* event) override;

        void mousePressEvent(QMouseEvent* event) override;
        void mouseMoveEvent(QMouseEvent* event) override;
        void mouseReleaseEvent(QMouseEvent* event) override;

        void wheelEvent(QWheelEvent* event) override;

        void focusOutEvent(QFocusEvent* event) override;

        void resizeEvent(QResizeEvent* event) override;
        void moveEvent(QMoveEvent* event) override;
        void scrollContentsBy(int dx, int dy) override;

        void showEvent(QShowEvent* showEvent) override;
        void hideEvent(QHideEvent* hideEvent) override;
        ////
        
        // AssetEditorSettingsNotificationBus
        void OnSettingsChanged() override;
        ////

        // AzToolsFramework::EditorEvents::Bus::Handler
        void OnEscape() override;
        ////

        bool GetIsEditing(){ return m_isEditing; }

    protected:

        void CreateBookmark(int bookmarkShortcut);
        void JumpToBookmark(int bookmarkShortcut);

    private:

        void CenterOnSceneMembers(const AZStd::vector<AZ::EntityId>& memberIds);

        void ConnectBoundsSignals();
        void DisconnectBoundsSignals();
        void OnBoundsChanged();

        void QueueSave();
        void SaveViewParams();
        void CalculateMinZoomBounds();
        void ClampScaleBounds();

        void OnRubberBandChanged(QRect rubberBandRect, QPointF fromScenePoint, QPointF toScenePoint);

        AZStd::pair<float, float> CalculateEdgePanning(const QPointF& globalPoint) const;
        void CalculateInternalRectangle();

        void ManageTickState();

        GraphCanvasGraphicsView(const GraphCanvasGraphicsView&) = delete;

        ViewId       m_viewId;
        AZ::EntityId m_sceneId;
        EditorId     m_editorId;
        
        bool m_isDragSelecting;

        bool   m_checkForEdges;
        float  m_scrollSpeed;
        AZStd::pair<float, float> m_edgePanning;

        qreal m_minZoom;
        qreal m_maxZoom;

        bool   m_checkForDrag;
        QPoint m_initialClick;
        
        bool m_ignoreValueChange;
        bool m_reapplyViewParams;

        ViewParams m_viewParams;

        QTimer m_timer;
        QTimer m_styleTimer;

        float   m_panCountdown;
        QPointF m_panVelocity;

        QPointF m_panningAggregator;

        AZStd::unique_ptr<FocusQueue> m_queuedFocus;

        AZStd::unique_ptr<AzToolsFramework::ToastNotificationsView> m_notificationsView;

        bool m_isEditing;

        QPointF m_offsets;
        QRectF m_internalRectangle;
    };
}
