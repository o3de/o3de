/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <QByteArray>

#include <AzCore/UserSettings/UserSettings.h>

#include <AzQtComponents/Components/DockMainWindow.h>

#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Editor/AssetEditorBus.h>
#include <GraphCanvas/Editor/EditorTypes.h>
#include <GraphCanvas/Styling/StyleManager.h>
#include <GraphCanvas/Types/ConstructPresets.h>
#include <GraphCanvas/Widgets/GraphCanvasEditor/GraphCanvasEditorCentralWidget.h>
#include <GraphCanvas/Widgets/NodePalette/NodePaletteWidget.h>
#include <GraphCanvas/Widgets/NodePalette/TreeItems/NodePaletteTreeItem.h>
#endif

namespace AzQtComponents
{
    class FancyDocking;
}

namespace GraphCanvas
{
    class BookmarkDockWidget;
    class NodePaletteDockWidget;
    class SceneContextMenu;

    class AssetEditorUserSettings
        : public AZ::UserSettings
    {
    public:
        AZ_RTTI(AssetEditorUserSettings, "{B4F3513D-40BF-4A74-AFAF-EC884D13DEE6}", AZ::UserSettings);
        AZ_CLASS_ALLOCATOR(AssetEditorUserSettings, AZ::SystemAllocator);

        static void Reflect(AZ::ReflectContext* context);

        AssetEditorUserSettings() = default;

        void SetLastWindowState(const QByteArray& windowState);
        QByteArray GetLastWindowState();

    private:
        AZStd::vector<AZ::u8> m_lastWindowState;
    };

    struct AssetEditorWindowConfig
    {
        virtual ~AssetEditorWindowConfig() = default;

        /// General AssetEditor config parameters
        EditorId m_editorId;
        AZStd::string_view m_baseStyleSheet;
        const char* m_mimeType = "";
        AZStd::string_view m_saveIdentifier;    /// This is used by the node palette and fancy docking and needs to be unique per node Editor

        /// Default panel names that can be overriden by the client
        QString m_nodePaletteTitle = QObject::tr("Node Palette");
        QString m_bookmarksTitle = QObject::tr("Bookmarks");

        /// Node Palette specific config parameters
        NodePaletteConfig m_nodePaletteConfig;

        /// Override this method so that a Node Palette dock panel and embedded node palettes in
        /// certain context menus can be populated on behalf of the client
        virtual GraphCanvasTreeItem* CreateNodePaletteRoot() = 0;
    };

