/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomToolsFramework/DynamicProperty/DynamicProperty.h>
#include <AtomToolsFramework/Util/Util.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/EntityUtils.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/Math/Color.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Vector4.h>
#include <AzCore/Serialization/IdUtils.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/std/containers/set.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzFramework/Asset/AssetCatalog.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzQtComponents/Components/Widgets/FileDialog.h>
#include <AzQtComponents/Components/Widgets/TabWidget.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/API/EntityCompositionRequestBus.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserModel.h>
#include <AzToolsFramework/ToolsComponents/EditorEntityIdContainer.h>
#include <AzToolsFramework/ToolsComponents/GenericComponentWrapper.h>
#include <AzToolsFramework/ToolsComponents/ToolsAssetCatalogBus.h>
#include <AzToolsFramework/UI/UICore/WidgetHelpers.h>
#include <GraphCanvas/Components/Connections/ConnectionBus.h>
#include <GraphCanvas/Components/GeometryBus.h>
#include <GraphCanvas/Components/GridBus.h>
#include <GraphCanvas/Components/MimeDataHandlerBus.h>
#include <GraphCanvas/Components/Nodes/NodeBus.h>
#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/ViewBus.h>
#include <GraphCanvas/Components/VisualBus.h>
#include <GraphCanvas/GraphCanvasBus.h>
#include <GraphCanvas/Styling/Parser.h>
#include <GraphCanvas/Styling/Style.h>
#include <GraphCanvas/Styling/StyleManager.h>
#include <GraphCanvas/Types/ConstructPresets.h>
#include <GraphCanvas/Utils/ConversionUtils.h>
#include <GraphCanvas/Utils/NodeNudgingController.h>
#include <GraphCanvas/Widgets/AssetEditorToolbar/AssetEditorToolbar.h>
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
#include <GraphCanvas/Widgets/GraphCanvasGraphicsView/GraphCanvasGraphicsView.h>
#include <GraphCanvas/Widgets/GraphCanvasMimeContainer.h>
#include <GraphCanvas/Widgets/MiniMapGraphicsView/MiniMapGraphicsView.h>
#include <GraphCanvas/Widgets/NodePalette/NodePaletteDockWidget.h>
#include <GraphCanvas/Widgets/NodePalette/TreeItems/IconDecoratedNodePaletteTreeItem.h>
#include <GraphCanvas/Widgets/NodePalette/TreeItems/NodePaletteTreeItem.h>
#include <Viewport/MaterialCanvasViewportWidget.h>
#include <Window/MaterialCanvasMainWindow.h>
#include <Window/ViewportSettingsInspector/ViewportSettingsInspector.h>

#include <QApplication>
#include <QClipboard>
#include <QCoreApplication>
#include <QDir>
#include <QDirIterator>
#include <QGraphicsScene>
#include <QGraphicsSceneEvent>
#include <QGraphicsView>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QKeySequence>
#include <QListView>
#include <QMessageBox>
#include <QMimeData>
#include <QProgressDialog>
#include <QShortcut>
#include <QSplitter>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWindow>

