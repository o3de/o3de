/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomToolsFramework/Util/Util.h>
#include <AzCore/IO/FileIO.h>
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

#include <Document/MaterialCanvasDocumentRequestBus.h>
#include <Window/MaterialCanvasGraphView.h>

#include <QApplication>
#include <QClipboard>
#include <QKeyEvent>
#include <QKeySequence>
#include <QMenu>
#include <QMenuBar>
#include <QMimeData>
#include <QVBoxLayout>
#include <QWindow>

namespace MaterialCanvas
{
    MaterialCanvasGraphView::MaterialCanvasGraphView(
        const AZ::Crc32& toolId,
        const AZ::Uuid& documentId,
        const CreateNodePaletteItemsCallback& createNodePaletteItemsCallback,
        QWidget* parent)
        : QWidget(parent)
        , m_toolId(toolId)
        , m_documentId(documentId)
        , m_createNodePaletteItemsCallback(createNodePaletteItemsCallback)
    {
        setLayout(new QVBoxLayout());

        m_editorToolbar = aznew GraphCanvas::AssetEditorToolbar(m_toolId);
        layout()->addWidget(m_editorToolbar);

        // Screenshot
        m_takeScreenshot = new QToolButton();
        m_takeScreenshot->setToolTip("Captures a full resolution screenshot of the entire graph or selected nodes into the clipboard");
        m_takeScreenshot->setIcon(QIcon(":/Icons/screenshot.png"));
        m_takeScreenshot->setEnabled(false);
        connect(m_takeScreenshot, &QToolButton::clicked, this, [this] {
            GraphCanvas::ViewId viewId;
            GraphCanvas::SceneRequestBus::EventResult(viewId, m_activeGraphId, &GraphCanvas::SceneRequests::GetViewId);
            GraphCanvas::ViewRequestBus::Event(viewId, &GraphCanvas::ViewRequests::ScreenshotSelection);
        });
        m_editorToolbar->AddCustomAction(m_takeScreenshot);

        m_graphicsView = new GraphCanvas::GraphCanvasGraphicsView(this);
        layout()->addWidget(m_graphicsView);

        m_presetEditor = aznew GraphCanvas::ConstructPresetDialog(nullptr);
        m_presetEditor->SetEditorId(m_toolId);

        m_presetWrapper = new AzQtComponents::WindowDecorationWrapper(AzQtComponents::WindowDecorationWrapper::OptionAutoTitleBarButtons);
        m_presetWrapper->setGuest(m_presetEditor);
        m_presetWrapper->hide();

        // Add a node palette for creating new nodes to the default scene context menu,
        // which is what is displayed when right-clicking on an empty space in the graph
        GraphCanvas::NodePaletteConfig nodePaletteConfig;
        nodePaletteConfig.m_editorId = m_toolId;
        nodePaletteConfig.m_mimeType = "materialcanvas/node-palette-mime-event";
        nodePaletteConfig.m_isInContextMenu = true;
        nodePaletteConfig.m_saveIdentifier = "MaterialCanvas_ContextMenu";
        nodePaletteConfig.m_rootTreeItem = m_createNodePaletteItemsCallback(m_toolId);
        m_sceneContextMenu = aznew GraphCanvas::SceneContextMenu(m_toolId, this);
        m_sceneContextMenu->AddNodePaletteMenuAction(nodePaletteConfig);

        // Setup the context menu with node palette for proposing a new node
        // when dropping a connection in an empty space in the graph
        nodePaletteConfig.m_rootTreeItem = m_createNodePaletteItemsCallback(m_toolId);
        m_createNodeProposalContextMenu = aznew GraphCanvas::EditorContextMenu(m_toolId, this);
        m_createNodeProposalContextMenu->AddNodePaletteMenuAction(nodePaletteConfig);

        AtomToolsFramework::AtomToolsDocumentNotificationBus::Handler::BusConnect(m_toolId);
        OnDocumentOpened(m_documentId);
    }

    MaterialCanvasGraphView::~MaterialCanvasGraphView()
    {
        OnDocumentOpened(AZ::Uuid::CreateNull());
        AtomToolsFramework::AtomToolsDocumentNotificationBus::Handler::BusDisconnect();
        delete m_presetEditor;
    }

