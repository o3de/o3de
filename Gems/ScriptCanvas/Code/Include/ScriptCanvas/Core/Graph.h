/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Component/EntityBus.h>
#include <AzCore/Outcome/Outcome.h>
#include <AzCore/std/containers/stack.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/parallel/mutex.h>
#include <ScriptCanvas/Core/GraphBus.h>
#include <ScriptCanvas/Core/GraphData.h>
#include <ScriptCanvas/Debugger/Bus.h>
#include <ScriptCanvas/Execution/ErrorBus.h>
#include <ScriptCanvas/Execution/ExecutionContext.h>
#include <ScriptCanvas/Debugger/StatusBus.h>
#include <ScriptCanvas/Debugger/ValidationEvents/ValidationEvent.h>

namespace ScriptCanvas
{
    class Node;
    class Slot;
    class Connection;
    class GraphVariableManagerRequests;

    //! Graph is the execution model of a ScriptCanvas graph.
    class Graph
        : public AZ::Component
        , protected GraphRequestBus::Handler
        , protected StatusRequestBus::Handler
        , private AZ::EntityBus::Handler
        , private ValidationRequestBus::Handler
    {
    private:
        struct ValidationStruct
        {
            AZ::Crc32 m_validationEventId;
            AZStd::string m_errorDescription;
        };

    public:
        friend Node;

        AZ_COMPONENT(Graph, "{C3267D77-EEDC-490E-9E42-F1D1F473E184}");

        static void Reflect(AZ::ReflectContext* context);

        Graph(const ScriptCanvasId& executionId = AZ::Entity::MakeId());
        ~Graph() override;

        void Init() override;
        void Activate() override;
        void Deactivate() override;

        const AZStd::vector<AZ::EntityId> GetNodesConst() const;
        AZStd::unordered_set<AZ::Entity*>& GetNodeEntities() { return m_graphData.m_nodes; }
        const AZStd::unordered_set<AZ::Entity*>& GetNodeEntities() const { return m_graphData.m_nodes; }

        const ScriptCanvas::ScriptCanvasId& GetScriptCanvasId() const { return m_scriptCanvasId; }

        void MarkVersion();
        const VersionData& GetVersion() const;

        void Parse(ValidationResults& validationResults);

        //// GraphRequestBus::Handler
        bool AddNode(const AZ::EntityId&) override;
        bool RemoveNode(const AZ::EntityId& nodeId) override;
        Node* FindNode(AZ::EntityId nodeID) const override;

        AZStd::vector<AZ::EntityId> GetNodes() const override;        

        Slot* FindSlot(const Endpoint& endpoint) const override;

        bool AddConnection(const AZ::EntityId&) override;
        void RemoveAllConnections();
        bool RemoveConnection(const AZ::EntityId& connectionId) override;
        AZStd::vector<AZ::EntityId> GetConnections() const override;
        AZStd::vector<Endpoint> GetConnectedEndpoints(const Endpoint& firstEndpoint) const override;
        AZStd::pair< EndpointMapConstIterator, EndpointMapConstIterator > GetConnectedEndpointIterators(const Endpoint& endpoint) const override;
        bool IsEndpointConnected(const Endpoint& endpoint) const override;
        bool FindConnection(AZ::Entity*& connectionEntity, const Endpoint& firstEndpoint, const Endpoint& otherEndpoint) const override;

        bool Connect(const AZ::EntityId& sourceNodeId, const SlotId& sourceSlotId, const AZ::EntityId& targetNodeId, const SlotId& targetSlotId) override;
        bool Disconnect(const AZ::EntityId& sourceNodeId, const SlotId& sourceSlotId, const AZ::EntityId& targetNodeId, const SlotId& targetSlotId) override;

        bool ConnectByEndpoint(const Endpoint& sourceEndpoint, const Endpoint& targetEndpoint) override;
        AZ::Outcome<void, AZStd::string> CanCreateConnectionBetween(const Endpoint& sourceEndpoint, const Endpoint& targetEndpoint) const override;

        AZ::Outcome<void, AZStd::string> CanConnectionExistBetween(const Endpoint& sourceEndpoint, const Endpoint& targetEndpoint) const override;

        bool DisconnectByEndpoint(const Endpoint& sourceEndpoint, const Endpoint& targetEndpoint) override;
        bool DisconnectById(const AZ::EntityId& connectionId) override;

        // Dependent Assets
        bool AddDependentAsset(AZ::EntityId nodeId, const AZ::TypeId assetType, const AZ::Data::AssetId assetId) override;
        bool RemoveDependentAsset(AZ::EntityId nodeId) override;


        //! Retrieves the Entity this Graph component is currently located on
        //! NOTE: There can be multiple Graph components on the same entity so calling FindComponent may not not return this GraphComponent
        AZ::Entity* GetGraphEntity() const override { return GetEntity(); }

        Graph* GetGraph() override { return this; }

        GraphData* GetGraphData() override { return &m_graphData; }
        const GraphData* GetGraphDataConst() const override { return &m_graphData; }
        const VariableData* GetVariableDataConst() const override { return const_cast<Graph*>(this)->GetVariableData(); }

        bool AddGraphData(const GraphData&) override;
        void RemoveGraphData(const GraphData&) override;

        bool IsBatchAddingGraphData() const override;

        AZStd::unordered_set<AZ::Entity*> CopyItems(const AZStd::unordered_set<AZ::Entity*>& entities) override;
        void AddItems(const AZStd::unordered_set<AZ::Entity*>& graphField) override;
        void RemoveItems(const AZStd::unordered_set<AZ::Entity*>& graphField) override;
        void RemoveItems(const AZStd::vector<AZ::Entity*>& graphField);
        AZStd::unordered_set<AZ::Entity*> GetItems() const override;

        bool AddItem(AZ::Entity* itemRef) override;
        bool RemoveItem(AZ::Entity* itemRef) override;
        ///////////////////////////////////////////////////////////

        // StatusRequestBus
        void ValidateGraph(ValidationResults& validationEvents) override;
        void ReportValidationResults(ValidationResults&) override { }
        ////

        AZStd::pair<ScriptCanvas::ScriptCanvasId, ValidationResults> GetValidationResults() override;

        virtual void ReportError(const Node& node, const AZStd::string& errorSource, const AZStd::string& errorMessage);

    protected:
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
        {
            (void)dependent;
        }

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC("ScriptCanvasRuntimeService", 0x776e1e3a));;
        }

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("ScriptCanvasService", 0x41fd58f3));
        }

        void PostActivate();

        void ValidateVariables(ValidationResults& validationResults);
        void ValidateScriptEvents(ValidationResults& validationResults);

        bool ValidateConnectionEndpoints(const AZ::EntityId& connectionRef, const AZStd::unordered_set<AZ::EntityId>& nodeRefs);

        AZ::Outcome<void, AZStd::vector<ValidationStruct> > ValidateNode(AZ::Entity* nodeEntity, ValidationResults& validationEvents) const;

        AZ::Outcome<void, ValidationStruct> ValidateConnection(AZ::Entity* connection) const;

        bool IsInDataFlowPath(const Node* sourceNode, const Node* targetNode) const;

        void RefreshConnectionValidity(bool warnOnRemoval = false);

        AZ::Data::AssetId GetAssetId() const override { return AZ::Data::AssetId(); }
        GraphIdentifier GetGraphIdentifier() const override { return GraphIdentifier(GetAssetId(), 0); }
        AZStd::string GetAssetName() const override { return "";  }
        AZ::EntityId GetRuntimeEntityId() const override { return GetEntity() ? GetEntityId() : AZ::EntityId(); }

        VariableId FindAssetVariableIdByRuntimeVariableId(VariableId runtimeId) const override { return runtimeId; }
        AZ::EntityId FindAssetNodeIdByRuntimeNodeId(AZ::EntityId editorNode) const override { return editorNode; }
        AZ::EntityId FindRuntimeNodeIdByAssetNodeId(AZ::EntityId runtimeNode) const override { return runtimeNode; }
        
        VariableData* GetVariableData() override;
        
        const GraphVariableMapping* GetVariables() const override;
        GraphVariable* FindVariable(AZStd::string_view propName) override;
        GraphVariable* FindVariableById(const VariableId& variableId) override;
        Data::Type GetVariableType(const VariableId& variableId) const;
        AZStd::string_view GetVariableName(const VariableId& variableId) const;

        bool IsGraphObserved() const override;
        void SetIsGraphObserved(bool isObserved) override;
        ////

        const AZStd::unordered_map<AZ::EntityId, Node* >& GetNodeMapping() const { return m_nodeMapping; }

    protected:
        void VersioningRemoveSlot(ScriptCanvas::Node& scriptCanvasNode, const SlotId& slotId);

        GraphData m_graphData;
        AZ::Data::AssetType m_assetType;
        
    private:
        ScriptCanvasId m_scriptCanvasId;
        ExecutionMode m_executionMode = ExecutionMode::Interpreted;
        VersionData m_versionData;

        GraphVariableManagerRequests* m_variableRequests = nullptr;

        // Keeps a mapping of the Node EntityId -> NodeComponent.
        // Saves looking up the NodeComponent every time we need the Node.
        AZStd::unordered_map<AZ::EntityId, Node* > m_nodeMapping;
        
        bool m_isObserved;
        bool m_batchAddingData;

        void OnEntityActivated(const AZ::EntityId&) override;
        class GraphEventHandler;
    };
}
