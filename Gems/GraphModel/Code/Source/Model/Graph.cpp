/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// AZ
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

// Graph Model
#include <GraphModel/Model/Graph.h>
#include <GraphModel/Model/GraphContext.h>
#include <GraphModel/Model/Node.h>
#include <GraphModel/Model/Slot.h>
#include <GraphModel/Model/Connection.h>

namespace GraphModel
{

    void Graph::Reflect(AZ::ReflectContext* context)
    {
        Node::Reflect(context);
        SlotIdData::Reflect(context);
        Slot::Reflect(context);
        Connection::Reflect(context);

        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<Graph>()
                ->Version(2)
                ->Field("m_nodes", &Graph::m_nodes)
                ->Field("m_connections", &Graph::m_connections)
                ->Field("m_uiMetadata", &Graph::m_uiMetadata)
                ->Field("m_nodeWrappings", &Graph::m_nodeWrappings)
                ;
        }
    }

    Graph::Graph(GraphContextPtr graphContext)
        : m_graphContext(graphContext)
    {

    }

    void Graph::PostLoadSetup(GraphContextPtr graphContext)
    {
        AZ_Assert(m_nextNodeId == 1, "This graph has been set up before");

        m_graphContext = graphContext;

        for (auto& [nodeId, node] : m_nodes)
        {
            node->PostLoadSetup(shared_from_this(), nodeId);

            // Find the highest NodeId in the graph so we can figure out what the next one should be
            m_nextNodeId = AZ::GetMax(m_nextNodeId, nodeId + 1);
        }

        for (auto& connection : m_connections)
        {
            connection->PostLoadSetup(shared_from_this());
        }

        AZStd::erase_if(m_connections, [](const auto& connection){
            return !connection || !connection->GetSourceSlot() || !connection->GetTargetSlot();
        });
    }

    NodeId Graph::PostLoadSetup(NodePtr node)
    {
        node->m_graph = shared_from_this();
        NodeId nodeId = AddNode(node);
        node->PostLoadSetup();

        return nodeId;
    }

    GraphContextPtr Graph::GetContext() const
    {
        AZ_Assert(m_graphContext, "Graph::m_graphContext is not set");
        return m_graphContext;
    }

    const char* Graph::GetSystemName() const
    { 
        return GetContext()->GetSystemName();
    }

    ConnectionPtr Graph::FindConnection(ConstSlotPtr sourceSlot, ConstSlotPtr targetSlot)
    {
        if (sourceSlot && targetSlot)
        {
            for (ConnectionPtr connection : m_connections)
            {
                if (connection->GetSourceSlot() == sourceSlot && connection->GetTargetSlot() == targetSlot)
                {
                    return connection;
                }
            }
        }

        return nullptr;
    }


    bool Graph::Contains(SlotPtr slot) const
    {
        if (slot)
        {
            for (const auto& nodePair : m_nodes)
            {
                if (nodePair.second->Contains(slot))
                {
                    return true;
                }
            }
        }

        return false;
    }


    NodePtr Graph::GetNode(NodeId nodeId)
    {
        auto nodeIter = m_nodes.find(nodeId);
        return nodeIter != m_nodes.end() ? nodeIter->second : nullptr;
    }


    const Graph::NodeMap& Graph::GetNodes()
    {
        return m_nodes;
    }

    Graph::ConstNodeMap Graph::GetNodes() const
    {
        return Graph::ConstNodeMap(m_nodes.begin(), m_nodes.end());
    }

    NodeId Graph::AddNode(NodePtr node)
    {
        AZ_Assert(Node::INVALID_NODE_ID == node->GetId(), "It appears this node already exists in a Graph");
        AZ_Assert(this == node->GetGraph().get(), "The Node was not created for this Graph");
        
        node->m_id = m_nextNodeId++;
        m_nodes.insert(AZStd::make_pair(node->m_id, node));

        return node->m_id;
    }


    bool Graph::RemoveNode(ConstNodePtr node)
    {
        // First delete any connections that are attached to the node.
        AZStd::erase_if(m_connections, [&](const auto& connection){
            return !connection || !connection->GetSourceSlot() || !connection->GetTargetSlot() || connection->GetSourceNode() == node || connection->GetTargetNode() == node;
        });

        // Also, remove any node wrapping stored for this node
        UnwrapNode(node);

        return m_nodes.erase(node->GetId()) != 0;
    }


    void Graph::WrapNode(NodePtr wrapperNode, NodePtr node, AZ::u32 layoutOrder)
    {
        AZ_Assert(m_nodes.find(wrapperNode->GetId()) != m_nodes.end(), "The wrapperNode must be in the graph before having a node wrapped on it");
        AZ_Assert(m_nodes.find(node->GetId()) != m_nodes.end(), "The node must be in the graph before being wrapped");
        AZ_Assert(wrapperNode->GetNodeType() == NodeType::WrapperNode, "The node containing the wrapped node must be of node type WrapperNode");
        AZ_Assert(node->GetNodeType() != NodeType::WrapperNode, "Nested WrapperNodes are not allowed");
        AZ_Assert(m_nodeWrappings.find(node->GetId()) == m_nodeWrappings.end(), "The specified node is already wrapped on another WrapperNode");

        m_nodeWrappings[node->GetId()] = AZStd::make_pair(wrapperNode->GetId(), layoutOrder);
    }


    void Graph::UnwrapNode(ConstNodePtr node)
    {
        m_nodeWrappings.erase(node->GetId());
    }


    bool Graph::IsNodeWrapped(NodePtr node) const
    {
        return m_nodeWrappings.contains(node->GetId());
    }


    const Graph::NodeWrappingMap& Graph::GetNodeWrappings()
    {
        return m_nodeWrappings;
    }


    const Graph::ConnectionList& Graph::GetConnections()
    {
        return m_connections;
    }


    ConnectionPtr Graph::AddConnection(SlotPtr sourceSlot, SlotPtr targetSlot)
    {
        if (ConnectionPtr existingConnection = FindConnection(sourceSlot, targetSlot))
        {
            return existingConnection;
        }

        if (Contains(sourceSlot) && Contains(targetSlot))
        {
            m_connections.push_back(AZStd::make_shared<Connection>(shared_from_this(), sourceSlot, targetSlot));
            return m_connections.back();
        }

        AZ_Error(GetSystemName(), false, "Tried to add a connection between slots that don't exist in this Graph.");
        return nullptr;
    }

    bool Graph::RemoveConnection(ConstConnectionPtr connection)
    {
        return AZStd::erase_if(m_connections, [&](const auto& existingConnection) {
            return existingConnection == connection ||
                (existingConnection && connection &&
                 existingConnection->GetSourceSlot() == connection->GetSourceSlot() &&
                 existingConnection->GetTargetSlot() == connection->GetTargetSlot());
        }) != 0;
    }


    AZStd::shared_ptr<Slot> Graph::FindSlot(const Endpoint& endpoint)
    {
        auto nodeIter = m_nodes.find(endpoint.first);
        return nodeIter != m_nodes.end() ? nodeIter->second->GetSlot(endpoint.second) : AZStd::shared_ptr<Slot>{};
    }


    void Graph::SetUiMetadata(const GraphModelIntegration::GraphCanvasMetadata& uiMetadata)
    {
        m_uiMetadata = uiMetadata;
    }


    const GraphModelIntegration::GraphCanvasMetadata& Graph::GetUiMetadata() const
    {
        return m_uiMetadata;
    }


    GraphModelIntegration::GraphCanvasMetadata& Graph::GetUiMetadata()
    {
        return m_uiMetadata;
    }

} // namespace GraphModel