namespace MaterialCanvas
{
    MaterialCanvasMainWindow::MaterialCanvasMainWindow(const AZ::Crc32& toolId, QWidget* parent)
        : Base(toolId, parent)
        , m_styleManager(toolId, "MaterialCanvas/StyleSheet/graphcanvas_style.json")
    {
        m_toolBar = new MaterialCanvasToolBar(m_toolId, this);
        m_toolBar->setObjectName("ToolBar");
        addToolBar(m_toolBar);

        m_editorToolbar = aznew GraphCanvas::AssetEditorToolbar(m_toolId);
        qobject_cast<QBoxLayout*>(centralWidget()->layout())->insertWidget(0, m_editorToolbar);

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

        m_assetBrowser->SetFilterState("", AZ::RPI::StreamingImageAsset::Group, true);
        m_assetBrowser->SetFilterState("", AZ::RPI::MaterialAsset::Group, true);

        m_materialInspector = new AtomToolsFramework::AtomToolsDocumentInspector(m_toolId, this);
        m_materialInspector->SetDocumentSettingsPrefix("/O3DE/Atom/MaterialCanvas/MaterialInspector");
        AddDockWidget("Inspector", m_materialInspector, Qt::RightDockWidgetArea);

        m_materialViewport = new MaterialCanvasViewportWidget(m_toolId, this);
        m_materialViewport->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        AddDockWidget("Viewport", m_materialViewport, Qt::RightDockWidgetArea);

        AddDockWidget("Viewport Settings", new ViewportSettingsInspector(m_toolId, this), Qt::LeftDockWidgetArea);
        SetDockWidgetVisible("Viewport Settings", false);

        AddDockWidget("MiniMap", aznew GraphCanvas::MiniMapDockWidget(m_toolId, this), Qt::RightDockWidgetArea);

        m_bookmarkDockWidget = aznew GraphCanvas::BookmarkDockWidget(m_toolId, this);
        AddDockWidget("Bookmarks", m_bookmarkDockWidget, Qt::BottomDockWidgetArea);

        GraphCanvas::NodePaletteConfig nodePaletteConfig;
        nodePaletteConfig.m_rootTreeItem = GetNodePaletteRootTreeItem();
        nodePaletteConfig.m_editorId = m_toolId;
        nodePaletteConfig.m_mimeType = "materialcanvas/node-palette-mime-event";
        nodePaletteConfig.m_isInContextMenu = false;
        nodePaletteConfig.m_saveIdentifier = "MaterialCanvas_ContextMenu";

        m_nodePalette = aznew GraphCanvas::NodePaletteDockWidget(this, "Node Palette", nodePaletteConfig);
        AddDockWidget("Node Palette", m_nodePalette, Qt::LeftDockWidgetArea);

        m_presetEditor = aznew GraphCanvas::ConstructPresetDialog(nullptr);
        m_presetEditor->SetEditorId(m_toolId);

        m_presetWrapper = new AzQtComponents::WindowDecorationWrapper(AzQtComponents::WindowDecorationWrapper::OptionAutoTitleBarButtons);
        m_presetWrapper->setGuest(m_presetEditor);
        m_presetWrapper->hide();

        // Add a node palette for creating new nodes to the default scene context menu,
        // which is what is displayed when right-clicking on an empty space in the graph
        GraphCanvas::NodePaletteConfig sceneContextMenuConfig = nodePaletteConfig;
        sceneContextMenuConfig.m_isInContextMenu = true;
        sceneContextMenuConfig.m_rootTreeItem = GetNodePaletteRootTreeItem();
        m_sceneContextMenu = aznew GraphCanvas::SceneContextMenu(m_toolId, this);
        m_sceneContextMenu->AddNodePaletteMenuAction(sceneContextMenuConfig);

        // Setup the context menu with node palette for proposing a new node
        // when dropping a connection in an empty space in the graph
        GraphCanvas::NodePaletteConfig nodeProposalConfig = nodePaletteConfig;
        nodeProposalConfig.m_isInContextMenu = true;
        nodeProposalConfig.m_rootTreeItem = GetNodePaletteRootTreeItem();
        m_createNodeProposalContextMenu = aznew GraphCanvas::EditorContextMenu(m_toolId, this);
        m_createNodeProposalContextMenu->AddNodePaletteMenuAction(nodeProposalConfig);

        AZStd::array<char, AZ::IO::MaxPathLength> unresolvedPath;
        AZ::IO::FileIOBase::GetInstance()->ResolvePath(
            "@products@/translation/materialcanvas_en_us.qm", unresolvedPath.data(), unresolvedPath.size());

        QString translationFilePath(unresolvedPath.data());
        if (m_translator.load(QLocale::Language::English, translationFilePath))
        {
            if (!qApp->installTranslator(&m_translator))
            {
                AZ_Warning("MaterialCanvas", false, "Error installing translation %s!", unresolvedPath.data());
            }
        }
        else
        {
            AZ_Warning("MaterialCanvas", false, "Error loading translation file %s", unresolvedPath.data());
        }

        CreateMenus();

        GraphCanvas::AssetEditorNotificationBus::Handler::BusConnect(m_toolId);
        GraphCanvas::AssetEditorRequestBus::Handler::BusConnect(m_toolId);
        GraphCanvas::AssetEditorSettingsRequestBus::Handler::BusConnect(m_toolId);
        AzToolsFramework::ToolsApplicationNotificationBus::Handler::BusConnect();
        AzToolsFramework::AssetSystemBus::Handler::BusConnect();

        OnDocumentOpened(AZ::Uuid::CreateNull());
    }

