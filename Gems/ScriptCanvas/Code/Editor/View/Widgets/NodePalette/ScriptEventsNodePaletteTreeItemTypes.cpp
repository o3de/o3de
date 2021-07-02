/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "precompiled.h"

#include <qmenu.h>
#include <QPixmap>

#include <AzToolsFramework/AssetEditor/AssetEditorUtils.h>

#include <Editor/Nodes/NodeCreateUtils.h>
#include <Editor/Nodes/NodeDisplayUtils.h>

#include <ScriptCanvas/Bus/RequestBus.h>
#include <ScriptCanvas/GraphCanvas/NodeDescriptorBus.h>
#include <ScriptCanvas/Libraries/Core/SendScriptEvent.h>

#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/VisualBus.h>
#include <GraphCanvas/Components/GridBus.h>
#include <GraphCanvas/Components/Nodes/Wrapper/WrapperNodeBus.h>
#include <GraphCanvas/Components/StyleBus.h>
#include <GraphCanvas/Widgets/GraphCanvasGraphicsView/GraphCanvasGraphicsView.h>

#include <Editor/GraphCanvas/GraphCanvasEditorNotificationBusId.h>
#include <Editor/View/Widgets/NodePalette/ScriptEventsNodePaletteTreeItemTypes.h>

namespace ScriptCanvasEditor
{
    //////////////////////////////////////
    // CreateScriptEventsHandlerMimeEvent
    //////////////////////////////////////

