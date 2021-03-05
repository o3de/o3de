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

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Component/EntityUtils.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Debug/Profiler.h>
#include <AzCore/Serialization/IdUtils.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/Utils.h>
#include <AzFramework/Entity/EntityContextBus.h>
#include <AzFramework/Entity/SliceEntityOwnershipServiceBus.h>
#include <AzFramework/Network/NetBindingHandlerBus.h>
#include <ScriptCanvas/Asset/RuntimeAsset.h>
#include <ScriptCanvas/Core/Node.h>
#include <ScriptCanvas/Core/ScriptCanvasBus.h>
#include <ScriptCanvas/Core/GraphScopedTypes.h>
#include <ScriptCanvas/Execution/RuntimeComponent.h>
#include <ScriptCanvas/Libraries/Core/ErrorHandler.h>
#include <ScriptCanvas/Libraries/Core/Start.h>
#include <ScriptCanvas/Variable/GraphVariableNetBindings.h>

namespace ScriptCanvas
{
    GraphInfo CreateGraphInfo(ScriptCanvasId scriptCanvasId, const GraphIdentifier& graphIdentifier)
    {
        AZ::EntityId runtimeEntity{};
        RuntimeRequestBus::EventResult(runtimeEntity, scriptCanvasId, &RuntimeRequests::GetRuntimeEntityId);
        return GraphInfo(runtimeEntity, graphIdentifier);
    }

    DatumValue CreateVariableDatumValue(ScriptCanvasId scriptCanvasId, const GraphVariable& graphVariable)
    {
        return CreateVariableDatumValue(scriptCanvasId, (*graphVariable.GetDatum()), graphVariable.GetVariableId());
    }

    DatumValue CreateVariableDatumValue([[maybe_unused]] ScriptCanvasId scriptCanvasId, const Datum& variableDatum, const VariableId& variableId)
    {
        /*
        VariableId assetVariableId{};
        RuntimeRequestBus::EventResult(assetVariableId, scriptCanvasId, &RuntimeRequests::FindAssetVariableIdByRuntimeVariableId, variable.GetVariableId());
        */

        // Make a Copy of the Variable with the original AssetId so we can remap it back to the display data.
        return DatumValue::Create(GraphVariable(variableDatum, variableId));
    }

    bool RuntimeComponent::VersionConverter([[maybe_unused]] AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
    {
        if (classElement.GetVersion() < 3)
        {
            classElement.RemoveElementByName(AZ_CRC("m_uniqueId", 0x52157a7a));
        }

        return true;
    }

    RuntimeComponent::RuntimeComponent(ScriptCanvasId scriptCanvasId)
        : RuntimeComponent({}, scriptCanvasId)
    {}

    RuntimeComponent::RuntimeComponent(AZ::Data::Asset<RuntimeAsset> runtimeAsset, ScriptCanvasId scriptCanvasId)
        : m_runtimeAsset(runtimeAsset)
        , m_scriptCanvasId(scriptCanvasId)
    {}

    RuntimeComponent::~RuntimeComponent()
    {
        for (auto& nodeEntity : m_runtimeData.m_graphData.m_nodes)
        {
            if (nodeEntity)
            {
                if (nodeEntity->GetState() == AZ::Entity::State::Active)
                {
                    nodeEntity->Deactivate();
                }
            }
        }

        for (auto& connectionEntity : m_runtimeData.m_graphData.m_connections)
        {
            if (connectionEntity)
            {
                if (connectionEntity->GetState() == AZ::Entity::State::Active)
                {
                    connectionEntity->Deactivate();
                }
            }
        }

        // Defer graph deletion to next frame as an executing graph in a dynamic slice can be deleted from a node
        auto oldRuntimeData(AZStd::move(m_runtimeData));
        // Use AZ::SystemTickBus, not AZ::TickBus otherwise the deferred delete will not happen if timed just before a level unload
        AZ::SystemTickBus::QueueFunction([oldRuntimeData]() mutable
        {
            oldRuntimeData.m_graphData.Clear(true);
            oldRuntimeData.m_variableData.Clear();
        });
    }

    void RuntimeComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<RuntimeComponent, AZ::Component>()
                ->Version(3, &RuntimeComponent::VersionConverter)
                ->Field("m_runtimeAsset", &RuntimeComponent::m_runtimeAsset)
                ->Field("m_variableOverrides", &RuntimeComponent::m_variableOverrides)
                ->Field("m_variableEntityIdMap", &RuntimeComponent::m_variableEntityIdMap)
                ;
        }