    MaterialCanvasMainWindow::~MaterialCanvasMainWindow()
    {
        GraphCanvas::AssetEditorNotificationBus::Handler::BusDisconnect();
        GraphCanvas::AssetEditorRequestBus::Handler::BusDisconnect();
        GraphCanvas::AssetEditorSettingsRequestBus::Handler::BusDisconnect();
        AzToolsFramework::ToolsApplicationNotificationBus::Handler::BusDisconnect();
        AzToolsFramework::AssetSystemBus::Handler::BusDisconnect();

        delete m_presetEditor;
    }

    void MaterialCanvasMainWindow::OnDocumentOpened(const AZ::Uuid& documentId)
    {
        Base::OnDocumentOpened(documentId);
        m_materialInspector->SetDocumentId(documentId);

#if 0
        GraphCanvas::AssetEditorNotificationBus::Event(m_toolId, &GraphCanvas::AssetEditorNotifications::PreOnActiveGraphChanged);
        GraphCanvas::AssetEditorNotificationBus::Event(m_toolId, &GraphCanvas::AssetEditorNotifications::OnActiveGraphChanged, m_activeGraphId);
        GraphCanvas::AssetEditorNotificationBus::Event(m_toolId, &GraphCanvas::AssetEditorNotifications::PostOnActiveGraphChanged);
        GraphCanvas::AssetEditorNotificationBus::Event(m_toolId, &GraphCanvas::AssetEditorNotifications::OnGraphLoaded, m_activeGraphId);
        GraphCanvas::SceneNotificationBus::MultiHandler::BusDisconnect(m_activeGraphId);
        GraphCanvas::AssetEditorNotificationBus::Event(m_toolId, &GraphCanvas::AssetEditorNotifications::OnGraphUnloaded, m_activeGraphId);

        GraphCanvas::SceneNotificationBus::MultiHandler::BusDisconnect(m_activeGraphId);
        GraphCanvas::AssetEditorNotificationBus::Event(
            m_toolId, &GraphCanvas::AssetEditorNotifications::OnGraphRefreshed, m_activeGraphId, m_activeGraphId);
        GraphCanvas::SceneNotificationBus::MultiHandler::BusConnect(m_activeGraphId);
        //GraphCanvas::SceneMimeDelegateRequestBus::Event(
        //    m_activeGraphId, &GraphCanvas::SceneMimeDelegateRequests::AddDelegate, m_entityMimeDelegateId);
        //GraphCanvas::SceneRequestBus::Event(
        //    m_activeGraphId, &GraphCanvas::SceneRequests::SetMimeType, Widget::NodePaletteDockWidget::GetMimeType());
        GraphCanvas::SceneMemberNotificationBus::Event(m_activeGraphId, &GraphCanvas::SceneMemberNotifications::OnSceneReady);
        GraphCanvas::AssetEditorNotificationBus::Event(m_toolId, &GraphCanvas::AssetEditorNotifications::OnGraphLoaded, m_activeGraphId);

        // The paste action refreshes based on the scene's mimetype
        RefreshPasteAction();

        bool enabled = false;

        if (m_activeGraphId.IsValid())
        {
            GraphCanvas::ViewId viewId;
            GraphCanvas::SceneRequestBus::EventResult(viewId, m_activeGraphId, &GraphCanvas::SceneRequests::GetViewId);

            if (viewId.IsValid())
            {
                enabled = true;
            }
            else
            {
                AZ_Error("MaterialCanvasEditor", viewId.IsValid(), "SceneRequest must return a valid ViewId");
            }
        }
#endif
        UpdateMenuActions();
    }

