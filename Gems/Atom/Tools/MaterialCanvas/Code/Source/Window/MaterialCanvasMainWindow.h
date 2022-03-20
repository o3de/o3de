/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/EntityId.h>
#include <AzFramework/Asset/AssetCatalogBus.h>
#include <AzQtComponents/Components/ToastNotification.h>
#include <AzQtComponents/Components/WindowDecorationWrapper.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserFilterModel.h>
#include <GraphCanvas/Components/Connections/ConnectionBus.h>
#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Editor/AssetEditorBus.h>
#include <GraphCanvas/Editor/EditorTypes.h>
#include <GraphCanvas/Styling/StyleManager.h>
#include <GraphCanvas/Widgets/Bookmarks/BookmarkDockWidget.h>
#include <GraphCanvas/Widgets/ConstructPresetDialog/ConstructPresetDialog.h>
#include <GraphCanvas/Widgets/NodePalette/TreeItems/NodePaletteTreeItem.h>

#include <AtomToolsFramework/Document/AtomToolsDocumentInspector.h>
#include <AtomToolsFramework/Document/AtomToolsDocumentMainWindow.h>

#include <Viewport/MaterialCanvasViewportWidget.h>
#include <Window/ToolBar/MaterialCanvasToolBar.h>

#include <QLabel>
#include <QMainWindow>
#include <QMimeData>
#include <QTimer>
#include <QToolButton>
#include <QTranslator>
#include <QVBoxLayout>
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
    //! MaterialCanvasMainWindow is the main class. Its responsibility is limited to initializing and connecting
    //! its panels, managing selection of assets, and performing high-level actions like saving. It contains...
    //! 2) MaterialCanvasViewport        - The user can see the selected Material applied to a model.
    //! 3) MaterialPropertyInspector  - The user edits the properties of the selected Material.
    class MaterialCanvasMainWindow
        : public AtomToolsFramework::AtomToolsDocumentMainWindow
        , private GraphCanvas::SceneNotificationBus::Handler
        , private GraphCanvas::AssetEditorSettingsRequestBus::Handler
        , private GraphCanvas::AssetEditorNotificationBus::Handler
        , private GraphCanvas::AssetEditorRequestBus::Handler
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(MaterialCanvasMainWindow, AZ::SystemAllocator, 0);

        using Base = AtomToolsFramework::AtomToolsDocumentMainWindow;

        MaterialCanvasMainWindow(const AZ::Crc32& toolId, QWidget* parent = 0);
        ~MaterialCanvasMainWindow();

    protected:
        // AtomToolsFramework::AtomToolsMainWindowRequestBus::Handler overrides...
        void ResizeViewportRenderTarget(uint32_t width, uint32_t height) override;
        void LockViewportRenderTargetSize(uint32_t width, uint32_t height) override;
        void UnlockViewportRenderTargetSize() override;

        // AtomToolsFramework::AtomToolsDocumentNotificationBus::Handler overrides...
        void OnDocumentOpened(const AZ::Uuid& documentId) override;

        // AtomToolsFramework::AtomToolsDocumentMainWindow overrides...
        void OpenSettings() override;
        void OpenHelp() override;

    private:
        ////////////////////////////////////////////////////////////////////////
        // AssetEditorRequestBus::Handler overrides
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
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // AssetEditorSettingsRequestBus::Handler overrides
        GraphCanvas::EditorConstructPresets* GetConstructPresets() const override;
        const GraphCanvas::ConstructTypePresetBucket* GetConstructTypePresetBucket(GraphCanvas::ConstructType constructType) const override;
        GraphCanvas::Styling::ConnectionCurveType GetConnectionCurveType() const override;
        GraphCanvas::Styling::ConnectionCurveType GetDataConnectionCurveType() const override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // AssetEditorNotificationBus::Handler overrides
        void OnActiveGraphChanged(const GraphCanvas::GraphId& graphId) override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // SceneNotificationBus::Handler overrides
        void OnSelectionChanged() override;
        ////////////////////////////////////////////////////////////////////////

    private:
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

        void CreateMenus();
        void UpdateMenus();
        void AlignSelected(const GraphCanvas::AlignConfig& alignConfig);
        void OpenPresetsEditor();
        GraphCanvas::GraphCanvasTreeItem* GetNodePaletteRootTreeItem() const;

        GraphCanvas::GraphId m_activeGraphId;

        AZ::EntityId m_entityMimeDelegateId;
        QTranslator m_translator;

        GraphCanvas::StyleManager m_styleManager;
        AzQtComponents::WindowDecorationWrapper* m_presetWrapper = {};
        GraphCanvas::ConstructPresetDialog* m_presetEditor = {};
        GraphCanvas::AssetEditorToolbar* m_editorToolbar = {};
        GraphCanvas::NodePaletteDockWidget* m_nodePalette = {};
        GraphCanvas::BookmarkDockWidget* m_bookmarkDockWidget = {};
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

        AtomToolsFramework::AtomToolsDocumentInspector* m_materialInspector = {};
        MaterialCanvasViewportWidget* m_materialViewport = {};
        MaterialCanvasToolBar* m_toolBar = {};
    };
} // namespace MaterialCanvas
