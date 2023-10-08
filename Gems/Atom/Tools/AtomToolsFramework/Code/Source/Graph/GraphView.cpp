/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomToolsFramework/Graph/GraphView.h>
#include <AtomToolsFramework/Util/Util.h>
#include <AzCore/IO/FileIO.h>
#include <AzQtComponents/Components/StyleManager.h>
#include <GraphCanvas/Components/Connections/ConnectionBus.h>
#include <GraphCanvas/Components/MimeDataHandlerBus.h>
#include <GraphCanvas/Components/Nodes/NodeBus.h>
#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/ViewBus.h>
#include <GraphCanvas/GraphCanvasBus.h>
#include <GraphCanvas/Types/ConstructPresets.h>
#include <GraphCanvas/Widgets/AssetEditorToolbar/AssetEditorToolbar.h>
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
#include <GraphCanvas/Widgets/GraphCanvasMimeContainer.h>
#include <GraphCanvas/Widgets/NodePalette/TreeItems/IconDecoratedNodePaletteTreeItem.h>
#include <GraphCanvas/Widgets/NodePalette/TreeItems/NodePaletteTreeItem.h>

#include <QApplication>
#include <QClipboard>
#include <QKeyEvent>
#include <QKeySequence>
#include <QMenu>
#include <QMenuBar>
#include <QMimeData>
#include <QVBoxLayout>
#include <QWindow>

namespace AtomToolsFramework
{
    GraphView::GraphView(
        const AZ::Crc32& toolId, const GraphCanvas::GraphId& activeGraphId, GraphViewSettingsPtr graphViewSettingsPtr, QWidget* parent)
        : QWidget(parent)
        , m_toolId(toolId)
        , m_graphViewSettingsPtr(graphViewSettingsPtr)
    {
        setLayout(new QVBoxLayout());

        m_editorToolbar = aznew GraphCanvas::AssetEditorToolbar(m_toolId);
        m_editorToolbar->setParent(this);
        layout()->addWidget(m_editorToolbar);

        // Screenshot
        m_takeScreenshot = new QToolButton(m_editorToolbar);
        m_takeScreenshot->setToolTip("Captures a full resolution screenshot of the entire graph or selected nodes into the clipboard");
        m_takeScreenshot->setIcon(QIcon(":/Icons/screenshot.png"));
        m_takeScreenshot->setEnabled(false);
        connect(m_takeScreenshot, &QToolButton::clicked, this, [this](){
            GraphCanvas::ViewId viewId;
            GraphCanvas::SceneRequestBus::EventResult(viewId, m_activeGraphId, &GraphCanvas::SceneRequests::GetViewId);
            GraphCanvas::ViewRequestBus::Event(viewId, &GraphCanvas::ViewRequests::ScreenshotSelection);
        });
        m_editorToolbar->AddCustomAction(m_takeScreenshot);

        m_graphicsView = new GraphCanvas::GraphCanvasGraphicsView(this, false);
        m_graphicsView->SetEditorId(m_toolId);
        layout()->addWidget(m_graphicsView);

        m_presetEditor = aznew GraphCanvas::ConstructPresetDialog(this);
        m_presetEditor->SetEditorId(m_toolId);

        m_presetWrapper = new AzQtComponents::WindowDecorationWrapper(AzQtComponents::WindowDecorationWrapper::OptionAutoTitleBarButtons, this);
        m_presetWrapper->setGuest(m_presetEditor);
        m_presetWrapper->hide();

        // Add a node palette for creating new nodes to the default scene context menu,
        // which is what is displayed when right-clicking on an empty space in the graph
        GraphCanvas::NodePaletteConfig nodePaletteConfig;
        nodePaletteConfig.m_editorId = m_toolId;
        nodePaletteConfig.m_mimeType = m_graphViewSettingsPtr->m_nodeMimeType.c_str();
        nodePaletteConfig.m_isInContextMenu = true;
        nodePaletteConfig.m_saveIdentifier = m_graphViewSettingsPtr->m_nodeSaveIdentifier;
        nodePaletteConfig.m_rootTreeItem = m_graphViewSettingsPtr->m_createNodeTreeItemsFn(m_toolId);
        m_sceneContextMenu = aznew GraphCanvas::SceneContextMenu(m_toolId, this);
        m_sceneContextMenu->AddNodePaletteMenuAction(nodePaletteConfig);

        // Setup the context menu with node palette for proposing a new node
        // when dropping a connection in an empty space in the graph
        nodePaletteConfig.m_rootTreeItem = m_graphViewSettingsPtr->m_createNodeTreeItemsFn(m_toolId);
        m_createNodeProposalContextMenu = aznew GraphCanvas::EditorContextMenu(m_toolId, this);
        m_createNodeProposalContextMenu->AddNodePaletteMenuAction(nodePaletteConfig);

        // Set up style sheet to fix highlighting in node palettes
        AzQtComponents::StyleManager::setStyleSheet(
            const_cast<GraphCanvas::NodePaletteWidget*>(m_sceneContextMenu->GetNodePalette()),
            QStringLiteral(":/GraphView/GraphView.qss"));

        AzQtComponents::StyleManager::setStyleSheet(
            const_cast<GraphCanvas::NodePaletteWidget*>(m_createNodeProposalContextMenu->GetNodePalette()),
            QStringLiteral(":/GraphView/GraphView.qss"));

        CreateActions();

        SetActiveGraphId(activeGraphId, true);
    }