    void MaterialCanvasMainWindow::ResizeViewportRenderTarget(uint32_t width, uint32_t height)
    {
        QSize requestedViewportSize = QSize(width, height) / devicePixelRatioF();
        QSize currentViewportSize = m_materialViewport->size();
        QSize offset = requestedViewportSize - currentViewportSize;
        QSize requestedWindowSize = size() + offset;
        resize(requestedWindowSize);

        AZ_Assert(
            m_materialViewport->size() == requestedViewportSize,
            "Resizing the window did not give the expected viewport size. Requested %d x %d but got %d x %d.",
            requestedViewportSize.width(), requestedViewportSize.height(), m_materialViewport->size().width(),
            m_materialViewport->size().height());

        [[maybe_unused]] QSize newDeviceSize = m_materialViewport->size();
        AZ_Warning(
            "Material Canvas", static_cast<uint32_t>(newDeviceSize.width()) == width && static_cast<uint32_t>(newDeviceSize.height()) == height,
            "Resizing the window did not give the expected frame size. Requested %d x %d but got %d x %d.", width, height,
            newDeviceSize.width(), newDeviceSize.height());
    }

    void MaterialCanvasMainWindow::LockViewportRenderTargetSize(uint32_t width, uint32_t height)
    {
        m_materialViewport->LockRenderTargetSize(width, height);
    }

    void MaterialCanvasMainWindow::UnlockViewportRenderTargetSize()
    {
        m_materialViewport->UnlockRenderTargetSize();
    }

    void MaterialCanvasMainWindow::OpenSettings()
    {
    }

    void MaterialCanvasMainWindow::OpenHelp()
    {
        QMessageBox::information(
            this, windowTitle(),
            R"(<html><head/><body>
            <p><h3><u>Material Canvas Controls</u></h3></p>
            <p><b>LMB</b> - pan camera</p>
            <p><b>RMB</b> or <b>Alt+LMB</b> - orbit camera around target</p>
            <p><b>MMB</b> or <b>Alt+MMB</b> - move camera on its xy plane</p>
            <p><b>Alt+RMB</b> or <b>LMB+RMB</b> - dolly camera on its z axis</p>
            <p><b>Ctrl+LMB</b> - rotate model</p>
            <p><b>Shift+LMB</b> - rotate environment</p>
            </body></html>)");
    }

    AZ::EntityId MaterialCanvasMainWindow::CreateNewGraph()
    {
        GraphCanvas::GraphId graphId;
        return graphId;
    }

    bool MaterialCanvasMainWindow::ContainsGraph([[maybe_unused]] const GraphCanvas::GraphId& graphId) const
    {
        return true;
    }

    bool MaterialCanvasMainWindow::CloseGraph([[maybe_unused]] const GraphCanvas::GraphId& graphId)
    {
        return true;
    }

    GraphCanvas::ContextMenuAction::SceneReaction MaterialCanvasMainWindow::ShowSceneContextMenu(const QPoint& screenPoint, const QPointF& scenePoint)
    {
        m_sceneContextMenu->ResetSourceSlotFilter();

        // We pass an invalid EntityId here since this is for the scene, there is no member to specify.
        return HandleContextMenu(*m_sceneContextMenu, AZ::EntityId(), screenPoint, scenePoint);
    }

    GraphCanvas::ContextMenuAction::SceneReaction MaterialCanvasMainWindow::ShowNodeContextMenu(
        const AZ::EntityId& nodeId, const QPoint& screenPoint, const QPointF& scenePoint)
    {
        GraphCanvas::NodeContextMenu contextMenu(m_toolId);
        return HandleContextMenu(contextMenu, nodeId, screenPoint, scenePoint);
    }

    GraphCanvas::ContextMenuAction::SceneReaction MaterialCanvasMainWindow::ShowCommentContextMenu(
        const AZ::EntityId& nodeId, const QPoint& screenPoint, const QPointF& scenePoint)
    {
        GraphCanvas::CommentContextMenu contextMenu(m_toolId);
        return HandleContextMenu(contextMenu, nodeId, screenPoint, scenePoint);
    }

    GraphCanvas::ContextMenuAction::SceneReaction MaterialCanvasMainWindow::ShowNodeGroupContextMenu(
        const AZ::EntityId& groupId, const QPoint& screenPoint, const QPointF& scenePoint)
    {
        GraphCanvas::NodeGroupContextMenu contextMenu(m_toolId);
        return HandleContextMenu(contextMenu, groupId, screenPoint, scenePoint);
    }