    void MaterialCanvasGraphView::OnDocumentOpened(const AZ::Uuid& documentId)
    {
        AtomToolsFramework::AtomToolsMainMenuRequestBus::Handler::BusDisconnect();
        GraphCanvas::AssetEditorNotificationBus::Handler::BusDisconnect();
        GraphCanvas::AssetEditorRequestBus::Handler::BusDisconnect();
        GraphCanvas::AssetEditorSettingsRequestBus::Handler::BusDisconnect();
        GraphCanvas::SceneNotificationBus::Handler::BusDisconnect();

        if (m_documentId == documentId)
        {
            m_activeGraphId = GraphCanvas::GraphId();
            MaterialCanvasDocumentRequestBus::EventResult(m_activeGraphId, documentId, &MaterialCanvasDocumentRequestBus::Events::GetGraphId);

            if (m_activeGraphId.IsValid())
            {
                AtomToolsFramework::AtomToolsMainMenuRequestBus::Handler::BusConnect(m_toolId);
                GraphCanvas::AssetEditorNotificationBus::Handler::BusConnect(m_toolId);
                GraphCanvas::AssetEditorRequestBus::Handler::BusConnect(m_toolId);
                GraphCanvas::AssetEditorSettingsRequestBus::Handler::BusConnect(m_toolId);
                GraphCanvas::SceneNotificationBus::Handler::BusConnect(m_activeGraphId);
            }

            m_graphicsView->SetEditorId(m_toolId);
            m_graphicsView->SetScene(m_activeGraphId);

            GraphCanvas::AssetEditorNotificationBus::Event(m_toolId, &GraphCanvas::AssetEditorNotifications::PreOnActiveGraphChanged);
            GraphCanvas::AssetEditorNotificationBus::Event(m_toolId, &GraphCanvas::AssetEditorNotifications::OnActiveGraphChanged, m_activeGraphId);
            GraphCanvas::AssetEditorNotificationBus::Event(m_toolId, &GraphCanvas::AssetEditorNotifications::PostOnActiveGraphChanged);
        }

        AtomToolsFramework::AtomToolsMainWindowRequestBus::Event(
            m_toolId, &AtomToolsFramework::AtomToolsMainWindowRequestBus::Events::QueueUpdateMenus, true);
    }

    void MaterialCanvasGraphView::OnDocumentClosed([[maybe_unused]] const AZ::Uuid& documentId)
    {
    }

    void MaterialCanvasGraphView::OnDocumentDestroyed([[maybe_unused]] const AZ::Uuid& documentId)
    {
    }

    AZ::s32 MaterialCanvasGraphView::GetMainMenuPriority() const
    {
        return 1;
    }