    GraphView::~GraphView()
    {
        SetActiveGraphId(GraphCanvas::GraphId(), false);
        delete m_presetEditor;
    }

    void GraphView::SetActiveGraphId(const GraphCanvas::GraphId& activeGraphId, bool notify)
    {
        // Disconnect from any previously connecting buses.
        // We are enforcing that only one graph is active and connected at any given time.
        AtomToolsMainMenuRequestBus::Handler::BusDisconnect();
        GraphCanvas::AssetEditorRequestBus::Handler::BusDisconnect();
        GraphCanvas::SceneNotificationBus::Handler::BusDisconnect();

        // Update the value of the active graph ID and only reconnect the buses if it's valid.
        m_activeGraphId = activeGraphId;

        // Valid or not, update the graphics view to reference the new ID
        m_graphicsView->SetScene(m_activeGraphId);

        if (m_activeGraphId.IsValid())
        {
            AtomToolsMainMenuRequestBus::Handler::BusConnect(m_toolId);
            GraphCanvas::AssetEditorRequestBus::Handler::BusConnect(m_toolId);
            GraphCanvas::SceneNotificationBus::Handler::BusConnect(m_activeGraphId);

            GraphCanvas::SceneRequestBus::Event(
                m_activeGraphId, &GraphCanvas::SceneRequests::SetMimeType, m_graphViewSettingsPtr->m_nodeMimeType.c_str());
        }

        if (notify)
        {
            // Notify any observers connected to the asset editor buses that the active graph has changed.
            // We are only managing one graph at a time, not using the asset editor buses, but this will update any other system that is.
            GraphCanvas::AssetEditorNotificationBus::Event(m_toolId, &GraphCanvas::AssetEditorNotifications::PreOnActiveGraphChanged);
            GraphCanvas::AssetEditorNotificationBus::Event(m_toolId, &GraphCanvas::AssetEditorNotifications::OnActiveGraphChanged, m_activeGraphId);
            GraphCanvas::AssetEditorNotificationBus::Event(m_toolId, &GraphCanvas::AssetEditorNotifications::PostOnActiveGraphChanged);
        }

        // Update main window menus with commands from this view.
        AtomToolsMainWindowRequestBus::Event(m_toolId, &AtomToolsMainWindowRequestBus::Events::QueueUpdateMenus, true);
    }