    GraphCanvas::ContextMenuAction::SceneReaction MaterialCanvasMainWindow::ShowCollapsedNodeGroupContextMenu(
        const AZ::EntityId& nodeId, const QPoint& screenPoint, const QPointF& scenePoint)
    {
        GraphCanvas::CollapsedNodeGroupContextMenu contextMenu(m_toolId);
        return HandleContextMenu(contextMenu, nodeId, screenPoint, scenePoint);
    }

    GraphCanvas::ContextMenuAction::SceneReaction MaterialCanvasMainWindow::ShowBookmarkContextMenu(
        const AZ::EntityId& bookmarkId, const QPoint& screenPoint, const QPointF& scenePoint)
    {
        GraphCanvas::BookmarkContextMenu contextMenu(m_toolId);
        return HandleContextMenu(contextMenu, bookmarkId, screenPoint, scenePoint);
    }

    GraphCanvas::ContextMenuAction::SceneReaction MaterialCanvasMainWindow::ShowConnectionContextMenu(
        const AZ::EntityId& connectionId, const QPoint& screenPoint, const QPointF& scenePoint)
    {
        GraphCanvas::ConnectionContextMenu contextMenu(m_toolId);
        return HandleContextMenu(contextMenu, connectionId, screenPoint, scenePoint);
    }

    GraphCanvas::ContextMenuAction::SceneReaction MaterialCanvasMainWindow::ShowSlotContextMenu(
        const AZ::EntityId& slotId, const QPoint& screenPoint, const QPointF& scenePoint)
    {
        GraphCanvas::SlotContextMenu contextMenu(m_toolId);
        return HandleContextMenu(contextMenu, slotId, screenPoint, scenePoint);
    }