    void MaterialCanvasGraphView::CreateMenus(QMenuBar* menuBar)
    {
        auto menuEdit = menuBar->findChild<QMenu*>("menuEdit");
        auto menuView = menuBar->findChild<QMenu*>("menuView");

        menuEdit->addSeparator();
        m_actionCut = menuEdit->addAction(tr("Cut"), [this] {
            GraphCanvas::SceneRequestBus::Event(m_activeGraphId, &GraphCanvas::SceneRequests::CutSelection);
        }, QKeySequence::Cut);
        m_actionCopy = menuEdit->addAction(tr("Copy"), [this] {
            GraphCanvas::SceneRequestBus::Event(m_activeGraphId, &GraphCanvas::SceneRequests::CopySelection);
        }, QKeySequence::Copy);
        m_actionPaste = menuEdit->addAction(tr("Paste"), [this] {
            GraphCanvas::SceneRequestBus::Event(m_activeGraphId, &GraphCanvas::SceneRequests::Paste);
        }, QKeySequence::Paste);
        m_actionDuplicate = menuEdit->addAction(tr("Duplicate"), [this] {
            GraphCanvas::SceneRequestBus::Event(m_activeGraphId, &GraphCanvas::SceneRequests::DuplicateSelection);
        });
        m_actionDelete = menuEdit->addAction(tr("Delete"), [this] {
            GraphCanvas::SceneRequestBus::Event(m_activeGraphId, &GraphCanvas::SceneRequests::DeleteSelection);
        }, QKeySequence::Delete);

        menuEdit->addSeparator();
        m_actionRemoveUnusedNodes = menuEdit->addAction(tr("Remove Unused Nodes"), [this] {
            GraphCanvas::SceneRequestBus::Event(m_activeGraphId, &GraphCanvas::SceneRequests::RemoveUnusedNodes);
        });
        m_actionRemoveUnusedElements = menuEdit->addAction(tr("Remove Unused Elements"), [this] {
            GraphCanvas::SceneRequestBus::Event(m_activeGraphId, &GraphCanvas::SceneRequests::RemoveUnusedElements);
        });

        menuEdit->addSeparator();
        m_actionSelectAll = menuEdit->addAction(tr("Select All"), [this] {
            GraphCanvas::SceneRequestBus::Event(m_activeGraphId, &GraphCanvas::SceneRequests::SelectAll);
        }, QKeySequence::SelectAll);
        m_actionSelectInputs = menuEdit->addAction(tr("Select Inputs"), [this] {
            GraphCanvas::SceneRequestBus::Event(
                m_activeGraphId, &GraphCanvas::SceneRequests::SelectAllRelative, GraphCanvas::ConnectionType::CT_Input);
        }, QKeySequence::Deselect);
        m_actionSelectOutputs = menuEdit->addAction(tr("Select Outputs"), [this] {
            GraphCanvas::SceneRequestBus::Event(
                m_activeGraphId, &GraphCanvas::SceneRequests::SelectAllRelative, GraphCanvas::ConnectionType::CT_Output);
        });
        m_actionSelectConnected = menuEdit->addAction(tr("Select Connected"), [this] {
            GraphCanvas::SceneRequestBus::Event(m_activeGraphId, &GraphCanvas::SceneRequests::SelectConnectedNodes);
        });
        m_actionSelectNone = menuEdit->addAction(tr("Clear Selection"), [this] {
            GraphCanvas::SceneRequestBus::Event(m_activeGraphId, &GraphCanvas::SceneRequests::ClearSelection);
        });
        m_actionSelectEnable = menuEdit->addAction(tr("Enable Selection"), [this] {
            GraphCanvas::SceneRequestBus::Event(m_activeGraphId, &GraphCanvas::SceneRequests::EnableSelection);
        });
        m_actionSelectDisable = menuEdit->addAction(tr("Disable Selection"), [this] {
            GraphCanvas::SceneRequestBus::Event(m_activeGraphId, &GraphCanvas::SceneRequests::DisableSelection);
        });

        menuEdit->addSeparator();
        m_actionScreenShot = menuEdit->addAction(tr("Screenshot"), [this] {
            GraphCanvas::ViewId viewId;
            GraphCanvas::SceneRequestBus::EventResult(viewId, m_activeGraphId, &GraphCanvas::SceneRequests::GetViewId);
            GraphCanvas::ViewRequestBus::Event(viewId, &GraphCanvas::ViewRequests::ScreenshotSelection);
        });

        menuEdit->addSeparator();
        m_actionAlignTop = menuEdit->addAction(tr("Align Top"), [this] {
            GraphCanvas::AlignConfig alignConfig;
            alignConfig.m_horAlign = GraphCanvas::GraphUtils::HorizontalAlignment::None;
            alignConfig.m_verAlign = GraphCanvas::GraphUtils::VerticalAlignment::Top;
            alignConfig.m_alignTime = GetAlignmentTime();
            AlignSelected(alignConfig);
        });
        m_actionAlignBottom = menuEdit->addAction(tr("Align Bottom"), [this] {
            GraphCanvas::AlignConfig alignConfig;
            alignConfig.m_horAlign = GraphCanvas::GraphUtils::HorizontalAlignment::None;
            alignConfig.m_verAlign = GraphCanvas::GraphUtils::VerticalAlignment::Bottom;
            alignConfig.m_alignTime = GetAlignmentTime();
            AlignSelected(alignConfig);
        });
        m_actionAlignLeft = menuEdit->addAction(tr("Align Left"), [this] {
            GraphCanvas::AlignConfig alignConfig;
            alignConfig.m_horAlign = GraphCanvas::GraphUtils::HorizontalAlignment::Left;
            alignConfig.m_verAlign = GraphCanvas::GraphUtils::VerticalAlignment::None;
            alignConfig.m_alignTime = GetAlignmentTime();
            AlignSelected(alignConfig);
        });
        m_actionAlignRight = menuEdit->addAction(tr("Align Right"), [this] {
            GraphCanvas::AlignConfig alignConfig;
            alignConfig.m_horAlign = GraphCanvas::GraphUtils::HorizontalAlignment::Right;
            alignConfig.m_verAlign = GraphCanvas::GraphUtils::VerticalAlignment::None;
            alignConfig.m_alignTime = GetAlignmentTime();
            AlignSelected(alignConfig);
        });

        menuView->addSeparator();
        m_actionPresetEditor = menuView->addAction(tr("Preset Editor"), [this] { OpenPresetsEditor(); });

        menuView->addSeparator();
        m_actionShowEntireGraph = menuView->addAction(tr("Show Entire Graph"), [this] {
            GraphCanvas::ViewId viewId;
            GraphCanvas::SceneRequestBus::EventResult(viewId, m_activeGraphId, &GraphCanvas::SceneRequests::GetViewId);
            GraphCanvas::ViewRequestBus::Event(viewId, &GraphCanvas::ViewRequests::ShowEntireGraph);
        });
        m_actionZoomIn = menuView->addAction(tr("Zoom In"), [this] {
            GraphCanvas::ViewId viewId;
            GraphCanvas::SceneRequestBus::EventResult(viewId, m_activeGraphId, &GraphCanvas::SceneRequests::GetViewId);
            GraphCanvas::ViewRequestBus::Event(viewId, &GraphCanvas::ViewRequests::ZoomIn);
        }, QKeySequence::ZoomIn);
        m_actionZoomOut = menuView->addAction(tr("Zoom Out"), [this] {
            GraphCanvas::ViewId viewId;
            GraphCanvas::SceneRequestBus::EventResult(viewId, m_activeGraphId, &GraphCanvas::SceneRequests::GetViewId);
            GraphCanvas::ViewRequestBus::Event(viewId, &GraphCanvas::ViewRequests::ZoomOut);
        }, QKeySequence::ZoomOut);
        m_actionZoomSelection = menuView->addAction(tr("Zoom Selection"), [this] {
            GraphCanvas::ViewId viewId;
            GraphCanvas::SceneRequestBus::EventResult(viewId, m_activeGraphId, &GraphCanvas::SceneRequests::GetViewId);
            GraphCanvas::ViewRequestBus::Event(viewId, &GraphCanvas::ViewRequests::CenterOnSelection);
        });

        menuView->addSeparator();
        m_actionGotoStartOfChain = menuView->addAction(tr("Goto Start Of Chain"), [this] {
            GraphCanvas::ViewId viewId;
            GraphCanvas::SceneRequestBus::EventResult(viewId, m_activeGraphId, &GraphCanvas::SceneRequests::GetViewId);
            GraphCanvas::ViewRequestBus::Event(viewId, &GraphCanvas::ViewRequests::CenterOnStartOfChain);
        }, QKeySequence::MoveToStartOfDocument);
        m_actionGotoEndOfChain = menuView->addAction(tr("Goto End Of Chain"), [this] {
            GraphCanvas::ViewId viewId;
            GraphCanvas::SceneRequestBus::EventResult(viewId, m_activeGraphId, &GraphCanvas::SceneRequests::GetViewId);
            GraphCanvas::ViewRequestBus::Event(viewId, &GraphCanvas::ViewRequests::CenterOnEndOfChain);
        }, QKeySequence::MoveToEndOfDocument);

        connect(menuEdit, &QMenu::aboutToShow, this, [this](){
            AtomToolsFramework::AtomToolsMainWindowRequestBus::Event(
                m_toolId, &AtomToolsFramework::AtomToolsMainWindowRequestBus::Events::QueueUpdateMenus, false);
        });
        connect(QApplication::clipboard(), &QClipboard::dataChanged, this, [this](){
            AtomToolsFramework::AtomToolsMainWindowRequestBus::Event(
                m_toolId, &AtomToolsFramework::AtomToolsMainWindowRequestBus::Events::QueueUpdateMenus, false);
        });
    }

