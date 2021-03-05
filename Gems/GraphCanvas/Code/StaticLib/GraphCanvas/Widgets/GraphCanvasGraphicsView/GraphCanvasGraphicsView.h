/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#pragma once

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

#include <AzCore/Component/TickBus.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/Memory/SystemAllocator.h>

#include <AzToolsFramework/API/ToolsApplicationAPI.h>

#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/ViewBus.h>
#include <GraphCanvas/Editor/AssetEditorBus.h>
#include <GraphCanvas/Widgets/ToastNotification/ToastNotification.h>

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
        const int KEYBOARD_MOVE = 50;
        const int WHEEL_ZOOM = 120;

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

        AZ_CLASS_ALLOCATOR(GraphCanvasGraphicsView, AZ::SystemAllocator, 0);

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

        void HideToastNotification(const ToastId& toastId) override;

        ToastId ShowToastNotification(const ToastConfiguration& toastConfiguration) override;
        ToastId ShowToastAtCursor(const ToastConfiguration& toastConfiguration) override;
        ToastId ShowToastAtPoint(const QPoint& screenPosition, const QPointF& anchorPoint, const ToastConfiguration& toastConfiguration) override;

        bool IsShowing() const;
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

        void focusOutEvent(QFocusEvent* event);

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

        void UpdateToastPosition();

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

        void OnNotificationHidden();
        void DisplayQueuedNotification();

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

        // These will display sequentially in a reserved part of the UI
        ToastId                  m_activeNotification;
        AZStd::vector< ToastId > m_queuedNotifications;

        // There could be more then the queued list in terms of general notifications
        // As some systems might want to re-use the systems for their own needs.
        AZStd::unordered_map< ToastId, ToastNotification* > m_notifications;

        bool m_isEditing;

        QPointF m_offsets;
        QRectF m_internalRectangle;
    };
}
