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

#include "FunctionNode.h"

#include <AzCore/Asset/AssetManager.h>

#include <ScriptCanvas/Asset/RuntimeAsset.h>
#include <ScriptCanvas/Core/GraphData.h>
#include <ScriptCanvas/Core/SlotNames.h>

#include <ScriptCanvas/Execution/RuntimeComponent.h>

#include <ScriptCanvas/Libraries/Core/MethodUtility.h>

#include <ScriptCanvas/Core/ModifiableDatumView.h>

#include <ScriptEvents/ScriptEventsBus.h>
#include <ScriptEvents/ScriptEventsAsset.h>

#include <ScriptCanvas/Variable/GraphVariableManagerComponent.h>
#include <ScriptCanvas/Variable/VariableData.h>
#include <ScriptCanvas/Variable/VariableBus.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Core
        {
            ////////////////////
            // DataSlotIdCache
            ////////////////////

            void FunctionNode::DataSlotIdCache::Reflect(AZ::ReflectContext* reflectContext)
            {
                AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext);

                if (serializeContext)
                {
                    serializeContext->Class<DataSlotIdCache>()
                        ->Field("InputSlot", &DataSlotIdCache::m_inputSlotId)
                        ->Field("OutputSlot", &DataSlotIdCache::m_outputSlotId)
                    ;
                }
            }

            /////////////////
            // FunctionNode
            /////////////////

            bool FunctionVersionConverter([[maybe_unused]] AZ::SerializeContext& serializeContext, AZ::SerializeContext::DataElementNode& rootElement)
            {
                rootElement.RemoveElementByName(AZ_CRC("m_runtimeAssetId", 0x82d37299));
                rootElement.RemoveElementByName(AZ_CRC("m_sourceAssetId", 0x7a4d00c9));

                return true;
            }

            FunctionNode::FunctionNode() 
                : m_asset(AZ::Data::AssetLoadBehavior::QueueLoad)
                , m_runtimeComponent(nullptr)
                , m_runtimeData(nullptr)
            {
            }

            void FunctionNode::OnInit()
            {
                if (m_asset.GetId().IsValid())
                {
                    Initialize(m_asset.GetId());
                }
            }

            void FunctionNode::OnInputSignal(const SlotId& entrySlotId)
            {
                if (!m_runtimeComponent)
                {
                    SCRIPTCANVAS_REPORT_ERROR((*this), "Could not call function (%s), this is usually because the game was started before a save operation completed.", GetNodeName().c_str());
                    return;
                }

                // Need to set what the entry point was, then activate
                m_runtimeComponent->Activate();

                auto scriptCanvasId = m_runtimeComponent->GetExecutionContext().GetScriptCanvasId();
                FunctionRequestBus::Handler::BusConnect(scriptCanvasId);

                if (!m_setupFunction)
                {
                    m_setupFunction = true;

                    // Need to rebuild it here because we clone the runtime data, so the entry points are stale
                    m_entryPoints.clear();
                    m_exitPoints.clear();

                    for (auto nodeEntity : m_runtimeComponent->GetGraphData()->m_nodes)
                    {
                        if (ExecutionNodeling* nodeling = azrtti_cast<ExecutionNodeling*>(nodeEntity->GetComponents()[0]))
                        {
                            ConnectionType connectionType = nodeling->GetConnectionType();

                            if (connectionType == ConnectionType::Input)
                            {
                                SlotId inputSlotId = m_executionSlotMapping[nodeling->GetIdentifier()];
                                m_entryPoints[inputSlotId] = nodeling;
                            }
                            else if (connectionType == ConnectionType::Output)
                            {
                                SlotId outputSlotId = m_executionSlotMapping[nodeling->GetIdentifier()];
                                m_exitPoints[nodeEntity->GetId()] = outputSlotId;
                            }
                        }
                    }

                    for (const Slot& slot : GetSlots())
                    {
                        if (slot.IsData() && slot.IsInput())
                        {
                            const Datum* data = FindDatum(slot.GetId());
                            GraphVariable* graphVariable = m_runtimeComponent->GetVariableData()->FindVariable(slot.GetName());

                            if (graphVariable)
                            {
                                m_inputSlots[slot.GetId()] = graphVariable;

                                ModifiableDatumView datumView;

                                graphVariable->ConfigureDatumView(datumView);

                                if (datumView.IsValid())
                                {
                                    datumView.AssignToDatum((*data));
                                }
                            }
                        }
                        else if (slot.IsData() && slot.IsOutput())
                        {
                            GraphVariable* graphVariable = m_runtimeComponent->GetVariableData()->FindVariable(slot.GetName());

                            if (graphVariable)
                            {
                                m_outputSlots[slot.GetId()] = graphVariable;
                            }
                        }
                    }
                }
                else
                {
                    for (const auto& slotIdPair : m_inputSlots)
                    {
                        const Datum* data = FindDatum(slotIdPair.first);
                        
                        ModifiableDatumView datumView;
                        slotIdPair.second->ConfigureDatumView(datumView);

                        if (datumView.IsValid())
                        {
                            datumView.AssignToDatum((*data));
                        }
                    }
                }

                auto entryNodeIter = m_entryPoints.find(entrySlotId);

                if (entryNodeIter != m_entryPoints.end())
                {
                    if (ExecutionNodeling* executionNode = azrtti_cast<ExecutionNodeling*>(entryNodeIter->second))
                    {
                        executionNode->SignalEntrySlots();
                    }
                }
            }

            void FunctionNode::PushDataOut()
            {
                for (const auto& slotPair : m_outputSlots)
                {
                    const Slot* slot = GetSlot(slotPair.first);
                    PushOutput((*slotPair.second->GetDatum()), *slot);
                }
            }

            void FunctionNode::OnSignalOut(ID nodeId, [[maybe_unused]] SlotId slotId)
            {
                auto slotIter = m_exitPoints.find(nodeId);

                if (slotIter != m_exitPoints.end())
                {
                    SignalOutput(slotIter->second);

                    if (!m_runtimeComponent->GetExecutionContext().HasQueuedExecution())
                    {
                        CompleteDeactivation();
                    }
                }
            }

            void FunctionNode::ConfigureNode(const AZ::Data::AssetId& assetId)
            {
                if (assetId.IsValid())
                {
                    AZ::Data::AssetBus::Handler::BusConnect(assetId);
                    GraphRequestBus::Event(GetOwningScriptCanvasId(), &GraphRequests::AddDependentAsset, GetEntityId(), azrtti_typeid<ScriptCanvasFunctionAsset>(), assetId);
                }

                PopulateNodeType();
            }

            FunctionNode::~FunctionNode()
            {
                AZ::EntityBus::Handler::BusDisconnect();
                FunctionRequestBus::Handler::BusDisconnect();
                AZ::Data::AssetBus::Handler::BusDisconnect();
            }

            RuntimeFunctionAsset* FunctionNode::GetAsset() const
            {
                return m_asset.GetAs<RuntimeFunctionAsset>();
            }

            AZ::Data::AssetId FunctionNode::GetAssetId() const
            {
                return m_asset.GetId();
            }

            void FunctionNode::BuildNode()
            {
                AZ::Data::Asset<ScriptCanvas::RuntimeFunctionAsset> asset = AZ::Data::AssetManager::Instance().GetAsset<ScriptCanvas::RuntimeFunctionAsset>(m_asset.GetId(), AZ::Data::AssetLoadBehavior::Default);
                asset.BlockUntilLoadComplete();
                
                m_executionSlotMapping.clear();
                m_dataSlotMapping.clear();

                BuildNodeImpl(asset, m_executionSlotMapping, m_dataSlotMapping);
            }

            void FunctionNode::BuildNodeImpl(const AZ::Data::Asset<ScriptCanvas::RuntimeFunctionAsset>& runtimeAsset, ExecutionSlotMap& executionMapping, DataSlotMap& dataSlotMapping)
            {
                const ScriptCanvas::FunctionRuntimeData& graphData = runtimeAsset.Get()->m_runtimeData;

                AZStd::vector<const ScriptCanvas::Nodes::Core::ExecutionNodeling*> nodes;

                AZStd::unordered_map<AZ::EntityId, const ScriptCanvas::Nodes::Core::ExecutionNodeling* > nodelingMapping;

                for (auto node : graphData.m_graphData.m_nodes)
                {
                    ScriptCanvas::Nodes::Core::ExecutionNodeling* executionNode = node->FindComponent<ScriptCanvas::Nodes::Core::ExecutionNodeling>();
                    if (executionNode)
                    {
                        nodelingMapping[node->GetId()] = executionNode;
                    }
                }

                int slotOffset = 0;

                for (const AZ::EntityId& entityId : graphData.m_executionNodeOrder)
                {
                    auto executionIter = nodelingMapping.find(entityId);

                    if (executionIter != nodelingMapping.end())
                    {
                        const ScriptCanvas::Nodes::Core::ExecutionNodeling* executionNode = executionIter->second;

                        ConnectionType connectionType = executionNode->GetConnectionType();

                        if (connectionType == ConnectionType::Unknown)
                        {
                            continue;
                        }

                        AZ::Uuid executionKeyId = executionNode->GetIdentifier();

                        bool isNewSlot = true;
                        ExecutionSlotConfiguration slotConfiguration(executionNode->GetDisplayName(), connectionType);

                        slotConfiguration.m_addUniqueSlotByNameAndType = false;

                        auto slotIter = m_executionSlotMapping.find(executionKeyId);

                        if (slotIter != m_executionSlotMapping.end())
                        {
                            slotConfiguration.m_slotId = slotIter->second;
                            isNewSlot = false;
                        }

                        SlotId slotId = InsertSlot(slotOffset, slotConfiguration, isNewSlot);

                        executionMapping[executionKeyId] = slotId;

                        ++slotOffset;
                    }
                }

                for (const VariableId& variableId : graphData.m_variableOrder)
                {
                    const GraphVariable* variable = graphData.m_variableData.FindVariable(variableId);

                    if (variable == nullptr)
                    {
                        return;
                    }

                    DataSlotIdCache cache;

                    auto slotIter = m_dataSlotMapping.find(variableId);

                    if (variable->IsInScope(VariableFlags::Scope::Input))
                    {
                        bool isNewSlot = true;

                        DataSlotConfiguration slotConfiguration(variable->GetDataType(), variable->GetVariableName(), ConnectionType::Input);

                        if (slotIter != m_dataSlotMapping.end())
                        {
                            if (slotIter->second.m_inputSlotId.IsValid())
                            {
                                slotConfiguration.m_slotId = slotIter->second.m_inputSlotId;
                                isNewSlot = false;
                            }
                        }

                        cache.m_inputSlotId = InsertSlot(slotOffset, slotConfiguration, isNewSlot);

                        if (isNewSlot)
                        {
                            if (ScriptCanvas::Data::IsValueType(variable->GetDataType()))
                            {
                                ModifiableDatumView modifiableDatumView;

                                FindModifiableDatumView(cache.m_inputSlotId, modifiableDatumView);

                                if (modifiableDatumView.IsValid())
                                {
                                    modifiableDatumView.HardCopyDatum((*variable->GetDatum()));
                                }
                            }
                        }

                        ++slotOffset;
                    }

                    if (variable->IsInScope(VariableFlags::Scope::Output))
                    {
                        bool isNewSlot = true;

                        DataSlotConfiguration slotConfiguration(variable->GetDataType(), variable->GetVariableName(), ConnectionType::Output);

                        if (slotIter != m_dataSlotMapping.end())
                        {
                            if (slotIter->second.m_outputSlotId.IsValid())
                            {
                                slotConfiguration.m_slotId = slotIter->second.m_outputSlotId;
                                isNewSlot = false;
                            }
                        }

                        cache.m_outputSlotId = InsertSlot(slotOffset, slotConfiguration, isNewSlot);
                        ++slotOffset;
                    }
                    
                    dataSlotMapping[variable->GetVariableId()] = cache;
                }

                m_savedFunctionVersion = runtimeAsset.Get()->GetData().m_version;

                SignalSlotsReordered();
            }

            void FunctionNode::Initialize(AZ::Data::AssetId assetId)
            {
                ConfigureNode(assetId);

                if (assetId.IsValid())
                {
                    m_asset = AZ::Data::AssetManager::Instance().GetAsset<RuntimeFunctionAsset>(assetId, m_asset.GetAutoLoadBehavior());
                    m_asset.BlockUntilLoadComplete();
                }
            }

            bool FunctionNode::IsOutOfDate() const
            {
                if (!m_asset)
                {
                    return true;
                }

                return m_savedFunctionVersion != m_asset.Get()->GetData().m_version;
            }

            UpdateResult FunctionNode::OnUpdateNode()
            {
                AZ::Data::Asset<RuntimeFunctionAsset> assetData = AZ::Data::AssetManager::Instance().GetAsset<RuntimeFunctionAsset>(m_asset.GetId(), m_asset.GetAutoLoadBehavior());

                assetData.BlockUntilLoadComplete();

                if (assetData.IsError())
                {
                    return UpdateResult::DeleteNode;
                }

                for (auto executionPair : m_executionSlotMapping)
                {
                    const bool removeConnections = false;
                    RemoveSlot(executionPair.second, removeConnections);
                }

                for (auto dataPair : m_dataSlotMapping)
                {
                    const bool removeConnections = false;

                    if (dataPair.second.m_inputSlotId.IsValid())
                    {
                        RemoveSlot(dataPair.second.m_inputSlotId, removeConnections);
                    }

                    if (dataPair.second.m_outputSlotId.IsValid())
                    {
                        RemoveSlot(dataPair.second.m_outputSlotId, removeConnections);
                    }
                }

                ExecutionSlotMap executionMapping;
                DataSlotMap dataMapping;

                BuildNodeImpl(assetData, executionMapping, dataMapping);

                m_executionSlotMapping = AZStd::move(executionMapping);
                m_dataSlotMapping = AZStd::move(dataMapping);
                m_asset = assetData;

                return UpdateResult::DirtyGraph;
            }

            AZStd::string FunctionNode::GetUpdateString() const
            {
                if (m_asset)
                {
                    return AZStd::string::format("Updated Function (%s)", GetName().c_str());
                }
                else
                {
                    return AZStd::string::format("Removed Function (%s)", m_asset.GetId().ToString<AZStd::string>().c_str());
                }
            }

            void FunctionNode::CompleteDeactivation()
            {
                if (m_runtimeComponent)
                {
                    auto scriptCanvasId = m_runtimeComponent->GetExecutionContext().GetScriptCanvasId();
                    if (FunctionRequestBus::Handler::BusIsConnectedId(scriptCanvasId))
                    {
                        FunctionRequestBus::Handler::BusDisconnect(scriptCanvasId);
                    }
                    PushDataOut();
                    m_runtimeComponent->Deactivate();
                }
            }

            void FunctionNode::OnDeactivate()
            {
                CompleteDeactivation();
            }

            void FunctionNode::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
            {
                m_asset = asset;

                if (m_executionContextEntity)
                {
                    AZ::EntityBus::Handler::BusDisconnect(m_executionContextEntity->GetId());
                }
                m_executionContextEntity = AZStd::make_unique<AZ::Entity>();
                // Listen for entity destruction events as it is possible for the Component
                // Application to destroy this entity in Destroy
                AZ::EntityBus::Handler::BusConnect(m_executionContextEntity->GetId());
                m_runtimeComponent = m_executionContextEntity->CreateComponent<RuntimeComponent>();
                m_executionContextEntity->Init();

                m_runtimeComponent->SetRuntimeAsset(m_asset);
                m_runtimeData = &m_asset.Get()->m_runtimeData;

                AZ::Data::Asset<RuntimeFunctionAsset> assetData = AZ::Data::AssetManager::Instance().GetAsset<RuntimeFunctionAsset>(asset.GetId(), AZ::Data::AssetLoadBehavior::Default);
                m_prettyName = assetData.Get()->m_runtimeData.m_name;
            }

            void FunctionNode::OnEntityDestruction(const AZ::EntityId& entityId)
            {
                // Disconnect from the Entity destruction and release the m_executionContextEntity
                // as it will be deleted by the remaining logic in the AZ::Entity destructor
                AZ::EntityBus::Handler::BusDisconnect(entityId);
                m_executionContextEntity.release();
                m_runtimeComponent = {};
                m_runtimeData = {};
                m_prettyName = AZStd::string{};
            }
        }
    }
}