    void MaterialCanvasGraphView::UpdateMenus([[maybe_unused]] QMenuBar* menuBar)
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

    AZ::EntityId MaterialCanvasGraphView::CreateNewGraph()
    {
        return m_activeGraphId;
    }

    bool MaterialCanvasGraphView::ContainsGraph([[maybe_unused]] const GraphCanvas::GraphId& graphId) const
    {
        return m_activeGraphId == graphId;
    }

    bool MaterialCanvasGraphView::CloseGraph([[maybe_unused]] const GraphCanvas::GraphId& graphId)
    {
        return false;
    }

    GraphCanvas::ContextMenuAction::SceneReaction MaterialCanvasGraphView::ShowSceneContextMenu(const QPoint& screenPoint, const QPointF& scenePoint)
    {
        m_sceneContextMenu->ResetSourceSlotFilter();

        // We pass an invalid EntityId here since this is for the scene, there is no member to specify.
        return HandleContextMenu(*m_sceneContextMenu, AZ::EntityId(), screenPoint, scenePoint);
    }

    GraphCanvas::ContextMenuAction::SceneReaction MaterialCanvasGraphView::ShowNodeContextMenu(
        const AZ::EntityId& nodeId, const QPoint& screenPoint, const QPointF& scenePoint)
    {
        GraphCanvas::NodeContextMenu contextMenu(m_toolId);
        return HandleContextMenu(contextMenu, nodeId, screenPoint, scenePoint);
    }

