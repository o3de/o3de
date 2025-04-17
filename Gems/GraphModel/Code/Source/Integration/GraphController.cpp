/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// AZ
#include <AzCore/Debug/Trace.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Vector4.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>

// Qt
#include <QGraphicsItem>
#include <QGraphicsLinearLayout>

// Graph Canvas
#include <GraphCanvas/Components/GeometryBus.h>
#include <GraphCanvas/Components/GridBus.h>
#include <GraphCanvas/Components/Nodes/NodeBus.h>
#include <GraphCanvas/Components/Nodes/NodeLayoutBus.h>
#include <GraphCanvas/Components/Nodes/NodeTitleBus.h>
#include <GraphCanvas/Components/Nodes/Wrapper/WrapperNodeBus.h>
#include <GraphCanvas/Components/Slots/Extender/ExtenderSlotBus.h>
#include <GraphCanvas/Components/ViewBus.h>
#include <GraphCanvas/GraphCanvasBus.h>
#include <GraphCanvas/Types/EntitySaveData.h>

// Graph Model
#include <GraphModel/Integration/BooleanDataInterface.h>
#include <GraphModel/Integration/FloatDataInterface.h>
#include <GraphModel/Integration/GraphCanvasMetadata.h>
#include <GraphModel/Integration/GraphController.h>
#include <GraphModel/Integration/Helpers.h>
#include <GraphModel/Integration/IntegerDataInterface.h>
#include <GraphModel/Integration/IntegrationBus.h>
#include <GraphModel/Integration/StringDataInterface.h>
#include <GraphModel/Integration/ThumbnailImageItem.h>
#include <GraphModel/Integration/VectorDataInterface.inl>
#include <GraphModel/Model/Connection.h>
#include <GraphModel/Model/DataType.h>
#include <GraphModel/Model/Graph.h>
#include <GraphModel/Model/GraphContext.h>
#include <GraphModel/Model/Node.h>

namespace GraphModelIntegration
{
    // Index of the thumbnail image we embed in our nodes (just after the title header)
    static const int NODE_THUMBNAIL_INDEX = 1;

    // Helpers static function definitions
    AZStd::string Helpers::GetTitlePaletteOverride(void* nodePtr, const AZ::TypeId& typeId)
    {
        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);
        AZ_Assert(serializeContext, "Failed to acquire application serialize context.");

        AZStd::string paletteOverride;

        const AZ::SerializeContext::ClassData* derivedClassData = serializeContext->FindClassData(typeId);
        if (!derivedClassData)
        {
            return paletteOverride;
        }

        // Use the EnumHierarchy API to retrive a list of TypeIds that this class derives from,
        // starting with the actual type and going backwards
        AZStd::vector<AZ::TypeId> typeIds;
        if (derivedClassData->m_azRtti)
        {
            derivedClassData->m_azRtti->EnumHierarchy(&RttiEnumHierarchyHelper, &typeIds);
        }

        // Look through all the derived TypeIds to see if the TitlePaletteOverride attribute
        // was set in the EditContext at any level
        for (auto currentTypeId : typeIds)
        {
            auto classData = serializeContext->FindClassData(currentTypeId);
            if (classData)
            {
                if (classData->m_editData)
                {
                    const AZ::Edit::ElementData* elementData = classData->m_editData->FindElementData(AZ::Edit::ClassElements::EditorData);
                    if (elementData)
                    {
                        if (auto titlePaletteAttribute = elementData->FindAttribute(Attributes::TitlePaletteOverride))
                        {
                            AZ::AttributeReader nameReader(nodePtr, titlePaletteAttribute);
                            nameReader.Read<AZStd::string>(paletteOverride);
                        }
                    }
                }
            }
        }