    GraphCanvas::Endpoint MaterialCanvasMainWindow::CreateNodeForProposal(
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

    void MaterialCanvasMainWindow::OnWrapperNodeActionWidgetClicked(
        [[maybe_unused]] const AZ::EntityId& wrapperNode,
        [[maybe_unused]] const QRect& actionWidgetBoundingRect,
        [[maybe_unused]] const QPointF& scenePoint,
        [[maybe_unused]] const QPoint& screenPoint)
    {
    }

    GraphCanvas::EditorConstructPresets* MaterialCanvasMainWindow::GetConstructPresets() const
    {
        return const_cast<GraphCanvas::EditorConstructPresets*>(&m_constructPresetDefaults);
    }

    const GraphCanvas::ConstructTypePresetBucket* MaterialCanvasMainWindow::GetConstructTypePresetBucket(
        GraphCanvas::ConstructType constructType) const
    {
        return m_constructPresetDefaults.FindPresetBucket(constructType);
    }

    GraphCanvas::Styling::ConnectionCurveType MaterialCanvasMainWindow::GetConnectionCurveType() const
    {
        return GraphCanvas::Styling::ConnectionCurveType::Curved;
    }

    GraphCanvas::Styling::ConnectionCurveType MaterialCanvasMainWindow::GetDataConnectionCurveType() const
    {
        return GraphCanvas::Styling::ConnectionCurveType::Curved;
    }

    void MaterialCanvasMainWindow::OnActiveGraphChanged(const GraphCanvas::GraphId& graphId)
    {
        m_activeGraphId = graphId;
        GraphCanvas::SceneNotificationBus::MultiHandler::BusDisconnect();
        GraphCanvas::SceneNotificationBus::MultiHandler::BusConnect(m_activeGraphId);
        UpdateMenuActions();
    }

    void MaterialCanvasMainWindow::OnSelectionChanged()
    {
        UpdateMenuActions();
    }

    GraphCanvas::Endpoint MaterialCanvasMainWindow::HandleProposedConnection(
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

    GraphCanvas::ContextMenuAction::SceneReaction MaterialCanvasMainWindow::HandleContextMenu(
        GraphCanvas::EditorContextMenu& editorContextMenu, const AZ::EntityId& memberId, const QPoint& screenPoint, const QPointF& scenePoint) const
    {
        AZ::Vector2 sceneVector(aznumeric_cast<float>(scenePoint.x()), aznumeric_cast<float>(scenePoint.y()));

        editorContextMenu.RefreshActions(m_activeGraphId, memberId);

        QAction* result = editorContextMenu.exec(screenPoint);

        GraphCanvas::ContextMenuAction* contextMenuAction = qobject_cast<GraphCanvas::ContextMenuAction*>(result);

        if (contextMenuAction)
        {
            return contextMenuAction->TriggerAction(m_activeGraphId, sceneVector);
        }
        else if (editorContextMenu.GetNodePalette())
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

    void MaterialCanvasMainWindow::CreateMenus()
    {
        m_menuEdit->addSeparator();
        m_actionCut = m_menuEdit->addAction(tr("Cut"), [this] {
            GraphCanvas::SceneRequestBus::Event(m_activeGraphId, &GraphCanvas::SceneRequests::CutSelection);
        }, QKeySequence::Cut);
        m_actionCopy = m_menuEdit->addAction(tr("Copy"), [this] {
            GraphCanvas::SceneRequestBus::Event(m_activeGraphId, &GraphCanvas::SceneRequests::CopySelection);
        }, QKeySequence::Copy);
        m_actionPaste = m_menuEdit->addAction(tr("Paste"), [this] {
            GraphCanvas::SceneRequestBus::Event(m_activeGraphId, &GraphCanvas::SceneRequests::Paste);
        }, QKeySequence::Paste);
        m_actionDuplicate = m_menuEdit->addAction(tr("Duplicate"), [this] {
            GraphCanvas::SceneRequestBus::Event(m_activeGraphId, &GraphCanvas::SceneRequests::DuplicateSelection);
        });
        m_actionDelete = m_menuEdit->addAction(tr("Delete"), [this] {
            GraphCanvas::SceneRequestBus::Event(m_activeGraphId, &GraphCanvas::SceneRequests::DeleteSelection);
        }, QKeySequence::Delete);

        m_menuEdit->addSeparator();
        m_actionRemoveUnusedNodes = m_menuEdit->addAction(tr("Remove Unused Nodes"), [this] {
            GraphCanvas::SceneRequestBus::Event(m_activeGraphId, &GraphCanvas::SceneRequests::RemoveUnusedNodes);
        });
        m_actionRemoveUnusedElements = m_menuEdit->addAction(tr("Remove Unused Elements"), [this] {
            GraphCanvas::SceneRequestBus::Event(m_activeGraphId, &GraphCanvas::SceneRequests::RemoveUnusedElements);
        });

        m_menuEdit->addSeparator();
        m_actionSelectAll = m_menuEdit->addAction(tr("Select All"), [this] {
            GraphCanvas::SceneRequestBus::Event(m_activeGraphId, &GraphCanvas::SceneRequests::SelectAll);
        });
        m_actionSelectInputs = m_menuEdit->addAction(tr("Select Inputs"), [this] {
            GraphCanvas::SceneRequestBus::Event(
                m_activeGraphId, &GraphCanvas::SceneRequests::SelectAllRelative, GraphCanvas::ConnectionType::CT_Input);
        });
        m_actionSelectOutputs = m_menuEdit->addAction(tr("Select Outputs"), [this] {
            GraphCanvas::SceneRequestBus::Event(
                m_activeGraphId, &GraphCanvas::SceneRequests::SelectAllRelative, GraphCanvas::ConnectionType::CT_Output);
        });
        m_actionSelectConnected = m_menuEdit->addAction(tr("Select Connected"), [this] {
            GraphCanvas::SceneRequestBus::Event(m_activeGraphId, &GraphCanvas::SceneRequests::SelectConnectedNodes);
        });
        m_actionSelectNone = m_menuEdit->addAction(tr("Clear Selection"), [this] {
            GraphCanvas::SceneRequestBus::Event(m_activeGraphId, &GraphCanvas::SceneRequests::ClearSelection);
        });
        m_actionSelectEnable = m_menuEdit->addAction(tr("Enable Selection"), [this] {
            GraphCanvas::SceneRequestBus::Event(m_activeGraphId, &GraphCanvas::SceneRequests::EnableSelection);
        });
        m_actionSelectDisable = m_menuEdit->addAction(tr("Disable Selection"), [this] {
            GraphCanvas::SceneRequestBus::Event(m_activeGraphId, &GraphCanvas::SceneRequests::DisableSelection);
        });

        m_menuEdit->addSeparator();
        m_actionScreenShot = m_menuEdit->addAction(tr("Screenshot"), [this] {
            GraphCanvas::ViewId viewId;
            GraphCanvas::SceneRequestBus::EventResult(viewId, m_activeGraphId, &GraphCanvas::SceneRequests::GetViewId);
            GraphCanvas::ViewRequestBus::Event(viewId, &GraphCanvas::ViewRequests::ScreenshotSelection);
        });

        m_menuEdit->addSeparator();
        m_actionAlignTop = m_menuEdit->addAction(tr("Align Top"), [this] {
            GraphCanvas::AlignConfig alignConfig;
            alignConfig.m_horAlign = GraphCanvas::GraphUtils::HorizontalAlignment::None;
            alignConfig.m_verAlign = GraphCanvas::GraphUtils::VerticalAlignment::Top;
            alignConfig.m_alignTime = GetAlignmentTime();
            AlignSelected(alignConfig);
        });
        m_actionAlignBottom = m_menuEdit->addAction(tr("Align Bottom"), [this] {
            GraphCanvas::AlignConfig alignConfig;
            alignConfig.m_horAlign = GraphCanvas::GraphUtils::HorizontalAlignment::None;
            alignConfig.m_verAlign = GraphCanvas::GraphUtils::VerticalAlignment::Bottom;
            alignConfig.m_alignTime = GetAlignmentTime();
            AlignSelected(alignConfig);
        });
        m_actionAlignLeft = m_menuEdit->addAction(tr("Align Left"), [this] {
            GraphCanvas::AlignConfig alignConfig;
            alignConfig.m_horAlign = GraphCanvas::GraphUtils::HorizontalAlignment::Left;
            alignConfig.m_verAlign = GraphCanvas::GraphUtils::VerticalAlignment::None;
            alignConfig.m_alignTime = GetAlignmentTime();
            AlignSelected(alignConfig);
        });
        m_actionAlignRight = m_menuEdit->addAction(tr("Align Right"), [this] {
            GraphCanvas::AlignConfig alignConfig;
            alignConfig.m_horAlign = GraphCanvas::GraphUtils::HorizontalAlignment::Right;
            alignConfig.m_verAlign = GraphCanvas::GraphUtils::VerticalAlignment::None;
            alignConfig.m_alignTime = GetAlignmentTime();
            AlignSelected(alignConfig);
        });

        m_menuView->addSeparator();
        m_actionPresetEditor = m_menuView->addAction(tr("Preset Editor"), [this] { OnViewPresetsEditor(); });

        m_menuView->addSeparator();
        m_actionShowEntireGraph = m_menuView->addAction(tr("Show Entire Graph"), [this] {
            GraphCanvas::ViewId viewId;
            GraphCanvas::SceneRequestBus::EventResult(viewId, m_activeGraphId, &GraphCanvas::SceneRequests::GetViewId);
            GraphCanvas::ViewRequestBus::Event(viewId, &GraphCanvas::ViewRequests::ShowEntireGraph);
        });
        m_actionZoomIn = m_menuView->addAction(tr("Zoom In"), [this] {
            GraphCanvas::ViewId viewId;
            GraphCanvas::SceneRequestBus::EventResult(viewId, m_activeGraphId, &GraphCanvas::SceneRequests::GetViewId);
            GraphCanvas::ViewRequestBus::Event(viewId, &GraphCanvas::ViewRequests::ZoomIn);
        });
        m_actionZoomIn->setShortcuts({ QKeySequence(Qt::CTRL + Qt::Key_Plus), QKeySequence(Qt::CTRL + Qt::Key_Equal) });
        m_actionZoomOut = m_menuView->addAction(tr("Zoom Out"), [this] {
            GraphCanvas::ViewId viewId;
            GraphCanvas::SceneRequestBus::EventResult(viewId, m_activeGraphId, &GraphCanvas::SceneRequests::GetViewId);
            GraphCanvas::ViewRequestBus::Event(viewId, &GraphCanvas::ViewRequests::ZoomOut);
        });
        m_actionZoomOut->setShortcuts({ QKeySequence(Qt::CTRL + Qt::Key_Minus), QKeySequence(Qt::CTRL + Qt::Key_hyphen) });
        m_actionZoomSelection = m_menuView->addAction(tr("Zoom Selection"), [this] {
            GraphCanvas::ViewId viewId;
            GraphCanvas::SceneRequestBus::EventResult(viewId, m_activeGraphId, &GraphCanvas::SceneRequests::GetViewId);
            GraphCanvas::ViewRequestBus::Event(viewId, &GraphCanvas::ViewRequests::CenterOnSelection);
        });

        m_menuView->addSeparator();
        m_actionGotoStartOfChain = m_menuView->addAction(tr("Goto Start Of Chain"), [this] {
            GraphCanvas::ViewId viewId;
            GraphCanvas::SceneRequestBus::EventResult(viewId, m_activeGraphId, &GraphCanvas::SceneRequests::GetViewId);
            GraphCanvas::ViewRequestBus::Event(viewId, &GraphCanvas::ViewRequests::CenterOnStartOfChain);
        });
        m_actionGotoEndOfChain = m_menuView->addAction(tr("Goto End Of Chain"), [this] {
            GraphCanvas::ViewId viewId;
            GraphCanvas::SceneRequestBus::EventResult(viewId, m_activeGraphId, &GraphCanvas::SceneRequests::GetViewId);
            GraphCanvas::ViewRequestBus::Event(viewId, &GraphCanvas::ViewRequests::CenterOnEndOfChain);
        });

        UpdateMenuActions();
        connect(m_menuEdit, &QMenu::aboutToShow, this, &MaterialCanvasMainWindow::UpdateMenuActions);
        connect(QApplication::clipboard(), &QClipboard::dataChanged, this, &MaterialCanvasMainWindow::UpdateMenuActions);
    }

    void MaterialCanvasMainWindow::UpdateMenuActions()
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
        const bool pasteableClipboard = hasGraph && !copyMimeType.empty() && QApplication::clipboard()->mimeData()->hasFormat(copyMimeType.c_str());

        m_actionCut->setEnabled(hasCopiableSelection);
        m_actionCopy->setEnabled(hasCopiableSelection);
        m_actionPaste->setEnabled(pasteableClipboard);
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

    void MaterialCanvasMainWindow::AlignSelected(const GraphCanvas::AlignConfig& alignConfig)
    {
        AZStd::vector<GraphCanvas::NodeId> selectedNodes;
        GraphCanvas::SceneRequestBus::EventResult(selectedNodes, m_activeGraphId, &GraphCanvas::SceneRequests::GetSelectedNodes);
        GraphCanvas::GraphUtils::AlignNodes(selectedNodes, alignConfig);
    }

    void MaterialCanvasMainWindow::OnViewPresetsEditor()
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

    GraphCanvas::GraphCanvasTreeItem* MaterialCanvasMainWindow::GetNodePaletteRootTreeItem() const
    {
        GraphCanvas::NodePaletteTreeItem* rootItem = aznew GraphCanvas::NodePaletteTreeItem("Root", m_toolId);
        auto nodeCategory1 = rootItem->CreateChildNode<GraphCanvas::IconDecoratedNodePaletteTreeItem>("Node Category 1", m_toolId);
        nodeCategory1->SetTitlePalette("NodeCategory1");
        auto nodeCategory2 = rootItem->CreateChildNode<GraphCanvas::IconDecoratedNodePaletteTreeItem>("Node Category 2", m_toolId);
        nodeCategory2->SetTitlePalette("NodeCategory2");
        auto nodeCategory3 = rootItem->CreateChildNode<GraphCanvas::IconDecoratedNodePaletteTreeItem>("Node Category 3", m_toolId);
        nodeCategory3->SetTitlePalette("NodeCategory3");
        auto nodeCategory4 = rootItem->CreateChildNode<GraphCanvas::IconDecoratedNodePaletteTreeItem>("Node Category 4", m_toolId);
        nodeCategory4->SetTitlePalette("NodeCategory4");
        auto nodeCategory5 = rootItem->CreateChildNode<GraphCanvas::IconDecoratedNodePaletteTreeItem>("Node Category 5", m_toolId);
        nodeCategory5->SetTitlePalette("NodeCategory5");
        return rootItem;
    }
} // namespace MaterialCanvas

#include <Window/moc_MaterialCanvasMainWindow.cpp>