    GraphCanvas::ContextMenuAction::SceneReaction MaterialCanvasGraphView::ShowCommentContextMenu(
        const AZ::EntityId& nodeId, const QPoint& screenPoint, const QPointF& scenePoint)
    {
        GraphCanvas::CommentContextMenu contextMenu(m_toolId);
        return HandleContextMenu(contextMenu, nodeId, screenPoint, scenePoint);
    }

    GraphCanvas::ContextMenuAction::SceneReaction MaterialCanvasGraphView::ShowNodeGroupContextMenu(
        const AZ::EntityId& groupId, const QPoint& screenPoint, const QPointF& scenePoint)
    {
        GraphCanvas::NodeGroupContextMenu contextMenu(m_toolId);
        return HandleContextMenu(contextMenu, groupId, screenPoint, scenePoint);
    }

    GraphCanvas::ContextMenuAction::SceneReaction MaterialCanvasGraphView::ShowCollapsedNodeGroupContextMenu(
        const AZ::EntityId& nodeId, const QPoint& screenPoint, const QPointF& scenePoint)
    {
        GraphCanvas::CollapsedNodeGroupContextMenu contextMenu(m_toolId);
        return HandleContextMenu(contextMenu, nodeId, screenPoint, scenePoint);
    }

    GraphCanvas::ContextMenuAction::SceneReaction MaterialCanvasGraphView::ShowBookmarkContextMenu(
        const AZ::EntityId& bookmarkId, const QPoint& screenPoint, const QPointF& scenePoint)
    {
        GraphCanvas::BookmarkContextMenu contextMenu(m_toolId);
        return HandleContextMenu(contextMenu, bookmarkId, screenPoint, scenePoint);
    }

    GraphCanvas::ContextMenuAction::SceneReaction MaterialCanvasGraphView::ShowConnectionContextMenu(
        const AZ::EntityId& connectionId, const QPoint& screenPoint, const QPointF& scenePoint)
    {
        GraphCanvas::ConnectionContextMenu contextMenu(m_toolId);
        return HandleContextMenu(contextMenu, connectionId, screenPoint, scenePoint);
    }

    GraphCanvas::ContextMenuAction::SceneReaction MaterialCanvasGraphView::ShowSlotContextMenu(
        const AZ::EntityId& slotId, const QPoint& screenPoint, const QPointF& scenePoint)
    {
        GraphCanvas::SlotContextMenu contextMenu(m_toolId);
        return HandleContextMenu(contextMenu, slotId, screenPoint, scenePoint);
    }

