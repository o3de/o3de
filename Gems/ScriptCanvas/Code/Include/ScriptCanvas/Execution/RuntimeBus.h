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

#pragma once

#include <ScriptCanvas/Core/Endpoint.h>

#include <AzCore/EBus/EBus.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string_view.h>

#include <ScriptCanvas/Core/ExecutionNotificationsBus.h>
#include <ScriptCanvas/Data/Data.h>
#include <ScriptCanvas/Variable/VariableCore.h>

namespace ScriptCanvas
{
    struct GraphData;
    class GraphVariable;
    class VariableData;
    
    using EndpointMapConstIterator = AZStd::unordered_multimap<Endpoint, Endpoint>::const_iterator;

    //! Runtime Request Bus for interfacing with the runtime execution component
    class RuntimeRequests : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        //! BusIdType represents a unique id for the execution component
        //! Because multiple Script Canvas graphs can execute on the same entity
        //! this is not an "EntityId" in the sense that it uniquely identifies an entity.
        using BusIdType = ScriptCanvasId;

        virtual VariableId FindAssetVariableIdByRuntimeVariableId(VariableId runtimeId) const = 0;
        
        virtual VariableId FindRuntimeVariableIdByAssetVariableId(VariableId assetId) const = 0;

        virtual AZ::EntityId FindAssetNodeIdByRuntimeNodeId(AZ::EntityId editorNode) const = 0;

        virtual AZ::Data::AssetId GetAssetId() const = 0;
        virtual GraphIdentifier GetGraphIdentifier() const = 0;
        virtual AZStd::string GetAssetName() const = 0;
        
        //! Looks up the nodeId within the bus handler
        virtual Node* FindNode(AZ::EntityId nodeId) const = 0;
        
        virtual AZ::EntityId FindRuntimeNodeIdByAssetNodeId(AZ::EntityId runtimeNode) const = 0;        
        
        //! Returns the entity id of the execution component
        virtual AZ::EntityId GetRuntimeEntityId() const = 0;
                
        //! Returns a container of nodes
        virtual AZStd::vector<AZ::EntityId> GetNodes() const = 0;
        //! Returns a container of connections
        virtual AZStd::vector<AZ::EntityId> GetConnections() const = 0;

        //! Returns a container of all endpoints to which the supplied endpoint is connected point
        virtual AZStd::vector<Endpoint> GetConnectedEndpoints(const Endpoint& endpoint) const = 0;

        virtual AZStd::pair< EndpointMapConstIterator, EndpointMapConstIterator > GetConnectedEndpointIterators(const Endpoint& endpoint) const = 0;

        //! Returns whether the given endpoint has any connections
        virtual bool IsEndpointConnected(const Endpoint& endpoint) const = 0;

        //! Retrieves GraphData structure which manages the nodes and connections of an executing graph
        virtual GraphData* GetGraphData() = 0;
        virtual const GraphData* GetGraphDataConst() const = 0;

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

        //! Returns the type associated with the specified variable.
        virtual Data::Type GetVariableType(const VariableId& variableId) const = 0;

        //! Looks up the variable name that the variable data is associated with in the handler of the bus
        virtual AZStd::string_view GetVariableName(const VariableId& variableId) const = 0;

        virtual bool IsGraphObserved() const = 0;
        virtual void SetIsGraphObserved(bool isObserved) = 0;

        virtual AZ::Data::AssetType GetAssetType() const = 0;
    };

    using RuntimeRequestBus = AZ::EBus<RuntimeRequests>;
}
