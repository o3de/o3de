/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

// AZ
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/enable_shared_from_this.h>

// Graph Model
#include <GraphModel/Integration/GraphCanvasMetadata.h>
#include <GraphModel/Model/Common.h>
#include <GraphModel/Model/GraphElement.h>

namespace AZ
{
    class ReflectContext;
}

namespace GraphModel
{
    //! This is the main class for the GraphModel framework. It provides a generic node graph data model
    //! that is originally intended for use with Graph Canvas providing the UI, but in theory it could
    //! be used with any node graph widget system. It is also designed with primarily editor processing 
    //! in mind, rather than runtime processing, so if this were to be used at runtime we may need some 
    //! improvements.
    //! 
    //! Data Model Goals
    //! * Keep it simple
    //!   (For example, avoid using the component entity system even though that's an established pattern 
    //!    in Script Canvas's data model)
    //! * It shouldn't know anything about GraphCanvas or any other UI representation; it's purely a data model
    //! * Make it suitible for multiple contexts
    //!  (For example, even though this is built for Shader Canvas initially, it should be generic enough 
    //!   to use for the Particle Editor or other contexts)
    //! * It shouldn't know anything about how the nodes will be used. 
    //!   (For example, any functionality specific to Shader Canvas, Particle Editors, or any other context 
    //!    should be added to some external subclasses of GraphModel classes)
    //!
    //! Key elements of a Graph include
    //! * Node       - The main building block of a Graph. Contains multiple input slots and output slots.
    //! * Slot       - Every node contains input slots and/or output slots that can be connected together
    //! * Endpoint   - A specific Slot on a specific Node; basically a {NodeID,SlotID} pair
    //! * Connection - A link from an output Endpoint to an input Endpoint (we could also say this is a link 
    //!                from an output Slot to an input Slot)
    //! * Metadata   - Every graph can contain generic metadata like comments and node groupings for example. 
    //!                But this is specific to the node graph UI system, and the Graph class just stores this 
    //!                in an abstract way to bundle the data together.
    //!
    //! For continued reading, see Node.h next.
    class Graph : public AZStd::enable_shared_from_this<Graph>
    {
    public:
        AZ_CLASS_ALLOCATOR(Graph, AZ::SystemAllocator, 0);
        AZ_RTTI(Graph, "{CBF5DC3C-A0A7-45F5-A207-06433A9A10C5}");
        static void Reflect(AZ::ReflectContext* context);

        typedef AZStd::unordered_map<NodeId, NodePtr> NodeMap;
        typedef AZStd::unordered_map<NodeId, ConstNodePtr> ConstNodeMap;

        // Used to store the mappings for our wrapped nodes, where the key is the NodeId of
        // the wrapped node, and the value is a pair of the NodeId for the parent WrapperNode
        // and the layout order for the wrapped node
        typedef AZStd::unordered_map<NodeId, AZStd::pair<NodeId, AZ::u32>> NodeWrappingMap;

        // We use a vector instead of set to maintain a consistent order in the serialized data, to reduce diffs
        typedef AZStd::vector<ConnectionPtr> ConnectionList;

        Graph() = default; // Needed by SerializeContext
        Graph(const Graph&) = delete;

        //! Constructor
        //! \param  graphContext  interface to client system specific data and functionality
        explicit Graph(GraphContextPtr graphContext);

        virtual ~Graph() = default;

        //! Initializion after the Graph has been serialized in.
        //! This must be called after building a Graph from serialized data
        //! in order to connect internal pointers between elements of the Graph
        //! and perform any other precedural setup that isn't stored in the 
        //! serialized data.
        //! \param  graphContext  interface to client system specific data and functionality
        void PostLoadSetup(GraphContextPtr graphContext);

        //! Add a node that has been deserialized to the graph
        //! This should only be necessary for cases like copy/paste where we
        //! need to load a deserialized node, but don't actually know the nodeId before-hand
        NodeId PostLoadSetup(NodePtr node);

        //! Returns the interface to client system specific data and functionality
        GraphContextPtr GetContext() const;

        //! This name is used for debug messages in GraphModel classes, to provide appropriate context for the user.
        //! It's a convenience function for GetContext()->GetSystemName()
        const char* GetSystemName() const;
        
        //! Adds a Node to the graph and gives it a unique ID
        NodeId AddNode(NodePtr node);

        //! Removes a Node and all connections between it and other Nodes in the graph
        bool RemoveNode(ConstNodePtr node);

        //! Wrap (embed) the node onto the specified wrapperNode
        //! The wrapperNode and node must already exist in the graph before being wrapped
        void WrapNode(NodePtr wrapperNode, NodePtr node, AZ::u32 layoutOrder = DefaultWrappedNodeLayoutOrder);

        //! Remove the wrapping from the specified node
        void UnwrapNode(ConstNodePtr node);

        //! Return if the specified node is a wrapped node
        bool IsNodeWrapped(NodePtr node) const;

        //! Return our full map of node wrappings
        const NodeWrappingMap& GetNodeWrappings();

        NodePtr GetNode(NodeId nodeId);
        const NodeMap& GetNodes();
        ConstNodeMap GetNodes() const;

        //! Adds a new connection between sourceSlot and targetSlot and returns the
        //! new Connection, or returns the existing Connection if one already exists.
        ConnectionPtr AddConnection(SlotPtr sourceSlot, SlotPtr targetSlot);

        //! Removes a connection from the Graph, and returns whether it was found and removed
        bool RemoveConnection(ConstConnectionPtr connection);

        const ConnectionList& GetConnections();

        //! Set/gets a bundle of generic metadata that is provided by the node graph UI
        //! system. This may include node positions, comment blocks, node groupings, and 
        //! bookmarks, for example.
        void SetUiMetadata(const GraphModelIntegration::GraphCanvasMetadata& uiMetadata);
        const GraphModelIntegration::GraphCanvasMetadata& GetUiMetadata() const;
        GraphModelIntegration::GraphCanvasMetadata& GetUiMetadata();

        AZStd::shared_ptr<Slot> FindSlot(const Endpoint& endpoint);

    protected:
        bool Contains(SlotPtr slot) const;
        ConnectionPtr FindConnection(ConstSlotPtr sourceSlot, ConstSlotPtr targetSlot);

    private:
        NodeMap m_nodes;

        int m_nextNodeId = 1; //!< NodeIds are unique within each Graph. This is a simple counter for generating the next ID.

        ConnectionList m_connections;

        //! Used to store and serialize metadata from the graph UI, like node positions, comments, group boxes, etc.
        GraphModelIntegration::GraphCanvasMetadata m_uiMetadata;

        //! Used to store all of our node <-> wrapper node mappings
        NodeWrappingMap m_nodeWrappings;

        GraphContextPtr m_graphContext; //!< interface to client system specific data and functionality
    };

} // namespace GraphModel
