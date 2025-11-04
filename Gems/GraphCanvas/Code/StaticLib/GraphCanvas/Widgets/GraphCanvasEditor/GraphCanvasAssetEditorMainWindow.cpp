/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <QApplication>
#include <QClipboard>
#include <QCloseEvent>
#include <QLayout>
#include <QList>
#include <QMenu>
#include <QMenuBar>
#include <QTimer>

#include <AzQtComponents/Components/DockTabWidget.h>
#include <AzQtComponents/Components/FancyDocking.h>

#include <GraphCanvas/Components/VisualBus.h>
#include <GraphCanvas/Components/Nodes/NodeBus.h>
#include <GraphCanvas/Styling/StyleManager.h>
#include <GraphCanvas/Widgets/Bookmarks/BookmarkDockWidget.h>
#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenuActions/GeneralMenuActions/GeneralMenuActions.h>
#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenus/BookmarkContextMenu.h>
#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenus/CollapsedNodeGroupContextMenu.h>
#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenus/CommentContextMenu.h>
#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenus/ConnectionContextMenu.h>
#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenus/NodeContextMenu.h>
#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenus/NodeGroupContextMenu.h>
#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenus/SceneContextMenu.h>
#include <GraphCanvas/Widgets/EditorContextMenu/ContextMenus/SlotContextMenu.h>
#include <GraphCanvas/Widgets/EditorContextMenu/EditorContextMenu.h>
#include <GraphCanvas/Widgets/GraphCanvasEditor/GraphCanvasAssetEditorMainWindow.h>
#include <GraphCanvas/Widgets/GraphCanvasEditor/GraphCanvasEditorCentralWidget.h>
#include <GraphCanvas/Widgets/GraphCanvasEditor/GraphCanvasEditorDockWidget.h>
#include <GraphCanvas/Widgets/NodePalette/NodePaletteDockWidget.h>
#include <GraphCanvas/Widgets/NodePalette/TreeItems/NodePaletteTreeItem.h>

namespace GraphCanvas
{
    // Default size percentage that the Node Palette dock widget will take up
    static const float DEFAULT_NODE_PALETTE_SIZE = 0.15f;

