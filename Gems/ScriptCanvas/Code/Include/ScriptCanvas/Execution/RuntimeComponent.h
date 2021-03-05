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

#include <AzCore/Component/Component.h>
#include <AzCore/Component/EntityBus.h>
#include <AzCore/Outcome/Outcome.h>
#include <AzCore/std/containers/stack.h>
#include <AzCore/std/containers/unordered_map.h>

#include <AzFramework/Network/NetBindable.h>

#include <ScriptCanvas/Asset/RuntimeAsset.h>
#include <ScriptCanvas/Core/Core.h>
#include <ScriptCanvas/Core/GraphData.h>
#include <ScriptCanvas/Core/ExecutionNotificationsBus.h>
#include <ScriptCanvas/Execution/ExecutionContext.h>
#include <ScriptCanvas/Execution/RuntimeBus.h>
#include <ScriptCanvas/Variable/GraphVariableNetBindings.h>
#include <ScriptCanvas/Variable/VariableBus.h>
#include <ScriptCanvas/Variable/VariableCore.h>
#include <ScriptCanvas/Variable/VariableData.h>


namespace ScriptCanvas
{
    using VariableIdMap = AZStd::unordered_map<VariableId, VariableId>;

    //! Runtime Component responsible for loading a ScriptCanvas Graph from a runtime
    //! asset.
    //! It contains none of the Graph functionality of Validating Connections, as well as adding
    //! and removing nodes. Furthermore none of the functionality to remove and add variables
    //! exist in this component.
    //! It is assumed that the graph has runtime graph has already been validated and compiled
    //! at this point.
    //! This component should only be used at runtime 
    class RuntimeComponent
        : public AZ::Component
        , public AzFramework::NetBindable
        , protected RuntimeRequestBus::Handler
        , protected VariableNotificationBus::MultiHandler
        , protected VariableRequestBus::MultiHandler
        , private AZ::Data::AssetBus::Handler
        , public AZ::EntityBus::Handler
    {
    public:
        friend class Node;

        AZ_COMPONENT(RuntimeComponent, "{95BFD916-E832-4956-837D-525DE8384282}", NetBindable);

        static void Reflect(AZ::ReflectContext* context);
        static bool VersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement);

        static AZStd::vector<RuntimeComponent*> FindAllByEditorAssetId(AZ::Data::AssetId editorScriptCanvasAssetId);
        
        RuntimeComponent(ScriptCanvasId uniqueId = AZ::Entity::MakeId());
        RuntimeComponent(AZ::Data::Asset<RuntimeAsset> runtimeAsset, ScriptCanvasId uniqueId = AZ::Entity::MakeId());

        ~RuntimeComponent() override;

        //// AZ::Component
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////

        void CreateNetBindingTable();

        //////////////////////////////////////////////////////////////////////////
        // NetBindable
        GridMate::ReplicaChunkPtr GetNetworkBinding() override;
        void SetNetworkBinding(GridMate::ReplicaChunkPtr chunk) override;
        void UnbindFromNetwork() override;
        //////////////////////////////////////////////////////////////////////////

        ScriptCanvasId GetScriptCanvasId() const { return m_scriptCanvasId; };

        void SetRuntimeAsset(const AZ::Data::Asset<AZ::Data::AssetData>& runtimeAsset);

        const AZStd::vector<AZ::EntityId> GetNodesConst() const;

        // RuntimeComponentRequestBus
        ////

        //// RuntimeRequestBus::Handler
        VariableId FindAssetVariableIdByRuntimeVariableId(VariableId runtimeId) const override;
        VariableId FindRuntimeVariableIdByAssetVariableId(VariableId runtimeId) const override;
        AZ::EntityId FindAssetNodeIdByRuntimeNodeId(AZ::EntityId runtimeNode) const override;
        AZ::Data::AssetId GetAssetId() const override { return m_runtimeAsset.GetId(); }
        GraphIdentifier GetGraphIdentifier() const override;
        AZStd::string GetAssetName() const override;
        Node* FindNode(AZ::EntityId nodeId) const override;
        AZ::EntityId FindRuntimeNodeIdByAssetNodeId(AZ::EntityId editorNode) const override;        
        AZ::EntityId GetRuntimeEntityId() const override { return GetEntity() ? GetEntityId() : AZ::EntityId(); }
        AZStd::vector<AZ::EntityId> GetNodes() const override;
        AZStd::vector<AZ::EntityId> GetConnections() const override;
        AZStd::vector<Endpoint> GetConnectedEndpoints(const Endpoint& firstEndpoint) const override;
        AZStd::pair< EndpointMapConstIterator, EndpointMapConstIterator > GetConnectedEndpointIterators(const Endpoint& endpoint) const override;

        bool IsEndpointConnected(const Endpoint& endpoint) const override;
        GraphData* GetGraphData() override;
        const GraphData* GetGraphDataConst() const override { return const_cast<RuntimeComponent*>(this)->GetGraphData(); }