    GraphCanvas::Endpoint MaterialCanvasGraphView::CreateNodeForProposal(
        const AZ::EntityId& connectionId, const GraphCanvas::Endpoint& endpoint, const QPointF& scenePoint, const QPoint& screenPoint)
    {
        GraphCanvas::Endpoint retVal;

        m_createNodeProposalContextMenu->FilterForSourceSlot(m_activeGraphId, endpoint.GetSlotId());
        m_createNodeProposalContextMenu->RefreshActions(m_activeGraphId, connectionId);

        m_createNodeProposalContextMenu->exec(screenPoint);

        GraphCanvas::GraphCanvasMimeEvent* mimeEvent = m_createNodeProposalContextMenu->GetNodePalette()->GetContextMenuEvent();
        if (mimeEvent)
        {
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

    void MaterialCanvasGraphView::OnWrapperNodeActionWidgetClicked(
        [[maybe_unused]] const AZ::EntityId& wrapperNode,
        [[maybe_unused]] const QRect& actionWidgetBoundingRect,
        [[maybe_unused]] const QPointF& scenePoint,
        [[maybe_unused]] const QPoint& screenPoint)
    {
    }

    GraphCanvas::EditorConstructPresets* MaterialCanvasGraphView::GetConstructPresets() const
    {
        return const_cast<GraphCanvas::EditorConstructPresets*>(&m_constructPresetDefaults);
    }

    const GraphCanvas::ConstructTypePresetBucket* MaterialCanvasGraphView::GetConstructTypePresetBucket(
        GraphCanvas::ConstructType constructType) const
    {
        return m_constructPresetDefaults.FindPresetBucket(constructType);
    }

    GraphCanvas::Styling::ConnectionCurveType MaterialCanvasGraphView::GetConnectionCurveType() const
    {
        return GraphCanvas::Styling::ConnectionCurveType::Curved;
    }

    GraphCanvas::Styling::ConnectionCurveType MaterialCanvasGraphView::GetDataConnectionCurveType() const
    {
        return GraphCanvas::Styling::ConnectionCurveType::Curved;
    }

    void MaterialCanvasGraphView::OnActiveGraphChanged(const GraphCanvas::GraphId& graphId)
    {
        m_activeGraphId = graphId;
        GraphCanvas::SceneNotificationBus::Handler::BusDisconnect();
        GraphCanvas::SceneNotificationBus::Handler::BusConnect(m_activeGraphId);
        GraphCanvas::SceneRequestBus::Event(m_activeGraphId, &GraphCanvas::SceneRequests::SetMimeType, "materialcanvas/node-palette-mime-event");
        AtomToolsFramework::AtomToolsMainWindowRequestBus::Event(
            m_toolId, &AtomToolsFramework::AtomToolsMainWindowRequestBus::Events::QueueUpdateMenus, true);
    }

    void MaterialCanvasGraphView::OnSelectionChanged()
    {
        AtomToolsFramework::AtomToolsMainWindowRequestBus::Event(
            m_toolId, &AtomToolsFramework::AtomToolsMainWindowRequestBus::Events::QueueUpdateMenus, false);
    }

    GraphCanvas::Endpoint MaterialCanvasGraphView::HandleProposedConnection(
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

    GraphCanvas::ContextMenuAction::SceneReaction MaterialCanvasGraphView::HandleContextMenu(
        GraphCanvas::EditorContextMenu& editorContextMenu, const AZ::EntityId& memberId, const QPoint& screenPoint, const QPointF& scenePoint) const
    {
        AZ::Vector2 sceneVector(aznumeric_cast<float>(scenePoint.x()), aznumeric_cast<float>(scenePoint.y()));

        editorContextMenu.RefreshActions(m_activeGraphId, memberId);

        QAction* result = editorContextMenu.exec(screenPoint);

        if (auto contextMenuAction = qobject_cast<GraphCanvas::ContextMenuAction*>(result))
        {
            return contextMenuAction->TriggerAction(m_activeGraphId, sceneVector);
        }

        if (editorContextMenu.GetNodePalette())
        {
            // Handle creating node from any node palette embedded in an GraphCanvas::EditorContextMenu.
            GraphCanvas::GraphCanvasMimeEvent* mimeEvent = editorContextMenu.GetNodePalette()->GetContextMenuEvent();
            if (mimeEvent)
            {
                AZ::Vector2 dropPos(aznumeric_cast<float>(scenePoint.x()), aznumeric_cast<float>(scenePoint.y()));
                if (mimeEvent->ExecuteEvent(dropPos, dropPos, m_activeGraphId))
                {
                    GraphCanvas::NodeId nodeId = mimeEvent->GetCreatedNodeId();
                    if (nodeId.IsValid())
                    {
                        GraphCanvas::SceneRequestBus::Event(m_activeGraphId, &GraphCanvas::SceneRequests::ClearSelection);
                        GraphCanvas::VisualRequestBus::Event(nodeId, &GraphCanvas::VisualRequests::SetVisible, true);

                        GraphCanvas::SceneNotificationBus::Event(m_activeGraphId, &GraphCanvas::SceneNotifications::PostCreationEvent);
                    }
                }
            }
        }

        return GraphCanvas::ContextMenuAction::SceneReaction::Nothing;
    }

    void MaterialCanvasGraphView::AlignSelected(const GraphCanvas::AlignConfig& alignConfig)
    {
        AZStd::vector<GraphCanvas::NodeId> selectedNodes;
        GraphCanvas::SceneRequestBus::EventResult(selectedNodes, m_activeGraphId, &GraphCanvas::SceneRequests::GetSelectedNodes);
        GraphCanvas::GraphUtils::AlignNodes(selectedNodes, alignConfig);
    }

    void MaterialCanvasGraphView::OpenPresetsEditor()
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
} // namespace MaterialCanvas

#include <Window/moc_MaterialCanvasGraphView.cpp>