    class AssetEditorMainWindow
        : public AzQtComponents::DockMainWindow
        , private GraphCanvas::AssetEditorRequestBus::Handler
        , private GraphCanvas::AssetEditorSettingsRequestBus::Handler
        , private GraphCanvas::AssetEditorNotificationBus::Handler
        , private GraphCanvas::SceneNotificationBus::Handler
    {
        Q_OBJECT // AUTOMOC
    public:
        AZ_CLASS_ALLOCATOR(AssetEditorMainWindow, AZ::SystemAllocator);
        
        explicit AssetEditorMainWindow(AssetEditorWindowConfig* config, QWidget* parent = nullptr);
        virtual ~AssetEditorMainWindow();
        
        void SetupUI();
        void SetDropAreaText(AZStd::string_view text);        

        const EditorId& GetEditorId() const;

        GraphCanvas::GraphId GetActiveGraphCanvasGraphId() const;
        
    protected:
        void closeEvent(QCloseEvent* event) override;

        /// Create a new graph (EditorDockWidget) and pass it to our CentralDockWindow
        DockWidgetId CreateEditorDockWidget(const QString& title = QString());

        /// This base implementation provides an EditorDockWidget that creates
        /// and configures a scene and corresponding graphics view. The client
        /// can override this if a custom dock widget is desired.
        virtual EditorDockWidget* CreateDockWidget(const QString& title, QWidget* parent) const;

        /// Return a list of the currently open GraphIds for asset editor
        AZStd::vector<GraphCanvas::GraphId> GetOpenGraphIds();

        /// This base implementation will configure the default layout of all
        /// dock widgets provided. The client can extend or replace this
        /// depending if they would like to add new dock widgets or start
        /// from a blank slate.
        /// Returns false if any of the open dock widgets refuse to close during the reset
        virtual bool ConfigureDefaultLayout();

        /// Clients should override this to handle any additional logic when opening new editor dock widgets.
        virtual void OnEditorOpened(EditorDockWidget* dockWidget);
        /// Clients should override this to handle any additional logic when closing an editor dock widget.
        virtual void OnEditorClosing(EditorDockWidget* dockWidget);

        /// Close a specified editor dock widget (graph)
        bool CloseEditor(DockWidgetId dockWidgetId);
        /// Close all open editor dock widgets (graphs)
        bool CloseAllEditors();

        /// Set the focus to an existing dock widget
        bool FocusDockWidget(DockWidgetId dockWidgetId);

        AssetEditorCentralDockWindow* GetCentralDockWindow() const;

        virtual void RefreshMenu();
        virtual QMenu* AddFileMenu();
        virtual QAction* AddFileNewAction(QMenu* menu);
        virtual QAction* AddFileOpenAction(QMenu* menu);
        virtual QAction* AddFileSaveAction(QMenu* menu);
        virtual QAction* AddFileSaveAsAction(QMenu* menu);
        virtual QAction* AddFileCloseAction(QMenu* menu);
        virtual QMenu* AddEditMenu();
        virtual QAction* AddEditCutAction(QMenu* menu);
        virtual QAction* AddEditCopyAction(QMenu* menu);
        virtual QAction* AddEditPasteAction(QMenu* menu);
        virtual QAction* AddEditDuplicateAction(QMenu* menu);
        virtual QAction* AddEditDeleteAction(QMenu* menu);
        virtual void UpdateMenuActions();
        virtual void UpdateEditMenuActions();
        virtual void UpdatePasteAction();
        virtual QMenu* AddViewMenu();

        ////////////////////////////////////////////////////////////////////////
        // AssetEditorRequestBus::Handler overrides
        AZ::EntityId CreateNewGraph() override;
        bool ContainsGraph(const GraphCanvas::GraphId& graphId) const override;
        bool CloseGraph(const GraphId& graphId) override;
        ContextMenuAction::SceneReaction ShowSceneContextMenu(const QPoint& screenPoint, const QPointF& scenePoint) override;
        ContextMenuAction::SceneReaction ShowNodeContextMenu(const AZ::EntityId& nodeId, const QPoint& screenPoint, const QPointF& scenePoint) override;
        ContextMenuAction::SceneReaction ShowCommentContextMenu(const AZ::EntityId& nodeId, const QPoint& screenPoint, const QPointF& scenePoint) override;
        ContextMenuAction::SceneReaction ShowNodeGroupContextMenu(const AZ::EntityId& groupId, const QPoint& screenPoint, const QPointF& scenePoint) override;
        ContextMenuAction::SceneReaction ShowCollapsedNodeGroupContextMenu(const AZ::EntityId& nodeId, const QPoint& screenPoint, const QPointF& scenePoint) override;
        ContextMenuAction::SceneReaction ShowBookmarkContextMenu(const AZ::EntityId& bookmarkId, const QPoint& screenPoint, const QPointF& scenePoint) override;
        ContextMenuAction::SceneReaction ShowConnectionContextMenu(const AZ::EntityId& connectionId, const QPoint& screenPoint, const QPointF& scenePoint) override;
        ContextMenuAction::SceneReaction ShowSlotContextMenu(const AZ::EntityId& slotId, const QPoint& screenPoint, const QPointF& scenePoint) override;
        Endpoint CreateNodeForProposal(const AZ::EntityId& connectionId, const Endpoint& endpoint, const QPointF& scenePoint, const QPoint& screenPoint) override;
        void OnWrapperNodeActionWidgetClicked(const AZ::EntityId& wrapperNode, const QRect& actionWidgetBoundingRect, const QPointF& scenePoint, const QPoint& screenPoint) override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // AssetEditorSettingsRequestBus::Handler overrides
        EditorConstructPresets* GetConstructPresets() const override;
        const ConstructTypePresetBucket* GetConstructTypePresetBucket(ConstructType constructType) const override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // AssetEditorNotificationBus::Handler overrides
        void OnActiveGraphChanged(const GraphId& graphId) override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // SceneNotificationBus::Handler overrides
        void OnSelectionChanged() override;
        ////////////////////////////////////////////////////////////////////////

        ContextMenuAction::SceneReaction HandleContextMenu(EditorContextMenu& editorContextMenu, const AZ::EntityId& memberId, const QPoint& screenPoint, const QPointF& scenePoint) const;
        Endpoint HandleProposedConnection(const GraphId& graphId, const ConnectionId& connectionId, const Endpoint& endpoint, const NodeId& proposedNode, const QPoint& screenPoint);

        NodePaletteDockWidget* m_nodePalette = nullptr;
        BookmarkDockWidget* m_bookmarkDockWidget = nullptr;
        
        SceneContextMenu* m_sceneContextMenu = nullptr;
        EditorContextMenu* m_createNodeProposalContextMenu = nullptr;

        QAction* m_cutSelectedAction = nullptr;
        QAction* m_copySelectedAction = nullptr;
        QAction* m_pasteSelectedAction = nullptr;
        QAction* m_duplicateSelectedAction = nullptr;
        QAction* m_deleteSelectedAction = nullptr;

        EditorConstructPresets m_constructPresetDefaults;
        
    private:
        void SetDefaultLayout();

        void RestoreWindowState();
        void SaveWindowState();

        StyleManager m_styleManager;
        AssetEditorWindowConfig* m_config;
        AzQtComponents::FancyDocking* m_fancyDockingManager;
        AZStd::intrusive_ptr<AssetEditorUserSettings> m_settings;
        GraphId m_activeGraphId;
    };
}