        return paletteOverride;
    }

    void Helpers::RttiEnumHierarchyHelper(const AZ::TypeId& typeId, void* userData)
    {
        AZStd::vector<AZ::TypeId>* typeIds = reinterpret_cast<AZStd::vector<AZ::TypeId>*>(userData);
        typeIds->push_back(typeId);
    }


    ////////////////////////////////////////////////////////////////////////////////////
    // GraphElementMap

    void GraphController::GraphElementMap::Add(AZ::EntityId graphCanvasId, GraphModel::GraphElementPtr graphElement)
    {
        Remove(graphCanvasId);
        Remove(graphElement);
        m_graphElementToUi[graphElement.get()] = graphCanvasId;
        m_uiToGraphElement[graphCanvasId] = graphElement;
    }

    void GraphController::GraphElementMap::Remove(AZ::EntityId graphCanvasId)
    {
        const auto iter = m_uiToGraphElement.find(graphCanvasId);
        if (iter != m_uiToGraphElement.end())
        {
            m_graphElementToUi.erase(iter->second.get());
            m_uiToGraphElement.erase(iter);
        }
    }

    void GraphController::GraphElementMap::Remove(GraphModel::ConstGraphElementPtr graphElement)
    {
        const auto iter = m_graphElementToUi.find(graphElement.get());
        if (iter != m_graphElementToUi.end())
        {
            m_uiToGraphElement.erase(iter->second);
            m_graphElementToUi.erase(iter);
        }
    }

    GraphModel::GraphElementPtr GraphController::GraphElementMap::Find(AZ::EntityId graphCanvasId)
    {
        const auto iter = m_uiToGraphElement.find(graphCanvasId);
        return iter != m_uiToGraphElement.end() ? iter->second : nullptr;
    }

    GraphModel::ConstGraphElementPtr GraphController::GraphElementMap::Find(AZ::EntityId graphCanvasId) const
    {
        const auto iter = m_uiToGraphElement.find(graphCanvasId);
        return iter != m_uiToGraphElement.end() ? iter->second : nullptr;
    }

    AZ::EntityId GraphController::GraphElementMap::Find(GraphModel::ConstGraphElementPtr graphElement) const
    {
        const auto iter = m_graphElementToUi.find(graphElement.get());
        return iter != m_graphElementToUi.end() ? iter->second : AZ::EntityId();
    }

    ////////////////////////////////////////////////////////////////////////////////////
    // GraphElementMapCollection

    const GraphController::GraphElementMap* GraphController::GraphElementMapCollection::GetMapFor(
        GraphModel::ConstGraphElementPtr graphElement) const
    {
        using namespace GraphModel;

        if (azrtti_istypeof<Node>(graphElement.get()))
        {
            return &m_nodeMap;
        }

        if (azrtti_istypeof<Slot>(graphElement.get()))
        {
            return &m_slotMap;
        }

        if (azrtti_istypeof<Connection>(graphElement.get()))
        {
            return &m_connectionMap;
        }

        AZ_Assert(false, "Could not determine correct GraphElementMap");
        return nullptr;
    }

    GraphController::GraphElementMap* GraphController::GraphElementMapCollection::GetMapFor(GraphModel::ConstGraphElementPtr graphElement)
    {
        // Non-const overload implementation
        const GraphElementMapCollection* constThis = this;
        return const_cast<GraphController::GraphElementMap*>(constThis->GetMapFor(graphElement));
    }

    void GraphController::GraphElementMapCollection::Add(AZ::EntityId graphCanvasId, GraphModel::GraphElementPtr graphElement)
    {
        using namespace GraphModel;

        if (graphElement)
        {
            GetMapFor(graphElement)->Add(graphCanvasId, graphElement);
        }
    }

    void GraphController::GraphElementMapCollection::Remove(AZ::EntityId graphCanvasId)
    {
        for (GraphElementMap* map : m_allMaps)
        {
            map->Remove(graphCanvasId);
        }
    }

    void GraphController::GraphElementMapCollection::Remove(GraphModel::ConstGraphElementPtr graphElement)
    {
        GetMapFor(graphElement)->Remove(graphElement);
    }

    AZ::EntityId GraphController::GraphElementMapCollection::Find(GraphModel::ConstGraphElementPtr graphElement) const
    {
        return GetMapFor(graphElement)->Find(graphElement);
    }

    ////////////////////////////////////////////////////////////////////////////////////
    // GraphController

    GraphCanvas::ConnectionType ToGraphCanvasConnectionType(const GraphModel::SlotDirection& direction)
    {
        switch (direction)
        {
        case GraphModel::SlotDirection::Input:
            return GraphCanvas::ConnectionType::CT_Input;
        case GraphModel::SlotDirection::Output:
            return GraphCanvas::ConnectionType::CT_Output;
        default:
            break;
        }

        AZ_Assert(false, "Invalid SlotDirection");
        return GraphCanvas::ConnectionType::CT_Invalid;
    }

    GraphCanvas::SlotGroup ToGraphCanvasSlotGroup(const GraphModel::SlotType& slotType)
    {
        switch (slotType)
        {
        case GraphModel::SlotType::Data:
            return GraphCanvas::SlotGroups::DataGroup;
        case GraphModel::SlotType::Event:
            return GraphCanvas::SlotGroups::ExecutionGroup;
        case GraphModel::SlotType::Property:
            return GraphCanvas::SlotGroups::PropertyGroup;
        default:
            break;
        }

        AZ_Assert(false, "Invalid SlotType");
        return GraphCanvas::SlotGroups::Invalid;
    }

    GraphController::GraphController(GraphModel::GraphPtr graph, AZ::EntityId graphCanvasSceneId)
        : m_graph(graph)
        , m_graphCanvasSceneId(graphCanvasSceneId)
    {
        AZ::ComponentApplicationBus::BroadcastResult(m_serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);
        AZ_Assert(m_serializeContext, "Failed to acquire application serialize context.");

        GraphCanvas::GraphModelRequestBus::Handler::BusConnect(m_graphCanvasSceneId);
        GraphCanvas::SceneNotificationBus::Handler::BusConnect(m_graphCanvasSceneId);
        GraphControllerRequestBus::Handler::BusConnect(m_graphCanvasSceneId);

        CreateFullGraphUi();
    }

    GraphController::~GraphController()
    {
        GraphControllerRequestBus::Handler::BusDisconnect();
        GraphCanvas::SceneNotificationBus::Handler::BusDisconnect();
        GraphCanvas::GraphModelRequestBus::Handler::BusDisconnect();
    }

    GraphModel::GraphPtr GraphController::GetGraph()
    {
        return m_graph;
    }

    const GraphModel::GraphPtr GraphController::GetGraph() const
    {
        return m_graph;
    }

    const AZ::EntityId GraphController::GetGraphCanvasSceneId() const
    {
        return m_graphCanvasSceneId;
    }

    void GraphController::CreateFullGraphUi()
    {
        using namespace GraphModel;

        // This notification is needed by the graph canvassing component prior to repopulating the entire scene.
        GraphCanvas::SceneRequestBus::Event(GetGraphCanvasSceneId(), &GraphCanvas::SceneRequests::SignalLoadStart);

        GraphCanvasMetadata* graphCanvasMetadata = GetGraphMetadata();

        // Connect the EntitySaveDataRequestBus using a router. This will allow the graph controller to inject custom selection save
        // data for all graph canvas objects. This should only be connected while serializing save data for the current graph.
        GraphCanvas::EntitySaveDataRequestBus::Router::BusRouterConnect();

        // Create UI for all the Nodes
        for (const auto& pair : m_graph->GetNodes())
        {
            const NodeId nodeId = pair.first;
            NodePtr node = pair.second;

            AZStd::shared_ptr<GraphCanvas::EntitySaveDataContainer> container;
            auto metadataIter = graphCanvasMetadata->m_nodeMetadata.find(nodeId);
            if (metadataIter != graphCanvasMetadata->m_nodeMetadata.end() && metadataIter->second)
            {
                container = metadataIter->second;
            }

            AZ::EntityId nodeUiId = CreateNodeUi(nodeId, node, AZ::Vector2::CreateZero());

            if (container)
            {
                GraphCanvas::EntitySaveDataRequestBus::Event(nodeUiId, &GraphCanvas::EntitySaveDataRequests::ReadSaveData, *container);
            }
        }

        // Wrap any nodes stored in the node wrappings
        for (auto& pair : m_graph->GetNodeWrappings())
        {
            GraphModel::NodePtr node = m_graph->GetNode(pair.first);
            GraphModel::NodePtr wrapperNode = m_graph->GetNode(pair.second.first);
            AZ::u32 layoutOrder = pair.second.second;
            WrapNodeUi(wrapperNode, node, layoutOrder);
        }

        // Create UI for all the Connections
        for (ConnectionPtr connection : m_graph->GetConnections())
        {
            CreateConnectionUi(connection);
        }

        // Load graph canvas metadata for the scene. This will recreate all of the utility types like comments, bookmarks, groups, etc. 
        if (graphCanvasMetadata->m_sceneMetadata)
        {
            GraphCanvas::EntitySaveDataRequestBus::Event(
                GetGraphCanvasSceneId(), &GraphCanvas::EntitySaveDataRequests::ReadSaveData, *graphCanvasMetadata->m_sceneMetadata);
        }

        // Disconnect the EntitySaveDataRequestBus after save data serialization as completed
        GraphCanvas::EntitySaveDataRequestBus::Router::BusRouterDisconnect();

        // After the graph has been reconstructed, this signal will inform the scene, node groups, and other types to update their state
        // after all of the graph elements are in place. This is necessary for node groups to reclaim nodes that were contained within them
        // when the graph was saved.
        GraphCanvas::SceneRequestBus::Event(GetGraphCanvasSceneId(), &GraphCanvas::SceneRequests::SignalLoadEnd);
    }

    AZ::Entity* GraphController::CreateSlotUi(GraphModel::SlotPtr slot, AZ::EntityId nodeUiId)
    {
        using namespace GraphModel;

        GraphCanvas::SlotConfiguration slotConfig;
        slotConfig.m_name = !slot->GetDisplayName().empty() ? slot->GetDisplayName() : slot->GetName();
        slotConfig.m_tooltip = slot->GetDescription();
        slotConfig.m_connectionType = ToGraphCanvasConnectionType(slot->GetSlotDirection());
        slotConfig.m_slotGroup = ToGraphCanvasSlotGroup(slot->GetSlotType());

        const AZ::EntityId stylingParent = nodeUiId;
        AZ::Entity* graphCanvasSlotEntity = nullptr;

        switch (slot->GetSlotType())
        {
        case SlotType::Data:
            {
                GraphCanvas::DataSlotConfiguration dataConfig(slotConfig);
                dataConfig.m_dataSlotType = GraphCanvas::DataSlotType::Value;
                dataConfig.m_typeId = slot->GetDataType()->GetTypeUuid();
                GraphCanvas::GraphCanvasRequestBus::BroadcastResult(
                    graphCanvasSlotEntity, &GraphCanvas::GraphCanvasRequests::CreateSlot, stylingParent, dataConfig);
            }

            break;
        case SlotType::Event:
            {
                GraphCanvas::ExecutionSlotConfiguration eventConfig(slotConfig);
                GraphCanvas::GraphCanvasRequestBus::BroadcastResult(
                    graphCanvasSlotEntity, &GraphCanvas::GraphCanvasRequests::CreateSlot, stylingParent, eventConfig);
            }
            break;
        case SlotType::Property:
            {
                GraphCanvas::GraphCanvasRequestBus::BroadcastResult(
                    graphCanvasSlotEntity, &GraphCanvas::GraphCanvasRequests::CreatePropertySlot, stylingParent, 0, slotConfig);
            }
            break;
        default:
            AZ_Assert(false, "Invalid SlotType");
        }

        AZ_Assert(graphCanvasSlotEntity, "Unable to create GraphCanvas Slot");

        graphCanvasSlotEntity->Init();
        graphCanvasSlotEntity->Activate();

        m_elementMap.Add(graphCanvasSlotEntity->GetId(), slot);

        GraphCanvas::NodeRequestBus::Event(nodeUiId, &GraphCanvas::NodeRequests::AddSlot, graphCanvasSlotEntity->GetId());

        return graphCanvasSlotEntity;
    }

    AZ::EntityId GraphController::CreateNodeUi(
        [[maybe_unused]] GraphModel::NodeId nodeId, GraphModel::NodePtr node, const AZ::Vector2& scenePosition)
    {
        using namespace GraphModel;

        // Create the node...
        const char* nodeStyle = "";
        const AZ::Entity* graphCanvasNode = nullptr;
        const NodeType& nodeType = node->GetNodeType();
        switch (nodeType)
        {
        case NodeType::GeneralNode:
            GraphCanvas::GraphCanvasRequestBus::BroadcastResult(
                graphCanvasNode, &GraphCanvas::GraphCanvasRequests::CreateGeneralNodeAndActivate, nodeStyle);
            break;

        case NodeType::WrapperNode:
            GraphCanvas::GraphCanvasRequestBus::BroadcastResult(
                graphCanvasNode, &GraphCanvas::GraphCanvasRequests::CreateWrapperNodeAndActivate, nodeStyle);
            break;
        }

        AZ_Assert(graphCanvasNode, "Unable to create GraphCanvas Node");
        const AZ::EntityId nodeUiId = graphCanvasNode->GetId();
        m_elementMap.Add(nodeUiId, node);

        GraphCanvas::NodeTitleRequestBus::Event(nodeUiId, &GraphCanvas::NodeTitleRequests::SetTitle, node->GetTitle());
        GraphCanvas::NodeTitleRequestBus::Event(nodeUiId, &GraphCanvas::NodeTitleRequests::SetSubTitle, node->GetSubTitle());

        // Set the palette override for this node if one has been specified
        AZStd::string paletteOverride = Helpers::GetTitlePaletteOverride(node.get(), azrtti_typeid(node.get()));
        if (!paletteOverride.empty())
        {
            GraphCanvas::NodeTitleRequestBus::Event(nodeUiId, &GraphCanvas::NodeTitleRequests::SetPaletteOverride, paletteOverride);
        }

        // Create the slots...
        //    Note that SlotDefinitions are stored in a list in the order defined by the author.
        //    That's why we loop through SlotDefinitions instead of the actual Slots, which are stored in a map.
        for (SlotDefinitionPtr slotDefinition : node->GetSlotDefinitions())
        {
            if (!slotDefinition->IsVisibleOnNode())
            {
                continue;
            }

            const AZStd::string& slotName = slotDefinition->GetName();
            GraphCanvas::ExtenderId extenderId;

            if (slotDefinition->SupportsExtendability())
            {
                for (GraphModel::SlotPtr slot : node->GetExtendableSlots(slotName))
                {
                    CreateSlotUi(slot, nodeUiId);
                }

                // Keep a mapping of the extenderId/SlotName for this node
                extenderId = AZ_CRC(slotName);
                auto it = m_nodeExtenderIds.find(nodeUiId);
                if (it != m_nodeExtenderIds.end())
                {
                    it->second[extenderId] = slotName;
                }
                else
                {
                    AZStd::unordered_map<GraphCanvas::ExtenderId, GraphModel::SlotName> newNodeMap;
                    newNodeMap[extenderId] = slotName;

                    m_nodeExtenderIds[nodeUiId] = newNodeMap;
                }
            }
            else
            {
                CreateSlotUi(node->GetSlot(slotName), nodeUiId);
            }

            // For an extendable slot, we also need to create the extension slot that allows
            // the user to add more slots
            if (slotDefinition->SupportsExtendability())
            {
                GraphCanvas::ExtenderSlotConfiguration extenderConfig;
                extenderConfig.m_extenderId = extenderId;
                extenderConfig.m_name = slotDefinition->GetExtensionLabel();
                extenderConfig.m_tooltip = slotDefinition->GetExtensionTooltip();
                extenderConfig.m_connectionType = ToGraphCanvasConnectionType(slotDefinition->GetSlotDirection());
                extenderConfig.m_slotGroup = ToGraphCanvasSlotGroup(slotDefinition->GetSlotType());

                const AZ::EntityId stylingParent = nodeUiId;
                AZ::Entity* extensionEntity = nullptr;
                GraphCanvas::GraphCanvasRequestBus::BroadcastResult(
                    extensionEntity, &GraphCanvas::GraphCanvasRequests::CreateSlot, stylingParent, extenderConfig);

                extensionEntity->Init();
                extensionEntity->Activate();

                GraphCanvas::NodeRequestBus::Event(nodeUiId, &GraphCanvas::NodeRequests::AddSlot, extensionEntity->GetId());
            }
        }

        GraphCanvas::SceneRequestBus::Event(GetGraphCanvasSceneId(), &GraphCanvas::SceneRequests::AddNode, nodeUiId, scenePosition, false);
        return nodeUiId;
    }

    void GraphController::CreateConnectionUi(GraphModel::ConnectionPtr connection)
    {
        AZ::EntityId sourceNodeUiId = m_elementMap.Find(connection->GetSourceNode());
        AZ::EntityId targetNodeUiId = m_elementMap.Find(connection->GetTargetNode());

        AZ::EntityId sourceSlotUiId = m_elementMap.Find(connection->GetSourceSlot());
        AZ::EntityId targetSlotUiId = m_elementMap.Find(connection->GetTargetSlot());

        m_isCreatingConnectionUi = true;

        AZ::EntityId connectionUiId;
        GraphCanvas::SceneRequestBus::EventResult(
            connectionUiId,
            GetGraphCanvasSceneId(),
            &GraphCanvas::SceneRequests::CreateConnectionBetween,
            GraphCanvas::Endpoint(sourceNodeUiId, sourceSlotUiId),
            GraphCanvas::Endpoint(targetNodeUiId, targetSlotUiId));

        m_elementMap.Add(connectionUiId, connection);

        m_isCreatingConnectionUi = false;
    }

    GraphCanvas::NodeId GraphController::AddNode(GraphModel::NodePtr node, AZ::Vector2& sceneDropPosition)
    {
        AZ_Assert(node, "Node was null");

        const GraphModel::NodeId nodeId = m_graph->AddNode(node);

        AZ::EntityId graphCanvasNodeId = CreateNodeUi(nodeId, node, sceneDropPosition);

        GraphCanvas::SceneMemberUIRequestBus::Event(graphCanvasNodeId, &GraphCanvas::SceneMemberUIRequests::SetSelected, true);

        SaveMetadata(graphCanvasNodeId);

        // Offset the sceneDropPosition so if multiple nodes are dragged into the scene at the same time, the don't stack exactly on top of
        // each other
        AZ::EntityId gridId;
        GraphCanvas::SceneRequestBus::EventResult(gridId, GetGraphCanvasSceneId(), &GraphCanvas::SceneRequests::GetGrid);
        AZ::Vector2 offset = AZ::Vector2::CreateZero();
        GraphCanvas::GridRequestBus::EventResult(offset, gridId, &GraphCanvas::GridRequests::GetMinorPitch);
        sceneDropPosition += offset;

        return graphCanvasNodeId;
    }

    bool GraphController::RemoveNode(GraphModel::NodePtr node)
    {
       const AZ::EntityId nodeUiId = m_elementMap.Find(node);
        if (nodeUiId.IsValid())
        {
            AzToolsFramework::EntityIdSet entityIds = { nodeUiId };
            GraphCanvas::SceneRequestBus::Event(GetGraphCanvasSceneId(), &GraphCanvas::SceneRequests::Delete, entityIds);

            return true;
        }

        return false;
    }

    AZ::Vector2 GraphController::GetPosition(GraphModel::NodePtr node) const
    {
        AZ::Vector2 position = AZ::Vector2::CreateZero();
        const AZ::EntityId nodeUiId = m_elementMap.Find(node);
        if (nodeUiId.IsValid())
        {
            GraphCanvas::GeometryRequestBus::EventResult(position, nodeUiId, &GraphCanvas::GeometryRequests::GetPosition);
        }

        return position;
    }

    void GraphController::WrapNodeInternal(GraphModel::NodePtr wrapperNode, GraphModel::NodePtr node, AZ::u32 layoutOrder)
    {
        AZ::EntityId wrapperNodeUiId = m_elementMap.Find(wrapperNode);
        if (!wrapperNodeUiId.IsValid())
        {
            // The parent WrapperNode needs to be added to the scene before we can wrap a child node
            return;
        }

        GraphControllerNotificationBus::Event(
            m_graphCanvasSceneId, &GraphControllerNotifications::PreOnGraphModelNodeWrapped, wrapperNode, node);

        AZ::EntityId nodeUiId = m_elementMap.Find(node);
        if (!nodeUiId.IsValid())
        {
            // If the node to be wrapped hasn't been added to the scene yet,
            // add it before wrapping it
            AZ::Vector2 dropPosition(0, 0);
            nodeUiId = AddNode(node, dropPosition);
        }

        m_graph->WrapNode(wrapperNode, node, layoutOrder);

        WrapNodeUi(wrapperNode, node, layoutOrder);

        GraphControllerNotificationBus::Event(
            m_graphCanvasSceneId, &GraphControllerNotifications::OnGraphModelNodeWrapped, wrapperNode, node);
    }

    void GraphController::WrapNode(GraphModel::NodePtr wrapperNode, GraphModel::NodePtr node)
    {
        WrapNodeInternal(wrapperNode, node);
    }

    void GraphController::WrapNodeOrdered(GraphModel::NodePtr wrapperNode, GraphModel::NodePtr node, AZ::u32 layoutOrder)
    {
        WrapNodeInternal(wrapperNode, node, layoutOrder);
    }

    void GraphController::UnwrapNode(GraphModel::NodePtr wrapperNode, GraphModel::NodePtr node)
    {
        AZ::EntityId wrapperNodeUiId = m_elementMap.Find(wrapperNode);
        AZ::EntityId nodeUiId = m_elementMap.Find(node);
        if (!wrapperNodeUiId.IsValid() || !nodeUiId.IsValid())
        {
            return;
        }

        m_graph->UnwrapNode(node);

        // Unwrap the node from the parent WrapperNode
        GraphCanvas::WrappedNodeConfiguration configuration;
        GraphCanvas::WrapperNodeRequestBus::Event(wrapperNodeUiId, &GraphCanvas::WrapperNodeRequests::UnwrapNode, nodeUiId);

        GraphControllerNotificationBus::Event(
            m_graphCanvasSceneId, &GraphControllerNotifications::OnGraphModelNodeUnwrapped, wrapperNode, node);
    }

    bool GraphController::IsNodeWrapped(GraphModel::NodePtr node) const
    {
        return m_graph->IsNodeWrapped(node);
    }

    void GraphController::WrapNodeUi(GraphModel::NodePtr wrapperNode, GraphModel::NodePtr node, AZ::u32 layoutOrder)
    {
        AZ::EntityId wrapperNodeUiId = m_elementMap.Find(wrapperNode);
        AZ::EntityId nodeUiId = m_elementMap.Find(node);
        if (!wrapperNodeUiId.IsValid() || !nodeUiId.IsValid())
        {
            return;
        }

        // Wrap the node in the parent WrapperNode with the given layout order
        GraphCanvas::WrappedNodeConfiguration configuration;
        configuration.m_layoutOrder = layoutOrder;
        GraphCanvas::WrapperNodeRequestBus::Event(wrapperNodeUiId, &GraphCanvas::WrapperNodeRequests::WrapNode, nodeUiId, configuration);
    }

    void GraphController::SetWrapperNodeActionString(GraphModel::NodePtr node, const char* actionString)
    {
        AZ::EntityId nodeUiId = m_elementMap.Find(node);
        if (!nodeUiId.IsValid())
        {
            return;
        }

        GraphCanvas::WrapperNodeRequestBus::Event(nodeUiId, &GraphCanvas::WrapperNodeRequests::SetActionString, actionString);
    }

    GraphModel::ConnectionPtr GraphController::AddConnection(GraphModel::SlotPtr sourceSlot, GraphModel::SlotPtr targetSlot)
    {
        GraphModel::ConnectionPtr newConnection = CreateConnection(sourceSlot, targetSlot);
        if (newConnection)
        {
            CreateConnectionUi(newConnection);
        }
        return newConnection;
    }

    GraphModel::ConnectionPtr GraphController::AddConnectionBySlotId(
        GraphModel::NodePtr sourceNode,
        const GraphModel::SlotId& sourceSlotId,
        GraphModel::NodePtr targetNode,
        const GraphModel::SlotId& targetSlotId)
    {
        GraphModel::SlotPtr sourceSlot = sourceNode->GetSlot(sourceSlotId);
        GraphModel::SlotPtr targetSlot = targetNode->GetSlot(targetSlotId);
        return AddConnection(sourceSlot, targetSlot);
    }

    bool GraphController::AreSlotsConnected(
        GraphModel::NodePtr sourceNode,
        const GraphModel::SlotId& sourceSlotId,
        GraphModel::NodePtr targetNode,
        const GraphModel::SlotId& targetSlotId) const
    {
        if (!sourceNode || !targetNode)
        {
            return false;
        }

        const GraphModel::SlotPtr sourceSlot = sourceNode->GetSlot(sourceSlotId);
        const GraphModel::SlotPtr targetSlot = targetNode->GetSlot(targetSlotId);
        if (!sourceSlot || !targetSlot)
        {
            return false;
        }

        // Check all connections on the source slot to see if they match the target node and slot
        for (const auto& connection : sourceSlot->GetConnections())
        {
            if (connection->GetTargetNode() == targetNode && connection->GetTargetSlot() == targetSlot)
            {
                return true;
            }
        }

        return false;
    }

    bool GraphController::RemoveConnection(GraphModel::ConnectionPtr connection)
    {
        const AZ::EntityId connectionUiId = m_elementMap.Find(connection);
        if (connectionUiId.IsValid())
        {
            AZStd::unordered_set<AZ::EntityId> deleteIds = { connectionUiId };

            // This general Delete method will in turn call SceneRequests::RemoveConnection,
            // but just calling RemoveConnection by itself won't actually delete the ConnectionComponent itself.
            GraphCanvas::SceneRequestBus::Event(GetGraphCanvasSceneId(), &GraphCanvas::SceneRequests::Delete, deleteIds);

            return true;
        }

        return false;
    }

    GraphModel::SlotId GraphController::ExtendSlot(GraphModel::NodePtr node, const GraphModel::SlotName& slotName)
    {
        GraphModel::SlotPtr newSlot = node->AddExtendedSlot(slotName);
        if (newSlot)
        {
            AZ::EntityId nodeUiId = m_elementMap.Find(node);
            CreateSlotUi(newSlot, nodeUiId);

            return newSlot->GetSlotId();
        }

        return GraphModel::SlotId();
    }

    GraphModel::NodePtr GraphController::GetNodeById(const GraphCanvas::NodeId& nodeId)
    {
        return m_elementMap.Find<GraphModel::Node>(nodeId);
    }

    GraphModel::NodePtrList GraphController::GetNodesFromGraphNodeIds(const AZStd::vector<GraphCanvas::NodeId>& nodeIds)
    {
        GraphModel::NodePtrList nodeList;
        nodeList.reserve(nodeIds.size());

        for (const auto& nodeId : nodeIds)
        {
            if (GraphModel::NodePtr nodePtr = m_elementMap.Find<GraphModel::Node>(nodeId))
            {
                nodeList.push_back(nodePtr);
            }
        }

        return nodeList;
    }

    GraphCanvas::NodeId GraphController::GetNodeIdByNode(GraphModel::NodePtr node) const
    {
        const GraphCanvas::NodeId nodeId = m_elementMap.Find(node);
        return nodeId.IsValid() ? nodeId : GraphCanvas::NodeId();
    }

    GraphCanvas::SlotId GraphController::GetSlotIdBySlot(GraphModel::SlotPtr slot) const
    {
        const GraphCanvas::SlotId slotId = m_elementMap.Find(slot);
        return slotId.IsValid() ? slotId : GraphCanvas::SlotId();
    }

    GraphModel::NodePtrList GraphController::GetNodes()
    {
        const auto& nodeMap = m_graph->GetNodes();
        GraphModel::NodePtrList nodes;
        nodes.reserve(nodeMap.size());

        for (auto& pair : nodeMap)
        {
            nodes.push_back(pair.second);
        }

        return nodes;
    }

    GraphModel::NodePtrList GraphController::GetSelectedNodes()
    {
        AzToolsFramework::EntityIdList selectedNodeIds;
        GraphCanvas::SceneRequestBus::EventResult(selectedNodeIds, m_graphCanvasSceneId, &GraphCanvas::SceneRequests::GetSelectedItems);

        return GetNodesFromGraphNodeIds(selectedNodeIds);
    }

    void GraphController::SetSelected(GraphModel::NodePtrList nodes, bool selected)
    {
        for (const auto& node : nodes)
        {
            const AZ::EntityId nodeId = m_elementMap.Find(node);
            if (nodeId.IsValid())
            {
                GraphCanvas::SceneMemberUIRequestBus::Event(nodeId, &GraphCanvas::SceneMemberUIRequests::SetSelected, selected);
                SaveMetadata(nodeId);
            }
        }
    }

    void GraphController::ClearSelection()
    {
        GraphCanvas::SceneRequestBus::Event(m_graphCanvasSceneId, &GraphCanvas::SceneRequests::ClearSelection);
    }

    void GraphController::EnableNode(GraphModel::NodePtr node)
    {
        const AZ::EntityId nodeId = m_elementMap.Find(node);
        if (nodeId.IsValid())
        {
            GraphCanvas::SceneRequestBus::Event(m_graphCanvasSceneId, &GraphCanvas::SceneRequests::Enable, nodeId);
        }
    }

    void GraphController::DisableNode(GraphModel::NodePtr node)
    {
        const AZ::EntityId nodeId = m_elementMap.Find(node);
        if (nodeId.IsValid())
        {
            GraphCanvas::SceneRequestBus::Event(m_graphCanvasSceneId, &GraphCanvas::SceneRequests::Disable, nodeId);
        }
    }

    void GraphController::CenterOnNodes(GraphModel::NodePtrList nodes)
    {
        AZStd::vector<AZ::Vector3> points;
        points.reserve(nodes.size() * 2);

        // Find all the position points for our nodes are are selecting
        // The Aabb class has functionality for creating a box from a series of points
        // so we are using that/Vector3 and just ignoring the Z value
        for (const auto& node : nodes)
        {
            const AZ::EntityId nodeId = m_elementMap.Find(node);
            AZ::Vector2 position = AZ::Vector2::CreateZero();
            GraphCanvas::GeometryRequestBus::EventResult(position, nodeId, &GraphCanvas::GeometryRequests::GetPosition);

            // Add the top-left corner position of the node
            points.emplace_back(position);

            // Add the bottom-right corner position of the node as well, so that
            // when we center the view, it will contain the entire node
            QGraphicsItem* nodeItem = nullptr;
            GraphCanvas::SceneMemberUIRequestBus::EventResult(nodeItem, nodeId, &GraphCanvas::SceneMemberUIRequests::GetRootGraphicsItem);
            if (nodeItem)
            {
                const QRectF nodeRect = nodeItem->boundingRect();
                points.emplace_back(position + AZ::Vector2(aznumeric_cast<float>(nodeRect.width()), aznumeric_cast<float>(nodeRect.height())));
            }
        }

        // Create a bounding box using all of our points so that we can center around
        // all of the nodes
        const AZ::Aabb boundingBox = AZ::Aabb::CreatePoints(points.data(), (int)points.size());
        const AZ::Vector3 topLeft = boundingBox.GetMin();
        QRectF boundingRect(topLeft.GetX(), topLeft.GetY(), boundingBox.GetXExtent(), boundingBox.GetYExtent());

        // Center the view on our desired area
        GraphCanvas::ViewId viewId;
        GraphCanvas::SceneRequestBus::EventResult(viewId, m_graphCanvasSceneId, &GraphCanvas::SceneRequests::GetViewId);
        GraphCanvas::ViewRequestBus::Event(viewId, &GraphCanvas::ViewRequests::CenterOnArea, boundingRect);
    }

    AZ::Vector2 GraphController::GetMajorPitch() const
    {
        AZ::EntityId gridId;
        AZ::Vector2 gridMajorPitch;
        GraphCanvas::SceneRequestBus::EventResult(gridId, m_graphCanvasSceneId, &GraphCanvas::SceneRequests::GetGrid);
        GraphCanvas::GridRequestBus::EventResult(gridMajorPitch, gridId, &GraphCanvas::GridRequests::GetMajorPitch);

        return gridMajorPitch;
    }

    void GraphController::OnNodeAdded(const AZ::EntityId& nodeUiId, bool)
    {
        if (const GraphModel::NodePtr node = m_elementMap.Find<GraphModel::Node>(nodeUiId))
        {
            GraphControllerNotificationBus::Event(m_graphCanvasSceneId, &GraphControllerNotifications::OnGraphModelNodeAdded, node);
        }
    }

    void GraphController::OnNodeRemoved(const AZ::EntityId& nodeUiId)
    {
        if (const GraphModel::NodePtr node = m_elementMap.Find<GraphModel::Node>(nodeUiId))
        {
            GraphControllerNotificationBus::Event(m_graphCanvasSceneId, &GraphControllerNotifications::PreOnGraphModelNodeRemoved, node);

            // Remove any thumbnail reference for this node when it is removed from the graph
            // The ThumbnailItem will be deleted by the Node layout itself
            m_nodeThumbnails.erase(node->GetId());

            // When a node gets removed, we need to remove all of its slots
            // from our m_elementMap as well
            for (const auto& it : node->GetSlots())
            {
                m_elementMap.Remove(it.second);
            }

            m_graph->RemoveNode(node);
            m_elementMap.Remove(node);

            GraphControllerNotificationBus::Event(m_graphCanvasSceneId, &GraphControllerNotifications::OnGraphModelNodeRemoved, node);
        }
    }

    void GraphController::OnConnectionAdded(const AZ::EntityId& connectionUiId)
    {
        if (const GraphModel::ConnectionPtr connection = m_elementMap.Find<GraphModel::Connection>(connectionUiId))
        {
            GraphCanvas::NodeUIRequestBus::Event(m_elementMap.Find(connection->GetSourceNode()), &GraphCanvas::NodeUIRequests::AdjustSize);
            GraphCanvas::NodeUIRequestBus::Event(m_elementMap.Find(connection->GetTargetNode()), &GraphCanvas::NodeUIRequests::AdjustSize);
        }
    }

    void GraphController::OnConnectionRemoved(const AZ::EntityId& connectionUiId)
    {
        if (const GraphModel::ConnectionPtr connection = m_elementMap.Find<GraphModel::Connection>(connectionUiId))
        {
            GraphControllerNotificationBus::Event(
                m_graphCanvasSceneId, &GraphControllerNotifications::OnGraphModelConnectionRemoved, connection);
            GraphCanvas::NodeUIRequestBus::Event(m_elementMap.Find(connection->GetSourceNode()), &GraphCanvas::NodeUIRequests::AdjustSize);
            GraphCanvas::NodeUIRequestBus::Event(m_elementMap.Find(connection->GetTargetNode()), &GraphCanvas::NodeUIRequests::AdjustSize);
            m_graph->RemoveConnection(connection);
            m_elementMap.Remove(connection);
        }
    }

    void GraphController::OnEntitiesSerialized(GraphCanvas::GraphSerialization& serializationTarget)
    {
        GraphModelSerialization serialization;

        // Create mappings of the serialized nodes/slots so that we can properly associate
        // the GraphCanvas nodes/slots that get deserialized later with the GraphModel counterparts
        const auto& nodeWrappings = m_graph->GetNodeWrappings();
        for (const auto& nodeEntity : serializationTarget.GetGraphData().m_nodes)
        {
            const GraphCanvas::NodeId nodeUiId = nodeEntity->GetId();
            const GraphModel::NodePtr node = m_elementMap.Find<GraphModel::Node>(nodeUiId);

            if (!node)
            {
                continue;
            }

            // Keep a mapping of the serialized GraphCanvas nodeId with the serialized GraphModel node
            // The node is being serialized to an object stream and deserialized as needed. This prevents any objects or smart pointers from
            // lingering after related systems have been destroyed.
            GraphModelSerialization::SerializedNodeBuffer serializedNodeBuffer;
            AZ::IO::ByteContainerStream<decltype(serializedNodeBuffer)> serializedNodeStream(&serializedNodeBuffer);
            AZ::Utils::SaveObjectToStream(serializedNodeStream, AZ::ObjectStream::ST_BINARY, &node);
            serialization.m_serializedNodes[nodeUiId] = serializedNodeBuffer;

            // Keep a mapping of the serialized GraphCanvas slotIds with their serialized GraphModel slots
            serialization.m_serializedSlotMappings[nodeUiId] = GraphModelSerialization::SerializedSlotMapping();
            for (const auto& slotPair : node->GetSlots())
            {
                const GraphModel::SlotId& slotId = slotPair.first;
                const GraphModel::SlotPtr slot = slotPair.second;
                const AZ::EntityId slotUiId = m_elementMap.Find(slot);
                if (slotUiId.IsValid())
                {
                    serialization.m_serializedSlotMappings[nodeUiId][slotId] = slotUiId;
                }
            }

            // Keep track of any serialized wrapped nodes, since these will need to be
            // handled separately after the deserialization is complete
            if (const auto it = nodeWrappings.find(node->GetId()); it != nodeWrappings.end())
            {
                GraphModel::NodePtr wrapperNode = m_graph->GetNode(it->second.first);
                AZ::u32 layoutOrder = it->second.second;

                AZ::EntityId wrapperNodeUiId = m_elementMap.Find(wrapperNode);
                AZ_Assert(wrapperNodeUiId.IsValid(), "Invalid wrapper node reference for node [%d]", wrapperNode->GetId());

                serialization.m_serializedNodeWrappings[nodeUiId] = AZStd::make_pair(wrapperNodeUiId, layoutOrder);
            }
        }

        GraphManagerRequestBus::Broadcast(&GraphManagerRequests::SetSerializedMappings, serialization);
    }

    void GraphController::OnEntitiesDeserialized(const GraphCanvas::GraphSerialization& serializationSource)
    {
        GraphModelSerialization serialization;
        GraphManagerRequestBus::BroadcastResult(serialization, &GraphManagerRequests::GetSerializedMappings);

        for (const auto& it : serialization.m_serializedNodes)
        {
            const auto& serializedNodeId = it.first;
            const auto serializedNodeBuffer = it.second;

            // Recreate the notes previously serialized to the stream.
            GraphModel::NodePtr newNode;
            AZ::IO::ByteContainerStream<decltype(serializedNodeBuffer)> serializedNodeStream(&serializedNodeBuffer);
            AZ::Utils::LoadObjectFromStreamInPlace(serializedNodeStream, newNode);

            // Load the new node into our graph
            m_graph->PostLoadSetup(newNode);

            // Re-map our new node to the deserialized GraphCanvas node
            AZ::EntityId newNodeUiId = serializationSource.FindRemappedEntityId(serializedNodeId);
            m_elementMap.Add(newNodeUiId, newNode);

            auto slotMapIt = serialization.m_serializedSlotMappings.find(serializedNodeId);
            if (slotMapIt == serialization.m_serializedSlotMappings.end())
            {
                continue;
            }

            const GraphModelSerialization::SerializedSlotMapping& serializedNodeSlots = slotMapIt->second;
            for (auto slotPair : newNode->GetSlots())
            {
                GraphModel::SlotId& slotId = slotPair.first;
                GraphModel::SlotPtr slot = slotPair.second;

                auto slotIt = serializedNodeSlots.find(slotId);
                if (slotIt == serializedNodeSlots.end())
                {
                    continue;
                }

                GraphCanvas::SlotId serializedSlotUiId = slotIt->second;
                GraphCanvas::SlotId newSlotUiId = serializationSource.FindRemappedEntityId(serializedSlotUiId);
                if (!newSlotUiId.IsValid())
                {
                    continue;
                }

                // Re-map our new slot to the deserialized GraphCanvas slot
                m_elementMap.Add(newSlotUiId, slot);
            }
        }
    }

    void GraphController::OnEntitiesDeserializationComplete(const GraphCanvas::GraphSerialization& serializationSource)
    {
        GraphModelSerialization serialization;
        GraphManagerRequestBus::BroadcastResult(serialization, &GraphManagerRequests::GetSerializedMappings);

        // We need to handle the wrapped nodes after all the nodes have been deserialized
        // so that the wrapper nodes will be active/ready to accept the nodes being
        // wrapped onto them.
        for (auto it : serialization.m_serializedNodeWrappings)
        {
            GraphCanvas::NodeId serializedNodeId = it.first;
            GraphCanvas::NodeId wrapperNodeId = it.second.first;
            AZ::u32 layoutOrder = it.second.second;

            AZ::EntityId newNodeId = serializationSource.FindRemappedEntityId(serializedNodeId);
            AZ::EntityId newWrapperNodeId = serializationSource.FindRemappedEntityId(wrapperNodeId);
            GraphModel::NodePtr newNode = m_elementMap.Find<GraphModel::Node>(newNodeId);
            GraphModel::NodePtr newWrapperNode = m_elementMap.Find<GraphModel::Node>(newWrapperNodeId);

            if (newNode && newWrapperNode)
            {
                WrapNodeInternal(newWrapperNode, newNode, layoutOrder);
            }
        }
    }

    void GraphController::OnNodeIsBeingEdited(bool isBeingEditeed)
    {
        if (isBeingEditeed)
        {
            GraphCanvas::GraphModelRequestBus::Event(
                m_graphCanvasSceneId, &GraphCanvas::GraphModelRequests::RequestPushPreventUndoStateUpdate);
        }
        else
        {
            GraphCanvas::GraphModelRequestBus::Event(
                m_graphCanvasSceneId, &GraphCanvas::GraphModelRequests::RequestPopPreventUndoStateUpdate);
            GraphCanvas::GraphModelRequestBus::Event(m_graphCanvasSceneId, &GraphCanvas::GraphModelRequests::RequestUndoPoint);
        }
    }

    void GraphController::OnSelectionChanged()
    {
        bool loading = false;
        GraphCanvas::SceneRequestBus::EventResult(loading, GetGraphCanvasSceneId(), &GraphCanvas::SceneRequests::IsLoading);
        bool pasting = false;
        GraphCanvas::SceneRequestBus::EventResult(pasting, GetGraphCanvasSceneId(), &GraphCanvas::SceneRequests::IsPasting);
        if (loading || pasting)
        {
            return;
        }

        // Save selection save data and other metadata every time the selection changes unless this is during a reload or a face operation
        SaveMetadata(GetGraphCanvasSceneId());
        GraphCanvas::SceneRequestBus::Event(
            GetGraphCanvasSceneId(),
            [&](GraphCanvas::SceneRequests* scene)
            {
                for (const auto& element : scene->GetNodes())
                {
                    SaveMetadata(element);
                }
                for (const auto& element : scene->GetConnections())
                {
                    SaveMetadata(element);
                }
            });
    }

    void GraphController::WriteSaveData(GraphCanvas::EntitySaveDataContainer& saveDataContainer) const
    {
        // Store selection save data for the current graph element
        const AZ::EntityId nodeUiId = *GraphCanvas::EntitySaveDataRequestBus::GetCurrentBusId();

        // Save data will only be stored for selected items to minimize file size
        bool selected = false;
        GraphCanvas::SceneMemberUIRequestBus::EventResult(selected, nodeUiId, &GraphCanvas::SceneMemberUIRequests::IsSelected);
        if (selected)
        {
            auto data = saveDataContainer.FindCreateSaveData<GraphCanvasSelectionData>();
            data->m_selected = selected;
        }
    }

    void GraphController::ReadSaveData(const GraphCanvas::EntitySaveDataContainer& saveDataContainer)
    {
        // Restore selection and position data for the current graph element
        const AZ::EntityId nodeUiId = *GraphCanvas::EntitySaveDataRequestBus::GetCurrentBusId();

        if (auto data = saveDataContainer.FindSaveData<GraphCanvasSelectionData>())
        {
            GraphCanvas::SceneMemberUIRequestBus::Event(nodeUiId, &GraphCanvas::SceneMemberUIRequests::SetSelected, data->m_selected);
        }

        if (auto data = saveDataContainer.FindSaveData<GraphCanvas::GeometrySaveData>())
        {
            GraphCanvas::GeometryRequestBus::Event(nodeUiId, &GraphCanvas::GeometryRequests::SetPosition, data->m_position);
        }
    }

    GraphModel::ConnectionPtr GraphController::CreateConnection(GraphModel::SlotPtr sourceSlot, GraphModel::SlotPtr targetSlot)
    {
        if (!sourceSlot || !targetSlot)
        {
            return nullptr;
        }

        // Remove existing connections on target slot
        for (GraphModel::ConnectionPtr connection : targetSlot->GetConnections())
        {
            RemoveConnection(connection);
            // No need to clean up the maps here because the OnConnectionRemoved() callback will handle that
        }

        GraphModel::ConnectionPtr connection = m_graph->AddConnection(sourceSlot, targetSlot);
        GraphControllerNotificationBus::Event(
            m_graphCanvasSceneId, &GraphControllerNotifications::OnGraphModelConnectionAdded, connection);
        GraphCanvas::NodeUIRequestBus::Event(m_elementMap.Find(connection->GetSourceNode()), &GraphCanvas::NodeUIRequests::AdjustSize);
        GraphCanvas::NodeUIRequestBus::Event(m_elementMap.Find(connection->GetTargetNode()), &GraphCanvas::NodeUIRequests::AdjustSize);
        return connection;
    }

    bool GraphController::CreateConnection(
        const AZ::EntityId& connectionUiId, const GraphCanvas::Endpoint& sourcePoint, const GraphCanvas::Endpoint& targetPoint)
    {
        using namespace GraphModel;

        if (m_isCreatingConnectionUi)
        {
            // We're already creating the connection further up the callstack
            return true;
        }

        if (!sourcePoint.IsValid() || !targetPoint.IsValid())
        {
            return false;
        }

        SlotPtr sourceSlot = m_elementMap.Find<Slot>(sourcePoint.GetSlotId());
        SlotPtr targetSlot = m_elementMap.Find<Slot>(targetPoint.GetSlotId());

        // Handle the cases where this Connection already exists in our model
        ConnectionPtr connection = m_elementMap.Find<Connection>(connectionUiId);
        if (connection)
        {
            // If the connection being created has the same source and target as the existing connection, then either:
            //  1. The user cancelled the action after disconnecting from the slot
            //   OR
            //  2. The user reconnected to the same slot after disconnecting it
            // So in either case, we can just return true since the model should remain the same and
            // GraphCanvas has already done the right thing display wise
            if (connection->GetSourceSlot() == sourceSlot && connection->GetTargetSlot() == targetSlot)
            {
                return true;
            }
            // Otherwise, the user has disconnected an existing connection from a slot and
            // has connected it to a different slot, so we need to remove the pre-existing
            // Connection from our model. GraphCanvas has already deleted the previous connection
            // from the UI when GraphCanvas::GraphModelRequestBus::DisconnectConnection is invoked.
            else
            {
                OnConnectionRemoved(connectionUiId);
            }
        }

        ConnectionPtr newConnection = CreateConnection(sourceSlot, targetSlot);
        if (newConnection)
        {
            m_elementMap.Add(connectionUiId, newConnection);
            return true;
        }

        return false;
    }

    bool GraphController::CheckForLoopback(GraphModel::NodePtr sourceNode, GraphModel::NodePtr targetNode) const
    {
        // TODO: In the future, we could add support here for the client to choose if
        // loopbacks should be supported or not.

        // If at any point the target and source nodes are the same,
        // then we've detected a connection loop
        if (targetNode == sourceNode)
        {
            return true;
        }

        for (auto slotIt : sourceNode->GetSlots())
        {
            GraphModel::SlotPtr slot = slotIt.second;

            // We only care about input slots because we are crawling upstream
            if (slot->GetSlotDirection() != GraphModel::SlotDirection::Input)
            {
                continue;
            }

            // Check for loopback on any of the connected input slots
            for (GraphModel::ConnectionPtr connection : slot->GetConnections())
            {
                if (CheckForLoopback(connection->GetSourceNode(), targetNode))
                {
                    return true;
                }
            }
        }

        return false;
    }

    bool GraphController::IsValidConnection(const GraphCanvas::Endpoint& sourcePoint, const GraphCanvas::Endpoint& targetPoint) const
    {
        if (!sourcePoint.IsValid() || !targetPoint.IsValid())
        {
            return false;
        }

        auto sourceSlot = m_elementMap.Find<GraphModel::Slot>(sourcePoint.GetSlotId());
        auto targetSlot = m_elementMap.Find<GraphModel::Slot>(targetPoint.GetSlotId());

        // Make sure both slots are in our element map
        if (!sourceSlot || !targetSlot)
        {
            return false;
        }

        bool dataTypesMatch = false;
        GraphModel::DataTypePtr sourceSlotDataType = sourceSlot->GetDataType();
        GraphModel::DataTypePtr targetSlotDataType = targetSlot->GetDataType();
        if (sourceSlotDataType == nullptr && targetSlotDataType == nullptr)
        {
            // If both data types are null, this means the slots are both event types,
            // so this is considered valid
            AZ_Assert(
                sourceSlot->GetSlotType() == GraphModel::SlotType::Event, "Source slot has a null data type but is not an Event type slot");
            AZ_Assert(
                targetSlot->GetSlotType() == GraphModel::SlotType::Event, "Target slot has a null data type but is not an Event type slot");
            dataTypesMatch = true;
        }
        else if (sourceSlotDataType == nullptr || targetSlotDataType == nullptr)
        {
            // If one of the data types is null but the other isn't, then this is invalid
            dataTypesMatch = false;
        }
        else
        {
            // The source slot data type must be supported by the target slot
            dataTypesMatch = targetSlot->IsSupportedDataType(sourceSlotDataType);
        }

        return dataTypesMatch && !CheckForLoopback(sourceSlot->GetParentNode(), targetSlot->GetParentNode());
    }

    //! Helper function to create a GraphCanvas::NodePropertyDisplay and a data interface for editing input pin values
    //! \typename  DataInterfaceType  One of the data interface types. Ex: BooleanDataInterface
    //! \typename  CreateDisplayFunctionType  Function pointer type should be filled automatically
    //! \param  inputSlot the input slot
    //! \param  createDisplayFunction  GraphCanvasRequests EBus function for creating the NodePropertyDisplay
    template<typename DataInterfaceType, typename CreateDisplayFunctionType>
    GraphCanvas::NodePropertyDisplay* CreatePropertyDisplay(GraphModel::SlotPtr inputSlot, CreateDisplayFunctionType createDisplayFunction)
    {
        GraphCanvas::NodePropertyDisplay* dataDisplay = nullptr;
        if (inputSlot)
        {
            GraphCanvas::DataInterface* dataInterface = aznew DataInterfaceType(inputSlot);
            GraphCanvas::GraphCanvasRequestBus::BroadcastResult(
                dataDisplay, createDisplayFunction, static_cast<DataInterfaceType*>(dataInterface));
            if (!dataDisplay)
            {
                delete dataInterface;
            }
        }
        return dataDisplay;
    }

    GraphCanvas::NodePropertyDisplay* GraphController::CreatePropertySlotPropertyDisplay(
        [[maybe_unused]] const AZ::Crc32& propertyId,
        [[maybe_unused]] const GraphCanvas::NodeId& nodeUiId,
        const GraphCanvas::SlotId& slotUiId) const
    {
        // CONST CAST WARNING: The CreatePropertySlotPropertyDisplay graph canvas interface is const, but probably shouldn't be, because it
        // expects a non-const NodePropertyDisplay*. We need non-const version of m_elementMap in order to create a non-const
        // NodePropertyDisplay
        GraphModel::SlotPtr inputSlot = const_cast<GraphController*>(this)->m_elementMap.Find<GraphModel::Slot>(slotUiId);
        GraphCanvas::NodePropertyDisplay* display = CreateSlotPropertyDisplay(inputSlot);
        if (display)
        {
            display->SetNodeId(nodeUiId);
            display->SetSlotId(slotUiId);
        }
        return display;
    }

    GraphCanvas::NodePropertyDisplay* GraphController::CreateDataSlotPropertyDisplay(
        [[maybe_unused]] const AZ::Uuid& dataTypeUuid,
        [[maybe_unused]] const GraphCanvas::NodeId& nodeUiId,
        const GraphCanvas::SlotId& slotUiId) const
    {
#if defined(AZ_ENABLE_TRACING)
        GraphModel::DataTypePtr dataType = m_graph->GetContext()->GetDataType(dataTypeUuid);
        AZ_Assert(
            dataType->GetTypeUuid() == dataTypeUuid,
            "Creating property display for mismatched type. dataTypeUuid=%s. Slot TypeName=%s TypeID=%s.",
            dataTypeUuid.ToString<AZStd::string>().c_str(),
            dataType->GetCppName().c_str(),
            dataType->GetTypeUuidString().c_str());
#endif // AZ_ENABLE_TRACING

        // CONST CAST WARNING: The CreateDataSlotPropertyDisplay graph canvas interface is const, but probably shouldn't be, because it
        // expects a non-const NodePropertyDisplay*. We need non-const version of m_elementMap in order to create a non-const
        // NodePropertyDisplay
        GraphModel::SlotPtr inputSlot = const_cast<GraphController*>(this)->m_elementMap.Find<GraphModel::Slot>(slotUiId);
        GraphCanvas::NodePropertyDisplay* display = CreateSlotPropertyDisplay(inputSlot);
        if (display)
        {
            display->SetNodeId(nodeUiId);
            display->SetSlotId(slotUiId);
        }
        return display;
    }

    GraphCanvas::NodePropertyDisplay* GraphController::CreateSlotPropertyDisplay(GraphModel::SlotPtr inputSlot) const
    {
        if (!inputSlot || !inputSlot->IsVisibleOnNode() || !inputSlot->IsEditableOnNode())
        {
            return nullptr;
        }

        AZ_Assert(
            inputSlot->GetSlotDirection() == GraphModel::SlotDirection::Input, "Property value displays are only meant for input slots");

        const AZ::Uuid dataTypeUuid = inputSlot->GetDataType()->GetTypeUuid();

        if (dataTypeUuid == azrtti_typeid<bool>())
        {
            return CreatePropertyDisplay<BooleanDataInterface>(
                inputSlot, &GraphCanvas::GraphCanvasRequests::CreateBooleanNodePropertyDisplay);
        }
        if (dataTypeUuid == azrtti_typeid<int>())
        {
            return CreatePropertyDisplay<IntegerDataInterface<int>>(
                inputSlot, &GraphCanvas::GraphCanvasRequests::CreateNumericNodePropertyDisplay);
        }
        if (dataTypeUuid == azrtti_typeid<unsigned int>())
        {
            return CreatePropertyDisplay<IntegerDataInterface<unsigned int>>(
                inputSlot, &GraphCanvas::GraphCanvasRequests::CreateNumericNodePropertyDisplay);
        }
        if (dataTypeUuid == azrtti_typeid<long>())
        {
            return CreatePropertyDisplay<IntegerDataInterface<long>>(
                inputSlot, &GraphCanvas::GraphCanvasRequests::CreateNumericNodePropertyDisplay);
        }
        if (dataTypeUuid == azrtti_typeid<unsigned long>())
        {
            return CreatePropertyDisplay<IntegerDataInterface<unsigned long>>(
                inputSlot, &GraphCanvas::GraphCanvasRequests::CreateNumericNodePropertyDisplay);
        }
        if (dataTypeUuid == azrtti_typeid<float>())
        {
            return CreatePropertyDisplay<FloatDataInterface>(
                inputSlot, &GraphCanvas::GraphCanvasRequests::CreateNumericNodePropertyDisplay);
        }
        if (dataTypeUuid == azrtti_typeid<AZ::Vector2>())
        {
            return CreatePropertyDisplay<VectorDataInterface<AZ::Vector2, 2>>(
                inputSlot, &GraphCanvas::GraphCanvasRequests::CreateVectorNodePropertyDisplay);
        }
        if (dataTypeUuid == azrtti_typeid<AZ::Vector3>())
        {
            return CreatePropertyDisplay<VectorDataInterface<AZ::Vector3, 3>>(
                inputSlot, &GraphCanvas::GraphCanvasRequests::CreateVectorNodePropertyDisplay);
        }
        if (dataTypeUuid == azrtti_typeid<AZ::Vector4>())
        {
            return CreatePropertyDisplay<VectorDataInterface<AZ::Vector4, 4>>(
                inputSlot, &GraphCanvas::GraphCanvasRequests::CreateVectorNodePropertyDisplay);
        }
        if (dataTypeUuid == azrtti_typeid<AZStd::string>())
        {
            return CreatePropertyDisplay<StringDataInterface>(
                inputSlot, &GraphCanvas::GraphCanvasRequests::CreateStringNodePropertyDisplay);
        }

        return nullptr;
    }

    void GraphController::RequestUndoPoint()
    {
        if (m_preventUndoStateUpdateCount <= 0)
        {
            m_preventUndoStateUpdateCount = 0;
            GraphControllerNotificationBus::Event(m_graphCanvasSceneId, &GraphControllerNotifications::OnGraphModelRequestUndoPoint);
            IntegrationBus::Broadcast(&IntegrationBusInterface::SignalSceneDirty, m_graphCanvasSceneId);
        }
    }

    void GraphController::RequestPushPreventUndoStateUpdate()
    {
        ++m_preventUndoStateUpdateCount;
    }

    void GraphController::RequestPopPreventUndoStateUpdate()
    {
        if (m_preventUndoStateUpdateCount > 0)
        {
            --m_preventUndoStateUpdateCount;
        }
    }

    void GraphController::TriggerUndo()
    {
        GraphControllerNotificationBus::Event(m_graphCanvasSceneId, &GraphControllerNotifications::OnGraphModelTriggerUndo);
    }

    void GraphController::TriggerRedo()
    {
        GraphControllerNotificationBus::Event(m_graphCanvasSceneId, &GraphControllerNotifications::OnGraphModelTriggerRedo);
    }

    void GraphController::EnableNodes(const AZStd::unordered_set<GraphCanvas::NodeId>& nodeIds)
    {
        AZ_UNUSED(nodeIds);
    }

    void GraphController::DisableNodes(const AZStd::unordered_set<GraphCanvas::NodeId>& nodeIds)
    {
        AZ_UNUSED(nodeIds);
    }

    AZStd::string GraphController::GetDataTypeString(const AZ::Uuid& typeId)
    {
        return m_graph->GetContext()->GetDataType(typeId)->GetDisplayName();
    }

    GraphCanvasMetadata* GraphController::GetGraphMetadata()
    {
        GraphCanvasMetadata* graphCanvasMetadata = &m_graph->GetUiMetadata();
        AZ_Assert(graphCanvasMetadata, "GraphCanvasMetadata not initialized");
        return graphCanvasMetadata;
    }

    void GraphController::OnSaveDataDirtied(const AZ::EntityId& savedElement)
    {
        SaveMetadata(savedElement);
    }

    void GraphController::ResetSlotToDefaultValue(const GraphCanvas::Endpoint& endpoint)
    {
        if (auto slot = m_elementMap.Find<GraphModel::Slot>(endpoint.GetSlotId()))
        {
            GraphCanvas::ScopedGraphUndoBatch undoBatch(m_graphCanvasSceneId);
            slot->SetValue(slot->GetDefaultValue());
        }
    }

    void GraphController::RemoveSlot(const GraphCanvas::Endpoint& endpoint)
    {
        GraphCanvas::ScopedGraphUndoBatch undoBatch(m_graphCanvasSceneId);
        const GraphCanvas::NodeId& nodeId = endpoint.GetNodeId();
        const GraphCanvas::SlotId& slotId = endpoint.GetSlotId();
        auto node = m_elementMap.Find<GraphModel::Node>(nodeId);
        auto slot = m_elementMap.Find<GraphModel::Slot>(slotId);

        if (node && slot)
        {
            node->DeleteSlot(slot);

            // We need to actually remove the slot, the GraphModelRequestBus::RemoveSlot is a request, not a notification that the slot has
            // been removed
            GraphCanvas::NodeRequestBus::Event(nodeId, &GraphCanvas::NodeRequests::RemoveSlot, slotId);
        }
    }

    bool GraphController::IsSlotRemovable(const GraphCanvas::Endpoint& endpoint) const
    {
        auto node = m_elementMap.Find<GraphModel::Node>(endpoint.GetNodeId());
        auto slot = m_elementMap.Find<GraphModel::Slot>(endpoint.GetSlotId());
        return node && slot && node->CanDeleteSlot(slot);
    }

    GraphCanvas::SlotId GraphController::RequestExtension(
        const GraphCanvas::NodeId& nodeId, const GraphCanvas::ExtenderId& extenderId, GraphModelRequests::ExtensionRequestReason)
    {
        GraphCanvas::ScopedGraphUndoBatch undoBatch(m_graphCanvasSceneId);

        GraphModel::NodePtr node = m_elementMap.Find<GraphModel::Node>(nodeId);
        if (!node)
        {
            return GraphCanvas::SlotId{};
        }

        auto it = m_nodeExtenderIds.find(nodeId);
        if (it == m_nodeExtenderIds.end())
        {
            return GraphCanvas::SlotId{};
        }

        auto extenderIt = it->second.find(extenderId);
        if (extenderIt == it->second.end())
        {
            return GraphCanvas::SlotId{};
        }

        // The extension request will usually result in a new slot being added, unless
        // the maximum allowed slots for that definition has been reached, or the
        // Node has overriden the extension handling and rejected the new slot
        const GraphModel::SlotName& slotName = extenderIt->second;
        const GraphModel::SlotId& newSlotId = ExtendSlot(node, slotName);
        GraphModel::SlotPtr newSlot = node->GetSlot(newSlotId);
        if (!newSlot)
        {
            return GraphCanvas::SlotId{};
        }

        return m_elementMap.Find(newSlot);
    }

    bool GraphController::ShouldWrapperAcceptDrop(const GraphCanvas::NodeId& wrapperNode, const QMimeData* mimeData) const
    {
        AZ_UNUSED(wrapperNode);
        AZ_UNUSED(mimeData);
        return false;
    }

    void GraphController::AddWrapperDropTarget(const GraphCanvas::NodeId& wrapperNode)
    {
        AZ_UNUSED(wrapperNode);
    }

    void GraphController::RemoveWrapperDropTarget(const GraphCanvas::NodeId& wrapperNode)
    {
        AZ_UNUSED(wrapperNode);
    }

    void GraphController::SaveMetadata(const AZ::EntityId& graphCanvasElement)
    {
        bool loading = false;
        GraphCanvas::SceneRequestBus::EventResult(loading, GetGraphCanvasSceneId(), &GraphCanvas::SceneRequests::IsLoading);
        bool pasting = false;
        GraphCanvas::SceneRequestBus::EventResult(pasting, GetGraphCanvasSceneId(), &GraphCanvas::SceneRequests::IsPasting);
        if (loading || pasting)
        {
            return;
        }

        using namespace GraphModel;
        GraphCanvasMetadata* graphCanvasMetadata = GetGraphMetadata();

        // Connect the EntitySaveDataRequestBus using a router. This will allow the graph controller to inject custom selection save
        // data for all graph canvas objects. This should only be connected while serializing save data for the current graph.
        GraphCanvas::EntitySaveDataRequestBus::Router::BusRouterConnect();

        // Save into m_nodeMetadata
        if (const auto node = m_elementMap.Find<Node>(graphCanvasElement))
        {
            auto container = AZStd::make_shared<GraphCanvas::EntitySaveDataContainer>();
            GraphCanvas::EntitySaveDataRequestBus::Event(
                graphCanvasElement, &GraphCanvas::EntitySaveDataRequests::WriteSaveData, *container);

            const NodeId nodeId = node->GetId();
            graphCanvasMetadata->m_nodeMetadata[nodeId] = container;

            GraphControllerNotificationBus::Event(m_graphCanvasSceneId, &GraphControllerNotifications::OnGraphModelGraphModified, node);
        }
        // Save into m_sceneMetadata
        else if (graphCanvasElement == GetGraphCanvasSceneId())
        {
            auto container = AZStd::make_shared<GraphCanvas::EntitySaveDataContainer>();
            GraphCanvas::EntitySaveDataRequestBus::Event(
                graphCanvasElement, &GraphCanvas::EntitySaveDataRequests::WriteSaveData, *container);
            graphCanvasMetadata->m_sceneMetadata = container;

            GraphControllerNotificationBus::Event(m_graphCanvasSceneId, &GraphControllerNotifications::OnGraphModelGraphModified, nullptr);
        }

        // Disconnect the EntitySaveDataRequestBus after save data serialization as completed
        GraphCanvas::EntitySaveDataRequestBus::Router::BusRouterDisconnect();
    }

    QGraphicsLinearLayout* GraphController::GetLayoutFromNode(GraphModel::NodePtr node)
    {
        AZ::EntityId nodeUiId = m_elementMap.Find(node);
        QGraphicsLayout* layout = nullptr;
        GraphCanvas::NodeLayoutRequestBus::EventResult(layout, nodeUiId, &GraphCanvas::NodeLayoutRequests::GetLayout);
        if (layout)
        {
            // We can't do qobject_cast or dynamic_cast here since it doesn't derive from QObject
            // and it's a Qt class that wasn't compiled with rtti. This layout is created by
            // GraphCanvas though so we can rely on knowing the type.
            QGraphicsLinearLayout* linearLayout = static_cast<QGraphicsLinearLayout*>(layout);
            return linearLayout;
        }

        return nullptr;
    }

    void GraphController::SetThumbnailImageOnNode(GraphModel::NodePtr node, const QPixmap& image)
    {
        auto it = m_nodeThumbnails.find(node->GetId());
        if (it != m_nodeThumbnails.end())
        {
            // Update the image if the thumbnail already existed
            ThumbnailImageItem* item = azrtti_cast<ThumbnailImageItem*>(it->second);
            AZ_Assert(item, "Mismatch trying to set default image on a custom ThumbnailItem");
            item->UpdateImage(image);
        }
        else
        {
            // Find the layout for this Node so we can insert our thumbnail
            QGraphicsLinearLayout* layout = GetLayoutFromNode(node);
            if (!layout)
            {
                return;
            }

            // Create a new thumbnail item if we didn't have one before
            // The layout takes ownership of the item when inserted
            ThumbnailImageItem* newItem = new ThumbnailImageItem(image);
            layout->insertItem(NODE_THUMBNAIL_INDEX, newItem);
            m_nodeThumbnails[node->GetId()] = newItem;
        }
    }

    void GraphController::SetThumbnailOnNode(GraphModel::NodePtr node, ThumbnailItem* item)
    {
        // Remove any existing thumbnail on this node if one already exists
        auto it = m_nodeThumbnails.find(node->GetId());
        if (it != m_nodeThumbnails.end())
        {
            RemoveThumbnailFromNode(node);
        }

        QGraphicsLinearLayout* layout = GetLayoutFromNode(node);
        if (!layout)
        {
            AZ_Assert(false, "Couldn't find a layout for the node");
            return;
        }

        // Add the custom thumbnail item to the node
        layout->insertItem(NODE_THUMBNAIL_INDEX, item);
        m_nodeThumbnails[node->GetId()] = item;
    }

    void GraphController::RemoveThumbnailFromNode(GraphModel::NodePtr node)
    {
        auto it = m_nodeThumbnails.find(node->GetId());
        if (it != m_nodeThumbnails.end())
        {
            // Remove the thumbnail from our local tracking
            ThumbnailItem* item = it->second;
            m_nodeThumbnails.erase(it);

            QGraphicsLinearLayout* layout = GetLayoutFromNode(node);
            if (!layout)
            {
                AZ_Assert(false, "Couldn't find a layout for the node");
                return;
            }

            // Remove our item from the node layout, which releases ownership from the layout
            layout->removeItem(item);

            // If this was one of our ThumbnailImageItem's, then we need to delete it ourselves
            // since we allocated it. If someone created their own custom ThumbnailItem and
            // set it using SetThumbnailOnNode, they are in charge of deleting it after
            // calling RemoveThumbnailFromNode.
            ThumbnailImageItem* imageItem = azrtti_cast<ThumbnailImageItem*>(item);
            if (imageItem)
            {
                delete item;
            }
        }
    }
} // namespace GraphModelIntegration