    void AssetEditorUserSettings::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<AssetEditorUserSettings>()
                ->Version(1)
                ->Field("m_lastWindowState", &AssetEditorUserSettings::m_lastWindowState)
                ;
        }
    }

    void AssetEditorUserSettings::SetLastWindowState(const QByteArray& windowState)
    {
        m_lastWindowState.clear();
        m_lastWindowState.assign((AZ::u8*)windowState.begin(), (AZ::u8*)windowState.end());
    }

    QByteArray AssetEditorUserSettings::GetLastWindowState()
    {
        QByteArray windowState((const char*)m_lastWindowState.data(), (int)m_lastWindowState.size());

        return windowState;
    }

    //////////////////////////
    // AssetEditorMainWindow
    //////////////////////////
    AssetEditorMainWindow::AssetEditorMainWindow(AssetEditorWindowConfig* config, QWidget* parent)
        : AzQtComponents::DockMainWindow(parent, Qt::Widget | Qt::WindowMinMaxButtonsHint)
        , m_styleManager(config->m_editorId, config->m_baseStyleSheet)
        , m_config(config)
        , m_fancyDockingManager(new AzQtComponents::FancyDocking(this, config->m_saveIdentifier.data()))
    {
        m_settings = AZ::UserSettings::CreateFind<AssetEditorUserSettings>(config->m_editorId, AZ::UserSettings::CT_LOCAL);

        // Make sure this is done before the context menus get created, or else it will cause a crash
        AssetEditorRequestBus::Handler::BusConnect(config->m_editorId);
        AssetEditorSettingsRequestBus::Handler::BusConnect(config->m_editorId);
        AssetEditorNotificationBus::Handler::BusConnect(config->m_editorId);
        m_constructPresetDefaults.SetEditorId(config->m_editorId);

        NodePaletteConfig& nodePaletteConfig = m_config->m_nodePaletteConfig;
        nodePaletteConfig.m_editorId = config->m_editorId;
        nodePaletteConfig.m_mimeType = config->m_mimeType;
        nodePaletteConfig.m_saveIdentifier = config->m_saveIdentifier;

        SetupUI();

        QTimer::singleShot(0, [this]() {
            RefreshMenu();

            RestoreWindowState();
        });
    }
    
    AssetEditorMainWindow::~AssetEditorMainWindow()
    {
        AssetEditorNotificationBus::Handler::BusDisconnect();
        AssetEditorSettingsRequestBus::Handler::BusDisconnect();
        AssetEditorRequestBus::Handler::BusDisconnect();

        SaveWindowState();
    }

    void AssetEditorMainWindow::SetupUI()
    {
        setDockNestingEnabled(false);
        setDockOptions(dockOptions());
        setTabPosition(Qt::DockWidgetArea::AllDockWidgetAreas, QTabWidget::TabPosition::North);

        // Setup our central dock window that will contain the tabbed scene graphs, as well
        // as the empty drop area widget by default.  As part of this, we need to configure the
        // empty drop area widget to accept our mime type so the user can drag/drop a node from
        // the palette onto it, which will trigger creating a new graph
        const GraphCanvas::EditorId& editorId = GetEditorId();
        AssetEditorCentralDockWindow* centralWidget = aznew AssetEditorCentralDockWindow(editorId, m_config->m_saveIdentifier.data());
        QObject::connect(centralWidget, &AssetEditorCentralDockWindow::OnEditorClosing, this, &AssetEditorMainWindow::OnEditorClosing);
        GraphCanvasEditorEmptyDockWidget* emptyDockWidget = centralWidget->GetEmptyDockWidget();
        emptyDockWidget->SetEditorId(editorId);
        emptyDockWidget->RegisterAcceptedMimeType(m_config->m_mimeType);
        setCentralWidget(centralWidget);

        // Setup our default node palette
        m_nodePalette = aznew NodePaletteDockWidget(m_config->CreateNodePaletteRoot(), editorId, QObject::tr("Node Palette"), this, m_config->m_mimeType, false, m_config->m_saveIdentifier);
        m_nodePalette->setObjectName("NodePalette");
        m_nodePalette->setWindowTitle(m_config->m_nodePaletteTitle);

        // Setup the bookmark panel
        m_bookmarkDockWidget = aznew BookmarkDockWidget(editorId, this);
        m_bookmarkDockWidget->setObjectName("Bookmarks");
        m_bookmarkDockWidget->setWindowTitle(m_config->m_bookmarksTitle);

        // Add a node palette for creating new nodes to the default scene context menu,
        // which is what is displayed when right-clicking on an empty space in the graph
        NodePaletteConfig sceneContextMenuConfig = m_config->m_nodePaletteConfig;
        sceneContextMenuConfig.m_isInContextMenu = true;
        sceneContextMenuConfig.m_rootTreeItem = m_config->CreateNodePaletteRoot();
        m_sceneContextMenu = aznew SceneContextMenu(GetEditorId(), this);
        m_sceneContextMenu->AddNodePaletteMenuAction(sceneContextMenuConfig);

        // Setup the context menu with node palette for proposing a new node
        // when dropping a connection in an empty space in the graph
        NodePaletteConfig nodeProposalConfig = m_config->m_nodePaletteConfig;
        nodeProposalConfig.m_isInContextMenu = true;
        nodeProposalConfig.m_rootTreeItem = m_config->CreateNodePaletteRoot();
        m_createNodeProposalContextMenu = aznew EditorContextMenu(GetEditorId(), this);
        m_createNodeProposalContextMenu->AddNodePaletteMenuAction(nodeProposalConfig);
    }

    void AssetEditorMainWindow::SetDropAreaText(AZStd::string_view text)
    {
        static_cast<AssetEditorCentralDockWindow*>(centralWidget())->GetEmptyDockWidget()->SetDragTargetText(text.data());
    }

    void AssetEditorMainWindow::closeEvent(QCloseEvent* event)
    {
        // Try to close all the open graphs first, and if any of them refuse, then
        // don't close our window
        if (!CloseAllEditors())
        {
            event->ignore();
            return;
        }

        DockMainWindow::closeEvent(event);
    }

    DockWidgetId AssetEditorMainWindow::CreateEditorDockWidget(const QString& title)
    {
        EditorDockWidget* dockWidget = CreateDockWidget(title, this);

        // Set the mime type on the new graph that was created.
        GraphId graphId = dockWidget->GetGraphId();
        GraphCanvas::SceneRequestBus::Event(graphId, &GraphCanvas::SceneRequests::SetMimeType, m_config->m_mimeType);

        AssetEditorCentralDockWindow* centralDockWindow = GetCentralDockWindow();
        centralDockWindow->OnEditorOpened(dockWidget);

        OnEditorOpened(dockWidget);

        return dockWidget->GetDockWidgetId();
    }

    EditorDockWidget* AssetEditorMainWindow::CreateDockWidget(const QString& title, QWidget* parent) const
    {
        return aznew EditorDockWidget(GetEditorId(), title, parent);
    }

    AZStd::vector<GraphCanvas::GraphId> AssetEditorMainWindow::GetOpenGraphIds()
    {
        AZStd::vector<GraphCanvas::GraphId> graphIds;

        for (EditorDockWidget* dockWidget : GetCentralDockWindow()->GetEditorDockWidgets())
        {
            graphIds.push_back(dockWidget->GetGraphId());
        }

        return graphIds;
    }

    const EditorId& AssetEditorMainWindow::GetEditorId() const
    {
        return m_config->m_editorId;
    }

    GraphCanvas::GraphId AssetEditorMainWindow::GetActiveGraphCanvasGraphId() const
    {
        return m_activeGraphId;
    }

    void AssetEditorMainWindow::SetDefaultLayout()
    {
        // Disable updates while we restore the layout to avoid temporary glitches
        // as the panes are moved around
        setUpdatesEnabled(false);

        ConfigureDefaultLayout();

        // Re-enable updates now that we've finished adjusting the layout
        setUpdatesEnabled(true);
    }

    bool AssetEditorMainWindow::ConfigureDefaultLayout()
    {
        // Close our dock widgets first, if they refuse then stop restoring the default layout
        if (m_nodePalette && !m_nodePalette->close())
        {
            return false;
        }
        if (m_bookmarkDockWidget && !m_bookmarkDockWidget->close())
        {
            return false;
        }

        if (m_nodePalette)
        {
            addDockWidget(Qt::LeftDockWidgetArea, m_nodePalette);
            m_nodePalette->setFloating(false);
            m_nodePalette->show();

            resizeDocks({ m_nodePalette }, { static_cast<int>(size().width() * DEFAULT_NODE_PALETTE_SIZE) }, Qt::Horizontal);
        }

        if (m_bookmarkDockWidget)
        {
            // The Bookmarks panel won't be shown by default, but can be toggled on in the View menu
            addDockWidget(Qt::RightDockWidgetArea, m_bookmarkDockWidget);
            m_bookmarkDockWidget->setFloating(false);
            m_bookmarkDockWidget->hide();
        }

        return true;
    }

    void AssetEditorMainWindow::OnEditorOpened(EditorDockWidget* /*dockWidget*/)
    {
    }

    void AssetEditorMainWindow::OnEditorClosing(EditorDockWidget* /*dockWidget*/)
    {
    }

    bool AssetEditorMainWindow::CloseEditor(DockWidgetId dockWidgetId)
    {
        EditorDockWidget* editorDockWidget;
        EditorDockWidgetRequestBus::EventResult(editorDockWidget, dockWidgetId, &EditorDockWidgetRequests::AsEditorDockWidget);
        if (editorDockWidget)
        {
            return editorDockWidget->close();
        }

        return false;
    }

    bool AssetEditorMainWindow::CloseAllEditors()
    {
        return GetCentralDockWindow()->CloseAllEditors();
    }

    bool AssetEditorMainWindow::FocusDockWidget(DockWidgetId dockWidgetId)
    {
        // There are several possible scenarios for the dock wiget we are trying to focus:
        // If there's only one dock widget, it is docked normally by itself.  If there are
        // multiple, then they will be tabbed.  Additionally, whether there are one or multiple,
        // the specified dock widget could be floating separately.  Making sure we show/focus/raise
        // the dock widget will ensure it is shown in every possible scenario.
        EditorDockWidget* editorDockWidget;
        EditorDockWidgetRequestBus::EventResult(editorDockWidget, dockWidgetId, &EditorDockWidgetRequests::AsEditorDockWidget);
        if (editorDockWidget)
        {
            editorDockWidget->show();
            editorDockWidget->setFocus();
            editorDockWidget->raise();

            return true;
        }

        return false;
    }

    void AssetEditorMainWindow::RefreshMenu()
    {
        for (QAction* action : actions())
        {
            removeAction(action);
            action->deleteLater();
        }

        menuBar()->clear();

        AddFileMenu();
        AddEditMenu();
        AddViewMenu();

        UpdateMenuActions();

        QObject::connect(QApplication::clipboard(), &QClipboard::dataChanged, this, &AssetEditorMainWindow::UpdatePasteAction);
    }

    QMenu* AssetEditorMainWindow::AddFileMenu()
    {
        QMenu* menu = menuBar()->addMenu(QObject::tr("&File"));

        AddFileNewAction(menu);
        AddFileOpenAction(menu);

        menu->addSeparator();

        AddFileSaveAction(menu);
        AddFileSaveAsAction(menu);
        AddFileCloseAction(menu);

        return menu;
    }

    QAction* AssetEditorMainWindow::AddFileNewAction(QMenu* menu)
    {
        QAction* action = new QAction(QObject::tr("&New Asset"), this);
        action->setShortcut(QKeySequence::New);
        action->setShortcutContext(Qt::WidgetWithChildrenShortcut);
        addAction(action);
        QObject::connect(action, &QAction::triggered, [this] {
            CreateEditorDockWidget();
        });
        menu->addAction(action);

        return action;
    }

    QAction* AssetEditorMainWindow::AddFileOpenAction(QMenu* menu)
    {
        // Currently unused
        QAction* action = new QAction(QObject::tr("&Open"), this);
        action->setShortcut(QKeySequence::Open);
        action->setShortcutContext(Qt::WidgetWithChildrenShortcut);
        menu->addAction(action);

        return action;
    }

    QAction* AssetEditorMainWindow::AddFileSaveAction(QMenu* menu)
    {
        // Currently unused
        QAction* action = new QAction(QObject::tr("&Save"), this);
        action->setShortcut(QKeySequence::Save);
        action->setShortcutContext(Qt::WidgetWithChildrenShortcut);
        menu->addAction(action);

        return action;
    }

    QAction* AssetEditorMainWindow::AddFileSaveAsAction(QMenu* menu)
    {
        // Currently unused
        QAction* action = new QAction(QObject::tr("&Save As..."), this);
        action->setShortcut(QKeySequence(QObject::tr("Ctrl+Shift+S")));
        action->setShortcutContext(Qt::WidgetWithChildrenShortcut);
        menu->addAction(action);

        return action;
    }

    QAction* AssetEditorMainWindow::AddFileCloseAction(QMenu* menu)
    {
        QAction* action = new QAction(QObject::tr("Close"), this);
        action->setShortcut(QKeySequence::Close);
        action->setShortcutContext(Qt::WidgetWithChildrenShortcut);
        QObject::connect(action, &QAction::triggered, [this] {
            // We actually need to close the parent QDockWidget
            parentWidget()->close();
        });
        menu->addAction(action);

        return action;
    }

    QMenu* AssetEditorMainWindow::AddEditMenu()
    {
        QMenu* menu = menuBar()->addMenu(QObject::tr("&Edit"));

        AddEditCutAction(menu);
        AddEditCopyAction(menu);
        AddEditPasteAction(menu);
        AddEditDuplicateAction(menu);
        AddEditDeleteAction(menu);

        return menu;
    }

    QAction* AssetEditorMainWindow::AddEditCutAction(QMenu* menu)
    {
        m_cutSelectedAction = new QAction(QObject::tr("Cut"), this);
        m_cutSelectedAction->setShortcut(QKeySequence::Cut);
        m_cutSelectedAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
        addAction(m_cutSelectedAction);
        QObject::connect(m_cutSelectedAction, &QAction::triggered, [this] {
            SceneRequestBus::Event(GetActiveGraphCanvasGraphId(), &SceneRequests::CutSelection);
        });
        menu->addAction(m_cutSelectedAction);

        return m_cutSelectedAction;
    }

    QAction* AssetEditorMainWindow::AddEditCopyAction(QMenu* menu)
    {
        m_copySelectedAction = new QAction(QObject::tr("Copy"), this);
        m_copySelectedAction->setShortcut(QKeySequence::Copy);
        m_copySelectedAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
        addAction(m_copySelectedAction);
        QObject::connect(m_copySelectedAction, &QAction::triggered, [this] {
            SceneRequestBus::Event(GetActiveGraphCanvasGraphId(), &SceneRequests::CopySelection);
        });
        menu->addAction(m_copySelectedAction);

        return m_copySelectedAction;
    }

    QAction* AssetEditorMainWindow::AddEditPasteAction(QMenu* menu)
    {
        m_pasteSelectedAction = new QAction(QObject::tr("Paste"), this);
        m_pasteSelectedAction->setShortcut(QKeySequence::Paste);
        m_pasteSelectedAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
        addAction(m_pasteSelectedAction);
        QObject::connect(m_pasteSelectedAction, &QAction::triggered, [this] {
            SceneRequestBus::Event(GetActiveGraphCanvasGraphId(), &SceneRequests::Paste);
        });
        menu->addAction(m_pasteSelectedAction);

        return m_pasteSelectedAction;
    }

    QAction* AssetEditorMainWindow::AddEditDuplicateAction(QMenu* menu)
    {
        m_duplicateSelectedAction = new QAction(QObject::tr("Duplicate"), this);
        m_duplicateSelectedAction->setShortcut(QKeySequence("Ctrl+D"));
        m_duplicateSelectedAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
        addAction(m_duplicateSelectedAction);
        QObject::connect(m_duplicateSelectedAction, &QAction::triggered, [this] {
            SceneRequestBus::Event(GetActiveGraphCanvasGraphId(), &SceneRequests::DuplicateSelection);
        });
        menu->addAction(m_duplicateSelectedAction);

        return m_duplicateSelectedAction;
    }

    QAction* AssetEditorMainWindow::AddEditDeleteAction(QMenu* menu)
    {
        m_deleteSelectedAction = new QAction(QObject::tr("Delete"), this);
        m_deleteSelectedAction->setShortcut(QKeySequence::Delete);
        m_deleteSelectedAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
        addAction(m_deleteSelectedAction);
        QObject::connect(m_deleteSelectedAction, &QAction::triggered, [this] {
            SceneRequestBus::Event(GetActiveGraphCanvasGraphId(), &SceneRequests::DeleteSelection);
        });
        menu->addAction(m_deleteSelectedAction);

        return m_deleteSelectedAction;
    }

    void AssetEditorMainWindow::UpdateMenuActions()
    {
        UpdateEditMenuActions();
        UpdatePasteAction();
    }

    void AssetEditorMainWindow::UpdateEditMenuActions()
    {
        bool hasSelection = false;
        bool hasCopiableSelection = false;
        GraphId activeGraphId = GetActiveGraphCanvasGraphId();
        if (activeGraphId.IsValid())
        {
            AzToolsFramework::EntityIdList selectedItems;
            SceneRequestBus::EventResult(selectedItems, activeGraphId, &SceneRequests::GetSelectedItems);

            hasSelection = !selectedItems.empty();

            SceneRequestBus::EventResult(hasCopiableSelection, activeGraphId, &GraphCanvas::SceneRequests::HasCopiableSelection);
        }

        // Cut/Copy/Duplicate only works for specified items
        if (m_cutSelectedAction)
        {
            m_cutSelectedAction->setEnabled(hasCopiableSelection);
        }
        if (m_copySelectedAction)
        {
            m_copySelectedAction->setEnabled(hasCopiableSelection);
        }
        if (m_duplicateSelectedAction)
        {
            m_duplicateSelectedAction->setEnabled(hasCopiableSelection);
        }

        // Delete will work for anything that is selected
        if (m_deleteSelectedAction)
        {
            m_deleteSelectedAction->setEnabled(hasSelection);
        }
    }

    void AssetEditorMainWindow::UpdatePasteAction()
    {
        if (!m_pasteSelectedAction)
        {
            return;
        }

        // Enable the Paste action if the clipboard (if any) has a mime type that we support
        AZStd::string copyMimeType;
        GraphCanvas::SceneRequestBus::EventResult(copyMimeType, GetActiveGraphCanvasGraphId(), &GraphCanvas::SceneRequests::GetCopyMimeType);

        const bool pasteableClipboard = !copyMimeType.empty() && QApplication::clipboard()->mimeData()->hasFormat(copyMimeType.c_str());
        m_pasteSelectedAction->setEnabled(pasteableClipboard);
    }

    QMenu* AssetEditorMainWindow::AddViewMenu()
    {
        QMenu* menu = menuBar()->addMenu("&View");

        {
            // Automatically find any dock widgets for our main window and create checkable
            // menu options that will show/hide them
            QList<QDockWidget*> dockWidgets = findChildren<QDockWidget*>(QString(), Qt::FindDirectChildrenOnly);
            for (QDockWidget* dockWidget : dockWidgets)
            {
                QString name = dockWidget->windowTitle();
                QAction* action = new QAction(name, this);
                action->setCheckable(true);
                action->setChecked(dockWidget->isVisible());
                QObject::connect(dockWidget, &QDockWidget::visibilityChanged, action, &QAction::setChecked);
                QObject::connect(action, &QAction::triggered, [dockWidget, this](bool checked) {
                    if (checked)
                    {
                        // If the dock widget is tabbed, then set it as the active tab
                        AzQtComponents::DockTabWidget* tabWidget = AzQtComponents::DockTabWidget::ParentTabWidget(dockWidget);
                        if (tabWidget)
                        {
                            int index = tabWidget->indexOf(dockWidget);
                            tabWidget->setCurrentIndex(index);
                        }
                        // Otherwise just show the widget
                        else
                        {
                            m_fancyDockingManager->restoreDockWidget(dockWidget);
                            dockWidget->show();
                        }
                    }
                    else
                    {
                        dockWidget->hide();
                    }
                });
                menu->addAction(action);
            }
        }

        menu->addSeparator();

        {
            QAction* action = new QAction("Restore Default Layout", this);
            QObject::connect(action, &QAction::triggered, this, &AssetEditorMainWindow::SetDefaultLayout);
            menu->addAction(action);
        }

        return menu;
    }

    AZ::EntityId AssetEditorMainWindow::CreateNewGraph()
    {
        DockWidgetId dockWidgetId = CreateEditorDockWidget();
        GraphId graphId;
        EditorDockWidgetRequestBus::EventResult(graphId, dockWidgetId, &EditorDockWidgetRequests::GetGraphId);

        return graphId;
    }

    bool AssetEditorMainWindow::ContainsGraph(const GraphCanvas::GraphId& graphId) const
    {
        return GetCentralDockWindow()->GetEditorDockWidgetByGraphId(graphId);
    }

    bool AssetEditorMainWindow::CloseGraph(const GraphId& graphId)
    {
        EditorDockWidget* dockWidget = GetCentralDockWindow()->GetEditorDockWidgetByGraphId(graphId);
        if (dockWidget)
        {
            return CloseEditor(dockWidget->GetDockWidgetId());
        }

        return false;
    }

    ContextMenuAction::SceneReaction AssetEditorMainWindow::ShowSceneContextMenu(const QPoint& screenPoint, const QPointF& scenePoint)
    {
        m_sceneContextMenu->ResetSourceSlotFilter();

        // We pass an invalid EntityId here since this is for the scene, there is no member to specify.
        return HandleContextMenu(*m_sceneContextMenu, AZ::EntityId(), screenPoint, scenePoint);
    }

    ContextMenuAction::SceneReaction AssetEditorMainWindow::ShowNodeContextMenu(const AZ::EntityId& nodeId, const QPoint& screenPoint, const QPointF& scenePoint)
    {
        NodeContextMenu contextMenu(GetEditorId());
        return HandleContextMenu(contextMenu, nodeId, screenPoint, scenePoint);
    }

    ContextMenuAction::SceneReaction AssetEditorMainWindow::ShowCommentContextMenu(const AZ::EntityId& nodeId, const QPoint& screenPoint, const QPointF& scenePoint)
    {
        CommentContextMenu contextMenu(GetEditorId());
        return HandleContextMenu(contextMenu, nodeId, screenPoint, scenePoint);
    }

    ContextMenuAction::SceneReaction AssetEditorMainWindow::ShowNodeGroupContextMenu(const AZ::EntityId& groupId, const QPoint& screenPoint, const QPointF& scenePoint)
    {
        NodeGroupContextMenu contextMenu(GetEditorId());
        return HandleContextMenu(contextMenu, groupId, screenPoint, scenePoint);
    }

    ContextMenuAction::SceneReaction AssetEditorMainWindow::ShowCollapsedNodeGroupContextMenu(const AZ::EntityId& nodeId, const QPoint& screenPoint, const QPointF& scenePoint)
    {
        CollapsedNodeGroupContextMenu contextMenu(GetEditorId());
        return HandleContextMenu(contextMenu, nodeId, screenPoint, scenePoint);
    }

    ContextMenuAction::SceneReaction AssetEditorMainWindow::ShowBookmarkContextMenu(const AZ::EntityId& bookmarkId, const QPoint& screenPoint, const QPointF& scenePoint)
    {
        BookmarkContextMenu contextMenu(GetEditorId());
        return HandleContextMenu(contextMenu, bookmarkId, screenPoint, scenePoint);
    }

    ContextMenuAction::SceneReaction AssetEditorMainWindow::ShowConnectionContextMenu(const AZ::EntityId& connectionId, const QPoint& screenPoint, const QPointF& scenePoint)
    {
        ConnectionContextMenu contextMenu(GetEditorId());
        return HandleContextMenu(contextMenu, connectionId, screenPoint, scenePoint);
    }

    ContextMenuAction::SceneReaction AssetEditorMainWindow::ShowSlotContextMenu(const AZ::EntityId& slotId, const QPoint& screenPoint, const QPointF& scenePoint)
    {
        SlotContextMenu contextMenu(GetEditorId());
        return HandleContextMenu(contextMenu, slotId, screenPoint, scenePoint);
    }

    ContextMenuAction::SceneReaction AssetEditorMainWindow::HandleContextMenu(EditorContextMenu& editorContextMenu, const AZ::EntityId& memberId, const QPoint& screenPoint, const QPointF& scenePoint) const
    {
        AZ::Vector2 sceneVector(aznumeric_cast<float>(scenePoint.x()), aznumeric_cast<float>(scenePoint.y()));
        GraphId graphCanvasGraphId = GetActiveGraphCanvasGraphId();

        editorContextMenu.RefreshActions(graphCanvasGraphId, memberId);

        QAction* result = editorContextMenu.exec(screenPoint);

        ContextMenuAction* contextMenuAction = qobject_cast<ContextMenuAction*>(result);

        if (contextMenuAction)
        {
            return contextMenuAction->TriggerAction(graphCanvasGraphId, sceneVector);
        }
        else if (editorContextMenu.GetNodePalette())
        {
            // Handle creating node from any node palette embedded in an EditorContextMenu.
            GraphCanvasMimeEvent* mimeEvent = editorContextMenu.GetNodePalette()->GetContextMenuEvent();
            if (mimeEvent)
            {
                AZ::Vector2 dropPos(aznumeric_cast<float>(scenePoint.x()), aznumeric_cast<float>(scenePoint.y()));
                if (mimeEvent->ExecuteEvent(dropPos, dropPos, graphCanvasGraphId))
                {
                    NodeId nodeId = mimeEvent->GetCreatedNodeId();
                    if (nodeId.IsValid())
                    {
                        GraphCanvas::SceneRequestBus::Event(graphCanvasGraphId, &GraphCanvas::SceneRequests::ClearSelection);
                        VisualRequestBus::Event(nodeId, &VisualRequests::SetVisible, true);

                        GraphCanvas::SceneNotificationBus::Event(graphCanvasGraphId, &GraphCanvas::SceneNotifications::PostCreationEvent);
                    }
                }
            }
        }

        return ContextMenuAction::SceneReaction::Nothing;
    }

    Endpoint AssetEditorMainWindow::HandleProposedConnection(const GraphId& /*graphId*/, const ConnectionId& /*connectionId*/, const Endpoint& endpoint, const NodeId& proposedNode, const QPoint& screenPoint)
    {
        Endpoint retVal;

        ConnectionType connectionType = ConnectionType::CT_Invalid;
        SlotRequestBus::EventResult(connectionType, endpoint.GetSlotId(), &SlotRequests::GetConnectionType);

        NodeId currentTarget = proposedNode;

        while (!retVal.IsValid() && currentTarget.IsValid())
        {
            AZStd::vector<AZ::EntityId> targetSlotIds;
            NodeRequestBus::EventResult(targetSlotIds, currentTarget, &NodeRequests::GetSlotIds);

            // Find the list of endpoints on the created node that could create a valid connection
            // with the specified slot
            AZStd::list<Endpoint> endpoints;
            for (const auto& targetSlotId : targetSlotIds)
            {
                Endpoint proposedEndpoint(currentTarget, targetSlotId);

                bool canCreate = false;
                SlotRequestBus::EventResult(canCreate, endpoint.GetSlotId(), &SlotRequests::CanCreateConnectionTo, proposedEndpoint);

                if (canCreate)
                {
                    SlotGroup slotGroup = SlotGroups::Invalid;
                    SlotRequestBus::EventResult(slotGroup, targetSlotId, &SlotRequests::GetSlotGroup);

                    bool isVisible = slotGroup != SlotGroups::Invalid;
                    SlotLayoutRequestBus::EventResult(isVisible, currentTarget, &SlotLayoutRequests::IsSlotGroupVisible, slotGroup);

                    if (isVisible)
                    {
                        endpoints.push_back(proposedEndpoint);
                    }
                }
            }

            if (!endpoints.empty())
            {
                // If there is exactly one match, then we can just use that endpoint.
                if (endpoints.size() == 1)
                {
                    retVal = endpoints.front();
                }
                // Otherwise, since there are multiple possible matches, we need to display a simple menu for the
                // user to select which slot they want they want to be connected to the proposed endpoint.
                else
                {
                    QMenu menu;

                    for (Endpoint proposedEndpoint : endpoints)
                    {
                        QAction* action = aznew GraphCanvas::EndpointSelectionAction(proposedEndpoint);
                        menu.addAction(action);
                    }

                    QAction* result = menu.exec(screenPoint);

                    if (result)
                    {
                        auto* selectedEnpointAction = static_cast<GraphCanvas::EndpointSelectionAction*>(result);
                        retVal = selectedEnpointAction->GetEndpoint();
                    }
                    else
                    {
                        retVal.Clear();
                    }
                }

                if (retVal.IsValid())
                {
                    // Double safety check. This should be guaranteed by the previous checks. But just extra safety.
                    bool canCreateConnection = false;
                    SlotRequestBus::EventResult(canCreateConnection, endpoint.GetSlotId(), &SlotRequests::CanCreateConnectionTo, retVal);

                    if (!canCreateConnection)
                    {
                        retVal.Clear();
                    }
                }
            }
            else
            {
                retVal.Clear();
            }

            if (!retVal.IsValid())
            {
                bool isWrapped = false;
                NodeRequestBus::EventResult(isWrapped, currentTarget, &NodeRequests::IsWrapped);

                if (isWrapped)
                {
                    NodeRequestBus::EventResult(currentTarget, currentTarget, &NodeRequests::GetWrappingNode);
                }
                else
                {
                    currentTarget.SetInvalid();
                }
            }
        }

        return retVal;
    }

    Endpoint AssetEditorMainWindow::CreateNodeForProposal(const AZ::EntityId& connectionId, const Endpoint& endpoint, const QPointF& scenePoint, const QPoint& screenPoint)
    {
        GraphCanvas::Endpoint retVal;

        AZ::EntityId graphCanvasGraphId = GetActiveGraphCanvasGraphId();

        m_createNodeProposalContextMenu->FilterForSourceSlot(graphCanvasGraphId, endpoint.GetSlotId());
        m_createNodeProposalContextMenu->RefreshActions(graphCanvasGraphId, connectionId);

        m_createNodeProposalContextMenu->exec(screenPoint);

        GraphCanvasMimeEvent* mimeEvent = m_createNodeProposalContextMenu->GetNodePalette()->GetContextMenuEvent();
        if (mimeEvent)
        {
            AZ::Vector2 dropPos(aznumeric_cast<float>(scenePoint.x()), aznumeric_cast<float>(scenePoint.y()));
            if (mimeEvent->ExecuteEvent(dropPos, dropPos, graphCanvasGraphId))
            {
                NodeId nodeId = mimeEvent->GetCreatedNodeId();
                if (nodeId.IsValid())
                {
                    VisualRequestBus::Event(nodeId, &VisualRequests::SetVisible, false);
                    retVal = HandleProposedConnection(graphCanvasGraphId, connectionId, endpoint, nodeId, screenPoint);
                }

                if (retVal.IsValid())
                {
                    GraphUtils::CreateOpportunisticConnectionsBetween(endpoint, retVal);
                    VisualRequestBus::Event(nodeId, &VisualRequests::SetVisible, true);

                    AZ::Vector2 position;
                    GeometryRequestBus::EventResult(position, retVal.GetNodeId(), &GeometryRequests::GetPosition);

                    QPointF connectionPoint;
                    SlotUIRequestBus::EventResult(connectionPoint, retVal.GetSlotId(), &SlotUIRequests::GetConnectionPoint);

                    qreal verticalOffset = connectionPoint.y() - position.GetY();
                    position.SetY(aznumeric_cast<float>(scenePoint.y() - verticalOffset));

                    qreal horizontalOffset = connectionPoint.x() - position.GetX();
                    position.SetX(aznumeric_cast<float>(scenePoint.x() - horizontalOffset));

                    GeometryRequestBus::Event(retVal.GetNodeId(), &GeometryRequests::SetPosition, position);

                    SceneNotificationBus::Event(graphCanvasGraphId, &SceneNotifications::PostCreationEvent);
                }
                else
                {
                    GraphUtils::DeleteOutermostNode(graphCanvasGraphId, nodeId);
                }
            }
        }

        return retVal;
    }

    void AssetEditorMainWindow::OnWrapperNodeActionWidgetClicked(const AZ::EntityId& /*wrapperNode*/, const QRect& /*actionWidgetBoundingRect*/, const QPointF& /*scenePoint*/, const QPoint& /*screenPoint*/)
    {
    }

    EditorConstructPresets* AssetEditorMainWindow::GetConstructPresets() const
    {
        return const_cast<EditorConstructPresets*>(&m_constructPresetDefaults);
    }

    const ConstructTypePresetBucket* AssetEditorMainWindow::GetConstructTypePresetBucket(ConstructType constructType) const
    {
        return m_constructPresetDefaults.FindPresetBucket(constructType);
    }

    void AssetEditorMainWindow::OnActiveGraphChanged(const GraphId& graphId)
    {
        m_activeGraphId = graphId;

        SceneNotificationBus::Handler::BusDisconnect();
        SceneNotificationBus::Handler::BusConnect(m_activeGraphId);

        UpdateMenuActions();
    }

    void AssetEditorMainWindow::OnSelectionChanged()
    {
        UpdateEditMenuActions();
    }

    void AssetEditorMainWindow::RestoreWindowState()
    {
        // If there's no last saved layout, then restore the default
        QByteArray state = m_settings->GetLastWindowState();
        if (state.isEmpty())
        {
            SetDefaultLayout();
            return;
        }

        m_fancyDockingManager->restoreState(state);
    }

    void AssetEditorMainWindow::SaveWindowState()
    {
        m_settings->SetLastWindowState(m_fancyDockingManager->saveState());
    }

    AssetEditorCentralDockWindow* AssetEditorMainWindow::GetCentralDockWindow() const
    {
        return static_cast<AssetEditorCentralDockWindow*>(centralWidget());
    }
}
