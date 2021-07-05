/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "PrimitivesDeclarations.h"

#if defined(FUNCTION_LEGACY_SUPPORT_ENABLED)

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <ScriptCanvas/Asset/RuntimeAsset.h>
#include <ScriptCanvas/Core/Datum.h>
#include <ScriptCanvas/Core/EBusHandler.h>
#include <ScriptCanvas/Core/Graph.h>
#include <ScriptCanvas/Core/GraphData.h>
#include <ScriptCanvas/Core/ModifiableDatumView.h>
#include <ScriptCanvas/Core/Nodeable.h>
#include <ScriptCanvas/Core/NodeableNode.h>
#include <ScriptCanvas/Core/NodeableNodeOverloaded.h>
#include <ScriptCanvas/Core/ScriptCanvasBus.h>
#include <ScriptCanvas/Core/SlotNames.h>
#include <ScriptCanvas/Core/SubgraphInterfaceUtility.h>
#include <ScriptCanvas/Debugger/ValidationEvents/DataValidation/ScopedDataConnectionEvent.h>
#include <ScriptCanvas/Debugger/ValidationEvents/ParsingValidation/ParsingValidations.h>
#include <ScriptCanvas/Execution/RuntimeComponent.h>
#include <ScriptCanvas/Grammar/ParsingMetaData.h>
#include <ScriptCanvas/Libraries/Core/AzEventHandler.h>
#include <ScriptCanvas/Libraries/Core/EBusEventHandler.h>
#include <ScriptCanvas/Libraries/Core/FunctionDefinitionNode.h>
#include <ScriptCanvas/Libraries/Core/ExtractProperty.h>
#include <ScriptCanvas/Libraries/Core/ForEach.h>
#include <ScriptCanvas/Libraries/Core/FunctionCallNode.h>
#include <ScriptCanvas/Libraries/Core/Method.h>
#include <ScriptCanvas/Libraries/Core/MethodUtility.h>
#include <ScriptCanvas/Libraries/Core/Start.h>
#include <ScriptCanvas/Translation/TranslationUtilities.h>
#include <ScriptCanvas/Utils/VersionConverters.h>
#include <ScriptCanvas/Variable/GraphVariableManagerComponent.h>
#include <ScriptCanvas/Variable/VariableBus.h>
#include <ScriptCanvas/Variable/VariableData.h>
#include <ScriptEvents/ScriptEventsAsset.h>
#include <ScriptEvents/ScriptEventsBus.h>

#include "AbstractCodeModel.h"
#include "FunctionsLegacySupport.h"
#include "GrammarContextBus.h"
#include "ParsingUtilities.h"
#include "Primitives.h"

namespace ScriptCanvas
{
    namespace Grammar
    {

        void AbstractCodeModel::AddAllVariablesPreParse_LegacyFunctions()
        {
            // all variables assumed to be NOT persistent - in the live code they are reset when activated
            // variables with no scope In/Out is assumed to be persistent, this is a mess with tick handlers
            // warn on any variable attempted to be read before written 
            AZ_Assert(m_variableScopeMeaning == VariableScopeMeaning_LegacyFunctions::FunctionPrototype, "new graph type added without full support");
            auto& sourceVariables = m_source.m_variableData->GetVariables();

            AZStd::set<const GraphVariable*, GraphVariable::Comparator> sortedVariables;
            for (const auto& variablePair : sourceVariables)
            {
                sortedVariables.insert(&variablePair.second);
            }

            for (auto& sourceVariable : sortedVariables)
            {
                auto datum = sourceVariable->GetDatum();
                AZ_Assert(datum != nullptr, "the datum must be valid");
                AddVariable(*datum, sourceVariable->GetVariableName(), sourceVariable->GetVariableId());
            }
        }

        AZStd::vector<VariablePtr> AbstractCodeModel::FindSubGraphInputValues() const
        {
            return FindAllVariablesInVariableFlagScope(VariableFlags::Scope::Input);
        }

        AZStd::vector<VariablePtr> AbstractCodeModel::FindSubGraphOutputValues() const
        {
            return FindAllVariablesInVariableFlagScope(VariableFlags::Scope::Output);
        }

        AZStd::vector<VariablePtr> AbstractCodeModel::FindAllVariablesInVariableFlagScope(VariableFlags::Scope scope) const
        {
            AZStd::vector<VariablePtr> variables;
            auto& sourceVariables = m_source.m_variableData->GetVariables();

            for (auto& variable : m_variables)
            {
                if (IsSourceInScope(variable, scope))
                {
                    variables.push_back(variable);
                }
            }

            // sort by variable id
            AZStd::sort(variables.begin(), variables.end(), [](const auto& lhs, const auto& rhs) { return lhs->m_sourceVariableId < rhs->m_sourceVariableId; });
            return variables;
        }

        AZStd::vector<VariableConstPtr> AbstractCodeModel::GetLocalVariablesUser() const
        {
            AZStd::vector<VariableConstPtr> userLocalVariables;

            if (m_variableScopeMeaning == VariableScopeMeaning_LegacyFunctions::FunctionPrototype)
            {
                for (auto variable : m_variables)
                {
                    if (!variable->m_isMember
                        && variable->m_sourceVariableId.IsValid()
                        && !IsSourceInScope(variable, VariableFlags::Scope::Input)
                        && !IsSourceInScope(variable, VariableFlags::Scope::Output))
                    {
                        userLocalVariables.push_back(variable);
                    }
                }
            }

            return userLocalVariables;
        }

        VariableScopeMeaning_LegacyFunctions AbstractCodeModel::GetVariableScopeMeaning() const
        {
            return m_variableScopeMeaning;
        }

        bool SubgraphInterface::IsAllInputOutputShared() const
        {
            return m_isAllInputOutputShared;
        }

        void SubgraphInterface::MarkAllInputOutputShared()
        {
            m_isAllInputOutputShared = true;
        }
    } // namespace Grammar

    namespace Nodes
    {
        namespace Core
        {
            void FunctionCallNode::BuildNodeFromSubgraphInterface
            (const AZ::Data::Asset<ScriptCanvas::SubgraphInterfaceAsset>& runtimeAsset
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
        } // namespace Core
    } // namespace Nodes
}

#endif
