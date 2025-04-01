/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <qglobal.h>

#include <qrect.h>
#include <qpoint.h>
#include <qcolor.h>

#include <AzCore/EBus/EBus.h>
#include <AzCore/Math/Vector2.h>

#include <GraphCanvas/Components/ViewBus.h>
#include <GraphCanvas/GraphicsItems/AnimatedPulse.h>
#include <GraphCanvas/GraphicsItems/GlowOutlineGraphicsItem.h>
#include <GraphCanvas/GraphicsItems/ParticleGraphicsItem.h>
#include <GraphCanvas/GraphicsItems/Occluder.h>
#include <GraphCanvas/Types/GraphCanvasGraphData.h>
#include <GraphCanvas/Types/GraphCanvasGraphSerialization.h>

#include <GraphCanvas/Utils/GraphUtils.h>

QT_FORWARD_DECLARE_CLASS(QKeyEvent);
QT_FORWARD_DECLARE_CLASS(QGraphicsScene);
QT_FORWARD_DECLARE_CLASS(QGraphicsSceneContextMenuEvent);
QT_FORWARD_DECLARE_CLASS(QGraphicsSceneMouseEvent);
QT_FORWARD_DECLARE_CLASS(QMimeData);

namespace GraphCanvas
{
    enum DragSelectionType
    {
        // Items will be selected as they are dragged over.
        Realtime,

        // Items will be selected once a drag selection is complete.
        OnRelease
    };

    //! SceneRequests
    //! Requests that modify a scene.
    class SceneRequests
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;

        virtual void SetEditorId(const EditorId&) = 0;
        virtual EditorId GetEditorId() const = 0;

        //! Get the grid entity (for setting grid pitch)
        virtual AZ::EntityId GetGrid() const = 0;

        virtual GraphicsEffectId CreatePulse(const AnimatedPulseConfiguration& pulseConfiguration) = 0;
        virtual GraphicsEffectId CreatePulseAroundArea(const QRectF& area, int gridSteps, AnimatedPulseConfiguration& pulseConfiguration) = 0;
        virtual GraphicsEffectId CreatePulseAroundSceneMember(const AZ::EntityId& memberId, int gridSteps, AnimatedPulseConfiguration pulseConfiguration) = 0;
        virtual GraphicsEffectId CreateCircularPulse(const AZ::Vector2& centerPoint, float initialRadius, float finalRadius, AnimatedPulseConfiguration pulseConfiguration) = 0;

        virtual GraphicsEffectId CreateOccluder(const OccluderConfiguration& occluderConfiguration) = 0;

        virtual GraphicsEffectId CreateGlow(const FixedGlowOutlineConfiguration& configuration) = 0;
        virtual GraphicsEffectId CreateGlowOnSceneMember(const SceneMemberGlowOutlineConfiguration& configuration) = 0;

        virtual GraphicsEffectId CreateParticle(const ParticleConfiguration& configuration) = 0;
        virtual AZStd::vector< GraphicsEffectId > ExplodeSceneMember(const AZ::EntityId& memberId, float fillPercent) = 0;

        virtual void CancelGraphicsEffect(const GraphicsEffectId& effectId) = 0;

        //! Add a node to the scene.
        //! Nodes are owned by the scene and will follow the scene's entity life-cycle and be destroyed along with it.
        //! To avoid this, remove nodes before destroying the scene.
        //!
        //! Additionally, the node should not already be in another scene.
        //!
        //! # Parameters
        //! 1. The entity ID of the node to add.
        //! 2. A 2D vector indicating the position in scene space the node should initially have.
        virtual bool AddNode(const AZ::EntityId&, const AZ::Vector2&, bool isPaste) = 0;

        //! Add a list of nodes to the scene.
        //! Nodes are owned by the scene and will follow the scene's entity life-cycle and be destroyed along with it.
        //! To avoid this, remove nodes before destroying the scene.
        //!
        //! Additionally, the node should not already be in another scene.
        //!
        //! # Parameters
        //! 1. The entity ID of the nodes to add.
        virtual void AddNodes(const AZStd::vector<AZ::EntityId>&) = 0;

        //! Remove a node from the scene.
        virtual bool RemoveNode(const AZ::EntityId&) = 0;