        GraphVariableNetBindingTable::Reflect(context);
    }

    void RuntimeComponent::Init()
    {
        if (m_runtimeAsset.GetId().IsValid())
        {
            m_runtimeAsset.QueueLoad();
        }
    }

    void RuntimeComponent::Activate()
    {
        RuntimeRequestBus::Handler::BusConnect(GetScriptCanvasId());

        m_runtimeAsset.BlockUntilLoadComplete();
        AZ::Data::AssetBus::Handler::BusConnect(m_runtimeAsset.GetId());
    }

    void RuntimeComponent::Deactivate()
    {
        AZ::Data::AssetBus::Handler::BusDisconnect();
        m_executionContext.DeactivateContext();
        DeactivateGraph();
        VariableNotificationBus::MultiHandler::BusDisconnect();
        RuntimeRequestBus::Handler::BusDisconnect();
    }

    void RuntimeComponent::CreateNetBindingTable()
    {
        if (m_graphVariableNetBindingTable == nullptr)
        {
            m_graphVariableNetBindingTable = GraphVariableNetBindingTablePtr(aznew GraphVariableNetBindingTable());
        }

        m_graphVariableNetBindingTable->SetVariableMappings(m_assetToRuntimeVariableMap, m_runtimeToAssetVariableMap);
    }

    GridMate::ReplicaChunkPtr RuntimeComponent::GetNetworkBinding()
    {
        CreateNetBindingTable();
        return m_graphVariableNetBindingTable->GetNetworkBinding();
    }

    void RuntimeComponent::SetNetworkBinding(GridMate::ReplicaChunkPtr chunk)
    {
        CreateNetBindingTable();
        m_graphVariableNetBindingTable->SetNetworkBinding(chunk);
    }

    void RuntimeComponent::UnbindFromNetwork()
    {
        if (m_graphVariableNetBindingTable)
        {
            m_graphVariableNetBindingTable->UnbindFromNetwork();
            m_graphVariableNetBindingTable.reset();
        }
    }

    void RuntimeComponent::ActivateGraph()
    {
        AZ_PROFILE_SCOPE_DYNAMIC(AZ::Debug::ProfileCategory::ScriptCanvas, "ScriptCanvas::RuntimeComponent::ActivateGraph (%s)", m_runtimeAsset.GetId().ToString<AZStd::string>().c_str());

        {
            if (!m_executionContext.ActivateContext(GetScriptCanvasId()))
            {
                return;
            }

            RuntimeData* runtimeData = GetRuntimeData();

            // If there are no nodes, there's nothing to do, deactivate the graph's entity.
            if (runtimeData && runtimeData->m_graphData.m_nodes.empty())
            {
                DeactivateGraph();
                return;
            }
        }

        if (!m_createdAsset)
        {
            SetupAssetInstance();
        }
        else
        {
            ResetState();

            for (auto entryPoint : m_entryPoints)
            {
                AZ::Entity* entryEntity = entryPoint->GetEntity();

                if (entryEntity->GetState() != AZ::Entity::State::Active)
                {
                    entryEntity->Activate();
                }

                entryPoint->PostActivate();
            }
        }

        // If we still can't find a start node, there's nothing to do.
        if (m_entryPoints.empty())
        {
            AZ_Warning("Script Canvas", false, "Graph does not have any entry point nodes, it will not run.");
            return;
        }

        AZ::EntityBus::Handler::BusConnect(GetEntityId());
    }

    void RuntimeComponent::DeactivateGraph()
    {
        AZ_PROFILE_SCOPE_DYNAMIC(AZ::Debug::ProfileCategory::ScriptCanvas, "ScriptCanvas::RuntimeComponent::DeactivateGraph (%s)", m_runtimeAsset.GetId().ToString<AZStd::string>().c_str());
        VariableRequestBus::MultiHandler::BusDisconnect();

        for (auto entryPoint : m_entryPoints)
        {
            AZ::Entity* entryEntity = entryPoint->GetEntity();

            if (entryEntity && entryEntity->GetState() == AZ::Entity::State::Active)
            {
                entryEntity->Deactivate();
            }
        }

        m_executionContext.DeactivateContext();

        SC_EXECUTION_TRACE_GRAPH_DEACTIVATED(CreateDeactivationInfo());
    }

    void RuntimeComponent::SetupAssetInstance()
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::ScriptCanvas);

        if (m_createdAsset)
        {
            return;
        }

        m_createdAsset = true;

        AZ::SerializeContext* serializeContext = AZ::EntityUtils::GetApplicationSerializeContext();
        m_runtimeIdToAssetId.clear();
        m_assetIdToRuntimeId.clear();

        m_runtimeData.m_graphData.Clear(true);
        m_runtimeData.m_variableData.Clear();

        m_nodeLookupMap.clear();
        m_entryNodes.clear();
        m_entryPoints.clear();

        RuntimeData* assetRuntimeData = GetRuntimeData();

        // Clone GraphData
        serializeContext->CloneObjectInplace(m_runtimeData.m_graphData, &assetRuntimeData->m_graphData);

        for (AZ::Entity* nodeEntity : m_runtimeData.m_graphData.m_nodes)
        {
            AZ::EntityId runtimeNodeId = AZ::Entity::MakeId();
            m_assetIdToRuntimeId.emplace(nodeEntity->GetId(), runtimeNodeId);
            m_runtimeIdToAssetId.emplace(runtimeNodeId, nodeEntity->GetId());
            if (auto* node = AZ::EntityUtils::FindFirstDerivedComponent<Node>(nodeEntity))
            {
                // Set all nodes Graph unique Id references directly to the Runtime Unique Id
                node->SetOwningScriptCanvasId(GetScriptCanvasId());
                node->SetGraphEntityId(GetEntityId());

                m_nodeLookupMap[runtimeNodeId] = node;
            }
        }

        for (AZ::Entity* connectionEntity : m_runtimeData.m_graphData.m_connections)
        {
            AZ::EntityId runtimeConnectionId = AZ::Entity::MakeId();
            m_assetIdToRuntimeId.emplace(connectionEntity->GetId(), runtimeConnectionId);
            m_runtimeIdToAssetId.emplace(runtimeConnectionId, connectionEntity->GetId());
        }

        m_runtimeData.m_graphData.LoadDependentAssets();

        // Clone Variable Data
        serializeContext->CloneObjectInplace(m_runtimeData.m_variableData, &assetRuntimeData->m_variableData);

        AZStd::unordered_map<AZ::EntityId, AZ::EntityId> loadedGameEntityIdMap;

        // Looks up the EntityContext loaded game entity map
        AzFramework::EntityContextId owningContextId = AzFramework::EntityContextId::CreateNull();
        AzFramework::EntityIdContextQueryBus::EventResult(owningContextId, GetEntityId(), &AzFramework::EntityIdContextQueries::GetOwningContextId);
        if (!owningContextId.IsNull())
        {
            AzFramework::SliceEntityOwnershipServiceRequestBus::EventResult(loadedGameEntityIdMap, owningContextId,
                &AzFramework::SliceEntityOwnershipServiceRequests::GetLoadedEntityIdMap);
        }

        // Added an entity mapping of the GraphOwnerId Entity to this component's EntityId
        // Also add a mapping of the UniqueId to the graph uniqueId
        // as well as an identity mapping for the EntityId and the UniqueId
        AZStd::unordered_map<AZ::EntityId, AZ::EntityId> assetToRuntimeEntityIdMap
        {
            { ScriptCanvas::GraphOwnerId, GetEntityId() },
            { ScriptCanvas::UniqueId, GetScriptCanvasId() },
            { GetEntityId(), GetEntityId() },
            { GetScriptCanvasId(), GetScriptCanvasId() },
            { AZ::EntityId(), AZ::EntityId() } // Adds an Identity mapping for InvalidEntityId -> InvalidEntityId since it is valid to set an EntityId field to InvalidEntityId 
        };
        assetToRuntimeEntityIdMap.insert(m_assetIdToRuntimeId.begin(), m_assetIdToRuntimeId.end());
 
        // Lambda function remaps any known entities to their correct id otherwise it remaps unknown entity Ids to invalid
        auto worldEntityRemapper =
            [&assetToRuntimeEntityIdMap, &loadedGameEntityIdMap, this](const AZ::EntityId& contextEntityId) -> AZ::EntityId
            {
                auto loadedGameEntity = loadedGameEntityIdMap.find(contextEntityId);
                if (loadedGameEntity != loadedGameEntityIdMap.end())
                {
                    return loadedGameEntity->second;
                }
                else
                {
                    auto foundEntityIdIt = assetToRuntimeEntityIdMap.find(contextEntityId);
                    if (foundEntityIdIt != assetToRuntimeEntityIdMap.end())
                    {
                        return foundEntityIdIt->second;
                    }
                    else
                    {
                        AZ_Warning("Script Canvas", false, "(%s) Entity Id %s is not part of the current entity context loaded entities "
                            "and not an internal node/connection entity. It will be mapped to InvalidEntityId", GetAssetName().c_str(),
                            contextEntityId.ToString().data());
                        return AZ::EntityId(AZ::EntityId::InvalidEntityId);
                    }
                }
            };

        AZ::IdUtils::Remapper<AZ::EntityId>::RemapIdsAndIdRefs(&m_runtimeData, worldEntityRemapper, serializeContext);

        // Apply Variable Overrides and Remap VariableIds
        InitializeVariableState();

        m_runtimeData.m_graphData.BuildEndpointMap();

        CreateNetBindingTable();

        // Track which variables in this canvas should be synced over the network.
        auto& varData = m_runtimeData.m_variableData.GetVariables();
        for (auto& iter : varData)
        {
            GraphVariable& graphVariable = iter.second;
            if (graphVariable.IsSynchronized())
            {
                m_graphVariableNetBindingTable->AddDatum(&graphVariable);
                VariableNotificationBus::MultiHandler::BusConnect(graphVariable.GetGraphScopedId());
            }
        }

        bool entryPointFound = false;
        for (auto& nodeEntity : m_runtimeData.m_graphData.m_nodes)
        {
            auto node = AZ::EntityUtils::FindFirstDerivedComponent<ScriptCanvas::Node>(nodeEntity);
            if (!node)
            {
                AZ_Error("Script Canvas", false, "Entity %s is missing node component", nodeEntity->GetName().data());
                continue;
            }

            if (node->IsEntryPoint())
            {
                m_entryPoints.insert(node);

                if (auto startNode = azrtti_cast<Nodes::Core::Start*>(node))
                {
                    m_entryNodes.insert(startNode);
                }
            }

            if (nodeEntity->GetState() == AZ::Entity::State::Constructed)
            {
                nodeEntity->Init();
            }

            if (nodeEntity->GetState() == AZ::Entity::State::Init)
            {
                nodeEntity->Activate();
            }
        }

        for (auto& connectionEntity : m_runtimeData.m_graphData.m_connections)
        {
            if (connectionEntity->GetState() == AZ::Entity::State::Constructed)
            {
                connectionEntity->Init();
            }

            if (connectionEntity->GetState() == AZ::Entity::State::Init)
            {
                connectionEntity->Activate();
            }
        }

        SC_EXECUTION_TRACE_GRAPH_ACTIVATED(CreateActivationInfo());

        for (auto& nodePair : m_nodeLookupMap)
        {
            if (nodePair.second)
            {
                nodePair.second->PostActivate();
            }
            else
            {
                AZ_TracePrintf("Script Canvas", "Invalid node found");
            }
        }
    }

    ActivationInfo RuntimeComponent::CreateActivationInfo() const
    {
        return ActivationInfo(GraphInfo(GetRuntimeEntityId(), GetGraphIdentifier()), CreateVariableValues());
    }

    ActivationInfo RuntimeComponent::CreateDeactivationInfo() const
    {
        return ActivationInfo(GraphInfo(GetRuntimeEntityId(), GetGraphIdentifier()));
    }
    
    VariableValues RuntimeComponent::CreateVariableValues() const
    {
        VariableValues variableValues;

        auto& runtimeVariables = m_runtimeData.m_variableData.GetVariables();
        for (const auto& variablePair : runtimeVariables)
        {
            variableValues.emplace(variablePair.first, AZStd::make_pair(variablePair.second.GetVariableName(), CreateVariableDatumValue(GetScriptCanvasId(), variablePair.second)));
        }

        return variableValues;
    }

    void RuntimeComponent::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        m_runtimeAsset = asset;
        ActivateGraph();
    } 

    void RuntimeComponent::OnEntityActivated([[maybe_unused]] const AZ::EntityId& entityId)
    {
        for (auto startNode : m_entryNodes)
        {
            m_executionContext.AddToExecutionStack(*startNode, SlotId());
        }

        if (RuntimeRequestBus::Handler::BusIsConnected())
        {
            m_executionContext.Execute();
        }

        AZ::EntityBus::Handler::BusDisconnect();
    }

    void RuntimeComponent::CreateFromGraphData(const GraphData* graphData, const VariableData* variableData)
    {
        m_runtimeData.m_graphData = graphData ? *graphData : GraphData();
        m_runtimeData.m_variableData = variableData ? *variableData : VariableData();
    }

    void RuntimeComponent::OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset)
    {
        // TODO: deleting this potentially helps with a rare crash when "live editing"
        DeactivateGraph();
        m_createdAsset = false;
        OnAssetReady(asset);
    }

    void RuntimeComponent::SetRuntimeAsset(const AZ::Data::Asset<AZ::Data::AssetData>& runtimeAsset)
    {   
        AZ::Entity* entity = GetEntity();
        if (entity && entity->GetState() == AZ::Entity::State::Active)
        {
            DeactivateGraph();
            AZ::Data::AssetBus::Handler::BusDisconnect();
            if (runtimeAsset.GetId().IsValid())
            {
                auto& assetManager = AZ::Data::AssetManager::Instance();
                m_runtimeAsset = assetManager.GetAsset(runtimeAsset.GetId(), m_runtimeAsset.GetType().IsNull() ? azrtti_typeid<RuntimeAsset>() : m_runtimeAsset.GetType(), m_runtimeAsset.GetAutoLoadBehavior());
                AZ::Data::AssetBus::Handler::BusConnect(m_runtimeAsset.GetId());
            }
        }
        else
        {
            m_runtimeAsset = runtimeAsset;
        }
    }

    AZStd::vector<RuntimeComponent*> RuntimeComponent::FindAllByEditorAssetId(AZ::Data::AssetId editorScriptCanvasAssetId)
    {
        AZStd::vector<RuntimeComponent*> result;
        AZ::Data::AssetId runtimeAssetId(editorScriptCanvasAssetId.m_guid, AZ_CRC("RuntimeData", 0x163310ae));
        AZ::Data::AssetBus::EnumerateHandlersId(runtimeAssetId,
            [&result](AZ::Data::AssetEvents* assetEvent) -> bool
            {
                if (auto runtimeComponent = azrtti_cast<ScriptCanvas::RuntimeComponent*>(assetEvent))
                {
                    result.push_back(runtimeComponent);
                }

                // Continue enumeration.
                return true;
            }
        );

        return result;
    }
    
    VariableId RuntimeComponent::FindAssetVariableIdByRuntimeVariableId(VariableId runtimeId) const
    {
        auto iter = m_runtimeToAssetVariableMap.find(runtimeId);
        if (iter != m_runtimeToAssetVariableMap.end())
        {
            return iter->second;
        }

        return VariableId{};
    }

    VariableId RuntimeComponent::FindRuntimeVariableIdByAssetVariableId(VariableId assetId) const
    {
        auto iter = m_assetToRuntimeVariableMap.find(assetId);
        if (iter != m_assetToRuntimeVariableMap.end())
        {
            return iter->second;
        }

        return VariableId{};
    }

    AZ::EntityId RuntimeComponent::FindAssetNodeIdByRuntimeNodeId(AZ::EntityId runtimeNodeId) const
    {
        auto iter = m_runtimeIdToAssetId.find(runtimeNodeId);

        if (iter != m_runtimeIdToAssetId.end())
        {
            return iter->second;
        }

        return AZ::EntityId{};
    }
    
    Node* RuntimeComponent::FindNode(AZ::EntityId nodeId) const
    {
        auto nodeIter = m_nodeLookupMap.find(nodeId);

        if (nodeIter == m_nodeLookupMap.end())
        {
            return nullptr;
        }

        return nodeIter->second;
    }

    AZ::EntityId RuntimeComponent::FindRuntimeNodeIdByAssetNodeId(AZ::EntityId editorNodeId) const
    {
        auto iter = m_assetIdToRuntimeId.find(editorNodeId);

        if (iter != m_assetIdToRuntimeId.end())
        {
            return iter->second;
        }

        return AZ::EntityId{};
    }

    GraphIdentifier RuntimeComponent::GetGraphIdentifier() const
    {
        GraphIdentifier identifier;
        identifier.m_assetId = AZ::Data::AssetId(GetAssetId().m_guid, 0);

        // For now we can gloss over multiple instances of the same graph running on the same entity.
        // Once this becomes a supported case, we can toggle back on the component Id, or whatever it is we used
        // to identify it.
        //identifier.m_componentId = GetId();
        identifier.m_componentId = 0;

        return identifier;
    }

    AZStd::string RuntimeComponent::GetAssetName() const
    {
        AZStd::string graphName;
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(graphName, &AZ::Data::AssetCatalogRequests::GetAssetPathById, m_runtimeAsset.GetId());
        return graphName;
    }
    
    AZStd::vector<AZ::EntityId> RuntimeComponent::GetNodes() const
    {
        AZStd::vector<AZ::EntityId> entityIds;
        for (auto& nodeRef : m_runtimeData.m_graphData.m_nodes)
        {
            entityIds.push_back(nodeRef->GetId());
        }

        return entityIds;
    }

    const AZStd::vector<AZ::EntityId> RuntimeComponent::GetNodesConst() const
    {
        return GetNodes();
    }

    AZStd::vector<AZ::EntityId> RuntimeComponent::GetConnections() const
    {
        AZStd::vector<AZ::EntityId> entityIds;
        for (auto& connectionRef : m_runtimeData.m_graphData.m_connections)
        {
            entityIds.push_back(connectionRef->GetId());
        }

        return entityIds;
    }

    AZStd::vector<Endpoint> RuntimeComponent::GetConnectedEndpoints(const Endpoint& endpoint) const
    {
        AZStd::vector<Endpoint> connectedEndpoints;
        auto otherEndpointsRange = m_runtimeData.m_graphData.m_endpointMap.equal_range(endpoint);
        for (auto otherIt = otherEndpointsRange.first; otherIt != otherEndpointsRange.second; ++otherIt)
        {
            connectedEndpoints.push_back(otherIt->second);
        }
        return connectedEndpoints;
    }

    AZStd::pair< EndpointMapConstIterator, EndpointMapConstIterator > RuntimeComponent::GetConnectedEndpointIterators(const Endpoint& endpoint) const
    {
        return m_runtimeData.m_graphData.m_endpointMap.equal_range(endpoint);
    }

    bool RuntimeComponent::IsEndpointConnected(const Endpoint& endpoint) const
    {
        size_t connectionCount = m_runtimeData.m_graphData.m_endpointMap.count(endpoint);
        return connectionCount > 0;
    }

    GraphData* RuntimeComponent::GetGraphData()
    {
        return &m_runtimeData.m_graphData;
    }

    VariableData* RuntimeComponent::GetVariableData()
    {
        return &m_runtimeData.m_variableData;
    }

    void RuntimeComponent::SetVariableOverrides(const VariableData& overrideData)
    {
        m_variableOverrides = overrideData;
    }

    void RuntimeComponent::InitializeVariableState()
    {
        for (auto& graphVariable : m_runtimeData.m_variableData.GetVariables())
        {
            GraphVariable* graphVariableOverride = m_variableOverrides.FindVariable(graphVariable.first);

            if (graphVariableOverride)
            {
                graphVariable.second = (*graphVariableOverride);
            }
            else
            {
                if (!graphVariable.second.IsInScope(ScriptCanvas::VariableFlags::Scope::InOut))
                {
                    // In the case of activation/deactivation we want to store our initial state to return to it.
                    m_internalVariables.AddVariable(graphVariable.second.GetVariableName(), graphVariable.second);
                }
            }

            graphVariable.second.SetOwningScriptCanvasId(m_scriptCanvasId);
        }
    }

    void RuntimeComponent::ResetState()
    {
        for (auto& graphVariable : m_variableOverrides.GetVariables())
        {
            GraphVariable* variable = m_runtimeData.m_variableData.FindVariable(graphVariable.first);

            if (variable)
            {
                (*variable) = graphVariable.second;
                variable->SetOwningScriptCanvasId(m_scriptCanvasId);
            }
        }

        for (auto& graphVariable : m_internalVariables.GetVariables())
        {
            GraphVariable* variable = m_runtimeData.m_variableData.FindVariable(graphVariable.first);

            if (variable)
            {
                (*variable) = graphVariable.second;
                variable->SetOwningScriptCanvasId(m_scriptCanvasId);
            }
        }
    }

    void RuntimeComponent::SetVariableEntityIdMap(const AZStd::unordered_map<AZ::u64, AZ::EntityId> variableEntityIdMap)
    {
        m_variableEntityIdMap = variableEntityIdMap;
    }

    const GraphVariableMapping* RuntimeComponent::GetVariables() const
    {
        return &m_runtimeData.m_variableData.GetVariables();
    }

    GraphVariable* RuntimeComponent::FindVariable(AZStd::string_view propName)
    {
        return m_runtimeData.m_variableData.FindVariable(propName);
    }

    GraphVariable* RuntimeComponent::FindVariableById(const VariableId& variableId)
    {
        auto& variableMap = m_runtimeData.m_variableData.GetVariables();
        auto foundIt = variableMap.find(variableId);
        return foundIt != variableMap.end() ? &foundIt->second : nullptr;
    }

    Data::Type RuntimeComponent::GetVariableType(const VariableId& variableId) const
    {
        auto& variableMap = m_runtimeData.m_variableData.GetVariables();
        auto foundIt = variableMap.find(variableId);
        return foundIt != variableMap.end() ? foundIt->second.GetDatum()->GetType() : Data::Type::Invalid();
    }

    AZStd::string_view RuntimeComponent::GetVariableName(const VariableId& variableId) const
    {
        auto& variableMap = m_runtimeData.m_variableData.GetVariables();
        auto foundIt = variableMap.find(variableId);
        return foundIt != variableMap.end() ? AZStd::string_view(foundIt->second.GetVariableName()) : "";
    }

    bool RuntimeComponent::IsGraphObserved() const
    {
        return m_isObserved;
    }

    void RuntimeComponent::SetIsGraphObserved(bool observed)
    {
        m_isObserved = observed;
    }

    AZ::Data::AssetType RuntimeComponent::GetAssetType() const
    {
        return m_runtimeAsset.GetType().IsNull() ? azrtti_typeid<RuntimeAsset>() : m_runtimeAsset.GetType();
    }

    void RuntimeComponent::OnVariableValueChanged()
    {
        // :SCTODO: What if clients/proxies attempt to change replicated Datums?
        if (m_graphVariableNetBindingTable)
        {
            GridMate::ReplicaChunkPtr netBinding = m_graphVariableNetBindingTable->GetNetworkBinding();
            if (netBinding && netBinding->IsMaster())
            {
                GraphScopedVariableId scopedVariableId = *(VariableNotificationBus::GetCurrentBusId());
                GraphVariable* graphVariable = FindVariableById(scopedVariableId.m_identifier);

                if (graphVariable)
                {
                    m_graphVariableNetBindingTable->OnDatumChanged(*graphVariable);
                }
            }
        }
    }

    GraphVariable* RuntimeComponent::GetVariable()
    {
        GraphVariable* graphVariable = nullptr;

        if (auto scopedVariableId = VariableRequestBus::GetCurrentBusId())
        {
            graphVariable = m_runtimeData.m_variableData.FindVariable(scopedVariableId->m_identifier);
        }

        return graphVariable;
    }

    Data::Type RuntimeComponent::GetType() const
    {
        const GraphVariable* graphVariable = nullptr;
        if (auto scopedVariableId = VariableRequestBus::GetCurrentBusId())
        {
            graphVariable = m_runtimeData.m_variableData.FindVariable(scopedVariableId->m_identifier);
        }

        return graphVariable ? graphVariable->GetDatum()->GetType() : Data::Type::Invalid();
    }

    AZStd::string_view RuntimeComponent::GetName() const
    {
        const GraphVariable* graphVariable = nullptr;
        if (auto scopedVariableId = VariableRequestBus::GetCurrentBusId())
        {
            graphVariable = m_runtimeData.m_variableData.FindVariable(scopedVariableId->m_identifier);
        }

        return graphVariable ? graphVariable->GetVariableName() : "";
    }

    AZ::Outcome<void, AZStd::string> RuntimeComponent::RenameVariable(AZStd::string_view newVarName)
    {
        const GraphScopedVariableId* scopedVariableId = VariableRequestBus::GetCurrentBusId();
        if (!scopedVariableId)
        {
            return AZ::Failure(AZStd::string::format("No variable id was found, cannot rename variable to %s", newVarName.data()));
        }

        VariableId variableId = scopedVariableId->m_identifier;

        auto varDatumPair = FindVariableById(variableId);

        if (!varDatumPair)
        {
            return AZ::Failure(AZStd::string::format("Unable to find variable with Id %s on Entity %s. Cannot rename",
                variableId.ToString().data(), GetEntityId().ToString().data()));
        }

        GraphVariable* graphVariable = FindVariable(newVarName);
        if (graphVariable && graphVariable->GetVariableId() != variableId)
        {
            return AZ::Failure(AZStd::string::format("A variable with name %s already exist on Entity %s. Cannot rename",
                newVarName.data(), GetEntityId().ToString().data()));
        }

        if (!m_runtimeData.m_variableData.RenameVariable(variableId, newVarName))
        {
            return AZ::Failure(AZStd::string::format("Unable to rename variable with id %s to %s.",
                variableId.ToString().data(), newVarName.data()));
        }

        VariableNotificationBus::Event((*scopedVariableId), &VariableNotifications::OnVariableRenamed, newVarName);

        return AZ::Success();
    }
}