    void GraphView::CreateActions()
    {
        auto makeAction =
            [&](const QString& menuName, const QString& name, const AZStd::function<void()>& fn, const QKeySequence& shortcut) -> QAction*
        {
            QAction* action = new QAction(name, this);
            action->setShortcut(shortcut);
            action->setShortcutContext(Qt::WidgetWithChildrenShortcut);
            action->setProperty("menuName", menuName);
            QObject::connect(action, &QAction::triggered, this, fn);
            addAction(action);
            return action;
        };
        auto makeSeperator =
            [&](const QString& menuName) -> QAction*
        {
            QAction* action = new QAction(this);
            action->setSeparator(true);
            action->setProperty("menuName", menuName);
            addAction(action);
            return action;
        };

        makeSeperator("menuEdit");
        m_actionCut = makeAction("menuEdit", tr("Cut"), [this](){
            GraphCanvas::ScopedGraphUndoBatch undoBatch(m_activeGraphId);
            GraphCanvas::SceneRequestBus::Event(m_activeGraphId, &GraphCanvas::SceneRequests::CutSelection);
        }, QKeySequence::Cut);
        m_actionCopy = makeAction("menuEdit", tr("Copy"), [this](){
            GraphCanvas::SceneRequestBus::Event(m_activeGraphId, &GraphCanvas::SceneRequests::CopySelection);
        }, QKeySequence::Copy);
        m_actionPaste = makeAction("menuEdit", tr("Paste"), [this](){
            GraphCanvas::ScopedGraphUndoBatch undoBatch(m_activeGraphId);
            GraphCanvas::SceneRequestBus::Event(m_activeGraphId, &GraphCanvas::SceneRequests::Paste);
        }, QKeySequence::Paste);
        m_actionDuplicate = makeAction("menuEdit", tr("Duplicate"), [this](){
            GraphCanvas::ScopedGraphUndoBatch undoBatch(m_activeGraphId);
            GraphCanvas::SceneRequestBus::Event(m_activeGraphId, &GraphCanvas::SceneRequests::DuplicateSelection);
        }, QKeySequence(Qt::CTRL + Qt::Key_D));
        m_actionDelete = makeAction("menuEdit", tr("Delete"), [this](){
            GraphCanvas::ScopedGraphUndoBatch undoBatch(m_activeGraphId);
            GraphCanvas::SceneRequestBus::Event(m_activeGraphId, &GraphCanvas::SceneRequests::DeleteSelection);
        }, QKeySequence::Delete);

        makeSeperator("menuEdit");
        m_actionRemoveUnusedNodes = makeAction("menuEdit", tr("Remove Unused Nodes"), [this](){
            GraphCanvas::ScopedGraphUndoBatch undoBatch(m_activeGraphId);
            GraphCanvas::SceneRequestBus::Event(m_activeGraphId, &GraphCanvas::SceneRequests::RemoveUnusedNodes);
        }, {});
        m_actionRemoveUnusedElements = makeAction("menuEdit", tr("Remove Unused Elements"), [this](){
            GraphCanvas::ScopedGraphUndoBatch undoBatch(m_activeGraphId);
            GraphCanvas::SceneRequestBus::Event(m_activeGraphId, &GraphCanvas::SceneRequests::RemoveUnusedElements);
        }, {});

        makeSeperator("menuEdit");
        m_actionSelectAll = makeAction("menuEdit", tr("Select All"), [this](){
            GraphCanvas::SceneRequestBus::Event(m_activeGraphId, &GraphCanvas::SceneRequests::SelectAll);
        }, QKeySequence::SelectAll);
        m_actionSelectInputs = makeAction("menuEdit", tr("Select Inputs"), [this](){
            GraphCanvas::SceneRequestBus::Event(
                m_activeGraphId, &GraphCanvas::SceneRequests::SelectAllRelative, GraphCanvas::ConnectionType::CT_Input);
        }, QKeySequence(Qt::CTRL + Qt::Key_Left));
        m_actionSelectOutputs = makeAction("menuEdit", tr("Select Outputs"), [this](){
            GraphCanvas::SceneRequestBus::Event(
                m_activeGraphId, &GraphCanvas::SceneRequests::SelectAllRelative, GraphCanvas::ConnectionType::CT_Output);
        }, QKeySequence(Qt::CTRL + Qt::Key_Right));
        m_actionSelectConnected = makeAction("menuEdit", tr("Select Connected"), [this](){
            GraphCanvas::SceneRequestBus::Event(m_activeGraphId, &GraphCanvas::SceneRequests::SelectConnectedNodes);
        }, QKeySequence(Qt::CTRL + Qt::Key_Up));
        m_actionSelectNone = makeAction("menuEdit", tr("Clear Selection"), [this](){
            GraphCanvas::SceneRequestBus::Event(m_activeGraphId, &GraphCanvas::SceneRequests::ClearSelection);
        }, QKeySequence::Deselect);
        m_actionSelectEnable = makeAction("menuEdit", tr("Enable Selection"), [this](){
            GraphCanvas::SceneRequestBus::Event(m_activeGraphId, &GraphCanvas::SceneRequests::EnableSelection);
        }, QKeySequence(Qt::CTRL + Qt::Key_K, Qt::CTRL + Qt::Key_U));
        m_actionSelectDisable = makeAction("menuEdit", tr("Disable Selection"), [this](){
            GraphCanvas::SceneRequestBus::Event(m_activeGraphId, &GraphCanvas::SceneRequests::DisableSelection);
        }, QKeySequence(Qt::CTRL + Qt::Key_K, Qt::CTRL + Qt::Key_C));

        makeSeperator("menuEdit");
        m_actionScreenShot = makeAction("menuEdit", tr("Screenshot"), [this](){
            GraphCanvas::ViewId viewId;
            GraphCanvas::SceneRequestBus::EventResult(viewId, m_activeGraphId, &GraphCanvas::SceneRequests::GetViewId);
            GraphCanvas::ViewRequestBus::Event(viewId, &GraphCanvas::ViewRequests::ScreenshotSelection);
        }, QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_P));

