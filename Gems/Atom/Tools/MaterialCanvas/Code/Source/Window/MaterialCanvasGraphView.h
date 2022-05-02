/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AtomToolsFramework/Document/AtomToolsDocumentNotificationBus.h>
#include <AtomToolsFramework/Window/AtomToolsMainWindowRequestBus.h>
#include <AzCore/Component/EntityId.h>
#include <AzQtComponents/Components/WindowDecorationWrapper.h>
#include <GraphCanvas/Components/Connections/ConnectionBus.h>
#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Editor/AssetEditorBus.h>
#include <GraphCanvas/Editor/EditorTypes.h>
#include <GraphCanvas/Widgets/Bookmarks/BookmarkDockWidget.h>
#include <GraphCanvas/Widgets/ConstructPresetDialog/ConstructPresetDialog.h>
#include <GraphCanvas/Widgets/GraphCanvasGraphicsView/GraphCanvasGraphicsView.h>
#include <GraphCanvas/Widgets/NodePalette/TreeItems/NodePaletteTreeItem.h>
#include <MaterialCanvasUtil.h>

#include <QToolButton>
#include <QWidget>
#endif

namespace GraphCanvas
{
    class AssetEditorToolbar;
    class GraphCanvasMimeEvent;
    class MiniMapDockWidget;
    class ConstructPresetDialog;
    class AssetEditorToolbar;
    class NodePaletteDockWidget;
    class BookmarkDockWidget;
    class SceneContextMenu;
    class EditorContextMenu;

    namespace Styling
    {
        class StyleSheet;
    }
} // namespace GraphCanvas

namespace MaterialCanvas
{
    //! MaterialCanvasGraphView handles displaying and managing interactions for a single graph
    class MaterialCanvasGraphView
        : public QWidget
        , private AtomToolsFramework::AtomToolsMainMenuRequestBus::Handler
        , private AtomToolsFramework::AtomToolsDocumentNotificationBus::Handler
        , private GraphCanvas::SceneNotificationBus::Handler
        , private GraphCanvas::AssetEditorSettingsRequestBus::Handler
        , private GraphCanvas::AssetEditorNotificationBus::Handler
        , private GraphCanvas::AssetEditorRequestBus::Handler
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(MaterialCanvasGraphView, AZ::SystemAllocator, 0);

        MaterialCanvasGraphView(
            const AZ::Crc32& toolId,
            const AZ::Uuid& documentId,
            const CreateNodePaletteItemsCallback& createNodePaletteItemsCallback,
            QWidget* parent = 0);
        ~MaterialCanvasGraphView();

    protected:
        // AtomToolsFramework::AtomToolsDocumentNotificationBus::Handler overrides...
        void OnDocumentOpened(const AZ::Uuid& documentId) override;
        void OnDocumentClosed(const AZ::Uuid& documentId) override;
        void OnDocumentDestroyed(const AZ::Uuid& documentId) override;

        // AtomToolsFramework::AtomToolsMainMenuRequestBus::Handler overrides...
        AZ::s32 GetMainMenuPriority() const override;
        void CreateMenus(QMenuBar* menuBar) override;
        void UpdateMenus(QMenuBar* menuBar) override;

        // GraphCanvas::AssetEditorRequestBus::Handler overrides...
        AZ::EntityId CreateNewGraph() override;
        bool ContainsGraph(const GraphCanvas::GraphId& graphId) const override;
        bool CloseGraph(const GraphCanvas::GraphId& graphId) override;
        GraphCanvas::ContextMenuAction::SceneReaction ShowSceneContextMenu(const QPoint& screenPoint, const QPointF& scenePoint) override;
        GraphCanvas::ContextMenuAction::SceneReaction ShowNodeContextMenu(
            const AZ::EntityId& nodeId, const QPoint& screenPoint, const QPointF& scenePoint) override;
        GraphCanvas::ContextMenuAction::SceneReaction ShowCommentContextMenu(
            const AZ::EntityId& nodeId, const QPoint& screenPoint, const QPointF& scenePoint) override;
        GraphCanvas::ContextMenuAction::SceneReaction ShowNodeGroupContextMenu(
            const AZ::EntityId& groupId, const QPoint& screenPoint, const QPointF& scenePoint) override;
        GraphCanvas::ContextMenuAction::SceneReaction ShowCollapsedNodeGroupContextMenu(
            const AZ::EntityId& nodeId, const QPoint& screenPoint, const QPointF& scenePoint) override;
        GraphCanvas::ContextMenuAction::SceneReaction ShowBookmarkContextMenu(
            const AZ::EntityId& bookmarkId, const QPoint& screenPoint, const QPointF& scenePoint) override;
        GraphCanvas::ContextMenuAction::SceneReaction ShowConnectionContextMenu(
            const AZ::EntityId& connectionId, const QPoint& screenPoint, const QPointF& scenePoint) override;
        GraphCanvas::ContextMenuAction::SceneReaction ShowSlotContextMenu(
            const AZ::EntityId& slotId, const QPoint& screenPoint, const QPointF& scenePoint) override;
        GraphCanvas::Endpoint CreateNodeForProposal(
            const AZ::EntityId& connectionId,
            const GraphCanvas::Endpoint& endpoint,
            const QPointF& scenePoint,
            const QPoint& screenPoint) override;
        void OnWrapperNodeActionWidgetClicked(
            const AZ::EntityId& wrapperNode,
            const QRect& actionWidgetBoundingRect,
            const QPointF& scenePoint,
            const QPoint& screenPoint) override;