        //! Get the entity IDs of the nodes known to the scene.
        virtual AZStd::vector<AZ::EntityId> GetNodes() const = 0;

        //! Get the entity IDs of all selected nodes known to the scene.
        virtual AZStd::vector<AZ::EntityId> GetSelectedNodes() const = 0;

        //! Will remove a node from the graph, and attempt to stitch together as many
        //! connections were severed as is possible. Any ambiguous connections will be thrown out.
        virtual void DeleteNodeAndStitchConnections(const AZ::EntityId& node) = 0;

        //! Create a default connection (between two endpoints).
        //! The connection will link the specified endpoints and have a default visual. It will be styled.
        //! 
        //! # Parameters
        //! 1. The source endpoint.
        //! 2. The target endpoint.
        virtual AZ::EntityId CreateConnectionBetween(const Endpoint& sourceEndpoint, const Endpoint& targetEndpoint) = 0;

        //! Add a connection to the scene.
        //! The connection must be connected to two slots and both slots must be in the same scene.
        virtual bool AddConnection(const AZ::EntityId&) = 0;

        //! Add a list of connection to the scene.
        //! The connection must be connected to two slots and both slots must be in the same scene.
        virtual void AddConnections(const AZStd::vector<AZ::EntityId>&) = 0;

        //! Remove a connection from the scene.
        virtual bool RemoveConnection(const AZ::EntityId&) = 0;

        //! Get the entity IDs of the connections known to the scene.
        virtual AZStd::vector<AZ::EntityId> GetConnections() const = 0;

        //! Get the entity IDs of the selected connections known to the scene.
        virtual AZStd::vector<AZ::EntityId> GetSelectedConnections() const = 0;

        //! Returns whether or not the specified endpoint is connected to anything
        virtual bool IsEndpointConnected(const Endpoint& endpoint) const = 0;

        //! Get the entity IDs of the connections where one endpoint of the connection is the supplied endpoint
        virtual AZStd::vector<AZ::EntityId> GetConnectionsForEndpoint(const Endpoint& endpoint) const = 0;

        //! Get the IDs of the endpoint forming the other ends of all the connections this endpoint is apart of.
        virtual AZStd::vector<Endpoint> GetConnectedEndpoints(const Endpoint& endpoint) const = 0;

        //! Creates a connection between two endpoints. Will also create a connection with the underlying model.
        virtual bool CreateConnection(const Endpoint& sourceEndpoint, const Endpoint& targetEndpoint) = 0;

        //! Display a connection visually on the graph. Will not interact with the underlying model.
        virtual bool DisplayConnection(const Endpoint& sourceEndpoint, const Endpoint& targetEndpoint) = 0;

        //! Destroys a connection using both endpoints to look up the connection
        virtual bool Disconnect(const Endpoint& sourceEndpoint, const Endpoint& targetEndpoint) = 0;

        //! Destroys a connection using the supplied connection Id
        virtual bool DisconnectById(const AZ::EntityId& connectionId) = 0;

        //! Finds a connection using the specified endpoints
        //! A reference to the found connection is returned in the connectionEntity parameter
        virtual bool FindConnection(AZ::Entity*& connectionEntity, const Endpoint& firstEndpoint, const Endpoint& otherEndpoint) const = 0;
        
        //! Adds a Bookmark Anchor
        virtual bool AddBookmarkAnchor(const AZ::EntityId& bookmarkAnchorId, const AZ::Vector2& position) = 0;

        //! Removes the specified Bookmark Anchor
        virtual bool RemoveBookmarkAnchor(const AZ::EntityId& bookmarkAnchorId) = 0;

        //! Add an entity of any valid type to the scene.
        virtual bool Add(const AZ::EntityId&, bool) = 0;

        //! Remove an entity of any valid type from the scene.
        virtual bool Remove(const AZ::EntityId&) = 0;

        //! Shows a hidden entity in the scene
        virtual bool Show(const AZ::EntityId& graphMemeber) = 0;

        //! Hides the specified graph member from the scene.
        virtual bool Hide(const AZ::EntityId& graphMember) = 0;

        //! Returns whether or not the specified member is hidden or not
        virtual bool IsHidden(const AZ::EntityId& graphMember) const = 0;

        //! Enables the specified graph member in the graph
        virtual bool Enable(const NodeId& nodeId) = 0;

