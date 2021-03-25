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
#include <ScriptCanvas/Core/ModifiableDatumView.h>
#include <ScriptCanvas/Core/ScriptCanvasBus.h>
#include <ScriptCanvas/Core/SlotNames.h>
#include <ScriptCanvas/Core/SubgraphInterfaceUtility.h>
#include <ScriptCanvas/Execution/RuntimeComponent.h>
#include <ScriptCanvas/Libraries/Core/MethodUtility.h>
#include <ScriptCanvas/Utils/VersionConverters.h>
#include <ScriptCanvas/Variable/GraphVariableManagerComponent.h>
#include <ScriptCanvas/Variable/VariableBus.h>
#include <ScriptCanvas/Variable/VariableData.h>
#include <ScriptEvents/ScriptEventsAsset.h>
#include <ScriptEvents/ScriptEventsBus.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Core
        {
            /////////////////
            // FunctionNode
            /////////////////

            FunctionNode::FunctionNode() 
                : m_asset(AZ::Data::AssetLoadBehavior::QueueLoad)
            {}

            FunctionNode::~FunctionNode()
            {
                AZ::Data::AssetBus::Handler::BusDisconnect();
            }

            SlotExecution::In FunctionNode::AddExecutionInSlotFromInterface(const Grammar::In& in, int slotOffset, SlotId previousSlotId)
            {
                ExecutionSlotConfiguration config;
                config.m_name = in.displayName;
                config.m_displayGroup = in.displayName;
                config.SetConnectionType(ScriptCanvas::ConnectionType::Input);
                config.m_isLatent = false;
                if (previousSlotId.IsValid())
                {
                    config.m_slotId = previousSlotId;
                }

                SlotExecution::In slotMapIn;
                slotMapIn.slotId = InsertSlot(slotOffset, config, !previousSlotId.IsValid());
                slotMapIn.parsedName = in.parsedName;
                slotMapIn.interfaceSourceId = in.sourceID;
                return slotMapIn;
            }

            SlotExecution::Out FunctionNode::AddExecutionOutSlotFromInterface(const Grammar::In& in, const Grammar::Out& out, int slotOffset, SlotId previousSlotId)
            {
                ExecutionSlotConfiguration config;
                config.m_name = out.displayName;
                config.m_displayGroup = in.displayName;
                config.SetConnectionType(ScriptCanvas::ConnectionType::Output);
                config.m_isLatent = false;
                if (previousSlotId.IsValid())
                {
                    config.m_slotId = previousSlotId;
                }

                SlotExecution::Out slotMapOut;
                slotMapOut.slotId = InsertSlot(slotOffset, config, !previousSlotId.IsValid());
                slotMapOut.interfaceSourceId = out.sourceID;
                slotMapOut.name = out.displayName;
                return slotMapOut;
            }

            SlotExecution::Out FunctionNode::AddExecutionLatentOutSlotFromInterface(const Grammar::Out& latent, int slotOffset, SlotId previousSlotId)
            {
                ExecutionSlotConfiguration config;
                config.m_name = latent.displayName;
                config.m_displayGroup = latent.displayName;
                config.SetConnectionType(ScriptCanvas::ConnectionType::Output);
                config.m_isLatent = true;
                if (previousSlotId.IsValid())
                {
                    config.m_slotId = previousSlotId;
                }

                SlotExecution::Out slotMapLatentOut;
                slotMapLatentOut.slotId = InsertSlot(slotOffset, config, !previousSlotId.IsValid());
                slotMapLatentOut.name = latent.displayName;
                slotMapLatentOut.interfaceSourceId = latent.sourceID;
                return slotMapLatentOut;
            }

            SlotExecution::Inputs FunctionNode::AddDataInputSlotFromInterface(const Grammar::Inputs& inputs, const Grammar::FunctionSourceId& inSourceId, const AZStd::string& displayGroup, const SlotExecution::Map& previousMap, int& slotOffset)
            {
                SlotExecution::Inputs slotMapInputs;
                for (const auto& input : inputs)
                {
                    DataSlotConfiguration config;
                    config.m_name = input.displayName;
                    config.m_displayGroup = displayGroup;
                    config.m_addUniqueSlotByNameAndType = false;
                    config.SetConnectionType(ScriptCanvas::ConnectionType::Input);
                    config.DeepCopyFrom(input.datum);
                    auto previousSlotId = previousMap.FindInputSlotIdBySource(input.sourceID, inSourceId);
                    if (previousSlotId.IsValid())
                    {
                        config.m_slotId = previousSlotId;
                    }

                    SlotExecution::Input slotMapInput;
                    slotMapInput.slotId = InsertSlot(slotOffset++, config, !previousSlotId.IsValid());
                    slotMapInput.interfaceSourceId = input.sourceID;
                    slotMapInputs.push_back(slotMapInput);
                    if (!slotMapInput.slotId.IsValid())
                    {
                        return slotMapInputs;
                    }
                }
                return slotMapInputs;
            }

            SlotExecution::Outputs FunctionNode::AddDataOutputSlotFromInterface(const Grammar::Outputs& outputs, const AZStd::string& displayGroup, const SlotExecution::Map& previousMap, int& slotOffset)
            {
                AZ_UNUSED(displayGroup);

                SlotExecution::Outputs slotMapOutputs;
                for (const auto& output : outputs)
                {
                    DataSlotConfiguration config;
                    config.m_name = output.displayName;
                    config.m_displayGroup = "(shared across all execution for now)"; // displayGroup
                    config.SetConnectionType(ScriptCanvas::ConnectionType::Output);
                    config.SetType(output.type);
                    auto previousSlotId = previousMap.FindOutputSlotIdBySource(output.sourceID);
                    if (previousSlotId.IsValid())
                    {
                        config.m_slotId = previousSlotId;
                    }

                    SlotExecution::Output outputSlotMap = InsertSlot(slotOffset++, config, !previousSlotId.IsValid());
                    outputSlotMap.interfaceSourceId = output.sourceID;
                    slotMapOutputs.push_back(outputSlotMap);
                    if (!outputSlotMap.slotId.IsValid())
                    {
                        return slotMapOutputs;
                    }
                }
                return slotMapOutputs;
            }

            AZ::Outcome<Grammar::LexicalScope, void> FunctionNode::GetFunctionCallLexicalScope(const Slot* /*slot*/) const
            {
                return AZ::Success(m_slotExecutionMapSourceInterface.GetLexicalScope());
            }

            AZ::Outcome<AZStd::string, void> FunctionNode::GetFunctionCallName(const Slot* slot) const
            {
                if (auto in = m_slotExecutionMap.GetIn(slot->GetId()))
                {
                    return AZ::Success(in->parsedName);
                }
                else
                {
                    return AZ::Failure();
                }
            }

            AZStd::string FunctionNode::GetInterfaceName() const
            {
                return m_slotExecutionMapSourceInterface.GetName();
            }

            bool FunctionNode::IsNodeableNode() const
            {
                return !IsPure();
            }

            bool FunctionNode::IsPure() const
            {
                return m_slotExecutionMapSourceInterface.IsMarkedPure();
            }

            bool FunctionNode::IsSlotPure(const Slot* /*slot*/) const
            {
                // \todo optimizations are possible based on treating the slots separately
                return m_slotExecutionMapSourceInterface.IsMarkedPure();
            }

            void FunctionNode::OnInit()
            {
                if (m_asset.GetId().IsValid())
                {
                    Initialize(m_asset.GetId());
                }
            }

            void FunctionNode::ConfigureNode(const AZ::Data::AssetId&)
            {
                PopulateNodeType();
            }

            SubgraphInterfaceAsset* FunctionNode::GetAsset() const
            {
                return m_asset.GetAs<SubgraphInterfaceAsset>();
            }

            AZ::Data::AssetId FunctionNode::GetAssetId() const
            {
                return m_asset.GetId();
            }

            AZ::Outcome<DependencyReport, void> FunctionNode::GetDependencies() const
            {
                DependencyReport report;
                report.userSubgraphs.insert(m_slotExecutionMapSourceInterface.GetNamespacePath());
                report.userSubgraphAssetIds.insert(m_asset.GetId());
                return AZ::Success(AZStd::move(report));
            }

            const AZStd::string& FunctionNode::GetName() const
            {
                return m_prettyName;
            }

            void FunctionNode::BuildNode()
            {
                AZ::Data::Asset<ScriptCanvas::SubgraphInterfaceAsset> asset = AZ::Data::AssetManager::Instance().GetAsset<ScriptCanvas::SubgraphInterfaceAsset>(m_asset.GetId(), AZ::Data::AssetLoadBehavior::PreLoad);
                m_slotExecutionMapSourceInterface = Grammar::SubgraphInterface{};
                m_slotExecutionMap = SlotExecution::Map{};
                BuildNodeFromSubgraphInterface(asset, m_slotExecutionMap);
            }

            void FunctionNode::BuildNodeFromSubgraphInterface
                ( const AZ::Data::Asset<ScriptCanvas::SubgraphInterfaceAsset>& runtimeAsset
                , const SlotExecution::Map& previousMap)
            {
                // build the node here, from the asset topology, take the node/variable ordering from the function runtime data as a suggestion
                // deal with updates and conversions after
                const Grammar::SubgraphInterface& subgraphInterface = runtimeAsset.Get()->m_runtimeData.m_interface;
                m_prettyName = runtimeAsset.Get()->m_runtimeData.m_name;

                if (!subgraphInterface.IsAllInputOutputShared())
                {
                    AZ_Error("ScriptCanvas", false, "the current assumption is that there is no way to distinguish between the input/output of different nodelings");
                    return;
                }

                // for now, all outputs are shared
                Grammar::Outputs outputs;
                bool sharedOutputInitialized = false;

                SlotExecution::Ins slotMapIns;
                SlotExecution::Outs slotMapLatents;

                int slotOffset = 0;
                
                // add all ins->outs, in their display groups
                for (size_t indexIn = 0; indexIn < subgraphInterface.GetInCount(); ++indexIn)
                {
                    const Grammar::In& interfaceIn = subgraphInterface.GetIn(indexIn);

                    SlotExecution::In slotMapIn = AddExecutionInSlotFromInterface(interfaceIn, slotOffset++, previousMap.FindInSlotIdBySource(interfaceIn.sourceID));
                    if (!slotMapIn.slotId.IsValid())
                    {
                        AZ_Error("ScriptCanvas", false, "Failed to add Execution In slot from sub graph interface");
                        return;
                    }
                    slotMapIn.inputs = AddDataInputSlotFromInterface(interfaceIn.inputs, interfaceIn.sourceID, interfaceIn.displayName, previousMap, slotOffset);
                    for (auto& input : slotMapIn.inputs)
                    {
                        if (!input.slotId.IsValid())
                        {
                            AZ_Error("ScriptCanvas", false, "Failed to add Input slot from sub graph interface");
                            return;
                        }
                    }

                    for (auto& interfaceOut : interfaceIn.outs)
                    {
                        SlotExecution::Out slotMapOut = AddExecutionOutSlotFromInterface(interfaceIn, interfaceOut, slotOffset++, previousMap.FindOutSlotIdBySource(interfaceIn.sourceID, interfaceOut.sourceID));
                        if (!slotMapOut.slotId.IsValid())
                        {
                            AZ_Error("ScriptCanvas", false, "Failed to add Execution Out slot from sub graph interface");
                            return;
                        }
                        if (!sharedOutputInitialized)
                        {
                            outputs = interfaceOut.outputs;
                            sharedOutputInitialized = true;
                        }

                        slotMapIn.outs.push_back(slotMapOut);
                    }

                    slotMapIns.push_back(slotMapIn);
                }

                // add all latents in their display groups
                for (size_t indexLatent = 0; indexLatent < subgraphInterface.GetLatentOutCount(); ++indexLatent)
                {
                    const Grammar::Out& interfaceLatent = subgraphInterface.GetLatentOut(indexLatent);

                    SlotExecution::Out slotMapLatentOut = AddExecutionLatentOutSlotFromInterface(interfaceLatent, slotOffset++, previousMap.FindLatentSlotIdBySource(interfaceLatent.sourceID));
                    if (!slotMapLatentOut.slotId.IsValid())
                    {
                        AZ_Error("ScriptCanvas", false, "Failed to add Latent Out slot from sub graph interface");
                        return;
                    }
                    slotMapLatentOut.returnValues.values = AddDataInputSlotFromInterface(interfaceLatent.returnValues, interfaceLatent.sourceID, interfaceLatent.displayName, previousMap, slotOffset);
                    for (auto& input : slotMapLatentOut.returnValues.values)
                    {
                        if (!input.slotId.IsValid())
                        {
                            AZ_Error("ScriptCanvas", false, "Failed to add Input slot from sub graph interface");
                            return;
                        }
                    }

                    if (!sharedOutputInitialized)
                    {
                        outputs = interfaceLatent.outputs;
                        sharedOutputInitialized = true;
                    }

                    slotMapLatents.push_back(slotMapLatentOut);
                }

                // add all outputs one time, since they are currently all required to be part of all the signatures [\todo must fix] , in a variable display group
                SlotExecution::Outputs slotMapOutputs = AddDataOutputSlotFromInterface(outputs, "", previousMap, slotOffset);
                for (auto& output : slotMapOutputs)
                {
                    if (!output.slotId.IsValid())
                    {
                        AZ_Error("ScriptCanvas", false, "Failed to add Output slot from sub graph interface");
                        return;
                    }
                }
                if (!subgraphInterface.IsLatent())
                {
                    for (auto& slotMapIn : slotMapIns)
                    {
                        for (auto& slotMapOut : slotMapIn.outs)
                        {
                            slotMapOut.outputs = slotMapOutputs;
                        }
                    }
                }
                else
                {
                    for (auto& slotMapLatent : slotMapLatents)
                    {
                        slotMapLatent.outputs = slotMapOutputs;
                    }
                }
                
                // when returning variables: sort variables by source slot id, they are sorted in the slot map, so just take them from the slot map
                m_slotExecutionMap = AZStd::move(SlotExecution::Map(AZStd::move(slotMapIns), AZStd::move(slotMapLatents)));
                m_slotExecutionMapSourceInterface = subgraphInterface;
                m_asset = runtimeAsset;
                SignalSlotsReordered();
            }

            void FunctionNode::Initialize(AZ::Data::AssetId assetId)
            {
                ConfigureNode(assetId);

                static bool blockingLoad = true;
                // this is the only case where the subgraph id should not be modified
                AZ::Data::AssetId interfaceAssetId(assetId.m_guid, AZ_CRC("SubgraphInterface", 0xdfe6dc72));
                auto asset = AZ::Data::AssetManager::Instance().GetAsset<SubgraphInterfaceAsset>(interfaceAssetId, AZ::Data::AssetLoadBehavior::PreLoad);
                asset.BlockUntilLoadComplete();
                
                if (asset)
                {
                    // do not nuke the assetId in case an update will be attempted immediately after this call
                    m_asset = asset;
                }
            }

            bool FunctionNode::IsOutOfDate(const VersionData& graphVersion) const
            {
                bool isUnitTestingInProgress = false;
                ScriptCanvas::SystemRequestBus::BroadcastResult(isUnitTestingInProgress, &ScriptCanvas::SystemRequests::IsScriptUnitTestingInProgress);

                if (isUnitTestingInProgress)
                {
                    return false;
                }

                if (graphVersion.grammarVersion == GrammarVersion::Initial || graphVersion.runtimeVersion == RuntimeVersion::Initial)
                {
                    return true;
                }

                // #conversion_diagnostic
                AZ::Data::AssetId interfaceAssetId(m_asset.GetId().m_guid, AZ_CRC("SubgraphInterface", 0xdfe6dc72));
                if (interfaceAssetId != m_asset.GetId())
                {
                    AZ_Warning("ScriptCanvas", false, "FunctionNode %s wasn't saved out with the proper sub id", m_prettyName.data());
                }

                AZ::Data::Asset<SubgraphInterfaceAsset> asset = AZ::Data::AssetManager::Instance().GetAsset<SubgraphInterfaceAsset>(interfaceAssetId, AZ::Data::AssetLoadBehavior::PreLoad);
                asset.BlockUntilLoadComplete();

                if (!asset || !asset->IsReady())
                {
                    AZ_Warning("ScriptCanvas", false, "FunctionNode %s failed to load source asset.", m_prettyName.data());
                    return true;
                }

                const Grammar::SubgraphInterface* latestAssetInterface = asset ? &asset.Get()->GetData().m_interface : nullptr;

                if (!latestAssetInterface)
                {
                    AZ_Warning("ScriptCanvas", false, "FunctionNode %s failed to load latest interface from the source asset.", m_prettyName.data());
                    return true;
                }

                if (!(m_slotExecutionMapSourceInterface == *latestAssetInterface))
                {
                    return true;
                }

                return false;
            }

            UpdateResult FunctionNode::OnUpdateNode()
            {
                AZ::Data::AssetId interfaceAssetId(m_asset.GetId().m_guid, AZ_CRC("SubgraphInterface", 0xdfe6dc72));
                AZ::Data::Asset<SubgraphInterfaceAsset> assetData = AZ::Data::AssetManager::Instance().GetAsset<SubgraphInterfaceAsset>(interfaceAssetId, AZ::Data::AssetLoadBehavior::PreLoad);
                assetData.BlockUntilLoadComplete();

                if (!assetData || !assetData->IsReady())
                {
                    AZ_Warning("ScriptCanvas", false, "FunctionNode %s failed to load source asset, likely removed.", m_prettyName.data());
                    this->AddNodeDisabledFlag(NodeDisabledFlag::ErrorInUpdate);
                    return UpdateResult::DisableNode;
                }

                // connections will be removed when the version conversion is finalized after this function returns
                const bool k_DoNotRemoveConnections = false;
                const bool k_DoNotWarnOnMissingDataSlots = !m_slotExecutionMapSourceInterface.IsAllInputOutputShared();

                ExecutionSlotMap executionSlotMap;
                DataSlotMap dataSlotMap;
                if (m_slotExecutionMap.IsEmpty())
                {
                    const Grammar::SubgraphInterface& subgraphInterface = assetData.Get()->m_runtimeData.m_interface;
                    RemoveInsFromInterface(subgraphInterface.GetIns(), executionSlotMap, dataSlotMap, k_DoNotRemoveConnections, k_DoNotWarnOnMissingDataSlots);
                    RemoveOutsFromInterface(subgraphInterface.GetLatentOuts(), executionSlotMap, dataSlotMap, k_DoNotRemoveConnections, k_DoNotWarnOnMissingDataSlots);
                }
                else
                {
                    RemoveInsFromSlotExecution(m_slotExecutionMap.GetIns(), k_DoNotRemoveConnections, k_DoNotWarnOnMissingDataSlots);
                    RemoveOutsFromSlotExecution(m_slotExecutionMap.GetLatents(), k_DoNotRemoveConnections, k_DoNotWarnOnMissingDataSlots);
                }

                BuildNodeFromSubgraphInterface(assetData, m_slotExecutionMap);
                SanityCheckSlotsAndConnections(executionSlotMap, dataSlotMap);

                this->RemoveNodeDisabledFlag(NodeDisabledFlag::ErrorInUpdate);
                return UpdateResult::DirtyGraph;
            }

            void FunctionNode::RemoveInsFromInterface(const Grammar::Ins& ins,
                ExecutionSlotMap& executionSlotMap,
                DataSlotMap& dataSlotMap, bool removeConnection, bool warnOnMissingSlot)
            {
                for (auto& in : ins)
                {
                    if (auto inSlot = this->GetSlotByNameAndType(in.displayName, CombinedSlotType::ExecutionIn))
                    {
                        executionSlotMap.emplace(in.sourceID, inSlot->GetId());
                        RemoveSlot(inSlot->GetId(), removeConnection);

                        RemoveInputsFromInterface(in.inputs, dataSlotMap, removeConnection, warnOnMissingSlot);
                        RemoveOutsFromInterface(in.outs, executionSlotMap, dataSlotMap, removeConnection, warnOnMissingSlot);
                    }
                }
            }

            void FunctionNode::RemoveInsFromSlotExecution(const SlotExecution::Ins& ins, bool removeConnection, bool warnOnMissingSlot)
            {
                for (auto& in : ins)
                {
                    RemoveSlot(in.slotId, removeConnection);

                    RemoveInputsFromSlotExecution(in.inputs, removeConnection, warnOnMissingSlot);
                    RemoveOutsFromSlotExecution(in.outs, removeConnection, warnOnMissingSlot);
                }
            }

            void FunctionNode::RemoveInputsFromInterface(const Grammar::Inputs& inputs,
                DataSlotMap& dataSlotMap, bool removeConnection, bool warnOnMissingSlot)
            {
                for (auto& input : inputs)
                {
                    if (auto inputSlot = this->GetSlotByNameAndType(input.displayName, CombinedSlotType::DataIn))
                    {
                        DataSlotCache dataSlotCache;
                        dataSlotCache.m_slotId = inputSlot->GetId();
                        if (inputSlot->IsVariableReference())
                        {
                            dataSlotCache.m_variableReference = inputSlot->GetVariableReference();
                        }
                        else if (auto inputDatum = inputSlot->FindDatum())
                        {
                            dataSlotCache.m_datum.DeepCopyDatum(*inputDatum);
                        }

                        dataSlotMap.emplace(input.sourceID, dataSlotCache);
                        RemoveSlot(inputSlot->GetId(), removeConnection, warnOnMissingSlot);
                    }
                }
            }

            void FunctionNode::RemoveInputsFromSlotExecution(const SlotExecution::Inputs& inputs, bool removeConnection, bool warnOnMissingSlot)
            {
                for (auto& input : inputs)
                {
                    RemoveSlot(input.slotId, removeConnection, warnOnMissingSlot);
                }
            }

            void FunctionNode::RemoveOutsFromInterface(const Grammar::Outs& outs, ExecutionSlotMap& executionSlotMap, DataSlotMap& dataSlotMap, bool removeConnection, bool warnOnMissingSlot)
            {
                for (auto& out : outs)
                {
                    if (auto inSlot = this->GetSlotByNameAndType(out.displayName, CombinedSlotType::ExecutionOut))
                    {
                        executionSlotMap.emplace(out.sourceID, inSlot->GetId());
                        RemoveSlot(inSlot->GetId(), removeConnection);

                        RemoveInputsFromInterface(out.returnValues, dataSlotMap, removeConnection, warnOnMissingSlot);
                        RemoveOutputsFromInterface(out.outputs, dataSlotMap, removeConnection, warnOnMissingSlot);
                    }
                }
            }

            void FunctionNode::RemoveOutsFromSlotExecution(const SlotExecution::Outs& outs, bool removeConnection, bool warnOnMissingSlot)
            {
                for (auto& out : outs)
                {
                    RemoveSlot(out.slotId, removeConnection);

                    RemoveInputsFromSlotExecution(out.returnValues.values, removeConnection, warnOnMissingSlot);
                    RemoveOutputsFromSlotExecution(out.outputs, removeConnection, warnOnMissingSlot);
                }
            }

            void FunctionNode::RemoveOutputsFromInterface(const Grammar::Outputs& outputs, DataSlotMap& dataSlotMap, bool removeConnection, bool warnOnMissingSlot)
            {
                for (auto& output : outputs)
                {
                    if (auto outputSlot = this->GetSlotByNameAndType(output.displayName, CombinedSlotType::DataOut))
                    {
                        DataSlotCache dataSlotCache;
                        dataSlotCache.m_slotId = outputSlot->GetId();
                        if (outputSlot->IsVariableReference())
                        {
                            dataSlotCache.m_variableReference = outputSlot->GetVariableReference();
                        }
                        else if (auto outputDatum = outputSlot->FindDatum())
                        {
                            dataSlotCache.m_datum.DeepCopyDatum(*outputDatum);
                        }

                        dataSlotMap.emplace(output.sourceID, dataSlotCache);
                        RemoveSlot(outputSlot->GetId(), removeConnection, warnOnMissingSlot);
                    }
                }
            }

            void FunctionNode::RemoveOutputsFromSlotExecution(const SlotExecution::Outputs& outputs, bool removeConnection, bool warnOnMissingSlot)
            {
                for (auto& output : outputs)
                {
                    RemoveSlot(output.slotId, removeConnection, warnOnMissingSlot);
                }
            }

            void FunctionNode::SanityCheckSlotsAndConnections(const ExecutionSlotMap& executionSlotMap, const DataSlotMap& dataSlotMap)
            {
                auto graph = this->GetGraph();
                if (graph)
                {
                    ReplacementConnectionMap connectionMap;
                    SanityCheckInSlotsAndConnections(*graph, m_slotExecutionMap.GetIns(), executionSlotMap, dataSlotMap, connectionMap);
                    SanityCheckOutSlotsAndConnections(*graph, m_slotExecutionMap.GetLatents(), executionSlotMap, dataSlotMap, connectionMap);
                    for (auto connectionIter : connectionMap)
                    {
                        // need to remove old connection, or create new connection will fail because it triggers sanity check on removed slot
                        graph->RemoveConnection(connectionIter.first);

                        for (auto newEndpointPair : connectionIter.second)
                        {
                            if (newEndpointPair.first.IsValid() && newEndpointPair.second.IsValid())
                            {
                                graph->ConnectByEndpoint(newEndpointPair.first, newEndpointPair.second);
                            }
                        }
                    }
                }
            }

            void FunctionNode::SanityCheckInSlotsAndConnections(const Graph& graph, const SlotExecution::Ins& ins,
                const ExecutionSlotMap& executionSlotMap,
                const DataSlotMap& dataSlotMap,
                ReplacementConnectionMap& connectionMap)
            {
                if (!executionSlotMap.empty())
                {
                    for (auto& in : ins)
                    {
                        auto executionSlotIter = executionSlotMap.find(in.interfaceSourceId);
                        if (executionSlotIter != executionSlotMap.end())
                        {
                            if (in.slotId != executionSlotIter->second)
                            {
                                VersioningUtils::CreateRemapConnectionsForTargetEndpoint(graph, { this->GetEntityId(), executionSlotIter->second }, { this->GetEntityId(), in.slotId }, connectionMap);
                            }
                        }

                        SanityCheckInputSlotsAndConnections(graph, in.inputs, dataSlotMap, connectionMap);
                        SanityCheckOutSlotsAndConnections(graph, in.outs, executionSlotMap, dataSlotMap, connectionMap);
                    }
                }
            }

            void FunctionNode::SanityCheckInputSlotsAndConnections(const Graph& graph, const SlotExecution::Inputs& inputs,
                const DataSlotMap& dataSlotMap,
                ReplacementConnectionMap& connectionMap)
            {
                if (!dataSlotMap.empty())
                {
                    for (auto& input : inputs)
                    {
                        auto dataSlotIter = dataSlotMap.find(input.interfaceSourceId);
                        if (dataSlotIter != dataSlotMap.end())
                        {
                            if (input.slotId != dataSlotIter->second.m_slotId)
                            {
                                VersioningUtils::CopyOldValueToDataSlot(this->GetSlot(input.slotId), dataSlotIter->second.m_variableReference, &dataSlotIter->second.m_datum);
                                VersioningUtils::CreateRemapConnectionsForTargetEndpoint(graph, { this->GetEntityId(), dataSlotIter->second.m_slotId }, { this->GetEntityId(), input.slotId }, connectionMap);
                            }
                        }
                    }
                }
            }

            void FunctionNode::SanityCheckOutSlotsAndConnections(const Graph& graph, const SlotExecution::Outs& outs,
                const ExecutionSlotMap& executionSlotMap,
                const DataSlotMap& dataSlotMap,
                ReplacementConnectionMap& connectionMap)
            {
                if (!executionSlotMap.empty())
                {
                    for (auto& out : outs)
                    {
                        auto executionSlotIter = executionSlotMap.find(out.interfaceSourceId);
                        if (executionSlotIter != executionSlotMap.end())
                        {
                            if (out.slotId != executionSlotIter->second)
                            {
                                VersioningUtils::CreateRemapConnectionsForSourceEndpoint(graph, { this->GetEntityId(), executionSlotIter->second }, { this->GetEntityId(), out.slotId }, connectionMap);
                            }
                        }

                        SanityCheckInputSlotsAndConnections(graph, out.returnValues.values, dataSlotMap, connectionMap);
                        SanityCheckOutputSlotsAndConnections(graph, out.outputs, dataSlotMap, connectionMap);
                    }
                }
            }

            void FunctionNode::SanityCheckOutputSlotsAndConnections(const Graph& graph, const SlotExecution::Outputs& outputs,
                const DataSlotMap& dataSlotMap,
                ReplacementConnectionMap& connectionMap)
            {
                if (!dataSlotMap.empty())
                {
                    for (auto& output : outputs)
                    {
                        auto dataSlotIter = dataSlotMap.find(output.interfaceSourceId);
                        if (dataSlotIter != dataSlotMap.end())
                        {
                            if (output.slotId != dataSlotIter->second.m_slotId)
                            {
                                VersioningUtils::CopyOldValueToDataSlot(this->GetSlot(output.slotId), dataSlotIter->second.m_variableReference, &dataSlotIter->second.m_datum);
                                VersioningUtils::CreateRemapConnectionsForSourceEndpoint(graph, { this->GetEntityId(), dataSlotIter->second.m_slotId }, { this->GetEntityId(), output.slotId }, connectionMap);
                            }
                        }
                    }
                }
            }

            const SlotExecution::Map* FunctionNode::GetSlotExecutionMap() const
            {
                return &m_slotExecutionMap;
            }

            const Grammar::SubgraphInterface* FunctionNode::GetSubgraphInterface() const
            {
                return &m_slotExecutionMapSourceInterface;
            }

            AZStd::string FunctionNode::GetUpdateString() const
            {
                if (m_asset)
                {
                    return AZStd::string::format("Updated Function (%s)", GetName().c_str());
                }
                else
                {
                    return AZStd::string::format("Disabled Function (%s)", m_asset.GetId().ToString<AZStd::string>().c_str());
                }
            }

            void FunctionNode::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
            {
                AZ::Data::AssetId interfaceAssetId(m_asset.GetId().m_guid, AZ_CRC("SubgraphInterface", 0xdfe6dc72));
                m_asset = asset;
                AZ::Data::Asset<SubgraphInterfaceAsset> assetData = AZ::Data::AssetManager::Instance().GetAsset<SubgraphInterfaceAsset>(interfaceAssetId, AZ::Data::AssetLoadBehavior::PreLoad);

                if (!assetData)
                {
                    AZ_TracePrintf("SC", "Asset data unavailable in OnAssetReady");
                    return;
                }

                m_prettyName = assetData.Get()->m_runtimeData.m_name;
            }
        }
    }
}
