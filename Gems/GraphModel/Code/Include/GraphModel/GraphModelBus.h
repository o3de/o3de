/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// AZ
#include <AzCore/Component/Entity.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/Math/Vector2.h>

// Graph Canvas
#include <GraphCanvas/Editor/EditorTypes.h>

// Graph Model
#include <GraphModel/Model/Common.h>
#include <GraphModel/Model/Slot.h>

class QPixmap;
class QPoint;
class QPointF;
class QRect;

namespace GraphModelIntegration
{
    class ThumbnailItem;

    struct GraphModelSerialization
    {
        AZ_TYPE_INFO(GraphModelSerialization, "{0D4D420B-5D9E-429C-A567-DF8596439F5F}");

        using SerializedSlotMapping = AZStd::unordered_map<GraphModel::SlotId, GraphCanvas::SlotId>;
        using SerializedNodeBuffer = AZStd::vector<AZ::u8>;

        //! Keep track of any nodes and their slots that have been serialized
        AZStd::unordered_map<GraphCanvas::NodeId, SerializedNodeBuffer> m_serializedNodes;
        AZStd::unordered_map<GraphCanvas::NodeId, SerializedSlotMapping> m_serializedSlotMappings;

        //! Mapping of serialized nodeIds to their wrapper (parent) nodeId and layout order so they can be restored after deserialization
        using SerializedNodeWrappingMap = AZStd::unordered_map<GraphCanvas::NodeId, AZStd::pair<GraphCanvas::NodeId, AZ::u32>>;
        SerializedNodeWrappingMap m_serializedNodeWrappings;
    };

    //! GraphManagerRequests
    //! Create/delete for handling our Graph Controllers
    class GraphManagerRequests : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////

        //! Create a scene and a corresponding graph controller
        virtual AZ::Entity* CreateScene(GraphModel::GraphPtr graph, const GraphCanvas::EditorId editorId) = 0;

        //! Remove the graph controller for the scene
        virtual void RemoveScene(const GraphCanvas::GraphId& sceneId) = 0;

        //! Create a new Graph Controller for the given scene
        virtual void CreateGraphController(const GraphCanvas::GraphId& sceneId, GraphModel::GraphPtr graph) = 0;

        //! Delete the Graph Controller for the given scene
        virtual void DeleteGraphController(const GraphCanvas::GraphId& sceneId) = 0;

        //! Retrieve a reference to the Graph object for the specified Graph Controller (if it exists)
        virtual GraphModel::GraphPtr GetGraph(const GraphCanvas::GraphId& sceneId) = 0;

