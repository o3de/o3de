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
        {
            m_takeScreenshot = new QToolButton();
            m_takeScreenshot->setToolTip("Captures a full resolution screenshot of the entire graph or selected nodes into the clipboard");
            m_takeScreenshot->setIcon(QIcon(":/MaterialCanvasEditorResources/Resources/scriptcanvas_screenshot.png"));
            m_takeScreenshot->setEnabled(false);

            m_editorToolbar->AddCustomAction(m_takeScreenshot);
            connect(m_takeScreenshot, &QToolButton::clicked, this, &MaterialCanvasMainWindow::OnScreenshot);
        }

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

        UpdateMenuState(enabled);
#endif
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

        //UpdateMenuActions();
    }

    void MaterialCanvasMainWindow::OnSelectionChanged()
    {
        //UpdateEditMenuActions();
    }

    void MaterialCanvasMainWindow::OnNodeAdded([[maybe_unused]] const AZ::EntityId& nodeId, [[maybe_unused]] bool isPaste)
    {
        // Handle special-case where if a method node is created that has an AZ::Event output slot,
        // we will automatically create the AZ::Event Handler node for the user
        AZStd::vector<GraphCanvas::SlotId> outputDataSlotIds;
        GraphCanvas::NodeRequestBus::EventResult(
            outputDataSlotIds, nodeId, &GraphCanvas::NodeRequests::FindVisibleSlotIdsByType, GraphCanvas::CT_Output,
            GraphCanvas::SlotTypes::DataSlot);

#if 0
        for (const auto& slotId : outputDataSlotIds)
        {
            if (!IsInUndoRedo(m_activeGraphId) && !isPaste &&
                CreateAzEventHandlerSlotMenuAction::FindBehaviorMethodWithAzEventReturn(m_activeGraphId, slotId))
            {
                CreateAzEventHandlerSlotMenuAction eventHandlerAction(this);
                eventHandlerAction.RefreshAction(m_activeGraphId, slotId);

                AZ::Vector2 position;
                GraphCanvas::GeometryRequestBus::EventResult(position, nodeId, &GraphCanvas::GeometryRequests::GetPosition);

                eventHandlerAction.TriggerAction(m_activeGraphId, position);
                break;
            }
        }
#endif
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
#if 0
        // File menu
        connect(ui->action_New_Script, &QAction::triggered, this, &MaterialCanvasMainWindow::OnFileNew);
        ui->action_New_Script->setShortcut(QKeySequence(QKeySequence::New));

        connect(ui->action_Open, &QAction::triggered, this, &MaterialCanvasMainWindow::OnFileOpen);
        ui->action_Open->setShortcut(QKeySequence(QKeySequence::Open));

        connect(ui->action_UpgradeTool, &QAction::triggered, this, &MaterialCanvasMainWindow::RunUpgradeTool);
        ui->action_UpgradeTool->setVisible(true);

        // List of recent files.
        {
            QMenu* recentMenu = new QMenu("Open &Recent");

            for (int i = 0; i < m_recentActions.size(); ++i)
            {
                QAction* action = new QAction(this);
                action->setVisible(false);
                m_recentActions[i] = AZStd::make_pair(action, QMetaObject::Connection());
                recentMenu->addAction(action);
            }

            connect(recentMenu, &QMenu::aboutToShow, this, &MaterialCanvasMainWindow::UpdateRecentMenu);

            recentMenu->addSeparator();

            // Clear Recent Files.
            {
                QAction* action = new QAction("&Clear Recent Files", this);

                connect(
                    action, &QAction::triggered,
                    [this](bool /*checked*/)
                    {
                        ClearRecentFile();
                        UpdateRecentMenu();
                    });

                recentMenu->addAction(action);
            }

            ui->menuFile->insertMenu(ui->action_Save, recentMenu);
            ui->menuFile->insertSeparator(ui->action_Save);
        }

        connect(ui->action_Save, &QAction::triggered, this, &MaterialCanvasMainWindow::OnFileSaveCaller);
        ui->action_Save->setShortcut(QKeySequence(QKeySequence::Save));

        connect(ui->action_Save_As, &QAction::triggered, this, &MaterialCanvasMainWindow::OnFileSaveAsCaller);
        ui->action_Save_As->setShortcut(QKeySequence(tr("Ctrl+Shift+S", "File|Save As...")));

        connect(
            ui->action_Close, &QAction::triggered,
            [this](bool /*checked*/)
            {
                m_tabBar->tabCloseRequested(m_tabBar->currentIndex());
            });
        ui->action_Close->setShortcut(QKeySequence(QKeySequence::Close));

        // Edit Menu
        SetupEditMenu();

        // View menu
        connect(ui->action_ViewNodePalette, &QAction::triggered, this, &MaterialCanvasMainWindow::OnViewNodePalette);
        connect(ui->action_ViewMiniMap, &QAction::triggered, this, &MaterialCanvasMainWindow::OnViewMiniMap);

        connect(ui->action_ViewProperties, &QAction::triggered, this, &MaterialCanvasMainWindow::OnViewProperties);
        connect(ui->action_ViewBookmarks, &QAction::triggered, this, &MaterialCanvasMainWindow::OnBookmarks);

        connect(ui->action_ViewVariableManager, &QAction::triggered, this, &MaterialCanvasMainWindow::OnVariableManager);
        connect(m_variableDockWidget, &QDockWidget::visibilityChanged, this, &MaterialCanvasMainWindow::OnViewVisibilityChanged);

        connect(ui->action_ViewLogWindow, &QAction::triggered, this, &MaterialCanvasMainWindow::OnViewLogWindow);
        connect(m_loggingWindow, &QDockWidget::visibilityChanged, this, &MaterialCanvasMainWindow::OnViewVisibilityChanged);

        connect(ui->action_ViewDebugger, &QAction::triggered, this, &MaterialCanvasMainWindow::OnViewDebugger);
        connect(ui->action_ViewCommandLine, &QAction::triggered, this, &MaterialCanvasMainWindow::OnViewCommandLine);
        connect(ui->action_ViewLog, &QAction::triggered, this, &MaterialCanvasMainWindow::OnViewLog);

        connect(ui->action_GraphValidation, &QAction::triggered, this, &MaterialCanvasMainWindow::OnViewGraphValidation);
        connect(ui->action_Debugging, &QAction::triggered, this, &MaterialCanvasMainWindow::OnViewDebuggingWindow);

        connect(ui->action_ViewUnitTestManager, &QAction::triggered, this, &MaterialCanvasMainWindow::OnViewUnitTestManager);
        connect(ui->action_NodeStatistics, &QAction::triggered, this, &MaterialCanvasMainWindow::OnViewStatisticsPanel);
        connect(ui->action_PresetsEditor, &QAction::triggered, this, &MaterialCanvasMainWindow::OnViewPresetsEditor);

        connect(ui->action_ViewRestoreDefaultLayout, &QAction::triggered, this, &MaterialCanvasMainWindow::OnRestoreDefaultLayout);
#endif
    }

    void MaterialCanvasMainWindow::SetupEditMenu()
    {
#if 0
        ui->action_Undo->setShortcut(QKeySequence::Undo);
        ui->action_Cut->setShortcut(QKeySequence(QKeySequence::Cut));
        ui->action_Copy->setShortcut(QKeySequence(QKeySequence::Copy));
        ui->action_Paste->setShortcut(QKeySequence(QKeySequence::Paste));
        ui->action_Delete->setShortcut(QKeySequence(QKeySequence::Delete));

        connect(ui->menuEdit, &QMenu::aboutToShow, this, &MaterialCanvasMainWindow::OnEditMenuShow);

        // Edit Menu
        connect(ui->action_Undo, &QAction::triggered, this, &MaterialCanvasMainWindow::TriggerUndo);
        connect(ui->action_Redo, &QAction::triggered, this, &MaterialCanvasMainWindow::TriggerRedo);
        connect(ui->action_Cut, &QAction::triggered, this, &MaterialCanvasMainWindow::OnEditCut);
        connect(ui->action_Copy, &QAction::triggered, this, &MaterialCanvasMainWindow::OnEditCopy);
        connect(ui->action_Paste, &QAction::triggered, this, &MaterialCanvasMainWindow::OnEditPaste);
        connect(ui->action_Duplicate, &QAction::triggered, this, &MaterialCanvasMainWindow::OnEditDuplicate);
        connect(ui->action_Delete, &QAction::triggered, this, &MaterialCanvasMainWindow::OnEditDelete);
        connect(QApplication::clipboard(), &QClipboard::dataChanged, this, &MaterialCanvasMainWindow::RefreshPasteAction);
        connect(ui->action_RemoveUnusedNodes, &QAction::triggered, this, &MaterialCanvasMainWindow::OnRemoveUnusedNodes);
        connect(ui->action_RemoveUnusedVariables, &QAction::triggered, this, &MaterialCanvasMainWindow::OnRemoveUnusedVariables);
        connect(ui->action_RemoveUnusedElements, &QAction::triggered, this, &MaterialCanvasMainWindow::OnRemoveUnusedElements);
        connect(ui->action_Screenshot, &QAction::triggered, this, &MaterialCanvasMainWindow::OnScreenshot);
        connect(ui->action_SelectAll, &QAction::triggered, this, &MaterialCanvasMainWindow::OnSelectAll);
        connect(ui->action_SelectInputs, &QAction::triggered, this, &MaterialCanvasMainWindow::OnSelectInputs);
        connect(ui->action_SelectOutputs, &QAction::triggered, this, &MaterialCanvasMainWindow::OnSelectOutputs);
        connect(ui->action_SelectConnected, &QAction::triggered, this, &MaterialCanvasMainWindow::OnSelectConnected);
        connect(ui->action_ClearSelection, &QAction::triggered, this, &MaterialCanvasMainWindow::OnClearSelection);
        connect(ui->action_EnableSelection, &QAction::triggered, this, &MaterialCanvasMainWindow::OnEnableSelection);
        connect(ui->action_DisableSelection, &QAction::triggered, this, &MaterialCanvasMainWindow::OnDisableSelection);
        connect(ui->action_AlignTop, &QAction::triggered, this, &MaterialCanvasMainWindow::OnAlignTop);
        connect(ui->action_AlignBottom, &QAction::triggered, this, &MaterialCanvasMainWindow::OnAlignBottom);
        connect(ui->action_AlignLeft, &QAction::triggered, this, &MaterialCanvasMainWindow::OnAlignLeft);
        connect(ui->action_AlignRight, &QAction::triggered, this, &MaterialCanvasMainWindow::OnAlignRight);

        ui->action_ZoomIn->setShortcuts({ QKeySequence(Qt::CTRL + Qt::Key_Plus), QKeySequence(Qt::CTRL + Qt::Key_Equal) });

        // View Menu
        connect(ui->action_ShowEntireGraph, &QAction::triggered, this, &MaterialCanvasMainWindow::OnShowEntireGraph);
        connect(ui->action_ZoomIn, &QAction::triggered, this, &MaterialCanvasMainWindow::OnZoomIn);
        connect(ui->action_ZoomOut, &QAction::triggered, this, &MaterialCanvasMainWindow::OnZoomOut);
        connect(ui->action_ZoomSelection, &QAction::triggered, this, &MaterialCanvasMainWindow::OnZoomToSelection);
        connect(ui->action_GotoStartOfChain, &QAction::triggered, this, &MaterialCanvasMainWindow::OnGotoStartOfChain);
        connect(ui->action_GotoEndOfChain, &QAction::triggered, this, &MaterialCanvasMainWindow::OnGotoEndOfChain);
#endif
    }

    void MaterialCanvasMainWindow::OnEditCut()
    {
        GraphCanvas::SceneRequestBus::Event(m_activeGraphId, &GraphCanvas::SceneRequests::CutSelection);
    }

    void MaterialCanvasMainWindow::OnEditCopy()
    {
        GraphCanvas::SceneRequestBus::Event(m_activeGraphId, &GraphCanvas::SceneRequests::CopySelection);
    }

    void MaterialCanvasMainWindow::OnEditPaste()
    {
        GraphCanvas::SceneRequestBus::Event(m_activeGraphId, &GraphCanvas::SceneRequests::Paste);
    }

    void MaterialCanvasMainWindow::OnEditDuplicate()
    {
        GraphCanvas::SceneRequestBus::Event(m_activeGraphId, &GraphCanvas::SceneRequests::DuplicateSelection);
    }

    void MaterialCanvasMainWindow::OnEditDelete()
    {
        GraphCanvas::SceneRequestBus::Event(m_activeGraphId, &GraphCanvas::SceneRequests::DeleteSelection);
    }

    void MaterialCanvasMainWindow::OnRemoveUnusedNodes()
    {
        GraphCanvas::SceneRequestBus::Event(m_activeGraphId, &GraphCanvas::SceneRequests::RemoveUnusedNodes);
    }

    void MaterialCanvasMainWindow::OnRemoveUnusedElements()
    {
        GraphCanvas::SceneRequestBus::Event(m_activeGraphId, &GraphCanvas::SceneRequests::RemoveUnusedElements);
    }

    void MaterialCanvasMainWindow::OnScreenshot()
    {
        GraphCanvas::ViewId viewId;
        GraphCanvas::SceneRequestBus::EventResult(viewId, m_activeGraphId, &GraphCanvas::SceneRequests::GetViewId);
        GraphCanvas::ViewRequestBus::Event(viewId, &GraphCanvas::ViewRequests::ScreenshotSelection);
    }

    void MaterialCanvasMainWindow::OnSelectAll()
    {
        GraphCanvas::SceneRequestBus::Event(m_activeGraphId, &GraphCanvas::SceneRequests::SelectAll);
    }

    void MaterialCanvasMainWindow::OnSelectInputs()
    {
        GraphCanvas::SceneRequestBus::Event(
            m_activeGraphId, &GraphCanvas::SceneRequests::SelectAllRelative, GraphCanvas::ConnectionType::CT_Input);
    }

    void MaterialCanvasMainWindow::OnSelectOutputs()
    {
        GraphCanvas::SceneRequestBus::Event(
            m_activeGraphId, &GraphCanvas::SceneRequests::SelectAllRelative, GraphCanvas::ConnectionType::CT_Output);

        GraphCanvas::ViewId viewId;
        GraphCanvas::SceneRequestBus::EventResult(viewId, m_activeGraphId, &GraphCanvas::SceneRequests::GetViewId);
    }

    void MaterialCanvasMainWindow::OnSelectConnected()
    {
        GraphCanvas::SceneRequestBus::Event(m_activeGraphId, &GraphCanvas::SceneRequests::SelectConnectedNodes);
    }

    void MaterialCanvasMainWindow::OnClearSelection()
    {
        GraphCanvas::SceneRequestBus::Event(m_activeGraphId, &GraphCanvas::SceneRequests::ClearSelection);
    }

    void MaterialCanvasMainWindow::OnEnableSelection()
    {
        GraphCanvas::SceneRequestBus::Event(m_activeGraphId, &GraphCanvas::SceneRequests::EnableSelection);
    }

    void MaterialCanvasMainWindow::OnDisableSelection()
    {
        GraphCanvas::SceneRequestBus::Event(m_activeGraphId, &GraphCanvas::SceneRequests::DisableSelection);
    }

    void MaterialCanvasMainWindow::OnAlignTop()
    {
        GraphCanvas::AlignConfig alignConfig;

        alignConfig.m_horAlign = GraphCanvas::GraphUtils::HorizontalAlignment::None;
        alignConfig.m_verAlign = GraphCanvas::GraphUtils::VerticalAlignment::Top;
        alignConfig.m_alignTime = GetAlignmentTime();

        AlignSelected(alignConfig);
    }

    void MaterialCanvasMainWindow::OnAlignBottom()
    {
        GraphCanvas::AlignConfig alignConfig;

        alignConfig.m_horAlign = GraphCanvas::GraphUtils::HorizontalAlignment::None;
        alignConfig.m_verAlign = GraphCanvas::GraphUtils::VerticalAlignment::Bottom;
        alignConfig.m_alignTime = GetAlignmentTime();

        AlignSelected(alignConfig);
    }

    void MaterialCanvasMainWindow::OnAlignLeft()
    {
        GraphCanvas::AlignConfig alignConfig;

        alignConfig.m_horAlign = GraphCanvas::GraphUtils::HorizontalAlignment::Left;
        alignConfig.m_verAlign = GraphCanvas::GraphUtils::VerticalAlignment::None;
        alignConfig.m_alignTime = GetAlignmentTime();

        AlignSelected(alignConfig);
    }

    void MaterialCanvasMainWindow::OnAlignRight()
    {
        GraphCanvas::AlignConfig alignConfig;

        alignConfig.m_horAlign = GraphCanvas::GraphUtils::HorizontalAlignment::Right;
        alignConfig.m_verAlign = GraphCanvas::GraphUtils::VerticalAlignment::None;
        alignConfig.m_alignTime = GetAlignmentTime();

        AlignSelected(alignConfig);
    }

    void MaterialCanvasMainWindow::AlignSelected(const GraphCanvas::AlignConfig& alignConfig)
    {
        AZStd::vector<GraphCanvas::NodeId> selectedNodes;
        GraphCanvas::SceneRequestBus::EventResult(selectedNodes, m_activeGraphId, &GraphCanvas::SceneRequests::GetSelectedNodes);
        GraphCanvas::GraphUtils::AlignNodes(selectedNodes, alignConfig);
    }

    void MaterialCanvasMainWindow::OnShowEntireGraph()
    {
        GraphCanvas::ViewId viewId;
        GraphCanvas::SceneRequestBus::EventResult(viewId, m_activeGraphId, &GraphCanvas::SceneRequests::GetViewId);
        GraphCanvas::ViewRequestBus::Event(viewId, &GraphCanvas::ViewRequests::ShowEntireGraph);
    }

    void MaterialCanvasMainWindow::OnZoomIn()
    {
        GraphCanvas::ViewId viewId;
        GraphCanvas::SceneRequestBus::EventResult(viewId, m_activeGraphId, &GraphCanvas::SceneRequests::GetViewId);
        GraphCanvas::ViewRequestBus::Event(viewId, &GraphCanvas::ViewRequests::ZoomIn);
    }

    void MaterialCanvasMainWindow::OnZoomOut()
    {
        GraphCanvas::ViewId viewId;
        GraphCanvas::SceneRequestBus::EventResult(viewId, m_activeGraphId, &GraphCanvas::SceneRequests::GetViewId);
        GraphCanvas::ViewRequestBus::Event(viewId, &GraphCanvas::ViewRequests::ZoomOut);
    }

    void MaterialCanvasMainWindow::OnZoomToSelection()
    {
        GraphCanvas::ViewId viewId;
        GraphCanvas::SceneRequestBus::EventResult(viewId, m_activeGraphId, &GraphCanvas::SceneRequests::GetViewId);
        GraphCanvas::ViewRequestBus::Event(viewId, &GraphCanvas::ViewRequests::CenterOnSelection);
    }

    void MaterialCanvasMainWindow::OnGotoStartOfChain()
    {
        GraphCanvas::ViewId viewId;
        GraphCanvas::SceneRequestBus::EventResult(viewId, m_activeGraphId, &GraphCanvas::SceneRequests::GetViewId);
        GraphCanvas::ViewRequestBus::Event(viewId, &GraphCanvas::ViewRequests::CenterOnStartOfChain);
    }

    void MaterialCanvasMainWindow::OnGotoEndOfChain()
    {
        GraphCanvas::ViewId viewId;
        GraphCanvas::SceneRequestBus::EventResult(viewId, m_activeGraphId, &GraphCanvas::SceneRequests::GetViewId);
        GraphCanvas::ViewRequestBus::Event(viewId, &GraphCanvas::ViewRequests::CenterOnEndOfChain);
    }

    void MaterialCanvasMainWindow::RefreshSelection()
    {
        // Get the selected nodes.
        bool hasCopiableSelection = false;
        GraphCanvas::SceneRequestBus::EventResult(hasCopiableSelection, m_activeGraphId, &GraphCanvas::SceneRequests::HasCopiableSelection);

        AZStd::vector<AZ::EntityId> selection;
        GraphCanvas::SceneRequestBus::EventResult(selection, m_activeGraphId, &GraphCanvas::SceneRequests::GetSelectedItems);
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

#if 0
    void MaterialCanvasMainWindow::DeleteNodes(const AZ::EntityId& graphId, const AZStd::vector <AZ::EntityId>& nodes)
    {
        // clear the selection then delete the nodes that were selected
        GraphCanvas::SceneRequestBus::Event(graphId, &GraphCanvas::SceneRequests::ClearSelection);
        GraphCanvas::SceneRequestBus::Event(
            graphId, &GraphCanvas::SceneRequests::Delete, AZStd::unordered_set<AZ::EntityId>{ nodes.begin(), nodes.end() });
    }

    void MaterialCanvasMainWindow::DeleteConnections(const AZ::EntityId& graphId, const AZStd::vector<AZ::EntityId>& connections)
    {
        ScopedVariableSetter<bool> scopedIgnoreSelection(m_ignoreSelection, true);
        GraphCanvas::SceneRequestBus::Event(
            graphId, &GraphCanvas::SceneRequests::Delete,
            AZStd::unordered_set<AZ::EntityId>{ connections.begin(), connections.end() });
    }

    void MaterialCanvasMainWindow::DisconnectEndpoints(const AZ::EntityId& graphId, const AZStd::vector<GraphCanvas::Endpoint>& endpoints)
    {
        AZStd::unordered_set<AZ::EntityId> connections;
        for (const auto& endpoint : endpoints)
        {
            AZStd::vector<AZ::EntityId> endpointConnections;
            GraphCanvas::SceneRequestBus::EventResult(
                endpointConnections, graphId, &GraphCanvas::SceneRequests::GetConnectionsForEndpoint, endpoint);
            connections.insert(endpointConnections.begin(), endpointConnections.end());
        }
        DeleteConnections(graphId, { connections.begin(), connections.end() });
    }
    #endif

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
