/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "Core.h"
#include "Endpoint.h"

#include <ScriptCanvas/Variable/VariableCore.h>
#include <ScriptCanvas/Variable/GraphVariable.h>

#include <AzCore/EBus/EBus.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Outcome/Outcome.h>

namespace ScriptCanvas
{
    struct GraphData;
    class Graph;
    class Slot;

    using EndpointMapConstIterator = AZStd::unordered_multimap<Endpoint, Endpoint>::const_iterator;

    //! These are public graph requests
    class GraphRequests : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = ScriptCanvasId;

        //! Add a ScriptCanvas Node to the Graph
        virtual bool AddNode(const AZ::EntityId&) = 0;
        //! Remove a ScriptCanvas Node to the Graph
        virtual bool RemoveNode(const AZ::EntityId& nodeId) = 0;

        //! Add a ScriptCanvas Connection to the Graph
        virtual bool AddConnection(const AZ::EntityId&) = 0;
        //! Remove a ScriptCanvas Connection from the Graph
        virtual bool RemoveConnection(const AZ::EntityId& nodeId) = 0;

        //! Add an asset dependency to the Graph
        virtual bool AddDependentAsset(AZ::EntityId nodeId, const AZ::TypeId assetType, const AZ::Data::AssetId assetId) = 0;
        //! Remove an asset dependency from the Graph
        virtual bool RemoveDependentAsset(AZ::EntityId nodeId) = 0;

        virtual AZStd::vector<AZ::EntityId> GetNodes() const = 0;
        virtual AZStd::vector<AZ::EntityId> GetConnections() const = 0;
        virtual AZStd::vector<Endpoint> GetConnectedEndpoints(const Endpoint& firstEndpoint) const = 0;
        virtual bool FindConnection(AZ::Entity*& connectionEntity, const Endpoint& firstEndpoint, const Endpoint& otherEndpoint) const = 0;

        virtual Slot* FindSlot(const Endpoint& endpoint) const = 0;

        //! Retrieves the Entity this Graph component is located on
        //! NOTE: There can be multiple Graph components on the same entity so calling FindComponent may not not return this GraphComponent
        virtual AZ::Entity* GetGraphEntity() const = 0;

        //! Retrieves the Graph Component directly using the BusId
        virtual Graph* GetGraph() = 0;

        virtual bool Connect(const AZ::EntityId& sourceNodeId, const SlotId& sourceSlot, const AZ::EntityId& targetNodeId, const SlotId& targetSlot) = 0;
        virtual bool Disconnect(const AZ::EntityId& sourceNodeId, const SlotId& sourceSlot, const AZ::EntityId& targetNodeId, const SlotId& targetSlot) = 0;

        virtual bool ConnectByEndpoint(const Endpoint& sourceEndpoint, const Endpoint& targetEndpoint) = 0;

        //! Returns whether or not a new connecion can be created between two connections.
        //! This will take into account if the endpoints are already connected
        virtual AZ::Outcome<void, AZStd::string> CanCreateConnectionBetween(const Endpoint& sourceEndpoint, const Endpoint& targetEndpoint) const = 0;

        //! Returns whether or not a connection could exist between the two connections.
        //! Does not take into account if the endpoints are already connected.
        virtual AZ::Outcome<void, AZStd::string> CanConnectionExistBetween(const Endpoint& sourceEndpoint, const Endpoint& targetEndpoint) const = 0;

        virtual bool DisconnectByEndpoint(const Endpoint& sourceEndpoint, const Endpoint& targetEndpoint) = 0;
        virtual bool DisconnectById(const AZ::EntityId& connectionId) = 0;

        //! Copies any Node and Connection Entities that belong to the graph to a GraphSerializableField
        virtual AZStd::unordered_set<AZ::Entity*> CopyItems(const AZStd::unordered_set<AZ::Entity*>& entities) = 0;
        //! Adds any Node and Connection Entities to the graph
        virtual void AddItems(const AZStd::unordered_set<AZ::Entity*>& entities) = 0;
        //! Removes any Node and Connection Entities that belong to the graph 
        virtual void RemoveItems(const AZStd::unordered_set<AZ::Entity*>& entities) = 0;
        //! Retrieves any entities that can be be added to graphs
        virtual AZStd::unordered_set<AZ::Entity*> GetItems() const = 0;

        //! Add item to graph if the item is of the type that can be added to the graph
        virtual bool AddItem(AZ::Entity* itemEntity) = 0;
        //! Remove item if it is on the graph
        virtual bool RemoveItem(AZ::Entity* itemEntity) = 0;

        //! Retrieves a pointer the GraphData structure stored on the graph
        virtual GraphData* GetGraphData() = 0;
        virtual const GraphData* GetGraphDataConst() const = 0;

        // Adds nodes and connections in the GraphData structure to the graph
        virtual bool AddGraphData(const GraphData&) = 0;
        // Removes nodes and connections in the GraphData structure from the graph
        virtual void RemoveGraphData(const GraphData&) = 0;