        //! Get/set our serialized mappings of the GraphCanvas nodes/slots that correspond to
        //! GraphModel nodes/slots
        virtual const GraphModelSerialization& GetSerializedMappings() = 0;
        virtual void SetSerializedMappings(const GraphModelSerialization& serialization) = 0;
    };

    using GraphManagerRequestBus = AZ::EBus<GraphManagerRequests>;

    //! GraphControllerRequests
    //! Used to invoke functionality on specific Graph Controllers
    class GraphControllerRequests : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;
        //////////////////////////////////////////////////////////////////////////

        //! Add a new node at the specified position
        virtual GraphCanvas::NodeId AddNode(GraphModel::NodePtr node, AZ::Vector2& sceneDropPosition) = 0;

        //! Remove the specified node
        virtual bool RemoveNode(GraphModel::NodePtr node) = 0;

        //! Retrieve the position for the specified node
        virtual AZ::Vector2 GetPosition(GraphModel::NodePtr node) const = 0;

        //! Embed a node on a wrapper node
        virtual void WrapNode(GraphModel::NodePtr wrapperNode, GraphModel::NodePtr node) = 0;

        //! Embed a node on a wrapper node with a layout order configured
        virtual void WrapNodeOrdered(GraphModel::NodePtr wrapperNode, GraphModel::NodePtr node, AZ::u32 layoutOrder) = 0;

        //! Unwrap a node from a wrapper node
        //! This results in a no-op if node isn't actually wrapped on the wrapperNode
        virtual void UnwrapNode(GraphModel::NodePtr wrapperNode, GraphModel::NodePtr node) = 0;

        //! Return if the specified node is a wrapped node
        virtual bool IsNodeWrapped(GraphModel::NodePtr node) const = 0;

        //! Set the action string for the specified node (used by wrapper nodes for
        //! setting the action widget label)
        virtual void SetWrapperNodeActionString(GraphModel::NodePtr node, const char* actionString) = 0;

        //! Add a new connection between the specified source and target
        virtual GraphModel::ConnectionPtr AddConnection(GraphModel::SlotPtr sourceSlot, GraphModel::SlotPtr targetSlot) = 0;

        //! Create a new connection between the specified source and target specified slots
        virtual GraphModel::ConnectionPtr AddConnectionBySlotId(
            GraphModel::NodePtr sourceNode,
            GraphModel::SlotId sourceSlotId,
            GraphModel::NodePtr targetNode,
            GraphModel::SlotId targetSlotId) = 0;

        //! Check if there is a connection between the specified source and target specified slots
        virtual bool AreSlotsConnected(
            GraphModel::NodePtr sourceNode,
            GraphModel::SlotId sourceSlotId,
            GraphModel::NodePtr targetNode,
            GraphModel::SlotId targetSlotId) const = 0;

        //! Remove the specified connection
        virtual bool RemoveConnection(GraphModel::ConnectionPtr connection) = 0;

        //! Extend the given Slot on the specified node
        virtual GraphModel::SlotId ExtendSlot(GraphModel::NodePtr node, GraphModel::SlotName slotName) = 0;

        //! Returns a GraphModel::Node that corresponds to the Graph Canvas Node Id
        virtual GraphModel::NodePtr GetNodeById(const GraphCanvas::NodeId& nodeId) = 0;

        //! Retrieve the list of GraphModel::Nodes for the specified GraphCanvas node IDs
        virtual GraphModel::NodePtrList GetNodesFromGraphNodeIds(const AZStd::vector<GraphCanvas::NodeId>& nodeIds) = 0;

        //! Returns the GraphCanvas::NodeId that corresponds to the specified GraphModel::Node
        virtual GraphCanvas::NodeId GetNodeIdByNode(GraphModel::NodePtr node) const = 0;

        //! Returns the GraphCanvas::SlotId that corresponds to the specified GraphModel::Slot
        virtual GraphCanvas::SlotId GetSlotIdBySlot(GraphModel::SlotPtr slot) const = 0;

        //! Retrieve all of the nodes in our graph
        virtual GraphModel::NodePtrList GetNodes() = 0;

        //! Retrieve the selected nodes in our graph
        virtual GraphModel::NodePtrList GetSelectedNodes() = 0;

        //! Set the selected property on the specified Nodes
        virtual void SetSelected(GraphModel::NodePtrList nodes, bool selected) = 0;

        //! Clears the selection in the scene
        virtual void ClearSelection() = 0;

        //! Enable the specified node in the graph
        virtual void EnableNode(GraphModel::NodePtr node) = 0;

        //! Disable the specified node in the graph
        virtual void DisableNode(GraphModel::NodePtr node) = 0;

        //! Move the view to be centered on the given Nodes
        virtual void CenterOnNodes(GraphModel::NodePtrList nodes) = 0;

        //! Retrieve the major pitch of the grid for this scene graph
        virtual AZ::Vector2 GetMajorPitch() const = 0;

        //! Embed a thumbnail image on a specified node. This is the most straightforward use-case
        //! where the client just wants to show a static image. The thumnbnail image can be updated
        //! after being set using this same API.
        //! \param node Node to add the thumbnail on
        //! \param image Pixmap for the image of the thumbnail
        virtual void SetThumbnailImageOnNode(GraphModel::NodePtr node, const QPixmap& image) = 0;

        //! Embed a custom thumbnail item on a specified node. This allows the client to
        //! implement their own ThumbnailItem to display anything they want by overriding
        //! paint() method. Ownership of the ThumbnailItem is passed to the node layout.
        //! \param node Node to add the thumbnail on
        //! \param item Custom item for the thumbnail
        virtual void SetThumbnailOnNode(GraphModel::NodePtr node, ThumbnailItem* item) = 0;

        //! Remove the thumbnail from a specified node. If you created your own custom ThumbnailItem
        //! and set it using SetThumbnailOnNode, then ownership is passed back to whoever calls this
        //! method so they are in charge of deleting it.
        //! \param node Node to remove the thumbnail from
        virtual void RemoveThumbnailFromNode(GraphModel::NodePtr node) = 0;
    };

    using GraphControllerRequestBus = AZ::EBus<GraphControllerRequests>;

    //! GraphControllerNotifications
    //! Notifications about changes to the state of scene graphs.
    class GraphControllerNotifications : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;

        //! A node has been added to the scene.
        virtual void OnGraphModelNodeAdded(GraphModel::NodePtr /*node*/){};

        //! A node has been removed from the scene.
        virtual void OnGraphModelNodeRemoved(GraphModel::NodePtr /*node*/){};

        //! Invoked prior to a node being removed from the scene.
        virtual void PreOnGraphModelNodeRemoved(GraphModel::NodePtr /*node*/){};

        //! A connection has been added to the scene.
        virtual void OnGraphModelConnectionAdded(GraphModel::ConnectionPtr /*connection*/){};

        //! A connection has been removed from the scene.
        virtual void OnGraphModelConnectionRemoved(GraphModel::ConnectionPtr /*connection*/){};

        //! The specified node is about to be wrapped (embedded) onto the wrapperNode
        virtual void PreOnGraphModelNodeWrapped([[maybe_unused]] GraphModel::NodePtr wrapperNode, [[maybe_unused]] GraphModel::NodePtr node) {};

        //! The specified node has been wrapped (embedded) onto the wrapperNode
        virtual void OnGraphModelNodeWrapped(GraphModel::NodePtr /*wrapperNode*/, GraphModel::NodePtr /*node*/){};

        //! The specified node has been unwrapped (removed) from the wrapperNode
        virtual void OnGraphModelNodeUnwrapped(GraphModel::NodePtr /*wrapperNode*/, GraphModel::NodePtr /*node*/){};

        //! Sent whenever a graph model slot value changes
        //! \param slot The slot that was modified in the graph.
        virtual void OnGraphModelSlotModified(GraphModel::SlotPtr slot){};

        //! Something in the graph has been modified
        //! \param node The node that was modified in the graph.  If this is nullptr, some metadata on the graph itself was modified
        virtual void OnGraphModelGraphModified(GraphModel::NodePtr node){};

        //! A request has been made to record data for an undoable operation 
        virtual void OnGraphModelRequestUndoPoint(){};

        //! A request has been made to perform an undo operation
        virtual void OnGraphModelTriggerUndo(){};

        //! A request has been made to perform a redo operation
        virtual void OnGraphModelTriggerRedo(){};
    };

    using GraphControllerNotificationBus = AZ::EBus<GraphControllerNotifications>;
} // namespace GraphModelIntegration