    void CreateScriptEventsHandlerMimeEvent::Reflect(AZ::ReflectContext* reflectContext)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext))
        {
            serializeContext->Class<CreateScriptEventsHandlerMimeEvent, GraphCanvas::GraphCanvasMimeEvent>()
                ->Version(1)
                ->Field("m_assetId", &CreateScriptEventsHandlerMimeEvent::m_assetId)
                ;
        }
    }

    CreateScriptEventsHandlerMimeEvent::CreateScriptEventsHandlerMimeEvent(const AZ::Data::AssetId assetId, const ScriptEvents::Method& methodDefinition)
        : m_assetId(assetId)
        , m_methodDefinition(methodDefinition)
    {
    }

    ScriptCanvasEditor::NodeIdPair CreateScriptEventsHandlerMimeEvent::CreateNode(const ScriptCanvas::ScriptCanvasId& scriptCanvasId) const
    {
        return Nodes::CreateScriptEventReceiverNode(scriptCanvasId, m_assetId);
    }

    bool CreateScriptEventsHandlerMimeEvent::ExecuteEvent([[maybe_unused]] const AZ::Vector2& mouseDropPosition, AZ::Vector2& sceneDropPosition, const AZ::EntityId& graphCanvasGraphId)
    {
        ScriptCanvas::ScriptCanvasId scriptCanvasId;
        GeneralRequestBus::BroadcastResult(scriptCanvasId, &GeneralRequests::GetScriptCanvasId, graphCanvasGraphId);

        if (!scriptCanvasId.IsValid() || !graphCanvasGraphId.IsValid())
        {
            return false;
        }

        m_nodeIdPair = CreateNode(scriptCanvasId);

        if (m_nodeIdPair.m_graphCanvasId.IsValid() && m_nodeIdPair.m_scriptCanvasId.IsValid())
        {
            GraphCanvas::SceneRequestBus::Event(graphCanvasGraphId, &GraphCanvas::SceneRequests::AddNode, m_nodeIdPair.m_graphCanvasId, sceneDropPosition, false);
            GraphCanvas::SceneMemberUIRequestBus::Event(m_nodeIdPair.m_graphCanvasId, &GraphCanvas::SceneMemberUIRequests::SetSelected, true);

            ScriptCanvasEditor::NodeCreationNotificationBus::Event(scriptCanvasId, &ScriptCanvasEditor::NodeCreationNotifications::OnGraphCanvasNodeCreated, m_nodeIdPair.m_graphCanvasId);

            AZ::EntityId gridId;
            GraphCanvas::SceneRequestBus::EventResult(gridId, graphCanvasGraphId, &GraphCanvas::SceneRequests::GetGrid);

            AZ::Vector2 offset;
            GraphCanvas::GridRequestBus::EventResult(offset, gridId, &GraphCanvas::GridRequests::GetMinorPitch);

            sceneDropPosition += offset;
            return true;
        }
        else
        {
            if (m_nodeIdPair.m_graphCanvasId.IsValid())
            {
                AZ::ComponentApplicationBus::Broadcast(&AZ::ComponentApplicationBus::Events::DeleteEntity, m_nodeIdPair.m_graphCanvasId);
            }

            if (m_nodeIdPair.m_scriptCanvasId.IsValid())
            {
                AZ::ComponentApplicationBus::Broadcast(&AZ::ComponentApplicationBus::Events::DeleteEntity, m_nodeIdPair.m_scriptCanvasId);
            }
        }

        return false;
    }

    ///////////////////////////////
    // ScriptEventsPaletteTreeItem
    ///////////////////////////////

    ScriptEventsPaletteTreeItem::ScriptEventsPaletteTreeItem(const AZ::Data::Asset<ScriptEvents::ScriptEventsAsset> asset)
        : GraphCanvas::NodePaletteTreeItem(asset.GetAs<ScriptEvents::ScriptEventsAsset>()->m_definition.GetName().c_str(), ScriptCanvasEditor::AssetEditorId)
        , m_asset(asset)
        , m_editIcon(":/ScriptCanvasEditorResources/Resources/edit_icon.png")
    {
        if (GetName().isEmpty())
        {
            AZ::Data::AssetInfo assetInfo;
            AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetInfo, &AZ::Data::AssetCatalogRequests::GetAssetInfoById, m_asset.GetId());

            if (assetInfo.m_relativePath.empty())
            {
                SetName("<Unknown Script Event>");
            }
            else
            {
                SetName(assetInfo.m_relativePath.c_str());
            }
        }

        PopulateEvents(m_asset);

        AZ::Data::AssetBus::Handler::BusConnect(m_asset.GetId());
    }

    ScriptEventsPaletteTreeItem::~ScriptEventsPaletteTreeItem()
    {
        AZ::Data::AssetBus::Handler::BusDisconnect();
    }

    const ScriptEvents::ScriptEvent& ScriptEventsPaletteTreeItem::GetBusDefinition() const
    {
        ScriptEvents::ScriptEventsAsset* ScriptEventsAsset = m_asset.GetAs<ScriptEvents::ScriptEventsAsset>();
        return ScriptEventsAsset->m_definition;
    }

    void ScriptEventsPaletteTreeItem::OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        ScriptEvents::ScriptEventsAsset* data = asset.GetAs<ScriptEvents::ScriptEventsAsset>();
        if (data)
        {
            const ScriptEvents::ScriptEvent* previousDefinition = nullptr;
            ScriptEvents::ScriptEventsAsset* previousData = m_asset.GetAs<ScriptEvents::ScriptEventsAsset>();
            if (previousData)
            {
                previousDefinition = &data->m_definition;
            }

            const ScriptEvents::ScriptEvent& definition = data->m_definition;

#if defined(AZ_ENABLE_TRACING)
            bool recategorize = previousDefinition ? definition.GetCategory().compare(previousDefinition->GetCategory()) != 0 : false;
#endif
            AZ_Warning("ScriptCanvas", !recategorize, "Unable to recategorize ScriptEvents events while open. Please close and re-open the Script Canvas Editor to see the new categorization");

            if (definition.GetName().empty())
            {
                AZ::Data::AssetInfo assetInfo;
                AZ::Data::AssetCatalogRequestBus::BroadcastResult(assetInfo, &AZ::Data::AssetCatalogRequests::GetAssetInfoById, m_asset.GetId());

                if (assetInfo.m_relativePath.empty())
                {
                    SetName("<Unknown Script Event>");
                }
                else
                {
                    SetName(assetInfo.m_relativePath.c_str());
                }
            }
            else
            {
                SetName(definition.GetName().c_str());
                SetToolTip(definition.GetTooltip().c_str());
            }

            PopulateEvents(asset);

            m_asset = asset;
        }
    }

    QVariant ScriptEventsPaletteTreeItem::OnData(const QModelIndex& index, int role) const
    {
        if (index.column() == NodePaletteTreeItem::Column::Customization)
        {
            if (IsHovered())
            {
                if (role == Qt::DecorationRole)
                {
                    return m_editIcon;
                }
                else if (role == Qt::ToolTipRole)
                {
                    ScriptEvents::ScriptEventsAsset* data = m_asset.GetAs<ScriptEvents::ScriptEventsAsset>();
                    if (data)
                    {
                        const ScriptEvents::ScriptEvent& definition = data->m_definition;
                        return QString("Opens the Script Event Editor to edit the Script Event - %1.").arg(definition.GetName().c_str());
                    }
                }
            }
        }

        return GraphCanvas::NodePaletteTreeItem::OnData(index, role);
    }

    void ScriptEventsPaletteTreeItem::OnHoverStateChanged()
    {
        SignalDataChanged();
    }

    void ScriptEventsPaletteTreeItem::OnClicked(int row)
    {
        if (row == NodePaletteTreeItem::Column::Customization)
        {
            AzToolsFramework::OpenGenericAssetEditor(azrtti_typeid<ScriptEvents::ScriptEventsAsset>(), m_asset.GetId());
        }
    }

    bool ScriptEventsPaletteTreeItem::OnDoubleClicked(int row)
    {
        if (row != NodePaletteTreeItem::Column::Customization)
        {
            AzToolsFramework::OpenGenericAssetEditor(azrtti_typeid<ScriptEvents::ScriptEventsAsset>(), m_asset.GetId());
            return true;
        }

        return false;
    }

    void ScriptEventsPaletteTreeItem::PopulateEvents(AZ::Data::Asset<ScriptEvents::ScriptEventsAsset> asset)
    {
        ClearChildren();

        ScriptEvents::ScriptEventsAsset* data = asset.GetAs<ScriptEvents::ScriptEventsAsset>();
        if (data)
        {
            const ScriptEvents::ScriptEvent& definition = data->m_definition;

            for (const ScriptEvents::Method& methodDefinition : definition.GetMethods())
            {
                ScriptCanvas::EBusEventId eventId = ScriptCanvas::EBusEventId(methodDefinition.GetNameProperty().GetId().ToString<AZStd::string>().c_str());
                CreateChildNode<ScriptEventsEventNodePaletteTreeItem>(asset.GetId(), methodDefinition, eventId);
            }
        }
    }

    /////////////////////////////////////////////
    // CreateScriptEventsReceiverEventMimeEvent
    /////////////////////////////////////////////

    void CreateScriptEventsReceiverMimeEvent::Reflect(AZ::ReflectContext* reflectContext)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext))
        {
            serializeContext->Class<CreateScriptEventsReceiverMimeEvent, SpecializedCreateNodeMimeEvent>()
                ->Version(2)
                    ->Field("AssetId", &CreateScriptEventsReceiverMimeEvent::m_assetId)
                    ->Field("MethodDefinition", &CreateScriptEventsReceiverMimeEvent::m_methodDefinition)
                ;
        }
    }

    CreateScriptEventsReceiverMimeEvent::CreateScriptEventsReceiverMimeEvent(const AZ::Data::AssetId assetId, const ScriptEvents::Method& methodDefinition)
        : m_assetId(assetId)
        , m_methodDefinition(methodDefinition)
    {
        m_asset = AZ::Data::AssetManager::Instance().GetAsset<ScriptEvents::ScriptEventsAsset>(m_assetId, m_asset.GetAutoLoadBehavior());
    }

    NodeIdPair CreateScriptEventsReceiverMimeEvent::ConstructNode(const AZ::EntityId& graphCanvasGraphId, const AZ::Vector2& scenePosition)
    {
        NodeIdPair eventNode = CreateEventNode(graphCanvasGraphId, scenePosition);

        CreateScriptEventsHandlerMimeEvent ebusMimeEvent(m_asset.GetId(), m_methodDefinition);

        AZ::Vector2 position = scenePosition;
        if (ebusMimeEvent.ExecuteEvent(position, position, graphCanvasGraphId))
        {
            NodeIdPair handlerNode = ebusMimeEvent.GetCreatedPair();

            GraphCanvas::WrappedNodeConfiguration configuration;
            EBusHandlerNodeDescriptorRequestBus::EventResult(configuration, handlerNode.m_graphCanvasId, &EBusHandlerNodeDescriptorRequests::GetEventConfiguration, m_methodDefinition.GetEventId());

            GraphCanvas::WrapperNodeRequestBus::Event(handlerNode.m_graphCanvasId, &GraphCanvas::WrapperNodeRequests::WrapNode, eventNode.m_graphCanvasId, configuration);
        }

        return eventNode;
    }

    bool CreateScriptEventsReceiverMimeEvent::ExecuteEvent([[maybe_unused]] const AZ::Vector2& mousePosition, AZ::Vector2& sceneDropPosition, const AZ::EntityId& graphCanvasGraphId)
    {
        NodeIdPair eventNode = CreateEventNode(graphCanvasGraphId, sceneDropPosition);

        if (eventNode.m_graphCanvasId.IsValid())
        {
            GraphCanvas::SceneMemberUIRequestBus::Event(eventNode.m_graphCanvasId, &GraphCanvas::SceneMemberUIRequests::SetSelected, true);

            ScriptCanvas::ScriptCanvasId scriptCanvasId;
            GeneralRequestBus::BroadcastResult(scriptCanvasId, &GeneralRequests::GetScriptCanvasId, graphCanvasGraphId);

            ScriptCanvasEditor::NodeCreationNotificationBus::Event(scriptCanvasId, &ScriptCanvasEditor::NodeCreationNotifications::OnGraphCanvasNodeCreated, eventNode.m_graphCanvasId);

            AZ::EntityId gridId;
            GraphCanvas::SceneRequestBus::EventResult(gridId, graphCanvasGraphId, &GraphCanvas::SceneRequests::GetGrid);

            AZ::Vector2 offset;
            GraphCanvas::GridRequestBus::EventResult(offset, gridId, &GraphCanvas::GridRequests::GetMinorPitch);

            sceneDropPosition += offset;
        }
        return eventNode.m_graphCanvasId.IsValid();
    }

    NodeIdPair CreateScriptEventsReceiverMimeEvent::CreateEventNode(const AZ::EntityId& graphCanvasGraphId, const AZ::Vector2& scenePosition) const
    {
        ScriptCanvas::ScriptCanvasId scriptCanvasId;
        GeneralRequestBus::BroadcastResult(scriptCanvasId, &GeneralRequests::GetScriptCanvasId, graphCanvasGraphId);

        NodeIdPair nodeIdPair;
        nodeIdPair.m_graphCanvasId = Nodes::DisplayScriptEventNode(graphCanvasGraphId, m_asset.GetId(), m_methodDefinition);

        if (nodeIdPair.m_graphCanvasId.IsValid())
        {
            GraphCanvas::SceneRequestBus::Event(graphCanvasGraphId, &GraphCanvas::SceneRequests::AddNode, nodeIdPair.m_graphCanvasId, scenePosition, false);
        }

        return nodeIdPair;
    }

    ///////////////////////////////////////////
    // ScriptEventsHandlerEventPaletteTreeItem
    ///////////////////////////////////////////

    ScriptEventsHandlerEventPaletteTreeItem::ScriptEventsHandlerEventPaletteTreeItem(const AZ::Data::AssetId assetId, const ScriptEvents::Method& methodDefinition)
        : GraphCanvas::DraggableNodePaletteTreeItem(methodDefinition.GetName(), ScriptCanvasEditor::AssetEditorId)
        , m_assetId(assetId)
        , m_methodDefinition(methodDefinition)
    {
        SetToolTip(m_definition.GetTooltip().c_str());
        SetTitlePalette("HandlerNodeTitlePalette");
    }

    GraphCanvas::GraphCanvasMimeEvent* ScriptEventsHandlerEventPaletteTreeItem::CreateMimeEvent() const
    {
        return aznew CreateScriptEventsReceiverMimeEvent(m_assetId, m_methodDefinition);
    }

    //////////////////////////////////////////
    // CreateScriptEventsSenderMimeEvent
    //////////////////////////////////////////

    void CreateScriptEventsSenderMimeEvent::Reflect(AZ::ReflectContext* reflectContext)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext))
        {
            serializeContext->Class<CreateScriptEventsSenderMimeEvent, CreateNodeMimeEvent>()
                ->Version(2)
                ->Field("AssetId", &CreateScriptEventsSenderMimeEvent::m_assetId)
                ->Field("EventDefinition", &CreateScriptEventsSenderMimeEvent::m_methodDefinition)
                ;
        }

    }

    CreateScriptEventsSenderMimeEvent::CreateScriptEventsSenderMimeEvent(const AZ::Data::AssetId assetId, const ScriptEvents::Method& methodDefinition)
        : m_assetId(assetId)
        , m_methodDefinition(methodDefinition)
    {
    }

    const AZ::Data::Asset<ScriptEvents::ScriptEventsAsset> CreateScriptEventsSenderMimeEvent::GetAsset(AZ::Data::AssetLoadBehavior loadBehavior)
    {
        return AZ::Data::AssetManager::Instance().GetAsset<ScriptEvents::ScriptEventsAsset>(m_assetId, loadBehavior);
    }

    ScriptCanvasEditor::NodeIdPair CreateScriptEventsSenderMimeEvent::CreateNode(const ScriptCanvas::ScriptCanvasId& scriptCanvasId) const
    {
        return Nodes::CreateScriptEventSenderNode(scriptCanvasId, m_assetId, m_methodDefinition.GetEventId());
    }

    /////////////////////////////////////
    // ScriptEventsSenderPaletteTreeItem
    /////////////////////////////////////

    ScriptEventsSenderPaletteTreeItem::ScriptEventsSenderPaletteTreeItem(const AZ::Data::AssetId assetId, const ScriptEvents::Method& methodDefinition)
        : GraphCanvas::DraggableNodePaletteTreeItem(methodDefinition.GetName(), ScriptCanvasEditor::AssetEditorId)
        , m_assetId(assetId)
        , m_methodDefinition(methodDefinition)
    {
        SetToolTip(m_methodDefinition.GetTooltip().c_str());
        SetTitlePalette("MethodNodeTitlePalette");
    }

    GraphCanvas::GraphCanvasMimeEvent* ScriptEventsSenderPaletteTreeItem::CreateMimeEvent() const
    {
        return aznew CreateScriptEventsSenderMimeEvent(m_assetId, m_methodDefinition);
    }

    /////////////////////////////////////////////////
    // CreateSendOrReceiveScriptEventsMimeEvent
    /////////////////////////////////////////////////

    void CreateSendOrReceiveScriptEventsMimeEvent::Reflect(AZ::ReflectContext* reflectContext)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext))
        {
            serializeContext->Class<CreateSendOrReceiveScriptEventsMimeEvent, SpecializedCreateNodeMimeEvent>()
                ->Version(1)
                ->Field("AssetId", &CreateSendOrReceiveScriptEventsMimeEvent::m_assetId)
                ->Field("MethodDefinition", &CreateSendOrReceiveScriptEventsMimeEvent::m_methodDefinition)
                ->Field("EventId", &CreateSendOrReceiveScriptEventsMimeEvent::m_eventId)
                ;
        }
    }

    CreateSendOrReceiveScriptEventsMimeEvent::CreateSendOrReceiveScriptEventsMimeEvent(const AZ::Data::AssetId assetId, const ScriptEvents::Method& methodDefinition, const ScriptCanvas::EBusEventId& eventId)
        : m_assetId(assetId)
        , m_methodDefinition(methodDefinition)
        , m_eventId(eventId)
    {
        m_asset = AZ::Data::AssetManager::Instance().GetAsset<ScriptEvents::ScriptEventsAsset>(m_assetId, m_asset.GetAutoLoadBehavior());
    }

    bool CreateSendOrReceiveScriptEventsMimeEvent::ExecuteEvent([[maybe_unused]] const AZ::Vector2& mousePosition, AZ::Vector2& sceneDropPosition, const AZ::EntityId& graphCanvasGraphId)
    {
        ScriptCanvas::ScriptCanvasId scriptCanvasId;
        GeneralRequestBus::BroadcastResult(scriptCanvasId, &GeneralRequests::GetScriptCanvasId, graphCanvasGraphId);

        NodeIdPair nodeId = ConstructNode(graphCanvasGraphId, sceneDropPosition);

        if (nodeId.m_graphCanvasId.IsValid())
        {
            AZ::EntityId gridId;
            GraphCanvas::SceneRequestBus::EventResult(gridId, graphCanvasGraphId, &GraphCanvas::SceneRequests::GetGrid);

            AZ::Vector2 offset;
            GraphCanvas::GridRequestBus::EventResult(offset, gridId, &GraphCanvas::GridRequests::GetMinorPitch);

            sceneDropPosition += offset;
        }

        return nodeId.m_graphCanvasId.IsValid();
    }

    ScriptCanvasEditor::NodeIdPair CreateSendOrReceiveScriptEventsMimeEvent::ConstructNode(const AZ::EntityId& graphCanvasGraphId, const AZ::Vector2& scenePosition)
    {
        ScriptCanvas::ScriptCanvasId scriptCanvasId;
        GeneralRequestBus::BroadcastResult(scriptCanvasId, &GeneralRequests::GetScriptCanvasId, graphCanvasGraphId);

        ScriptCanvasEditor::NodeIdPair nodeIdPair;

        AZ::EntityId viewId;
        GraphCanvas::SceneRequestBus::EventResult(viewId, graphCanvasGraphId, &GraphCanvas::SceneRequests::GetViewId);

        GraphCanvas::GraphCanvasGraphicsView* graphicsView = nullptr;
        GraphCanvas::ViewRequestBus::EventResult(graphicsView, viewId, &GraphCanvas::ViewRequests::AsGraphicsView);

        if (graphicsView)
        {
            QMenu menu(graphicsView);

            QAction* createSender = new QAction(QString("Send %1").arg(m_methodDefinition.GetName().c_str()), &menu);

            const QPixmap* senderIconPixmap;
            GraphCanvas::StyleManagerRequestBus::EventResult(senderIconPixmap, ScriptCanvasEditor::AssetEditorId, &GraphCanvas::StyleManagerRequests::GetPaletteIcon, "NodePaletteTypeIcon", "MethodNodeTitlePalette");

            QIcon senderIcon((*senderIconPixmap));
            createSender->setIcon(senderIcon);

            menu.addAction(createSender);

            QAction* createReceiver = new QAction(QString("Receive %1").arg(m_methodDefinition.GetName().c_str()), &menu);

            const QPixmap* receiverIconPixmap;
            GraphCanvas::StyleManagerRequestBus::EventResult(receiverIconPixmap, ScriptCanvasEditor::AssetEditorId, &GraphCanvas::StyleManagerRequests::GetPaletteIcon, "NodePaletteTypeIcon", "HandlerNodeTitlePalette");

            QIcon receiverIcon((*receiverIconPixmap));
            createReceiver->setIcon(receiverIcon);

            menu.addAction(createReceiver);

            QAction* result = menu.exec(QCursor::pos());

            if (result == createSender)
            {
                CreateScriptEventsSenderMimeEvent createEBusSenderNode(m_assetId, m_methodDefinition);
                nodeIdPair = createEBusSenderNode.CreateNode(scriptCanvasId);

                if (nodeIdPair.m_graphCanvasId.IsValid())
                {
                    GraphCanvas::SceneRequestBus::Event(graphCanvasGraphId, &GraphCanvas::SceneRequests::AddNode, nodeIdPair.m_graphCanvasId, scenePosition, false);                    
                }
            }
            else if (result == createReceiver)
            {
                CreateScriptEventsReceiverMimeEvent createEBusHandlerNode(m_assetId, m_methodDefinition);
                nodeIdPair = createEBusHandlerNode.ConstructNode(graphCanvasGraphId, scenePosition);
            }

            if (nodeIdPair.m_graphCanvasId.IsValid())
            {
                GraphCanvas::SceneMemberUIRequestBus::Event(nodeIdPair.m_graphCanvasId, &GraphCanvas::SceneMemberUIRequests::SetSelected, true);
            }
        }

        return nodeIdPair;
    }

    AZStd::vector< GraphCanvas::GraphCanvasMimeEvent* > CreateSendOrReceiveScriptEventsMimeEvent::CreateMimeEvents() const
    {
        AZStd::vector< GraphCanvas::GraphCanvasMimeEvent* > mimeEvents;

        mimeEvents.push_back(aznew CreateScriptEventsSenderMimeEvent(m_assetId, m_methodDefinition));
        mimeEvents.push_back(aznew CreateScriptEventsReceiverMimeEvent(m_assetId, m_methodDefinition));

        return mimeEvents;
    }

    ////////////////////////////////////////
    // ScriptEventsEventNodePaletteTreeItem
    ////////////////////////////////////////

    ScriptEventsEventNodePaletteTreeItem::ScriptEventsEventNodePaletteTreeItem(const AZ::Data::AssetId& assetId, const ScriptEvents::Method& methodDefinition, const ScriptCanvas::EBusEventId& eventId)
        : GraphCanvas::DraggableNodePaletteTreeItem(methodDefinition.GetName().c_str(), ScriptCanvasEditor::AssetEditorId)
        , m_editIcon(":/ScriptCanvasEditorResources/Resources/edit_icon.png")
        , m_assetId(assetId)
        , m_methodDefinition(methodDefinition)
        , m_eventId(eventId)
    {
        m_asset = AZ::Data::AssetManager::Instance().GetAsset<ScriptEvents::ScriptEventsAsset>(assetId, m_asset.GetAutoLoadBehavior());

        SetToolTip(m_methodDefinition.GetTooltip().c_str());

        SetTitlePalette("MethodNodeTitlePalette");
        AddIconColorPalette("HandlerNodeTitlePalette");
    }

    GraphCanvas::GraphCanvasMimeEvent* ScriptEventsEventNodePaletteTreeItem::CreateMimeEvent() const
    {
        return aznew CreateSendOrReceiveScriptEventsMimeEvent(m_assetId, m_methodDefinition, m_eventId);
    }

    QVariant ScriptEventsEventNodePaletteTreeItem::OnData(const QModelIndex& index, int role) const
    {
        if (index.column() == NodePaletteTreeItem::Column::Customization)
        {
            if (IsHovered())
            {
                if (role == Qt::DecorationRole)
                {
                    return m_editIcon;
                }
                else if (role == Qt::ToolTipRole)
                {
                    const ScriptEvents::ScriptEvent& definition = m_asset.Get()->m_definition;
                    return QString("Opens the Script Events Editor to edit the Script Event - %1::%2.").arg(definition.GetName().c_str()).arg(m_methodDefinition.GetName().c_str());
                }
            }
        }

        return GraphCanvas::DraggableNodePaletteTreeItem::OnData(index, role);
    }

    ScriptCanvas::EBusBusId ScriptEventsEventNodePaletteTreeItem::GetBusIdentifier() const
    {
        return ScriptCanvas::EBusBusId(m_assetId.ToString<AZStd::string>().c_str());
    }

    ScriptCanvas::EBusEventId ScriptEventsEventNodePaletteTreeItem::GetEventIdentifier() const
    {
        return m_eventId;
    }

    void ScriptEventsEventNodePaletteTreeItem::OnHoverStateChanged()
    {
        SignalDataChanged();
    }

    void ScriptEventsEventNodePaletteTreeItem::OnClicked(int row)
    {
        if (row == NodePaletteTreeItem::Column::Customization)
        {
            AzToolsFramework::OpenGenericAssetEditor(azrtti_typeid<ScriptEvents::ScriptEventsAsset>(), m_assetId);
        }
    }

    bool ScriptEventsEventNodePaletteTreeItem::OnDoubleClicked(int row)
    {
        if (row != NodePaletteTreeItem::Column::Customization)
        {
            AzToolsFramework::OpenGenericAssetEditor(azrtti_typeid<ScriptEvents::ScriptEventsAsset>(), m_asset.GetId());
            return true;
        }

        return false;
    }
}