        //! Enables the specified graph members visually
        virtual void EnableVisualState(const NodeId& nodeId) = 0;

        //! Enables the selected elements in the graph.
        virtual void EnableSelection() = 0;

        //! Disables the specified graph member in the graph
        virtual bool Disable(const NodeId& nodeId) = 0;

        //! Disables the specified graph members visually
        virtual void DisableVisualState(const NodeId& nodeId) = 0;

        //! Disables the selected elements in the graph
        virtual void DisableSelection() = 0;

        //! Used during start-up to get the scene to process the queued enable/disable
        virtual void ProcessEnableDisableQueue() = 0;

        //! Clears the selection in the scene.
        virtual void ClearSelection() = 0;

        //! Set the selected area in the scene (within the rectangle between two points).
        //! Items within this area are selected immediately and can be retrieved with
        //! \ref GetSelectedItems
        virtual void SetSelectedArea(const AZ::Vector2&, const AZ::Vector2&) = 0;

        //! Selects all items in the scene
        virtual void SelectAll() = 0;

        //! Selects all the items connected to the specified node
        virtual void SelectConnectedNodes() = 0;

        //! Selects node by following the specified input direciton
        virtual void SelectAllRelative(ConnectionType relativeDirection) = 0;

        //! Whether or not there are selected items in the scene.
        virtual bool HasSelectedItems() const = 0;

        //! Whether or not there are multiple selected items in the scene.
        virtual bool HasMultipleSelection() const = 0;

        //! Returns whether or not there are items selected that should be copied.
        virtual bool HasCopiableSelection() const = 0;

        //! Returns whether or not there are entities at the specified point.
        virtual bool HasEntitiesAt(const AZ::Vector2&) const = 0;

        //! Get the selected items in the scene.
        virtual AZStd::vector<AZ::EntityId> GetSelectedItems() const = 0;

        //! Get the entities known to the scene at a given position in scene space.
        virtual AZStd::vector<AZ::EntityId> GetEntitiesAt(const AZ::Vector2&) const = 0;

        //! Get the entities known to the scene in the given rectangle.
        virtual AZStd::vector<AZ::EntityId> GetEntitiesInRect(const QRectF&, Qt::ItemSelectionMode) const = 0;

        //! Get the endpoints known to the scene in the given rectangle.
        virtual AZStd::vector<Endpoint> GetEndpointsInRect(const QRectF& rect) const = 0;

        //! Obtain the scene as a QGraphicsScene
        virtual QGraphicsScene* AsQGraphicsScene() = 0;

        //! Copies the selected nodes, connections and groups to the clipboard
        virtual void CopySelection() const = 0;

        //! Copies the specified entities to the clipboard
        virtual void Copy(const AZStd::vector<AZ::EntityId>&) const = 0;

        //! Serializes the specified entities to the given SceneSerializationHelper
        virtual void SerializeEntities(const AZStd::unordered_set<AZ::EntityId>& itemIds, GraphSerialization& serializationTarget) const = 0;

        //! Cuts the selected nodes, connections and groups to the clipboard
        virtual void CutSelection() = 0;

        //! Cuts the specified entities to the clipboard
        virtual void Cut(const AZStd::vector<AZ::EntityId>&) = 0;

        //! Paste nodes, connections and groups within the GraphCanvas clipboard to the scene
        virtual void Paste() = 0;

        //! Paste nodes, connections and groups within the GraphCanvas clipboard to the scene
        //! \param scenePos scene position where paste operation is to take place
        virtual void PasteAt(const QPointF& scenePos) = 0;

        //! Paste scene serialization at the given position.
        //! \param scenePos the position at which to deserialize the serialization.
        //! \param serializationSource is the data source from which data will be grabbed.
        virtual void DeserializeEntities(const QPointF& scenePos, const GraphSerialization& serializationSource) = 0;

        //! Duplicate the nodes, connections and groups currently selected to the scene
        virtual void DuplicateSelection() = 0;

        //! Duplicate the node, connections and group currently selected to the position given
        //! \param scenePos the position to duplicate the selections to
        virtual void DuplicateSelectionAt(const QPointF& scenePos) = 0;