        // Signals wether or not a batch of graph data is being added and some extra steps are needed
        // to maintain data integrity for dynamic nodes
        virtual bool IsBatchAddingGraphData() const = 0;

        virtual void SetIsGraphObserved(bool observed) = 0;
        virtual bool IsGraphObserved() const = 0;

        virtual VariableId FindAssetVariableIdByRuntimeVariableId(VariableId runtimeId) const = 0;

        virtual AZ::EntityId FindAssetNodeIdByRuntimeNodeId(AZ::EntityId editorNode) const = 0;

        virtual AZ::Data::AssetId GetAssetId() const = 0;

        virtual GraphIdentifier GetGraphIdentifier() const = 0;

        virtual AZStd::string GetAssetName() const = 0;

        //! Looks up the nodeId within the bus handler
        virtual Node* FindNode(AZ::EntityId nodeId) const = 0;

        virtual AZ::EntityId FindRuntimeNodeIdByAssetNodeId(AZ::EntityId runtimeNode) const = 0;

        //! Returns the entity id of the execution component
        virtual AZ::EntityId GetRuntimeEntityId() const = 0;

        virtual AZStd::pair< EndpointMapConstIterator, EndpointMapConstIterator > GetConnectedEndpointIterators(const Endpoint& endpoint) const = 0;

        //! Returns whether the given endpoint has any connections
        virtual bool IsEndpointConnected(const Endpoint& endpoint) const = 0;

        //! Retrieves VariableData structure which manages variable data for the execution component
        virtual VariableData* GetVariableData() = 0;
        virtual const VariableData* GetVariableDataConst() const = 0;

        //! Retrieves a map of variable id to variable name and variable datums pair
        virtual const GraphVariableMapping* GetVariables() const = 0;

        //! Searches for a variable with the specified name
        //! returns pointer to the first variable with the specified name or nullptr
        virtual GraphVariable* FindVariable(AZStd::string_view varName) = 0;

        //! Searches for a variable by VariableId
        //! returns a pair of <variable datum pointer, variable name> with the supplied id
        //! The variable datum pointer is non-null if the variable has been found
        virtual GraphVariable* FindVariableById(const VariableId& variableId) = 0;
    };
    using GraphRequestBus = AZ::EBus<GraphRequests>;

    class GraphNotifications : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = ScriptCanvasId;

        //! Notification when a node is added
        virtual void OnNodeAdded(const AZ::EntityId&) {}

        //! Notification when a node is removed
        virtual void OnNodeRemoved(const AZ::EntityId&) {}

        //! Notification when a connection is added
        virtual void OnConnectionAdded(const AZ::EntityId&) {}

        //! Notification when a connections is removed
        virtual void OnConnectionRemoved(const AZ::EntityId&) {}

        //! Notification when a connections is about to be removed
        virtual void OnPreConnectionRemoved(const AZ::EntityId&) {}

        //! Notification when a connection is completed
        virtual void OnConnectionComplete(const AZ::EntityId&) {}

        //! Notification when a connection is completed
        virtual void OnDisonnectionComplete(const AZ::EntityId&) {}

        //! Notification when a batch add for a graph begins
        virtual void OnBatchAddBegin() {}

        //! Notification when a batch add for a graph completes
        virtual void OnBatchAddComplete() {};
    };

    using GraphNotificationBus = AZ::EBus<GraphNotifications>;

    class GraphConfigurationRequests : public AZ::ComponentBus
    {
    public:
        virtual const ScriptCanvas::ScriptCanvasId& GetScriptCanvasId() const = 0;
    };

    using GraphConfigurationRequestBus = AZ::EBus<GraphConfigurationRequests>;

    // This bus is for anything co-components that needs to be configured with the graph.
    class GraphConfigurationNotifications : public AZ::ComponentBus
    {
    public:
        virtual void ConfigureScriptCanvasId(const ScriptCanvas::ScriptCanvasId& scriptCanvasId) = 0;
    };

    using GraphConfigurationNotificationBus = AZ::EBus<GraphConfigurationNotifications>;

    class EndpointNotifications : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = Endpoint;

        //! Notification when an endpoint has been connected.
        //! \param the target Endpoint. The source Endpoint can be obtained using EndpointNotificationBus::GetCurrentBusId().
        virtual void OnEndpointConnected([[maybe_unused]] const Endpoint& targetEndpoint) {}

        //! Notification when an endpoint has been disconnected.
        //! \param the target Endpoint. The source Endpoint can be obtained using EndpointNotificationBus::GetCurrentBusId().
        virtual void OnEndpointDisconnected([[maybe_unused]] const Endpoint& targetEndpoint) {}

        //! Notification when an endpoint has it's reference changed.
        virtual void OnEndpointReferenceChanged([[maybe_unused]] const VariableId& variableId) {}

        virtual void OnEndpointConvertedToReference() {}

        virtual void OnEndpointConvertedToValue() {}

        virtual void OnSlotRecreated() {};
    };

    using EndpointNotificationBus = AZ::EBus<EndpointNotifications>;
}
