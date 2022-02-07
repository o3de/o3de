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

// Qt
#include <QPixmap>

// Graph Canvas
#include <GraphCanvas/Editor/GraphModelBus.h>
#include <GraphCanvas/Components/Slots/SlotBus.h>
#include <GraphCanvas/Components/SceneBus.h>

// Graph Model
#include <GraphModel/GraphModelBus.h>
#include <GraphModel/Model/Common.h>
#include <GraphModel/Model/Slot.h>

class QGraphicsLinearLayout;

namespace GraphModelIntegration
{
    class GraphCanvasMetadata;
    class ThumbnailItem;

    //! This is the main class for binding the node graph data to the UI provided by Graph Canvas
    class GraphController
        : private GraphCanvas::GraphModelRequestBus::Handler
        , private GraphCanvas::SceneNotificationBus::Handler
        , public GraphControllerRequestBus::Handler
    {
    public:
        AZ_RTTI(GraphController, "{E8433794-4BAE-4B63-B5A5-6EE69DFF0793}");

        GraphController(GraphModel::GraphPtr graph, AZ::EntityId graphCanvasSceneId);
        ~GraphController() override;

        GraphController(const GraphController&) = delete;

        GraphModel::GraphPtr GetGraph() { return m_graph; }
        const GraphModel::GraphPtr GetGraph() const { return m_graph; }
        const AZ::EntityId GetGraphCanvasSceneId() const { return m_graphCanvasSceneId; }

        ////////////////////////////////////////////////////////////////////////////////////
        // GraphModel::GraphControllerRequestBus, connections

        //! Adds a node to the Graph and creates the corresponding Graph Canvas UI elements
        //! \param node  The node to add. This should be a freshly created Node that hasn't been added to the Graph yet.
        //! \param sceneDropPosition  The position in the Graph Cavnas scene where the Node was dropped
        GraphCanvas::NodeId AddNode(GraphModel::NodePtr node, AZ::Vector2& sceneDropPosition) override;
        bool RemoveNode(GraphModel::NodePtr node) override;
        AZ::Vector2 GetPosition(GraphModel::NodePtr node) const override;

        void WrapNode(GraphModel::NodePtr wrapperNode, GraphModel::NodePtr node) override;
        void WrapNodeOrdered(GraphModel::NodePtr wrapperNode, GraphModel::NodePtr node, AZ::u32 layoutOrder) override;
        void UnwrapNode(GraphModel::NodePtr wrapperNode, GraphModel::NodePtr node) override;
        void SetWrapperNodeActionString(GraphModel::NodePtr node, const char* actionString) override;

        GraphModel::ConnectionPtr AddConnection(GraphModel::SlotPtr sourceSlot, GraphModel::SlotPtr targetSlot) override;
        GraphModel::ConnectionPtr AddConnectionBySlotId(GraphModel::NodePtr sourceNode, GraphModel::SlotId sourceSlotId, GraphModel::NodePtr targetNode, GraphModel::SlotId targetSlotId) override;
        bool RemoveConnection(GraphModel::ConnectionPtr connection) override;
        GraphModel::SlotId ExtendSlot(GraphModel::NodePtr node, GraphModel::SlotName slotName) override;

        GraphModel::NodePtr GetNodeById(const GraphCanvas::NodeId& nodeId) override;
        GraphModel::NodePtrList GetNodesFromGraphNodeIds(const AZStd::vector<GraphCanvas::NodeId>& nodeIds) override;
        GraphCanvas::NodeId GetNodeIdByNode(GraphModel::NodePtr node) const override;
        GraphCanvas::SlotId GetSlotIdBySlot(GraphModel::SlotPtr slot) const override;

        GraphModel::NodePtrList GetNodes() override;
        GraphModel::NodePtrList GetSelectedNodes() override;
        void SetSelected(GraphModel::NodePtrList nodes, bool selected) override;
        void ClearSelection() override;
        void EnableNode(GraphModel::NodePtr node) override;
        void DisableNode(GraphModel::NodePtr node) override;
        void CenterOnNodes(GraphModel::NodePtrList nodes) override;
        AZ::Vector2 GetMajorPitch() const override;

        void SetThumbnailImageOnNode(GraphModel::NodePtr node, const QPixmap& image) override;
        void SetThumbnailOnNode(GraphModel::NodePtr node, ThumbnailItem* item) override;
        void RemoveThumbnailFromNode(GraphModel::NodePtr node) override;

        ////////////////////////////////////////////////////////////////////////////////////

    private:
        //! Helper method for retrieving the UI layout for a given node
        QGraphicsLinearLayout* GetLayoutFromNode(GraphModel::NodePtr node);

        //! Saves metadata for a Graph Canvas element into the Graph data model so it's ready
        //! to be serialized out with the data model. graphCanvasElement could be any number
        //! of entities including a node, comment, group, or the scene itself.
        void SaveMetadata(const AZ::EntityId& graphCanvasElement);

        //! Utility function for getting the GraphCanvasMetadata from the Graph data model
        GraphCanvasMetadata* GetGraphMetadata();

        ////////////////////////////////////////////////////////////////////////////////////
        // Functions for building Graph Canvas UI from our data model

        //! Creates the all Graph Canvas elements necessary for representing the Graph. This will be called once
        //! to instrument a Graph that was recently loaded.
        void CreateFullGraphUi();

        //! Creates the GraphCanvas slot UI representing a given Slot
        AZ::Entity* CreateSlotUi(GraphModel::SlotPtr slot, AZ::EntityId nodeUiId);

        //! Creates the GraphCanvas node UI represeting a given Node
        //! \param scenePosition  Pass in a lambda function that provides the node's position given it's GraphCanvas node EntityId.
        AZ::EntityId CreateNodeUi(GraphModel::NodeId nodeId, GraphModel::NodePtr node, AZStd::function<AZ::Vector2(AZ::EntityId/*nodeUiId*/)> getScenePosition);

        //! Utility function for adding a Graph Canvas node to a Graph Canvas scene
        void AddNodeUiToScene(AZ::EntityId graphCanvasNodeId, const AZ::Vector2& scenePosition);

        //! Creates the GraphCanvas UI represeting a given Connection
        void CreateConnectionUi(GraphModel::ConnectionPtr connection);

        //! Create a new GraphModel::Connection using the given source and target slots. This will also remove any existing connections on the target slot.
        GraphModel::ConnectionPtr CreateConnection(GraphModel::SlotPtr sourceSlot, GraphModel::SlotPtr targetSlot);

        //! Check if creating a connection between the specified target and source node would
        //! cause a connection loopback.
        bool CheckForLoopback(GraphModel::NodePtr sourceNode, GraphModel::NodePtr targetNode) const;

        void WrapNodeUi(GraphModel::NodePtr wrapperNode, GraphModel::NodePtr node, AZ::u32 layoutOrder = GraphModel::DefaultWrappedNodeLayoutOrder);
        void WrapNodeInternal(GraphModel::NodePtr wrapperNode, GraphModel::NodePtr node, AZ::u32 layoutOrder = GraphModel::DefaultWrappedNodeLayoutOrder);

        ////////////////////////////////////////////////////////////////////////////////////
        // GraphCanvas::SceneNotificationBus, connections
        void OnNodeAdded(const AZ::EntityId& nodeUiId, bool isPaste) override;
        void OnNodeRemoved(const AZ::EntityId& nodeUiId) override;
        void PreOnNodeRemoved(const AZ::EntityId& nodeUiId) override;
        void OnConnectionRemoved(const AZ::EntityId& connectionUiId) override;
        void OnEntitiesSerialized(GraphCanvas::GraphSerialization& serializationTarget) override;
        void OnEntitiesDeserialized(const GraphCanvas::GraphSerialization& serializationSource) override;
        void OnEntitiesDeserializationComplete(const GraphCanvas::GraphSerialization& serializationSource) override;

        ////////////////////////////////////////////////////////////////////////////////////
        // GraphCanvas::GraphModelRequestBus, connections

        void DisconnectConnection([[maybe_unused]] const AZ::EntityId& connectionUiId) override {}
        bool CreateConnection(const AZ::EntityId& connectionUiId, const GraphCanvas::Endpoint& sourcePoint, const GraphCanvas::Endpoint& targetPoint) override;

        bool IsValidConnection(const GraphCanvas::Endpoint& sourcePoint, const GraphCanvas::Endpoint& targetPoint) const override;

        ////////////////////////////////////////////////////////////////////////////////////
        // GraphCanvas::GraphModelRequestBus, undo

        // CJS TODO: I put this stuff in to handle making the file as dirty, but it looks like I might be able to get OnSaveDataDirtied to do that instead.
        void RequestUndoPoint() override;
        void RequestPushPreventUndoStateUpdate() override;
        void RequestPopPreventUndoStateUpdate() override;
        void TriggerUndo() override {}
        void TriggerRedo() override {}


        ////////////////////////////////////////////////////////////////////////////////////
        // GraphCanvas::GraphModelRequestBus, other

        void EnableNodes(const AZStd::unordered_set<GraphCanvas::NodeId>& nodeIds) override;
        void DisableNodes(const AZStd::unordered_set<GraphCanvas::NodeId>& nodeIds) override;

        AZStd::string GetDataTypeString(const AZ::Uuid& typeId) override;

        //! This is where we find all of the graph metadata (like node positions, comments, etc) and store it in the node graph for serialization
        // CJS TODO: Use this instead of the above undo functions
        void OnSaveDataDirtied(const AZ::EntityId& savedElement) override;

        void OnRemoveUnusedNodes() override {}
        void OnRemoveUnusedElements() override {}

        void ResetSlotToDefaultValue(const GraphCanvas::Endpoint& endpoint) override;

        /// Extendable slot handlers
        void RemoveSlot(const GraphCanvas::Endpoint& endpoint) override;
        bool IsSlotRemovable(const GraphCanvas::Endpoint& endpoint) const override;
        GraphCanvas::SlotId RequestExtension(const GraphCanvas::NodeId& nodeId, const GraphCanvas::ExtenderId& extenderId, GraphModelRequests::ExtensionRequestReason) override;

        bool ShouldWrapperAcceptDrop(const GraphCanvas::NodeId& wrapperNode, const QMimeData* mimeData) const override;
        void AddWrapperDropTarget(const GraphCanvas::NodeId& wrapperNode) override;
        void RemoveWrapperDropTarget(const GraphCanvas::NodeId& wrapperNode) override;

        ////////////////////////////////////////////////////////////////////////////////////
        // GraphCanvas::GraphModelRequestBus, node properties

        //! Creates a GraphCanvas::NodePropertyDisplay and a data interface for editing input values
        GraphCanvas::NodePropertyDisplay* CreateDataSlotPropertyDisplay(const AZ::Uuid& dataType, const GraphCanvas::NodeId& nodeId, const GraphCanvas::SlotId& slotId) const override;
        GraphCanvas::NodePropertyDisplay* CreatePropertySlotPropertyDisplay(const AZ::Crc32& propertyId, const GraphCanvas::NodeId& nodeUiId, const GraphCanvas::SlotId& slotUiId) const override;

        //! Common implementation for CreateDataSlotPropertyDisplay and CreatePropertySlotPropertyDisplay
        GraphCanvas::NodePropertyDisplay* CreateSlotPropertyDisplay(GraphModel::SlotPtr inputSlot) const;

        ////////////////////////////////////////////////////////////////////////////////////

        
        //! This class maps the association between our data model's GraphElements and Graph Camvas's UI elements
        class GraphElementMap
        {
        public:
            //! Adds a 1:1 mapping between a Graph Canvas UI element and a GraphElement
            void Add(AZ::EntityId graphCanvasId, GraphModel::GraphElementPtr graphElement);

            //! Removes the Graph Canvas EntityId and its associated GraphElement from the map
            void Remove(AZ::EntityId graphCanvasId);

            //! Removes the GraphElement and its associated Graph Canvas EntityId from the map
            void Remove(GraphModel::ConstGraphElementPtr graphElement);

            //! Find the GraphElement that corresponds to the given Graph Canvas EntityId.
            //! Returns nullptr if the mapping doesn't exist.
            GraphModel::GraphElementPtr Find(AZ::EntityId graphCanvasId);
            GraphModel::ConstGraphElementPtr Find(AZ::EntityId graphCanvasId) const;

            //! Find the Graph Canvas EntityId that corresponds to the given GraphElement.
            //! Returns an invalid EntityId if the mapping doesn't exist.
            AZ::EntityId Find(GraphModel::ConstGraphElementPtr graphElement) const;

        private:
            // shared_ptr can't be a key in a map so we use a raw pointer. This is fine because the two maps
            // are kept in sync, and the other map has a shared_ptr.
            typedef AZStd::unordered_map<const GraphModel::GraphElement*, AZ::EntityId /*graphCanvasId*/> GraphElementToUiMap;
            GraphElementToUiMap m_graphElementToUi;

            typedef AZStd::unordered_map<AZ::EntityId /*graphCanvasId*/, GraphModel::GraphElementPtr> UiToGraphElementMap;
            UiToGraphElementMap m_uiToGraphElement;
        };


        //! This class provides a collection of GraphElementMaps for the various types of elements. We could
        //! put all the elements in one GraphElementMap, but splitting them out makes debugging a lot easier.
        class GraphElementMapCollection
        {
        public:
            GraphElementMapCollection() = default;

            //! Adds a 1:1 mapping between a Graph Canvas UI element and a GraphElement.
            //! Automatically determines which GraphElementMap is appropriate.
            void Add(AZ::EntityId graphCanvasId, GraphModel::GraphElementPtr graphElement);

            void Remove(AZ::EntityId graphCanvasId);
            void Remove(GraphModel::ConstGraphElementPtr graphElement);


            //! Find the GraphElement that corresponds to the given Graph Canvas EntityId.
            //! Returns nullptr if the mapping doesn't exist, or the ElementType is wrong.
            template<typename ElementType>
            AZStd::shared_ptr<ElementType> Find(AZ::EntityId graphCanvasId);
            template<typename ElementType>
            AZStd::shared_ptr<const ElementType> Find(AZ::EntityId graphCanvasId) const;

            //! Find the Graph Canvas EntityId that corresponds to the given GraphElement.
            //! Returns an invalid EntityId if the mapping doesn't exist.
            AZ::EntityId Find(GraphModel::ConstGraphElementPtr graphElement) const;

        private:

            GraphElementMap m_nodeMap;
            GraphElementMap m_slotMap;
            GraphElementMap m_connectionMap;

            //! Returns which is the right GraphElementMap for graphElement based on its type
            GraphElementMap* GetMapFor(GraphModel::ConstGraphElementPtr graphElement);
            const GraphElementMap* GetMapFor(GraphModel::ConstGraphElementPtr graphElement) const;

            // This list allows for easy iteration over all the GraphElementMaps.
            AZStd::vector<GraphElementMap*> m_allMaps = { &m_nodeMap, &m_slotMap, &m_connectionMap };
        } m_elementMap;


        AZ::SerializeContext* m_serializeContext = nullptr;
        GraphModel::GraphPtr m_graph;
        AZ::EntityId m_graphCanvasSceneId;

        AZStd::unordered_map<GraphModel::NodeId, GraphModelIntegration::ThumbnailItem*> m_nodeThumbnails;
        AZStd::unordered_map<GraphCanvas::NodeId, AZStd::unordered_map<GraphCanvas::ExtenderId, GraphModel::SlotName>> m_nodeExtenderIds;

        bool m_isCreatingConnectionUi = false;
    };
       

    template<typename ElementType>
    AZStd::shared_ptr<ElementType> GraphController::GraphElementMapCollection::Find(AZ::EntityId graphCanvasId)
    {
        GraphModel::GraphElementPtr graphElement;

        for (GraphElementMap* map : m_allMaps)
        {
            graphElement = map->Find(graphCanvasId);
            if (graphElement)
            {
                break;
            }
        }

        return azrtti_cast<ElementType*>(graphElement);
    }


    template<typename ElementType>
    AZStd::shared_ptr<const ElementType> GraphController::GraphElementMapCollection::Find(AZ::EntityId graphCanvasId) const
    {
        GraphModel::ConstGraphElementPtr graphElement;

        for (GraphElementMap* map : m_allMaps)
        {
            graphElement = map->Find(graphCanvasId);
            if (graphElement)
            {
                break;
            }
        }

        return azrtti_cast<const ElementType*>(graphElement);
    }
}