        //! Duplicate the nodes, connections and groups specified by the input
        //! \param itemIds the id of the entity to be duplicated
        virtual void Duplicate(const AZStd::vector<AZ::EntityId>& itemIds) = 0;

        //! Duplicate the nodes, connections and groups to the position specified by the input
        //! \param itemIds the id of the entity to be duplicated
        //! \param scenePos the position to duplicate the selections to
        virtual void DuplicateAt(const AZStd::vector<AZ::EntityId>& itemIds, const QPointF& scenePos) = 0;

        //! Deletes the current selection from the scene.
        virtual void DeleteSelection() = 0;

        //! Delete nodes from supplied set that exist within the scene
        //! \param itemIds Set of ids to delete
        virtual void Delete(const AZStd::unordered_set<AZ::EntityId>&) = 0;

        virtual void ClearScene() = 0;

        //! Stops the scene from allowing the next context menu's from being created.
        //! \param suppressed Whether or not context menu's should be allowed.
        virtual void SuppressNextContextMenu() = 0;

        //! Get the string that the scene use to set the mime type of the clipboard object
        virtual const AZStd::string& GetCopyMimeType() const = 0;

        //! Set the mime type for the scene. Allows the generic event system to distinguish between
        //! different instance of GraphCanvas.
        virtual void SetMimeType(const char* mimeType) = 0;

        virtual void RegisterView(const AZ::EntityId& viewId) = 0;
        virtual void RemoveView(const AZ::EntityId& viewId) = 0;

        //! Retrieves the ViewId that this scene is registered with
        //! Returns registered ViewId if the Scene is registered with a view otherwise an invalid ViewId is returned
        virtual ViewId GetViewId() const = 0;

        //! Dispatches a Mime Drop Event to the this Scene
        virtual void DispatchSceneDropEvent(const AZ::Vector2& scenePos, const QMimeData* mimeData) = 0;

        //! Retrieves the user data associated with the SceneData structure stored in the Scene component
        //! If the user data is serializable then it will get serialized when the SceneData structure is serialized
        //! A pointer is returned instead of a reference, as EBuses cannot references. If this bus is listening the return value is never null
        virtual AZStd::any* GetUserData() = 0;
        virtual const AZStd::any* GetUserDataConst() const = 0;

        //! Retrieves the entity that the SceneRequests is addressed on
        virtual AZ::Entity* GetSceneEntity() const = 0;

        //! Returns a reference to the SceneData on the Scene, 
        //! The SceneRequests handler must be retrieved to invoke this method
        virtual GraphData* GetGraphData() = 0;
        virtual const GraphData* GetGraphDataConst() const = 0;

        //! Uses the supplied scene data to add nodes, connections, to the scene.
        //! \param sceneData structure containing data to add to the scene(nodes, connections, etc...)
        virtual bool AddGraphData(const GraphData&) = 0;

        //! Removes matching nodes, connections from the scene
        //! Note: User data is not modified
        //! \param sceneData structure containing data to remove from the scene
        virtual void RemoveGraphData(const GraphData&) = 0;

        //! Deletes matching nodes, connections from the scene
        //! \param sceneData structure containing data to delete from the scene
        virtual void DeleteGraphData(const GraphData&) = 0;

        //! Controls how drag selection is handled.
        //! Default value is OnRelease.
        virtual void SetDragSelectionType(DragSelectionType dragSelectionType) = 0;

        virtual void SignalDragSelectStart() = 0;
        virtual void SignalDragSelectEnd() = 0;

        virtual bool IsDragSelecting() const = 0;

        virtual void SignalConnectionDragBegin() = 0;
        virtual void SignalConnectionDragEnd() = 0;

        virtual bool IsDraggingConnection() const = 0;

        virtual void SignalDesplice() = 0;

        virtual QRectF GetSelectedSceneBoundingArea() const = 0;
        virtual QRectF GetSceneBoundingArea() const = 0;

        virtual void SignalLoadStart() = 0;
        virtual void SignalLoadEnd() = 0;
        virtual bool IsLoading() const = 0;
        virtual bool IsPasting() const = 0;

        virtual void RemoveUnusedNodes() = 0;
        virtual void RemoveUnusedElements() = 0;

        virtual void HandleProposalDaisyChainWithGroup(const NodeId& startNode, SlotType slotType, ConnectionType connectionType, const QPoint& screenPoint, const QPointF& focusPoint, AZ::EntityId groupTarget) = 0;