        VariableData* GetVariableData() override;
        const VariableData* GetVariableDataConst() const { return const_cast<RuntimeComponent*>(this)->GetVariableData(); }
        const GraphVariableMapping* GetVariables() const override;

        GraphVariable* FindVariable(AZStd::string_view propName) override;
        GraphVariable* FindVariableById(const VariableId& variableId) override;

        Data::Type GetVariableType(const VariableId& variableId) const override;
        AZStd::string_view GetVariableName(const VariableId& variableId) const override;

        bool IsGraphObserved() const override;
        void SetIsGraphObserved(bool isObserved) override;

        AZ::Data::AssetType GetAssetType() const override;
        ////

        //// VariableNotificationBus::Handler
        void OnVariableValueChanged() override;
        ////

        void SetVariableOverrides(const VariableData& overrideData);
        void InitializeVariableState();
        void ResetState();
        void SetVariableEntityIdMap(const AZStd::unordered_map<AZ::u64, AZ::EntityId> variableEntityIdMap);

        //// VariableRequestBus::Handler
        GraphVariable* GetVariable() override;
        const GraphVariable* GetVariableConst() const override { return const_cast<RuntimeComponent*>(this)->GetVariable(); }
        Data::Type GetType() const override;
        AZStd::string_view GetName() const override;
        AZ::Outcome<void, AZStd::string> RenameVariable(AZStd::string_view newVarName) override;
        ////

        //// AZ::Data::AssetBus
        void OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
        void OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
        ////

        // AZ::EntityBus
        void OnEntityActivated(const AZ::EntityId& entityId) override;
        ////

        bool IsInErrorState() const { return m_executionContext.IsInErrorState(); }

        void CreateFromGraphData(const GraphData* graphData, const VariableData* variableData);
        const ExecutionContext& GetExecutionContext() const { return m_executionContext; }

        void ActivateGraph();
        void DeactivateGraph();

    protected:
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            // Cannot be used with either the GraphComponent or the VariableManager Component
            incompatible.push_back(AZ_CRC("ScriptCanvasService", 0x41fd58f3));
            incompatible.push_back(AZ_CRC("ScriptCanvasVariableService", 0x819c8460));
        }
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("ScriptCanvasRuntimeService", 0x776e1e3a));
        }
                
        ActivationInfo CreateActivationInfo() const;
        ActivationInfo CreateDeactivationInfo() const;
        VariableValues CreateVariableValues() const;

        //// Execution
        void SetupAssetInstance();
        ////

        void SetAsset(AZ::Data::Asset<RuntimeAsset> runtimeAsset)
        {
            m_runtimeAsset = runtimeAsset;
        }

    private:
        ScriptCanvasId m_scriptCanvasId;
        RuntimeData m_runtimeData;
        AZ::Data::Asset<RuntimeAssetBase> m_runtimeAsset;

        //! Per instance variable data overrides for the runtime asset
        //! This is serialized when building this component from the EditorScriptCanvasComponent
        VariableData m_variableOverrides;

        //! Per instance variable initialization state. For the function asset.
        //! This is stored from the initial application.
        VariableData m_internalVariables;

        ExecutionContext m_executionContext;
        
        //! Script Canvas VariableId populated when the RuntimeAsset loads
        VariableIdMap m_assetToRuntimeVariableMap;
        VariableIdMap m_runtimeToAssetVariableMap;

        AZStd::unordered_set< Node* > m_entryNodes;
        AZStd::unordered_set< Node* > m_entryPoints;
        AZStd::unordered_map< AZ::EntityId, Node* > m_nodeLookupMap;        

        //! Script Canvas VariableId map populated by the EditorScriptCanvasComponent in build game Entity
        AZStd::unordered_map<AZ::u64, AZ::EntityId> m_variableEntityIdMap;
        
        //! Used to map runtime graphs back to asset sources, for use in debugging and logging against human written content in the editor
        AZStd::unordered_map<AZ::EntityId, AZ::EntityId> m_runtimeIdToAssetId;
        //! Used to map asset sources to runtime graphs, for use in debugging and logging against human written content in the editor
        AZStd::unordered_map<AZ::EntityId, AZ::EntityId> m_assetIdToRuntimeId;

        using GraphVariableNetBindingTablePtr = AZStd::unique_ptr<GraphVariableNetBindingTable>;
        //! Ptr to netbinding table used to track and execute networking-related logic for reflected variables.
        GraphVariableNetBindingTablePtr m_graphVariableNetBindingTable = nullptr;

        bool m_isObserved = false;
        bool m_createdAsset = false;


        RuntimeData* GetRuntimeData()
        {
            RuntimeData* runtimeData = nullptr;
            if (m_runtimeAsset.GetType() == azrtti_typeid<RuntimeFunctionAsset>())
            {
                runtimeData = &m_runtimeAsset.GetAs<RuntimeFunctionAsset>()->GetData();
            }
            else if (m_runtimeAsset.GetType() == azrtti_typeid<RuntimeAsset>())
            {
                runtimeData = &m_runtimeAsset.GetAs<RuntimeAsset>()->GetData();
            }

            return runtimeData;
        }
    };
}