        makeSeperator("menuEdit");
        m_actionAlignTop = makeAction("menuEdit", tr("Align Top"), [this](){
            GraphCanvas::AlignConfig alignConfig;
            alignConfig.m_horAlign = GraphCanvas::GraphUtils::HorizontalAlignment::None;
            alignConfig.m_verAlign = GraphCanvas::GraphUtils::VerticalAlignment::Top;
            GraphCanvas::AssetEditorSettingsRequestBus::EventResult(
                alignConfig.m_alignTime, m_toolId, &GraphCanvas::AssetEditorSettingsRequestBus::Events::GetAlignmentTime);
            AlignSelected(alignConfig);
        }, {});
        m_actionAlignBottom = makeAction("menuEdit", tr("Align Bottom"), [this](){
            GraphCanvas::AlignConfig alignConfig;
            alignConfig.m_horAlign = GraphCanvas::GraphUtils::HorizontalAlignment::None;
            alignConfig.m_verAlign = GraphCanvas::GraphUtils::VerticalAlignment::Bottom;
            GraphCanvas::AssetEditorSettingsRequestBus::EventResult(
                alignConfig.m_alignTime, m_toolId, &GraphCanvas::AssetEditorSettingsRequestBus::Events::GetAlignmentTime);
            AlignSelected(alignConfig);
        }, {});
        m_actionAlignLeft = makeAction("menuEdit", tr("Align Left"), [this](){
            GraphCanvas::AlignConfig alignConfig;
            alignConfig.m_horAlign = GraphCanvas::GraphUtils::HorizontalAlignment::Left;
            alignConfig.m_verAlign = GraphCanvas::GraphUtils::VerticalAlignment::None;
            GraphCanvas::AssetEditorSettingsRequestBus::EventResult(
                alignConfig.m_alignTime, m_toolId, &GraphCanvas::AssetEditorSettingsRequestBus::Events::GetAlignmentTime);
            AlignSelected(alignConfig);
        }, {});
        m_actionAlignRight = makeAction("menuEdit", tr("Align Right"), [this](){
            GraphCanvas::AlignConfig alignConfig;
            alignConfig.m_horAlign = GraphCanvas::GraphUtils::HorizontalAlignment::Right;
            alignConfig.m_verAlign = GraphCanvas::GraphUtils::VerticalAlignment::None;
            GraphCanvas::AssetEditorSettingsRequestBus::EventResult(
                alignConfig.m_alignTime, m_toolId, &GraphCanvas::AssetEditorSettingsRequestBus::Events::GetAlignmentTime);
            AlignSelected(alignConfig);
        }, {});

        makeSeperator("menuView");
        m_actionPresetEditor = makeAction("menuView", tr("Presets Editor"), [this](){ OpenPresetsEditor(); }, {});