        virtual void HandleProposalDaisyChain(const NodeId& startNode, SlotType slotType, ConnectionType connectionType, const QPoint& screenPoint, const QPointF& focusPoint);
        
        virtual void StartNudging(const AZStd::unordered_set<AZ::EntityId>& fixedNodes) = 0;
        virtual void FinalizeNudging() = 0;
        virtual void CancelNudging() = 0;

        // Helper method to find the topmost group at a particular point
        virtual AZ::EntityId FindTopmostGroupAtPoint(QPointF scenePoint) = 0;

        // Signals used to manage the state around the generic add position
        virtual QPointF SignalGenericAddPositionUseBegin() = 0;
        virtual void SignalGenericAddPositionUseEnd() = 0;
    };

    using SceneRequestBus = AZ::EBus<SceneRequests>;

    //! SceneNotifications
    //! Notifications about changes to the state of scenes.
    class SceneNotifications
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;

        //! A node has been added to the scene.
        virtual void OnNodeAdded(const AZ::EntityId& /*nodeId*/, bool /*isPaste*/) {}
        //! A node has been removed from the scene.
        virtual void OnNodeRemoved(const AZ::EntityId& /*nodeId*/) {}
        //! A node in the scene has been selected
        virtual void OnNodeSelected(const AZ::EntityId& /*nodeId*/, bool /*selected*/) {}

        //! A node in the scene has been moved
        virtual void OnNodePositionChanged(const AZ::EntityId& /*nodeId*/, const AZ::Vector2& /*position*/) {}

        //! A node in the scene is being edited
        virtual void OnNodeIsBeingEdited(bool /*isEditing*/){}

        //! A Scene Member was added to the scene
        virtual void OnSceneMemberAdded(const AZ::EntityId& /*sceneMemberId*/) {};

        //! A Scene Member was removed from the scene
        virtual void OnSceneMemberRemoved(const AZ::EntityId& /*sceneMemberId*/) {};

        //! A Scene Member was selected
        virtual void OnSceneMemberSelected(const AZ::EntityId& /*sceneMemberId*/) {};

        //! A Scene Member in the scene has been moved
        virtual void OnSceneMemberPositionChanged(const AZ::EntityId& /*sceneMemberId*/, const AZ::Vector2& /*position/*/) {};

        //! A Scene Member in the scene has begun being dragged.
        virtual void OnSceneMemberDragBegin() {}

        //! A Scene Member in the scene is finished being dragged.
        virtual void OnSceneMemberDragComplete() {}

        //! A node in the scene has been deleted
        virtual void OnPreNodeDeleted(const AZ::EntityId& /*nodeId*/) {}

        //! A connection has been added to the scene.
        virtual void OnConnectionAdded(const AZ::EntityId& /*connectionId*/) {}

        //! A connection has been removed from the scene.
        virtual void OnConnectionRemoved(const AZ::EntityId& /*connectionId*/) {}

        //! A connection in the scene has been selected
        virtual void OnConnectionSelected(const AZ::EntityId& /*connectionId*/, bool /*selected*/) {}

        //! A connection in the scene has been deleted
        virtual void OnPreConnectionDeleted(const AZ::EntityId& /*connectionId*/) {}

        //! Selected nodes, connections and groups have been serialized to the target serialization.
        virtual void OnEntitiesSerialized(GraphSerialization&) {}

        //! GraphCanvas nodes, connections and groups have been pasted from the clipboard
        //! The userData map contains any custom data serialized in from a copy operation.
        virtual void OnEntitiesDeserialized(const GraphSerialization&) {}

        //! Signalled once everything that was deserialized in a batch is complete
        virtual void OnEntitiesDeserializationComplete(const GraphSerialization&) {}

        //! Signalled when a paste event is received, and it does not contain the CopyMimeType.
        virtual void OnUnknownPaste(const QPointF& /*scenePos*/) {}

        //! Sent when a duplicate command begins
        virtual void OnDuplicateBegin() {}

        //! Sent when a duplicate command ends
        virtual void OnDuplicateEnd() {}

        //! Sent When a paste command begins
        virtual void OnPasteBegin() {}

        //! Sent when a paste command finishes
        virtual void OnPasteEnd() {}

        //! Sent when a copy branch begins
        virtual void OnCopyBegin() {}

        //! Sent after a scene has complete a copy branch.
        virtual void OnCopyEnd() {}

        //! Sent after a scene has completed a deletion batch
        virtual void PostDeletionEvent() {}

        //! Sent after the scene has successfully handled a creation event.
        virtual void PostCreationEvent() {}

        //! The scene's stylesheet was changed.
        virtual void OnStylesChanged() {}

        //! The selection in the scene has changed.
        virtual void OnSelectionChanged() {}

        //! A key was pressed in the scene
        virtual void OnKeyPressed(QKeyEvent*) {}
        //! A key was released in the scene
        virtual void OnKeyReleased(QKeyEvent*) {}

        //! Signals that a drag selection began
        virtual void OnDragSelectStart() {}

        //! Signals that a drag selection has ended
        virtual void OnDragSelectEnd() {}

        //! Signals that a connection drag has begun
        virtual void OnConnectionDragBegin() {}

        //! Signals that a conneciton drag has ended
        virtual void OnConnectionDragEnd() {}

        //! Signals that the scene registered a graphics view
        virtual void OnViewRegistered() {}

        virtual void OnGraphLoadBegin() {}

        // Signaled once the load is complete.
        virtual void OnGraphLoadComplete() {}

        // Signaled once the load is complete, but after the LoadComplete signal.
        // Used right now to let Groups remap their internals on load. Then deal with collapsing
        // in the finalize.
        virtual void PostOnGraphLoadComplete() {}
    };

    using SceneNotificationBus = AZ::EBus<SceneNotifications>;

    //! SceneMemberRequests
    //! An interface that allows generic retrieval of the scene entities belong to.
    //! This is used in, for example, the styling code.
    class SceneMemberRequests
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;

        //! Set the scene an entity is in.
        virtual void SetScene(const AZ::EntityId&) = 0;

        //! Remove this entity from any scene it's in.
        virtual void ClearScene(const AZ::EntityId&) = 0;

        //! Signals to the SceneMember that all of the Scene configuration done by the scene is complete
        virtual void SignalMemberSetupComplete() = 0;

        //! When the entity is being copied. Provides a hook for copying extra information.
        virtual void PrepareExtraCopyData(AZStd::unordered_set< AZ::EntityId >& /*contextualCopies*/) { };

        //! Get the scene that the entity belongs to (directly or indirectly), if any.
        virtual AZ::EntityId GetScene() const = 0;
    };

    using SceneMemberRequestBus = AZ::EBus<SceneMemberRequests>;

    //! SceneMemberNotifications
    //! Notifications about changes to the scene membership of entities
    class SceneMemberNotifications
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;

        //! When the entity is added to a scene, this event is emitted.
        virtual void OnSceneSet(const AZ::EntityId& /*sceneId*/) {}

        virtual void PreOnRemovedFromScene(const AZ::EntityId& /*sceneId*/) {}
        virtual void OnRemovedFromScene(const AZ::EntityId& /*sceneId*/) {}

        //! Signal sent once the scene is fully configured and ready to be displayed.
        virtual void OnSceneReady() {}

        //! Signals that a SceneMember is fully and handled by the SceneComponent.
        virtual void OnMemberSetupComplete() {}

        virtual void OnSceneMemberHidden() {}
        
        virtual void OnSceneMemberShown() {}

        virtual void OnSceneMemberExpandedFromGroup(const AZ::EntityId& /*groupId*/) {}

        virtual void OnSceneMemberCollapsedInGroup(const AZ::EntityId& /*groupId*/) {}
        
        virtual void OnSceneMemberAboutToSerialize(GraphSerialization& /*serializationTarget*/) {}

        //! Signals that a SceneMember was deserialized into a particular graph.
        //! Note: The graphId is being passed in order to ask questions about the graph.
        //!       and is not a signal that the element has been added to the particular graph yet.
        virtual void OnSceneMemberDeserialized(const AZ::EntityId& /*graphId*/, const GraphSerialization& /*serializationTarget*/) {}
    };

    using SceneMemberNotificationBus = AZ::EBus<SceneMemberNotifications>;    
}

DECLARE_EBUS_EXTERN(GraphCanvas::SceneRequests);