        // GraphCanvas::AssetEditorSettingsRequestBus::Handler overrides...
        GraphCanvas::EditorConstructPresets* GetConstructPresets() const override;
        const GraphCanvas::ConstructTypePresetBucket* GetConstructTypePresetBucket(GraphCanvas::ConstructType constructType) const override;
        GraphCanvas::Styling::ConnectionCurveType GetConnectionCurveType() const override;
        GraphCanvas::Styling::ConnectionCurveType GetDataConnectionCurveType() const override;

        // GraphCanvas::AssetEditorNotificationBus::Handler overrides...
        void OnActiveGraphChanged(const GraphCanvas::GraphId& graphId) override;

        // GraphCanvas::SceneNotificationBus::Handler overrides...
        void OnSelectionChanged() override;

        GraphCanvas::Endpoint HandleProposedConnection(
            const GraphCanvas::GraphId& graphId,
            const GraphCanvas::ConnectionId& connectionId,
            const GraphCanvas::Endpoint& endpoint,
            const GraphCanvas::NodeId& proposedNode,
            const QPoint& screenPoint);

        GraphCanvas::ContextMenuAction::SceneReaction HandleContextMenu(
            GraphCanvas::EditorContextMenu& editorContextMenu,
            const AZ::EntityId& memberId,
            const QPoint& screenPoint,
            const QPointF& scenePoint) const;

        void AlignSelected(const GraphCanvas::AlignConfig& alignConfig);
        void OpenPresetsEditor();

        const AZ::Crc32 m_toolId;
        const AZ::Uuid m_documentId;
        GraphCanvas::GraphId m_activeGraphId;

        CreateNodePaletteItemsCallback m_createNodePaletteItemsCallback;
        AzQtComponents::WindowDecorationWrapper* m_presetWrapper = {};
        GraphCanvas::ConstructPresetDialog* m_presetEditor = {};
        GraphCanvas::GraphCanvasGraphicsView* m_graphicsView = {};
        GraphCanvas::AssetEditorToolbar* m_editorToolbar = {};
        GraphCanvas::SceneContextMenu* m_sceneContextMenu = {};
        GraphCanvas::EditorContextMenu* m_createNodeProposalContextMenu = {};
        GraphCanvas::EditorConstructPresets m_constructPresetDefaults;

        QAction* m_actionCut = {};
        QAction* m_actionCopy = {};
        QAction* m_actionPaste = {};
        QAction* m_actionDelete = {};
        QAction* m_actionDuplicate = {};

        QAction* m_actionRemoveUnusedNodes = {};
        QAction* m_actionRemoveUnusedElements = {};

        QAction* m_actionSelectAll = {};
        QAction* m_actionSelectNone = {};
        QAction* m_actionSelectInputs = {};
        QAction* m_actionSelectOutputs = {};
        QAction* m_actionSelectConnected = {};
        QAction* m_actionSelectEnable = {};
        QAction* m_actionSelectDisable = {};

        QAction* m_actionScreenShot = {};

        QAction* m_actionAlignTop = {};
        QAction* m_actionAlignBottom = {};
        QAction* m_actionAlignLeft = {};
        QAction* m_actionAlignRight = {};

        QAction* m_actionPresetEditor = {};
        QAction* m_actionShowEntireGraph = {};
        QAction* m_actionZoomIn = {};
        QAction* m_actionZoomOut = {};
        QAction* m_actionZoomSelection = {};
        QAction* m_actionGotoStartOfChain = {};
        QAction* m_actionGotoEndOfChain = {};

        QToolButton* m_takeScreenshot = {};
    };
} // namespace MaterialCanvas
