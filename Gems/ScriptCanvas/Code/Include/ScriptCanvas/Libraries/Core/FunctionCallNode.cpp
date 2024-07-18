/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "FunctionCallNode.h"

#include <AzCore/Asset/AssetManager.h>
#include <ScriptCanvas/Asset/RuntimeAsset.h>
#include <ScriptCanvas/Core/GraphData.h>
#include <ScriptCanvas/Core/ModifiableDatumView.h>
#include <ScriptCanvas/Core/ScriptCanvasBus.h>
#include <ScriptCanvas/Core/SlotNames.h>
#include <ScriptCanvas/Core/SubgraphInterfaceUtility.h>
#include <ScriptCanvas/Execution/RuntimeComponent.h>
#include <ScriptCanvas/Utils/VersionConverters.h>
#include <ScriptCanvas/Variable/GraphVariableManagerComponent.h>
#include <ScriptCanvas/Variable/VariableBus.h>
#include <ScriptCanvas/Variable/VariableData.h>
#include <ScriptEvents/ScriptEventsAsset.h>
#include <ScriptEvents/ScriptEventsBus.h>
#include <ScriptCanvas/Core/SlotConfigurations.h>

#include "FunctionCallNodeIsOutOfDate.h"

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Core
        {
            /////////////////
            // FunctionCallNode
            /////////////////

            FunctionCallNode::FunctionCallNode() 
                : m_asset(AZ::Data::AssetLoadBehavior::NoLoad)
            {}

            FunctionCallNode::~FunctionCallNode()
            {
                AZ::Data::AssetBus::Handler::BusDisconnect();
            }

            SlotExecution::In FunctionCallNode::AddAllSlots(const Grammar::In& interfaceIn, int& slotOffset, const SlotExecution::Map& previousMap)
            {
                SlotExecution::In slotMapIn = AddExecutionInSlotFromInterface(interfaceIn, slotOffset, previousMap.FindInSlotIdBySource(interfaceIn.sourceID));
                ++slotOffset;
                if (!slotMapIn.slotId.IsValid())
                {
                    AZ_Error("ScriptCanvas", false, "Failed to add Execution In slot from sub graph interface");
                }

                slotMapIn.inputs = AddDataInputSlotsFromInterface(interfaceIn.inputs, interfaceIn.sourceID, interfaceIn.displayName, previousMap, slotOffset);
                for (auto& input : slotMapIn.inputs)
                {
                    if (!input.slotId.IsValid())
                    {
                        AZ_Error("ScriptCanvas", false, "Failed to add Input slot from sub graph interface");
                        break;;
                    }
                }

                for (auto& interfaceOut : interfaceIn.outs)
                {
                    slotMapIn.outs.push_back(AddAllSlots(interfaceIn, interfaceOut, slotOffset, previousMap));
                }

                return slotMapIn;
            }

            SlotExecution::Out FunctionCallNode::AddAllSlots(const Grammar::In& interfaceIn, const Grammar::Out& interfaceOut, int& slotOffset, const SlotExecution::Map& previousMap)
            {
                SlotExecution::Out slotMapOut = AddExecutionOutSlotFromInterface(interfaceIn, interfaceOut, slotOffset, previousMap.FindOutSlotIdBySource(interfaceIn.sourceID, interfaceOut.sourceID));
                ++slotOffset;

                if (!slotMapOut.slotId.IsValid())
                {
                    AZ_Error("ScriptCanvas", false, "Failed to add Execution Out slot from sub graph interface");
                }

                slotMapOut.outputs = AddDataOutputSlotsFromInterface(interfaceOut.outputs, "", previousMap, slotOffset);
                for (auto& output : slotMapOut.outputs)
                {
                    if (!output.slotId.IsValid())
                    {
                        AZ_Error("ScriptCanvas", false, "Failed to add Output slot from sub graph interface");
                        break;
                    }
                }

                return slotMapOut;
            }

            SlotExecution::Out FunctionCallNode::AddAllSlots(const Grammar::Out& interfaceLatent, int& slotOffset, const SlotExecution::Map& previousMap)
            {
                SlotExecution::Out slotMapLatentOut = AddExecutionLatentOutSlotFromInterface(interfaceLatent, slotOffset, previousMap.FindLatentSlotIdBySource(interfaceLatent.sourceID));
                ++slotOffset;

                if (!slotMapLatentOut.slotId.IsValid())
                {
                    AZ_Error("ScriptCanvas", false, "Failed to add Latent Out slot from sub graph interface");
                }

                slotMapLatentOut.returnValues.values = AddDataInputSlotsFromInterface(interfaceLatent.returnValues, interfaceLatent.sourceID, interfaceLatent.displayName, previousMap, slotOffset);
                for (auto& input : slotMapLatentOut.returnValues.values)
                {
                    if (!input.slotId.IsValid())
                    {
                        AZ_Error("ScriptCanvas", false, "Failed to add Input slot from sub graph interface");
                        break;
                    }
                }

                slotMapLatentOut.outputs = AddDataOutputSlotsFromInterface(interfaceLatent.outputs, "", previousMap, slotOffset);
                for (auto& output : slotMapLatentOut.outputs)
                {
                    if (!output.slotId.IsValid())
                    {
                        AZ_Error("ScriptCanvas", false, "Failed to add Output slot from sub graph interface");
                        break;
                    }
                }

                return slotMapLatentOut;
            }

            SlotExecution::In FunctionCallNode::AddExecutionInSlotFromInterface(const Grammar::In& in, int slotOffset, SlotId previousSlotId)
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

            SlotExecution::Out FunctionCallNode::AddExecutionOutSlotFromInterface(const Grammar::In& in, const Grammar::Out& out, int slotOffset, SlotId previousSlotId)
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

            SlotExecution::Out FunctionCallNode::AddExecutionLatentOutSlotFromInterface(const Grammar::Out& latent, int slotOffset, SlotId previousSlotId)
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

            SlotExecution::Inputs FunctionCallNode::AddDataInputSlotsFromInterface(const Grammar::Inputs& inputs, const Grammar::FunctionSourceId& inSourceId, const AZStd::string& displayGroup, const SlotExecution::Map& previousMap, int& slotOffset)
            {
                SlotExecution::Inputs slotMapInputs;
                for (const auto& input : inputs)
                {
                    DataSlotConfiguration config;
                    config.m_name = input.displayName;
                    config.m_displayGroup = displayGroup;
                    config.m_addUniqueSlotByNameAndType = false;
                    config.SetConnectionType(ScriptCanvas::ConnectionType::Input);
                    // For current use case, we don't need to deep copy datum from subgraphinterface
                    // Later if we need to make deep copy, we must verify the subgraphinterface is accurate.
                    // For example, when subgraphinterface shows datum as dynamic, we should create slot by using DynamicDataSlotConfiguration
                    config.CopyTypeAndValueFrom(input.datum);
                    auto previousSlotId = previousMap.FindInputSlotIdBySource(input.sourceID, inSourceId);
                    if (previousSlotId.IsValid())
                    {
                        config.m_slotId = previousSlotId;
                    }

                    SlotExecution::Input slotMapInput;
                    slotMapInput.slotId = InsertSlot(slotOffset, config, !previousSlotId.IsValid());
                    ++slotOffset;
                    slotMapInput.interfaceSourceId = input.sourceID;
                    slotMapInputs.push_back(slotMapInput);
                    if (!slotMapInput.slotId.IsValid())
                    {
                        return slotMapInputs;
                    }
                }
                return slotMapInputs;
            }

            SlotExecution::Outputs FunctionCallNode::AddDataOutputSlotsFromInterface(const Grammar::Outputs& outputs, const AZStd::string& displayGroup, const SlotExecution::Map& previousMap, int& slotOffset)
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

                    SlotExecution::Output outputSlotMap = InsertSlot(slotOffset, config, !previousSlotId.IsValid());
                    ++slotOffset;
                    outputSlotMap.interfaceSourceId = output.sourceID;
                    slotMapOutputs.push_back(outputSlotMap);
                    if (!outputSlotMap.slotId.IsValid())
                    {
                        return slotMapOutputs;
                    }
                }
                return slotMapOutputs;
            }


            void FunctionCallNode::BuildNode()
            {
                AZ::Data::Asset<ScriptCanvas::SubgraphInterfaceAsset> asset = AZ::Data::AssetManager::Instance().GetAsset<ScriptCanvas::SubgraphInterfaceAsset>(m_asset.GetId(), AZ::Data::AssetLoadBehavior::PreLoad);
                m_slotExecutionMapSourceInterface = Grammar::SubgraphInterface{};
                m_slotExecutionMap = SlotExecution::Map{};
                BuildNodeFromSubgraphInterface(asset, m_sourceId, m_slotExecutionMap);
            }

            void FunctionCallNode::BuildNodeFromSubgraphInterface
                ( const AZ::Data::Asset<ScriptCanvas::SubgraphInterfaceAsset>& runtimeAsset
                , const ScriptCanvas::Grammar::FunctionSourceId& sourceId
                , const SlotExecution::Map& previousMap)
            {
                const Grammar::SubgraphInterface& subgraphInterface = runtimeAsset.Get()->m_interfaceData.m_interface;

                if (subgraphInterface.IsUserNodeable() && Grammar::IsFunctionSourceIdNodeable(sourceId) && subgraphInterface.HasIn(sourceId))
                {
                    m_prettyName = runtimeAsset.Get()->m_interfaceData.m_name;
                    BuildUserNodeableNode(subgraphInterface, previousMap);
                }
                else if ((!Grammar::IsFunctionSourceIdNodeable(sourceId)) && subgraphInterface.HasIn(sourceId))
                {
                    BuildUserFunctionCallNode(subgraphInterface, sourceId, previousMap);
                }

                m_slotExecutionMapSourceInterface = subgraphInterface;
                m_asset = runtimeAsset;
                m_asset.SetAutoLoadBehavior(AZ::Data::AssetLoadBehavior::NoLoad);
                m_sourceId = sourceId;
                SignalSlotsReordered();
            }

            void FunctionCallNode::BuildUserFunctionCallNode
                ( const Grammar::SubgraphInterface& subgraphInterface
                , const ScriptCanvas::Grammar::FunctionSourceId& sourceId
                , const SlotExecution::Map& previousMap)
            {
                if (auto interfaceIn = subgraphInterface.FindIn(sourceId))
                {
                    int slotOffset = 0;
                    auto in = AddAllSlots(*interfaceIn, slotOffset, previousMap);
                    // #functions2 FunctionCallNode cleanup, naming: always have the two names...file name for the title bar, In name for the method/file name for the object
                    m_prettyName = interfaceIn->displayName;

                    SlotExecution::Ins slotMapIns;
                    slotMapIns.push_back(in);
                    SlotExecution::Outs slotMapLatents;
                    m_slotExecutionMap = AZStd::move(SlotExecution::Map(AZStd::move(slotMapIns), AZStd::move(slotMapLatents)));
                }
                else
                {
                    AZ_Error("ScriptCanvas", false, "Failed to add Execution In slot from sub graph interface, source id was missing");
                }
            }

            void FunctionCallNode::BuildUserNodeableNode(const Grammar::SubgraphInterface& subgraphInterface, const SlotExecution::Map& previousMap)
            {
                SlotExecution::Ins slotMapIns;
                SlotExecution::Outs slotMapLatents;
                int slotOffset = 0;

                for (size_t indexIn = 0; indexIn < subgraphInterface.GetInCount(); ++indexIn)
                {
                    const auto& in = subgraphInterface.GetIn(indexIn);

                    if (!in.isPure)
                    {
                        slotMapIns.push_back(AddAllSlots(subgraphInterface.GetIn(indexIn), slotOffset, previousMap));
                    }
                }

                for (size_t indexLatent = 0; indexLatent < subgraphInterface.GetLatentOutCount(); ++indexLatent)
                {
                    slotMapLatents.push_back(AddAllSlots(subgraphInterface.GetLatentOut(indexLatent), slotOffset, previousMap));
                }

                // when returning variables: sort variables by source slot id, they are sorted in the slot map, so just take them from the slot map
                m_slotExecutionMap = AZStd::move(SlotExecution::Map(AZStd::move(slotMapIns), AZStd::move(slotMapLatents)));
            }

            AZ::Outcome<Grammar::LexicalScope, void> FunctionCallNode::GetFunctionCallLexicalScope(const Slot* slot) const
            {
                if (slot)
                {
                    if (const auto slotIn = m_slotExecutionMap.GetIn(slot->GetId()))
                    {
                        if (slotIn->interfaceSourceId == m_sourceId)
                        {
                            if (const auto in = m_slotExecutionMapSourceInterface.FindIn(slotIn->interfaceSourceId))
                            {
                                return AZ::Success(m_slotExecutionMapSourceInterface.GetLexicalScope(*in));
                            }
                        }
                    }
                }

                return AZ::Success(m_slotExecutionMapSourceInterface.GetLexicalScope());
            }

            AZ::Outcome<AZStd::string, void> FunctionCallNode::GetFunctionCallName(const Slot* slot) const
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

            AZ::Outcome<AZStd::string, AZStd::string> FunctionCallNode::GetInterfaceNameFromAssetOrLastSave() const
            {
                if (auto subgraphInterface = GetSubgraphInterface())
                {
                    if (auto latestName = subgraphInterface->GetName(); latestName.IsSuccess())
                    {
                        return latestName;
                    }
                }

                if (auto savedName = m_slotExecutionMapSourceInterface.GetName(); savedName.IsSuccess())
                {
                    return savedName;
                }

                return AZ::Failure(AZStd::string("all interface names were empty"));
            }

            bool FunctionCallNode::IsEntryPoint() const
            {
                return m_slotExecutionMapSourceInterface.IsActiveDefaultObject()
                    || m_slotExecutionMapSourceInterface.IsLatent(); 
            }

            bool FunctionCallNode::IsNodeableNode() const
            {
                return m_slotExecutionMapSourceInterface.IsUserNodeable() && Grammar::IsFunctionSourceIdNodeable(m_sourceId) ;
            }

            bool FunctionCallNode::IsPure() const
            {
                auto inSlots = GetSlotsByType(CombinedSlotType::ExecutionIn);
                return inSlots.size() == 1 && IsSlotPure(inSlots.front());
            }

            bool FunctionCallNode::IsSlotPure(const Slot* slot) const
            {
                auto slotMapIn = slot ? m_slotExecutionMap.GetIn(slot->GetId()) : nullptr;
                auto in = slotMapIn ? m_slotExecutionMapSourceInterface.FindIn(slotMapIn->interfaceSourceId) : nullptr;
                return in && in->isPure;
            }

            void FunctionCallNode::OnInit()
            {
                if (m_asset.GetId().IsValid())
                {
                    Initialize(m_asset.GetId(), m_sourceId);
                }
            }

            SubgraphInterfaceAsset* FunctionCallNode::GetAsset() const
            {
                return m_asset.GetAs<SubgraphInterfaceAsset>();
            }

            AZ::Data::AssetId FunctionCallNode::GetAssetId() const
            {
                return m_asset.GetId();
            }

            const AZStd::string& FunctionCallNode::GetAssetHint() const
            {
                return m_asset.GetHint();
            }

            AZ::Outcome<DependencyReport, void> FunctionCallNode::GetDependencies() const
            {
                DependencyReport report;
                report.userSubgraphs.insert(m_slotExecutionMapSourceInterface.GetNamespacePath());
                report.userSubgraphAssetIds.insert(m_asset.GetId());
                return AZ::Success(AZStd::move(report));
            }

            const AZStd::string& FunctionCallNode::GetName() const
            {
                return m_prettyName;
            }

            Grammar::FunctionSourceId FunctionCallNode::GetSourceId() const
            {
                return m_sourceId;
            }

            void FunctionCallNode::Initialize(AZ::Data::AssetId assetId, const ScriptCanvas::Grammar::FunctionSourceId& sourceId)
            {
                PopulateNodeType();

                // this is the only case where the subgraph id should not be modified
                AZ::Data::AssetId interfaceAssetId(assetId.m_guid, AZ_CRC_CE("SubgraphInterface"));
                auto asset = AZ::Data::AssetManager::Instance().GetAsset<SubgraphInterfaceAsset>(interfaceAssetId, AZ::Data::AssetLoadBehavior::PreLoad);
                asset.BlockUntilLoadComplete();
                
                if (asset)
                {
                    // do not nuke the assetId in case an update will be attempted immediately after this call
                    m_asset = asset;
                    m_asset.SetAutoLoadBehavior(AZ::Data::AssetLoadBehavior::NoLoad);
                    m_sourceId = sourceId;
                }
            }

            bool FunctionCallNode::IsOutOfDate(const VersionData& graphVersion) const
            {
                if (graphVersion.grammarVersion < GrammarVersion::Current || graphVersion.runtimeVersion < RuntimeVersion::Current)
                {
                    return true;
                }

                FunctionCallNodeCompareConfig config;
                return IsOutOfDate(config, {}) != IsFunctionCallNodeOutOfDateResult::No;
            }

            IsFunctionCallNodeOutOfDateResult FunctionCallNode::IsOutOfDate(const FunctionCallNodeCompareConfig& config, const AZ::Uuid& graphId) const
            {
                bool isUnitTestingInProgress = false;
                ScriptCanvas::SystemRequestBus::BroadcastResult(isUnitTestingInProgress, &ScriptCanvas::SystemRequests::IsScriptUnitTestingInProgress);
                if (isUnitTestingInProgress)
                {
                    return IsFunctionCallNodeOutOfDateResult::No;
                }

                if ((!graphId.IsNull()) && graphId == m_asset.GetId().m_guid)
                {
                    return IsFunctionCallNodeOutOfDateResult::EvaluateAfterLocalDefinition;
                }

                AZ::Data::AssetId interfaceAssetId(m_asset.GetId().m_guid, AZ_CRC_CE("SubgraphInterface"));
                if (interfaceAssetId != m_asset.GetId())
                {
                    AZ_Warning("ScriptCanvas", false, "FunctionCallNode %s wasn't saved out with the proper sub id", m_prettyName.data());
                }

                AZ::Data::Asset<SubgraphInterfaceAsset> asset = AZ::Data::AssetManager::Instance().GetAsset<SubgraphInterfaceAsset>(interfaceAssetId, AZ::Data::AssetLoadBehavior::PreLoad);
                asset.BlockUntilLoadComplete();

                if (!asset || !asset->IsReady())
                {
                    AZ_Warning("ScriptCanvas", false, "FunctionCallNode %s failed to load source asset.", m_prettyName.data());
                    return IsFunctionCallNodeOutOfDateResult::Yes;
                }

                const Grammar::SubgraphInterface* latestAssetInterface = asset ? &asset.Get()->m_interfaceData.m_interface : nullptr;
                if (!latestAssetInterface)
                {
                    AZ_Warning("ScriptCanvas", false, "FunctionCallNode %s failed to load latest interface from the source asset.", m_prettyName.data());
                    return IsFunctionCallNodeOutOfDateResult::Yes;
                }

                IsFunctionCallOutOfDateConfig isOutOfDateConfig{ config, *this, m_slotExecutionMap, m_sourceId, m_slotExecutionMapSourceInterface, *latestAssetInterface };
                return IsFunctionCallNodeOutOfDate(isOutOfDateConfig) ? IsFunctionCallNodeOutOfDateResult::Yes : IsFunctionCallNodeOutOfDateResult::No;
            }

            UpdateResult FunctionCallNode::OnUpdateNode()
            {
                AZ::Data::AssetId interfaceAssetId(m_asset.GetId().m_guid, AZ_CRC_CE("SubgraphInterface"));
                AZ::Data::Asset<SubgraphInterfaceAsset> assetData = AZ::Data::AssetManager::Instance().GetAsset<SubgraphInterfaceAsset>(interfaceAssetId, AZ::Data::AssetLoadBehavior::PreLoad);
                assetData.BlockUntilLoadComplete();

                if (!assetData || !assetData->IsReady())
                {
                    AZ_Warning("ScriptCanvas", false, "FunctionCallNode %s failed to load source asset, likely removed.", m_prettyName.data());
                    this->AddNodeDisabledFlag(NodeDisabledFlag::ErrorInUpdate);
                    return UpdateResult::DisableNode;
                }

                FunctionCallNodeCompareConfig config;
                if (IsOutOfDate(config, m_asset.GetId().m_guid) != Nodes::Core::IsFunctionCallNodeOutOfDateResult::No)
                {
                    AZ_Warning("ScriptCanvas", false, "FunctionCallNode %s's source public interface has changed", m_prettyName.data());
                    this->AddNodeDisabledFlag(NodeDisabledFlag::ErrorInUpdate);
                    return UpdateResult::DisableNode;
                }

                // connections will be removed when the version conversion is finalized after this function returns
                const bool k_DoNotRemoveConnections = false;
                const bool k_DoNotWarnOnMissingDataSlots = false;

                ExecutionSlotMap executionSlotMap;
                DataSlotMap dataSlotMap;
                if (m_slotExecutionMap.IsEmpty())
                {
                    const Grammar::SubgraphInterface& subgraphInterface = assetData.Get()->m_interfaceData.m_interface;
                    RemoveInsFromInterface(subgraphInterface.GetIns(), executionSlotMap, dataSlotMap, k_DoNotRemoveConnections, k_DoNotWarnOnMissingDataSlots);
                    RemoveOutsFromInterface(subgraphInterface.GetLatentOuts(), executionSlotMap, dataSlotMap, k_DoNotRemoveConnections, k_DoNotWarnOnMissingDataSlots);
                }
                else
                {
                    RemoveInsFromSlotExecution(m_slotExecutionMap.GetIns(), k_DoNotRemoveConnections, k_DoNotWarnOnMissingDataSlots);
                    RemoveOutsFromSlotExecution(m_slotExecutionMap.GetLatents(), k_DoNotRemoveConnections, k_DoNotWarnOnMissingDataSlots);
                }

                BuildNodeFromSubgraphInterface(assetData, m_sourceId, m_slotExecutionMap);
                SanityCheckSlotsAndConnections(executionSlotMap, dataSlotMap);

                this->RemoveNodeDisabledFlag(NodeDisabledFlag::ErrorInUpdate);
                return UpdateResult::DirtyGraph;
            }

            void FunctionCallNode::RemoveInsFromInterface
                ( const Grammar::Ins& ins
                , ExecutionSlotMap& executionSlotMap
                , DataSlotMap& dataSlotMap
                , bool removeConnection
                , bool warnOnMissingSlot)
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

            void FunctionCallNode::RemoveInsFromSlotExecution(const SlotExecution::Ins& ins, bool removeConnection, bool warnOnMissingSlot)
            {
                for (auto& in : ins)
                {
                    RemoveSlot(in.slotId, removeConnection);

                    RemoveInputsFromSlotExecution(in.inputs, removeConnection, warnOnMissingSlot);
                    RemoveOutsFromSlotExecution(in.outs, removeConnection, warnOnMissingSlot);
                }
            }

            void FunctionCallNode::RemoveInputsFromInterface
                ( const Grammar::Inputs& inputs
                , DataSlotMap& dataSlotMap
                , bool removeConnection
                , bool warnOnMissingSlot)
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

            void FunctionCallNode::RemoveInputsFromSlotExecution(const SlotExecution::Inputs& inputs, bool removeConnection, bool warnOnMissingSlot)
            {
                for (auto& input : inputs)
                {
                    RemoveSlot(input.slotId, removeConnection, warnOnMissingSlot);
                }
            }

            void FunctionCallNode::RemoveOutsFromInterface(const Grammar::Outs& outs, ExecutionSlotMap& executionSlotMap, DataSlotMap& dataSlotMap, bool removeConnection, bool warnOnMissingSlot)
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

            void FunctionCallNode::RemoveOutsFromSlotExecution(const SlotExecution::Outs& outs, bool removeConnection, bool warnOnMissingSlot)
            {
                for (auto& out : outs)
                {
                    RemoveSlot(out.slotId, removeConnection);

                    RemoveInputsFromSlotExecution(out.returnValues.values, removeConnection, warnOnMissingSlot);
                    RemoveOutputsFromSlotExecution(out.outputs, removeConnection, warnOnMissingSlot);
                }
            }

            void FunctionCallNode::RemoveOutputsFromInterface(const Grammar::Outputs& outputs, DataSlotMap& dataSlotMap, bool removeConnection, bool warnOnMissingSlot)
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

            void FunctionCallNode::RemoveOutputsFromSlotExecution(const SlotExecution::Outputs& outputs, bool removeConnection, bool warnOnMissingSlot)
            {
                for (auto& output : outputs)
                {
                    RemoveSlot(output.slotId, removeConnection, warnOnMissingSlot);
                }
            }

            void FunctionCallNode::SanityCheckSlotsAndConnections(const ExecutionSlotMap& executionSlotMap, const DataSlotMap& dataSlotMap)
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

            void FunctionCallNode::SanityCheckInSlotsAndConnections(const Graph& graph, const SlotExecution::Ins& ins,
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

            void FunctionCallNode::SanityCheckInputSlotsAndConnections(const Graph& graph, const SlotExecution::Inputs& inputs,
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

            void FunctionCallNode::SanityCheckOutSlotsAndConnections(const Graph& graph, const SlotExecution::Outs& outs,
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

            void FunctionCallNode::SanityCheckOutputSlotsAndConnections(const Graph& graph, const SlotExecution::Outputs& outputs,
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

            const SlotExecution::Map* FunctionCallNode::GetSlotExecutionMap() const
            {
                return &m_slotExecutionMap;
            }

            const Grammar::SubgraphInterface& FunctionCallNode::GetSlotExecutionMapSource() const
            {
                return m_slotExecutionMapSourceInterface;
            }

            const Grammar::SubgraphInterface* FunctionCallNode::GetSubgraphInterface() const
            {
                if (m_asset && m_asset.Get())
                {
                    return &m_asset.Get()->m_interfaceData.m_interface;
                }

                return nullptr;
            }

            AZStd::string FunctionCallNode::GetUpdateString() const
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

            void FunctionCallNode::OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset)
            {
                AZ::Data::AssetId interfaceAssetId(m_asset.GetId().m_guid, AZ_CRC_CE("SubgraphInterface"));
                m_asset = asset;
                AZ::Data::Asset<SubgraphInterfaceAsset> assetData = AZ::Data::AssetManager::Instance().GetAsset<SubgraphInterfaceAsset>(interfaceAssetId, AZ::Data::AssetLoadBehavior::PreLoad);
                m_asset.SetAutoLoadBehavior(AZ::Data::AssetLoadBehavior::NoLoad);

                if (!assetData)
                {
                    AZ_TracePrintf("SC", "Asset data unavailable in OnAssetReady");
                    return;
                }

                m_prettyName = assetData.Get()->m_interfaceData.m_name;
            }
        }
    }
}
