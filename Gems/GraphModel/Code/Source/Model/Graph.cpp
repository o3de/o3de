/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

// AZ
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

// Graph Model
#include <GraphModel/Model/Graph.h>
#include <GraphModel/Model/IGraphContext.h>
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
                ->Version(1)
                ->Field("m_nodes", &Graph::m_nodes)
                ->Field("m_connections", &Graph::m_connections)
                ->Field("m_uiMetadata", &Graph::m_uiMetadata)
                ->Field("m_nodeWrappings", &Graph::m_nodeWrappings)
                ;
        }
    }

    Graph::Graph(IGraphContextPtr graphContext)
        : m_graphContext(graphContext)
    {

    }

    void Graph::PostLoadSetup(IGraphContextPtr graphContext)
    {
        AZ_Assert(m_nextNodeId == 1, "This graph has been set up before");

        m_graphContext = graphContext;

        for (auto& pair : m_nodes)
        {
            const NodeId nodeId = pair.first;
            pair.second->PostLoadSetup(shared_from_this(), nodeId);

            // Find the highest NodeId in the graph so we can figure out
            // what the next one should be
            m_nextNodeId = AZ::GetMax(m_nextNodeId, nodeId + 1);
        }

        for (auto it = m_connections.begin(); it != m_connections.end();)
        {
            ConnectionPtr connection = *it;
            connection->PostLoadSetup(shared_from_this());

            if (!connection->GetSourceSlot() || !connection->GetTargetSlot())
            {
                // Discard any cached connections if the source or target slot no longer exists
                m_connections.erase(it);
            }
            else
            {
                // Valid slots, so update each slot's local cache of its connections
                connection->GetSourceSlot()->m_connections.push_back(connection);
                connection->GetTargetSlot()->m_connections.push_back(connection);

                ++it;
            }
        }
    }

    NodeId Graph::PostLoadSetup(NodePtr node)
    {
        node->m_graph = shared_from_this();
        NodeId nodeId = AddNode(node);
        node->PostLoadSetup();

        return nodeId;
    }

    IGraphContextPtr Graph::GetContext() const
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
        if (!sourceSlot || !targetSlot)
        {
            return nullptr;
        }

        for (ConnectionPtr searchConnection : m_connections)
        {
            if (searchConnection->GetSourceSlot() == sourceSlot && searchConnection->GetTargetSlot() == targetSlot)
            {
                return searchConnection;
            }
        }

        return nullptr;
    }


    bool Graph::Contains(SlotPtr slot) const
    {
        if (!slot)
        {
            return false;
        }

        for (auto pair : m_nodes)
        {
            if (pair.second->Contains(slot))
            {
                return true;
            }
        }

        return false;
    }


    NodePtr Graph::GetNode(NodeId nodeId)
    {
        auto nodeIter = m_nodes.find(nodeId);
        if (nodeIter != m_nodes.end())
        {
            return nodeIter->second;
        }

        return nullptr;
    }


    const Graph::NodeMap& Graph::GetNodes()
    {
        return m_nodes;
    }

    Graph::ConstNodeMap Graph::GetNodes() const
    {
        Graph::ConstNodeMap constNodes;
        AZStd::for_each(m_nodes.begin(), m_nodes.end(), [&](auto pair) { constNodes.insert(pair); });
        return constNodes;
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
        // It looks like this code is never run because the connections are always 
        // deleted individually first. But still have this hear for completeness.
        for (int i = static_cast<int>(m_connections.size()) - 1; i >= 0; --i)
        {
            ConnectionPtr connection = m_connections[i];
            if (connection->GetSourceNode() == node || connection->GetTargetNode() == node)
            {
                RemoveConnection(&m_connections[i]);
            }
        }

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
        auto it = m_nodeWrappings.find(node->GetId());
        if (it != m_nodeWrappings.end())
        {
            m_nodeWrappings.erase(it);
        }
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
        else if (Contains(sourceSlot) && Contains(targetSlot))
        {
            ConnectionPtr newConnection = AZStd::make_shared<Connection>(shared_from_this(), sourceSlot, targetSlot);

            m_connections.push_back(newConnection);
            sourceSlot->m_connections.push_back(newConnection);
            targetSlot->m_connections.push_back(newConnection);

            return newConnection;
        }
        else
        {
            AZ_Error(GetSystemName(), false, "Tried to add a connection between slots that don't exist in this Graph.");
            return nullptr;
        }
    }

    bool Graph::RemoveConnection(ConnectionList::iterator iter)
    {
        if (iter != m_connections.end())
        {
            ConnectionPtr connection = *iter;

            // Remove the cached connection pointers from the slots
            auto shouldRemove = [&connection](auto entry) {
                ConstConnectionPtr entryPtr = entry.lock();
                return !entryPtr || entryPtr == connection;
            };
            (*iter)->GetSourceSlot()->m_connections.remove_if(shouldRemove);
            (*iter)->GetTargetSlot()->m_connections.remove_if(shouldRemove);

            // Remove the actual connection
            m_connections.erase(iter);

#if defined(AZ_ENABLE_TRACING)
            auto iter = AZStd::find(m_connections.begin(), m_connections.end(), connection);
            AZ_Assert(iter == m_connections.end(), "Graph is broken. The same connection object was found multiple times.");
#endif

            return true;
        }
        else
        {
            return false;
        }
    }

    bool Graph::RemoveConnection(ConstConnectionPtr connection)
    {
        auto iter = AZStd::find(m_connections.begin(), m_connections.end(), connection);
        return RemoveConnection(iter);
    }


    AZStd::shared_ptr<Slot> Graph::FindSlot(const Endpoint& endpoint)
    {
        AZStd::shared_ptr<Slot> slot;

        auto nodeIter = m_nodes.find(endpoint.first);
        if (nodeIter != m_nodes.end())
        {
            slot = nodeIter->second->GetSlot(endpoint.second);
        }

        return slot;
    }


    void Graph::SetUiMetadata(const AZStd::any& uiMetadata)
    {
        m_uiMetadata = uiMetadata;
    }


    const AZStd::any& Graph::GetUiMetadata() const
    {
        return m_uiMetadata;
    }


    AZStd::any& Graph::GetUiMetadata()
    {
        return m_uiMetadata;
    }

} // namespace GraphModel