        makeSeperator("menuView");
        m_actionShowEntireGraph = makeAction("menuView", tr("Show Entire Graph"), [this](){
            GraphCanvas::ViewId viewId;
            GraphCanvas::SceneRequestBus::EventResult(viewId, m_activeGraphId, &GraphCanvas::SceneRequests::GetViewId);
            GraphCanvas::ViewRequestBus::Event(viewId, &GraphCanvas::ViewRequests::ShowEntireGraph);
        }, QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_Down));
        m_actionZoomIn = makeAction("menuView", tr("Zoom In"), [this](){
            GraphCanvas::ViewId viewId;
            GraphCanvas::SceneRequestBus::EventResult(viewId, m_activeGraphId, &GraphCanvas::SceneRequests::GetViewId);
            GraphCanvas::ViewRequestBus::Event(viewId, &GraphCanvas::ViewRequests::ZoomIn);
        }, QKeySequence::ZoomIn);
        m_actionZoomOut = makeAction("menuView", tr("Zoom Out"), [this](){
            GraphCanvas::ViewId viewId;
            GraphCanvas::SceneRequestBus::EventResult(viewId, m_activeGraphId, &GraphCanvas::SceneRequests::GetViewId);
            GraphCanvas::ViewRequestBus::Event(viewId, &GraphCanvas::ViewRequests::ZoomOut);
        }, QKeySequence::ZoomOut);
        m_actionZoomSelection = makeAction("menuView", tr("Zoom Selection"), [this](){
            GraphCanvas::ViewId viewId;
            GraphCanvas::SceneRequestBus::EventResult(viewId, m_activeGraphId, &GraphCanvas::SceneRequests::GetViewId);
            GraphCanvas::ViewRequestBus::Event(viewId, &GraphCanvas::ViewRequests::CenterOnSelection);
        }, QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_Up));

        makeSeperator("menuView");
        m_actionGotoStartOfChain = makeAction("menuView", tr("Goto Start Of Chain"), [this](){
            GraphCanvas::ViewId viewId;
            GraphCanvas::SceneRequestBus::EventResult(viewId, m_activeGraphId, &GraphCanvas::SceneRequests::GetViewId);
            GraphCanvas::ViewRequestBus::Event(viewId, &GraphCanvas::ViewRequests::CenterOnStartOfChain);
        }, QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_Left));
        m_actionGotoEndOfChain = makeAction("menuView", tr("Goto End Of Chain"), [this](){
            GraphCanvas::ViewId viewId;
            GraphCanvas::SceneRequestBus::EventResult(viewId, m_activeGraphId, &GraphCanvas::SceneRequests::GetViewId);
            GraphCanvas::ViewRequestBus::Event(viewId, &GraphCanvas::ViewRequests::CenterOnEndOfChain);
        }, QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_Right));
    }

    void GraphView::CreateMenus(QMenuBar* menuBar)
    {
        for (auto action : actions())
        {
            if (auto menuName = action->property("menuName"); menuName.isValid())
            {
                if (auto menu = menuBar->findChild<QMenu*>(menuName.toString()))
                {
                    menu->addAction(action);
                }
            }
        }
    }

    void GraphView::UpdateMenus([[maybe_unused]] QMenuBar* menuBar)
    {
        const bool hasGraph = m_activeGraphId.IsValid();

        bool hasSelection = false;
        bool hasCopiableSelection = false;
        if (hasGraph)
        {
            AzToolsFramework::EntityIdList selectedItems;
            GraphCanvas::SceneRequestBus::EventResult(selectedItems, m_activeGraphId, &GraphCanvas::SceneRequests::GetSelectedItems);
            hasSelection = !selectedItems.empty();

            GraphCanvas::SceneRequestBus::EventResult(
                hasCopiableSelection, m_activeGraphId, &GraphCanvas::SceneRequests::HasCopiableSelection);
        }

        // Enable the Paste action if the clipboard (if any) has a mime type that we support
        AZStd::string copyMimeType;
        GraphCanvas::SceneRequestBus::EventResult(copyMimeType, m_activeGraphId, &GraphCanvas::SceneRequests::GetCopyMimeType);
        const bool canPaste = hasGraph && !copyMimeType.empty() && QApplication::clipboard()->mimeData()->hasFormat(copyMimeType.c_str());

        m_actionCut->setEnabled(hasCopiableSelection);
        m_actionCopy->setEnabled(hasCopiableSelection);
        m_actionPaste->setEnabled(canPaste);
        m_actionDelete->setEnabled(hasSelection);
        m_actionDuplicate->setEnabled(hasCopiableSelection);

        m_actionRemoveUnusedNodes->setEnabled(hasGraph);
        m_actionRemoveUnusedElements->setEnabled(hasGraph);

        m_actionSelectAll->setEnabled(hasGraph);
        m_actionSelectNone->setEnabled(hasSelection);
        m_actionSelectInputs->setEnabled(hasGraph);
        m_actionSelectOutputs->setEnabled(hasGraph);
        m_actionSelectConnected->setEnabled(hasGraph);
        m_actionSelectEnable->setEnabled(hasGraph);
        m_actionSelectDisable->setEnabled(hasGraph);

        m_actionScreenShot->setEnabled(hasGraph);

        m_actionAlignTop->setEnabled(hasSelection);
        m_actionAlignBottom->setEnabled(hasSelection);
        m_actionAlignLeft->setEnabled(hasSelection);
        m_actionAlignRight->setEnabled(hasSelection);

        m_actionPresetEditor->setEnabled(hasGraph);
        m_actionShowEntireGraph->setEnabled(hasGraph);
        m_actionZoomIn->setEnabled(hasGraph);
        m_actionZoomOut->setEnabled(hasGraph);
        m_actionZoomSelection->setEnabled(hasSelection);
        m_actionGotoStartOfChain->setEnabled(hasGraph);
        m_actionGotoEndOfChain->setEnabled(hasGraph);

        m_takeScreenshot->setEnabled(hasGraph);
    }

    AZ::s32 GraphView::GetMainMenuPriority() const
    {
        // Return a priority that will place menus for the view below menus for the main window
        return 1;
    }

    AZ::EntityId GraphView::CreateNewGraph()
    {
        return m_activeGraphId;
    }

    bool GraphView::ContainsGraph([[maybe_unused]] const GraphCanvas::GraphId& graphId) const
    {
        return m_activeGraphId == graphId;
    }

    bool GraphView::CloseGraph([[maybe_unused]] const GraphCanvas::GraphId& graphId)
    {
        return false;
    }

    GraphCanvas::ContextMenuAction::SceneReaction GraphView::ShowSceneContextMenu(const QPoint& screenPoint, const QPointF& scenePoint)
    {
        m_sceneContextMenu->ResetSourceSlotFilter();

        // We pass an invalid EntityId here since this is for the scene, there is no member to specify.
        return HandleContextMenu(*m_sceneContextMenu, AZ::EntityId(), screenPoint, scenePoint);
    }

    GraphCanvas::ContextMenuAction::SceneReaction GraphView::ShowNodeContextMenu(
        const AZ::EntityId& nodeId, const QPoint& screenPoint, const QPointF& scenePoint)
    {
        GraphCanvas::NodeContextMenu contextMenu(m_toolId);
        return HandleContextMenu(contextMenu, nodeId, screenPoint, scenePoint);
    }

    GraphCanvas::ContextMenuAction::SceneReaction GraphView::ShowCommentContextMenu(
        const AZ::EntityId& nodeId, const QPoint& screenPoint, const QPointF& scenePoint)
    {
        GraphCanvas::CommentContextMenu contextMenu(m_toolId);
        return HandleContextMenu(contextMenu, nodeId, screenPoint, scenePoint);
    }

    GraphCanvas::ContextMenuAction::SceneReaction GraphView::ShowNodeGroupContextMenu(
        const AZ::EntityId& groupId, const QPoint& screenPoint, const QPointF& scenePoint)
    {
        GraphCanvas::NodeGroupContextMenu contextMenu(m_toolId);
        return HandleContextMenu(contextMenu, groupId, screenPoint, scenePoint);
    }

    GraphCanvas::ContextMenuAction::SceneReaction GraphView::ShowCollapsedNodeGroupContextMenu(
        const AZ::EntityId& nodeId, const QPoint& screenPoint, const QPointF& scenePoint)
    {
        GraphCanvas::CollapsedNodeGroupContextMenu contextMenu(m_toolId);
        return HandleContextMenu(contextMenu, nodeId, screenPoint, scenePoint);
    }

    GraphCanvas::ContextMenuAction::SceneReaction GraphView::ShowBookmarkContextMenu(
        const AZ::EntityId& bookmarkId, const QPoint& screenPoint, const QPointF& scenePoint)
    {
        GraphCanvas::BookmarkContextMenu contextMenu(m_toolId);
        return HandleContextMenu(contextMenu, bookmarkId, screenPoint, scenePoint);
    }

    GraphCanvas::ContextMenuAction::SceneReaction GraphView::ShowConnectionContextMenu(
        const AZ::EntityId& connectionId, const QPoint& screenPoint, const QPointF& scenePoint)
    {
        GraphCanvas::ConnectionContextMenu contextMenu(m_toolId);
        return HandleContextMenu(contextMenu, connectionId, screenPoint, scenePoint);
    }

    GraphCanvas::ContextMenuAction::SceneReaction GraphView::ShowSlotContextMenu(
        const AZ::EntityId& slotId, const QPoint& screenPoint, const QPointF& scenePoint)
    {
        GraphCanvas::SlotContextMenu contextMenu(m_toolId);
        return HandleContextMenu(contextMenu, slotId, screenPoint, scenePoint);
    }

    GraphCanvas::Endpoint GraphView::CreateNodeForProposal(
        const AZ::EntityId& connectionId, const GraphCanvas::Endpoint& endpoint, const QPointF& scenePoint, const QPoint& screenPoint)
    {
        GraphCanvas::Endpoint retVal;

        m_createNodeProposalContextMenu->FilterForSourceSlot(m_activeGraphId, endpoint.GetSlotId());
        m_createNodeProposalContextMenu->RefreshActions(m_activeGraphId, connectionId);

        m_createNodeProposalContextMenu->exec(screenPoint);

        GraphCanvas::GraphCanvasMimeEvent* mimeEvent = m_createNodeProposalContextMenu->GetNodePalette()->GetContextMenuEvent();
        if (mimeEvent)
        {
            GraphCanvas::ScopedGraphUndoBatch undoBatch(m_activeGraphId);

            AZ::Vector2 dropPos(aznumeric_cast<float>(scenePoint.x()), aznumeric_cast<float>(scenePoint.y()));
            if (mimeEvent->ExecuteEvent(dropPos, dropPos, m_activeGraphId))
            {
                GraphCanvas::NodeId nodeId = mimeEvent->GetCreatedNodeId();
                if (nodeId.IsValid())
                {
                    GraphCanvas::VisualRequestBus::Event(nodeId, &GraphCanvas::VisualRequests::SetVisible, false);
                    retVal = HandleProposedConnection(m_activeGraphId, connectionId, endpoint, nodeId, screenPoint);
                }

                if (retVal.IsValid())
                {
                    GraphCanvas::GraphUtils::CreateOpportunisticConnectionsBetween(endpoint, retVal);
                    GraphCanvas::VisualRequestBus::Event(nodeId, &GraphCanvas::VisualRequests::SetVisible, true);

                    AZ::Vector2 position;
                    GraphCanvas::GeometryRequestBus::EventResult(position, retVal.GetNodeId(), &GraphCanvas::GeometryRequests::GetPosition);

                    QPointF connectionPoint;
                    GraphCanvas::SlotUIRequestBus::EventResult(
                        connectionPoint, retVal.GetSlotId(), &GraphCanvas::SlotUIRequests::GetConnectionPoint);

                    qreal verticalOffset = connectionPoint.y() - position.GetY();
                    position.SetY(aznumeric_cast<float>(scenePoint.y() - verticalOffset));

                    qreal horizontalOffset = connectionPoint.x() - position.GetX();
                    position.SetX(aznumeric_cast<float>(scenePoint.x() - horizontalOffset));

                    GraphCanvas::GeometryRequestBus::Event(retVal.GetNodeId(), &GraphCanvas::GeometryRequests::SetPosition, position);

                    GraphCanvas::SceneNotificationBus::Event(m_activeGraphId, &GraphCanvas::SceneNotifications::PostCreationEvent);
                }
                else
                {
                    GraphCanvas::GraphUtils::DeleteOutermostNode(m_activeGraphId, nodeId);
                }
            }
        }

        return retVal;
    }

    void GraphView::OnWrapperNodeActionWidgetClicked(
        [[maybe_unused]] const AZ::EntityId& wrapperNode,
        [[maybe_unused]] const QRect& actionWidgetBoundingRect,
        [[maybe_unused]] const QPointF& scenePoint,
        [[maybe_unused]] const QPoint& screenPoint)
    {
    }

    void GraphView::OnSelectionChanged()
    {
        AtomToolsMainWindowRequestBus::Event(m_toolId, &AtomToolsMainWindowRequestBus::Events::QueueUpdateMenus, false);
    }

    GraphCanvas::Endpoint GraphView::HandleProposedConnection(
        [[maybe_unused]] const GraphCanvas::GraphId& graphId,
        [[maybe_unused]] const GraphCanvas::ConnectionId& connectionId,
        const GraphCanvas::Endpoint& endpoint,
        const GraphCanvas::NodeId& proposedNode,
        const QPoint& screenPoint)
    {
        GraphCanvas::Endpoint retVal;

        GraphCanvas::ConnectionType connectionType = GraphCanvas::ConnectionType::CT_Invalid;
        GraphCanvas::SlotRequestBus::EventResult(connectionType, endpoint.GetSlotId(), &GraphCanvas::SlotRequests::GetConnectionType);

        GraphCanvas::NodeId currentTarget = proposedNode;

        while (!retVal.IsValid() && currentTarget.IsValid())
        {
            AZStd::vector<AZ::EntityId> targetSlotIds;
            GraphCanvas::NodeRequestBus::EventResult(targetSlotIds, currentTarget, &GraphCanvas::NodeRequests::GetSlotIds);

            // Find the list of endpoints on the created node that could create a valid connection
            // with the specified slot
            AZStd::list<GraphCanvas::Endpoint> endpoints;
            for (const auto& targetSlotId : targetSlotIds)
            {
                GraphCanvas::Endpoint proposedEndpoint(currentTarget, targetSlotId);

                bool canCreate = false;
                GraphCanvas::SlotRequestBus::EventResult(canCreate, endpoint.GetSlotId(), &GraphCanvas::SlotRequests::CanCreateConnectionTo, proposedEndpoint);

                if (canCreate)
                {
                    GraphCanvas::SlotGroup slotGroup = GraphCanvas::SlotGroups::Invalid;
                    GraphCanvas::SlotRequestBus::EventResult(slotGroup, targetSlotId, &GraphCanvas::SlotRequests::GetSlotGroup);

                    bool isVisible = slotGroup != GraphCanvas::SlotGroups::Invalid;
                    GraphCanvas::SlotLayoutRequestBus::EventResult(isVisible, currentTarget, &GraphCanvas::SlotLayoutRequests::IsSlotGroupVisible, slotGroup);

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

                    for (GraphCanvas::Endpoint proposedEndpoint : endpoints)
                    {
                        menu.addAction(aznew GraphCanvas::EndpointSelectionAction(proposedEndpoint));
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
                    GraphCanvas::SlotRequestBus::EventResult(canCreateConnection, endpoint.GetSlotId(), &GraphCanvas::SlotRequests::CanCreateConnectionTo, retVal);

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
                GraphCanvas::NodeRequestBus::EventResult(isWrapped, currentTarget, &GraphCanvas::NodeRequests::IsWrapped);

                if (isWrapped)
                {
                    GraphCanvas::NodeRequestBus::EventResult(currentTarget, currentTarget, &GraphCanvas::NodeRequests::GetWrappingNode);
                }
                else
                {
                    currentTarget.SetInvalid();
                }
            }
        }

        return retVal;
    }

    GraphCanvas::ContextMenuAction::SceneReaction GraphView::HandleContextMenu(
        GraphCanvas::EditorContextMenu& editorContextMenu, const AZ::EntityId& memberId, const QPoint& screenPoint, const QPointF& scenePoint) const
    {
        AZ::Vector2 sceneVector(aznumeric_cast<float>(scenePoint.x()), aznumeric_cast<float>(scenePoint.y()));

        editorContextMenu.RefreshActions(m_activeGraphId, memberId);

        QAction* result = editorContextMenu.exec(screenPoint);

        if (auto contextMenuAction = qobject_cast<GraphCanvas::ContextMenuAction*>(result))
        {
            GraphCanvas::ScopedGraphUndoBatch undoBatch(m_activeGraphId);
            return contextMenuAction->TriggerAction(m_activeGraphId, sceneVector);
        }

        if (editorContextMenu.GetNodePalette())
        {
            // Handle creating node from any node palette embedded in an GraphCanvas::EditorContextMenu.
            GraphCanvas::GraphCanvasMimeEvent* mimeEvent = editorContextMenu.GetNodePalette()->GetContextMenuEvent();
            if (mimeEvent)
            {
                GraphCanvas::ScopedGraphUndoBatch undoBatch(m_activeGraphId);

                AZ::Vector2 dropPos(aznumeric_cast<float>(scenePoint.x()), aznumeric_cast<float>(scenePoint.y()));
                if (mimeEvent->ExecuteEvent(dropPos, dropPos, m_activeGraphId))
                {
                    GraphCanvas::NodeId nodeId = mimeEvent->GetCreatedNodeId();
                    if (nodeId.IsValid())
                    {
                        GraphCanvas::SceneRequestBus::Event(m_activeGraphId, &GraphCanvas::SceneRequests::ClearSelection);
                        GraphCanvas::VisualRequestBus::Event(nodeId, &GraphCanvas::VisualRequests::SetVisible, true);
                        GraphCanvas::SceneMemberUIRequestBus::Event(nodeId, &GraphCanvas::SceneMemberUIRequests::SetSelected, true);
                        GraphCanvas::SceneNotificationBus::Event(m_activeGraphId, &GraphCanvas::SceneNotifications::PostCreationEvent);
                    }
                }
            }
        }

        return GraphCanvas::ContextMenuAction::SceneReaction::Nothing;
    }

    void GraphView::AlignSelected(const GraphCanvas::AlignConfig& alignConfig)
    {
        GraphCanvas::ScopedGraphUndoBatch undoBatch(m_activeGraphId);

        AZStd::vector<GraphCanvas::NodeId> selectedNodes;
        GraphCanvas::SceneRequestBus::EventResult(selectedNodes, m_activeGraphId, &GraphCanvas::SceneRequests::GetSelectedNodes);
        GraphCanvas::GraphUtils::AlignNodes(selectedNodes, alignConfig);
    }

    void GraphView::OpenPresetsEditor()
    {
        if (m_presetEditor && m_presetWrapper)
        {
            QSize boundingBox = size();
            QPointF newPosition =
                mapToGlobal(QPoint(aznumeric_cast<int>(boundingBox.width() * 0.5f), aznumeric_cast<int>(boundingBox.height() * 0.5f)));

            m_presetEditor->show();

            m_presetWrapper->show();
            m_presetWrapper->raise();
            m_presetWrapper->activateWindow();

            QRect geometry = m_presetWrapper->geometry();
            QSize originalSize = geometry.size();

            newPosition.setX(newPosition.x() - geometry.width() * 0.5f);
            newPosition.setY(newPosition.y() - geometry.height() * 0.5f);

            geometry.setTopLeft(newPosition.toPoint());

            geometry.setWidth(originalSize.width());
            geometry.setHeight(originalSize.height());

            m_presetWrapper->setGeometry(geometry);
        }
    }
} // namespace AtomToolsFramework

#include <AtomToolsFramework/Graph/moc_GraphView.cpp>
