/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/containers/unordered_map.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <Builder/ScriptCanvasBuilder.h>
#include <ScriptCanvas/Core/Datum.h>
#include <ScriptCanvas/Core/EBusHandler.h>
#include <ScriptCanvas/Core/Graph.h>
#include <ScriptCanvas/Core/Nodeable.h>
#include <ScriptCanvas/Core/NodeableNode.h>
#include <ScriptCanvas/Core/NodeableNodeOverloaded.h>
#include <ScriptCanvas/Core/SubgraphInterfaceUtility.h>
#include <ScriptCanvas/Debugger/ValidationEvents/DataValidation/ScopedDataConnectionEvent.h>
#include <ScriptCanvas/Debugger/ValidationEvents/ParsingValidation/ParsingValidations.h>
#include <ScriptCanvas/Grammar/ParsingMetaData.h>
#include <ScriptCanvas/Libraries/Core/AzEventHandler.h>
#include <ScriptCanvas/Libraries/Core/EBusEventHandler.h>
#include <ScriptCanvas/Libraries/Core/FunctionDefinitionNode.h>
#include <ScriptCanvas/Libraries/Core/ExtractProperty.h>
#include <ScriptCanvas/Libraries/Core/ForEach.h>
#include <ScriptCanvas/Libraries/Core/FunctionCallNode.h>
#include <ScriptCanvas/Libraries/Core/FunctionCallNodeIsOutOfDate.h>
#include <ScriptCanvas/Libraries/Core/Method.h>
#include <ScriptCanvas/Libraries/Core/Start.h>
#include <ScriptCanvas/Translation/TranslationUtilities.h>
#include <ScriptCanvas/Variable/VariableData.h>

#include "AbstractCodeModel.h"
#include "ExecutionTraversalListeners.h"
#include "ParsingUtilities.h"
#include "Primitives.h"

namespace AbstractCodeModelCpp
{
    using namespace ScriptCanvas;
    using namespace ScriptCanvas::Grammar;

    AZStd::unordered_set< const Nodes::Core::FunctionDefinitionNode*> Intersection
    (const AZStd::unordered_multimap<const Nodes::Core::FunctionDefinitionNode*, ExecutionTreePtr>& lhs
        , const AZStd::unordered_set< const Nodes::Core::FunctionDefinitionNode*>& rhs)
    {
        AZStd::unordered_set< const Nodes::Core::FunctionDefinitionNode*> intersection;

        for (auto candidate : lhs)
        {
            if (rhs.contains(candidate.first))
            {
                intersection.insert(candidate.first);
            }
        }

        return intersection;
    }

    EndpointsResolved GetParentNodes(const Node* node)
    {
        EndpointsResolved resolved;

        if (node)
        {
            auto slots = node->GetSlotsByType(CombinedSlotType::ExecutionIn);
            for (auto slot : slots)
            {
                if (slot)
                {
                    auto nodesInSlot = node->GetConnectedNodes(*slot);
                    resolved.insert(resolved.end(), nodesInSlot.begin(), nodesInSlot.end());
                }
            }
        }

        return resolved;
    }

    bool IsConnectedToUserInRecurse(const Node* node, AZStd::unordered_set<const Slot*>& previousPath);

    bool IsConnectedToUserIn(const Node* node)
    {
        AZStd::unordered_set<const Slot*> previousPath;
        return IsConnectedToUserInRecurse(node, previousPath);
    }

    bool IsConnectedToUserInRecurse(const Node* node, AZStd::unordered_set<const Slot*>& previousPath)
    {
        auto parents = GetParentNodes(node);

        for (auto parent : parents)
        {
            if (previousPath.contains(parent.second))
            {
                continue;
            }

            previousPath.insert(parent.second);

            auto nodeling = azrtti_cast<const Nodes::Core::FunctionDefinitionNode*>(parent.first);
            if (nodeling && nodeling->IsExecutionEntry())
            {
                return true;
            }

            if (parent.first && IsConnectedToUserInRecurse(parent.first, previousPath))
            {
                return true;
            }
        }

        return false;
    }
}

namespace ScriptCanvas
{
    namespace Grammar
    {
        using namespace Internal;
        using namespace NodeCompatiliblity;

        AbstractCodeModel::AbstractCodeModel(const Source& source, bool /*terminateOnError*/, bool /*terminateOnInternalError*/)
            : m_source(source)
            , m_graphScope(AZStd::make_shared<Scope>())
        {
            Parse();
        }

        AbstractCodeModel::~AbstractCodeModel()
        {
            if (m_start)
            {
                m_start->Clear();
                m_start = nullptr;
            }

            m_graphScope = nullptr;
            m_variables.clear();

            for (auto iter : m_functions)
            {
                if (auto mutableIter = AZStd::const_pointer_cast<ExecutionTree>(iter))
                {
                    mutableIter->Clear();
                }
            }
            m_functions.clear();

            m_userInsThatRequireTopology.clear();
            m_userOutsThatRequireTopology.clear();

            for (auto iter : m_ebusHandlingByNode)
            {
                if (iter.second)
                {
                    iter.second->Clear();
                }
            }
            m_ebusHandlingByNode.clear();

            for (auto iter : m_eventHandlingByNode)
            {
                if (iter.second)
                {
                    iter.second->Clear();
                }
            }

            for (auto iter : m_nodeablesByNode)
            {
                if (auto mutableIter = AZStd::const_pointer_cast<NodeableParse>(iter.second))
                {
                    mutableIter->Clear();
                }
            }
            m_nodeablesByNode.clear();

            for (auto iter : m_variableWriteHandlingBySlot)
            {
                if (auto mutableIter = AZStd::const_pointer_cast<VariableWriteHandling>(iter.second))
                {
                    mutableIter->Clear();
                }
            }
            m_variableWriteHandlingBySlot.clear();
            m_variableWriteHandlingByVariable.clear();

            m_userNodeables.clear();
        }

        void AbstractCodeModel::AccountForEBusConnectionControl(ExecutionTreePtr execution)
        {
            if (execution->GetSymbol() == Symbol::FunctionDefinition)
            {
                return;
            }

            const auto id = execution->GetId();

            if (!(id.m_node && id.m_node->IsEventHandler()))
            {
                return;
            }

            auto ebusIter = m_ebusHandlingByNode.find(id.m_node);
            if (ebusIter != m_ebusHandlingByNode.end())
            {
                AccountForEBusConnectionControl(execution, ebusIter->second);
            }
            else
            {
                for (auto slot : id.m_node->GetOnVariableHandlingDataSlots())
                {
                    auto variableIter = m_variableWriteHandlingBySlot.find(slot);
                    if (variableIter != m_variableWriteHandlingBySlot.end())
                    {
                        AccountForEBusConnectionControl(execution, variableIter->second);
                    }
                }
            }
        }

        void AbstractCodeModel::AddAllVariablesPreParse()
        {
            auto& sourceVariables = m_source.m_variableData->GetVariables();

            AZStd::set<const GraphVariable*, GraphVariable::Comparator> sortedVariables;
            for (const auto& variablePair : sourceVariables)
            {
                if (variablePair.second.GetScope() != VariableFlags::Scope::FunctionReadOnly)
                {
                    sortedVariables.insert(&variablePair.second);
                }

                if (auto datum = variablePair.second.GetDatum())
                {
                    // #functions2 slot<->variable consider getting all variables from the UX variable manager, or from the ACM and looking them up in the variable manager for ordering
                    m_sourceVariableByDatum.insert(AZStd::make_pair(datum, &variablePair.second));
                }

            }

            for (auto& sourceVariable : sortedVariables)
            {
                auto datum = sourceVariable->GetDatum();
                AZ_Assert(datum != nullptr, "the datum must be valid");

                // #functions2 slot<->variable check to verify if it is a member variable
                auto variable = sourceVariable->GetScope() == VariableFlags::Scope::Graph
                    ? AddMemberVariable(*datum, sourceVariable->GetVariableName(), sourceVariable->GetVariableId())
                    : AddVariable(*datum, sourceVariable->GetVariableName(), sourceVariable->GetVariableId());

                variable->m_isExposedToConstruction = sourceVariable->IsComponentProperty();
                // also, all nodeables with !empty editor data have to be exposed
                // \todo future optimizations will involve checking equality against a default constructed object   
            }
        }

        void AbstractCodeModel::AddDebugInformation()
        {
            auto roots = ModAllExecutionRoots();

            for (auto root : roots)
            {
                AddDebugInformationFunctionDefinition(root);

                for (size_t index(0); index < root->GetChildrenCount(); ++index)
                {
                    AddDebugInformation(root->ModChild(index));
                }
            }
        }

        void AbstractCodeModel::AddDebugInformation(ExecutionChild& execution)
        {
            if (execution.m_execution)
            {
                ParseDebugInformation(execution.m_execution);

                for (size_t index(0); index < execution.m_execution->GetChildrenCount(); ++index)
                {
                    AddDebugInformation(execution.m_execution->ModChild(index));
                }
            }
        }

        void AbstractCodeModel::AddDebugInformationFunctionDefinition(ExecutionTreePtr execution)
        {
            AddDebugInformationOut(execution);

            if (execution->HasReturnValues())
            {
                DebugExecution returnValues;
                returnValues.m_data.reserve(execution->GetReturnValueCount());
                returnValues.m_namedEndpoint = execution->GetId().m_node->CreateNamedEndpoint(execution->GetId().m_slot->GetId());

                for (size_t index(0); index < execution->GetReturnValueCount(); ++index)
                {
                    returnValues.m_data.push_back(execution->GetReturnValue(index).second->m_sourceDebug);
                }

                m_debugMapReverse.m_return[execution] = m_debugMap.m_returns.size();
                m_debugMap.m_returns.push_back(returnValues);
            }
        }

        void AbstractCodeModel::AddDebugInformationIn(ExecutionTreePtr execution)
        {
            if (execution->GetSymbol() == Symbol::FunctionDefinition)
            {
                AddDebugInformationFunctionDefinition(execution);
            }
            else if (execution->GetId().m_node && execution->GetId().m_slot)
            {
                DebugExecution inDebug;
                inDebug.m_namedEndpoint = execution->GetId().m_node->CreateNamedEndpoint(execution->GetId().m_slot->GetId());
                inDebug.m_data.reserve(execution->GetInputCount());

                for (size_t index(0); index < execution->GetInputCount(); ++index)
                {
                    inDebug.m_data.push_back(execution->GetInput(index).m_sourceDebug);
                }

                m_debugMapReverse.m_in[execution] = m_debugMap.m_ins.size();
                m_debugMap.m_ins.push_back(inDebug);
            }
        }

        void AbstractCodeModel::AddDebugInformationOut(ExecutionTreePtr execution)
        {
            if (execution->GetId().m_node)
            {
                for (size_t index(0); index < execution->GetChildrenCount(); ++index)
                {
                    auto& child = execution->GetChild(index);

                    DebugExecution debugOut;
                    debugOut.m_namedEndpoint = execution->GetId().m_node->CreateNamedEndpoint(child.m_slot ? child.m_slot->GetId() : SlotId());
                    debugOut.m_data.resize(child.m_output.size());

                    for (auto output : child.m_output)
                    {
                        auto outputAssignment = output.second;
                        if (output.first)
                        {
                            debugOut.m_data.push_back(DebugDataSource::FromSelfSlot(*output.first));
                        }
                        else
                        {
                            if (outputAssignment && outputAssignment->m_source)
                            {
                                debugOut.m_data.push_back(DebugDataSource::FromInternal(outputAssignment->m_source->m_datum.GetType()));
                            }
                            else
                            {
                                debugOut.m_data.push_back(DebugDataSource::FromInternal());
                            }
                        }

                        if (outputAssignment->m_source->m_sourceVariableId.IsValid())
                        {
                            DebugDataSource variableChange;
                            variableChange.m_slotDatumType = outputAssignment->m_source->m_datum.GetType();
                            variableChange.m_source = outputAssignment->m_source->m_sourceVariableId;
                            m_debugMapReverse.m_variableSets[outputAssignment] = m_debugMap.m_variables.size();
                            m_debugMap.m_variables.push_back(variableChange);
                        }

                        for (size_t index2(0); index2 < outputAssignment->m_assignments.size(); ++index2)
                        {
                            auto& assignment = outputAssignment->m_assignments[index2];

                            if (assignment->m_sourceVariableId.IsValid())
                            {
                                DebugDataSource variableChange;
                                variableChange.m_slotDatumType = assignment->m_datum.GetType();
                                variableChange.m_source = assignment->m_sourceVariableId;
                                m_debugMapReverse.m_assignments[outputAssignment].insert({ index2, m_debugMap.m_variables.size() });
                                m_debugMap.m_variables.push_back(variableChange);
                            }
                        }
                    }

                    m_debugMapReverse.m_out[execution].push_back(m_debugMap.m_outs.size());
                    m_debugMap.m_outs.push_back(debugOut);
                }
            }
        }

        void AbstractCodeModel::AddDebugInfiniteLoopDetectionInLoop(ExecutionTreePtr execution)
        {
            execution->MarkInfiniteLoopDetectionPoint();
            auto counterName = m_graphScope->AddVariableName("loopIterationCounter");
            m_implicitVariablesByNode.insert({ execution, AZStd::make_shared<Variable>(Datum(Data::Type::Number(), Datum::eOriginality::Original), counterName, TraitsFlags(0)) });
        }

        void AbstractCodeModel::AddDebugInfiniteLoopDetectionInHandler(ExecutionTreePtr execution)
        {
            execution->MarkInfiniteLoopDetectionPoint();
            auto variable = AddMemberVariable(Datum(Data::Type::Number(), Datum::eOriginality::Original), "handlerIterationCounter");
            variable->m_isDebugOnly = true;
            m_implicitVariablesByNode.insert({ execution, variable });
        }

        void AbstractCodeModel::AddError(ExecutionTreeConstPtr execution, ValidationConstPtr&& error) const
        {
            if (execution && execution->GetRoot())
            {
                AZStd::string pretty;
                Grammar::PrettyPrint(pretty, execution->GetRoot(), execution);
                AZ_TracePrintf("Script Canvas", pretty.c_str());
            }

            AbstractCodeModel* mutableThis = const_cast<AbstractCodeModel*>(this);
            mutableThis->m_isErrorFree = false;
            AddValidation(AZStd::move(error));
        }

        void AbstractCodeModel::AddError(const AZ::EntityId& nodeId, ExecutionTreeConstPtr execution, const AZStd::string_view error) const
        {
            AddError(execution, aznew Internal::ParseError(nodeId, error));
        }

        void AbstractCodeModel::AddExecutionMapIn
        (UserInParseTopologyResult /*result*/
            , ExecutionTreeConstPtr root
            , const AZStd::vector<ExecutionTreeConstPtr>& outCalls
            , AZStd::string_view defaultOutName
            , const Nodes::Core::FunctionDefinitionNode* nodelingIn
            , const AZStd::unordered_set<const ScriptCanvas::Nodes::Core::FunctionDefinitionNode*>& uniqueNodelingsOut)
        {
            In in;
            SetDisplayAndParsedNameSafe(in, root->GetName());
            in.sourceID = nodelingIn->GetIdentifier();

            const auto defaultOutId = MakeDefaultOutId(in.sourceID);

            const auto& functionInput = root->GetChild(0).m_output;
            for (auto& input : functionInput)
            {
                in.inputs.push_back
                ({ GetOriginalVariableName(input.second->m_source, nodelingIn)
                , input.second->m_source->m_name
                , input.second->m_source->m_datum
                , input.second->m_source->m_sourceVariableId });
            }

            if (!root->HasExplicitUserOutCalls())
            {
                // there is a single out, default or not
                Out out;

                if (outCalls.empty())
                {
                    SetDisplayAndParsedName(out, defaultOutName);
                    out.sourceID = defaultOutId;
                }
                else
                {
                    if (uniqueNodelingsOut.empty())
                    {
                        AddError(root->GetNodeId(), root, "Explicit Out call provided with no nodeling out");
                        return;
                    }

                    SetDisplayAndParsedName(out, outCalls[0]->GetName());
                    out.sourceID = (*uniqueNodelingsOut.begin())->GetIdentifier();
                }

                for (size_t returnValueIndex = 0; returnValueIndex < root->GetReturnValueCount(); ++returnValueIndex)
                {
                    const auto& returnValueVariable = root->GetReturnValue(returnValueIndex).second->m_source;
                    out.outputs.push_back
                    ({ GetOriginalVariableName(returnValueVariable, *uniqueNodelingsOut.begin())
                    , returnValueVariable->m_name
                    , returnValueVariable->m_datum.GetType()
                    , returnValueVariable->m_sourceVariableId });
                }

                in.outs.push_back(AZStd::move(out));
            }
            else
            {
                if (outCalls.size() < 2)
                {
                    AddError(root->GetNodeId(), root, ScriptCanvas::ParseErrors::NotEnoughBranchesForReturn);
                    return;
                }

                for (auto& outCall : outCalls)
                {
                    Out out;
                    const auto& outCallID = outCall->GetId();
                    auto nodelingCanBeNull = azrtti_cast<const Nodes::Core::FunctionDefinitionNode*>(outCallID.m_node);

                    if (nodelingCanBeNull)
                    {
                        SetDisplayAndParsedName(out, nodelingCanBeNull->GetDisplayName());
                        out.sourceID = nodelingCanBeNull->GetIdentifier();
                    }
                    else
                    {
                        SetDisplayAndParsedName(out, defaultOutName);
                        out.sourceID = defaultOutId;
                    }

                    for (size_t inputIndex = 0; inputIndex < outCall->GetInputCount(); ++inputIndex)
                    {
                        const auto& returnValueVariable = outCall->GetInput(inputIndex).m_value;
                        out.outputs.push_back
                        ({ GetOriginalVariableName(returnValueVariable, outCallID.m_node)
                        , returnValueVariable->m_name
                        , returnValueVariable->m_datum.GetType()
                        , returnValueVariable->m_sourceVariableId });
                    }

                    AZStd::const_pointer_cast<ExecutionTree>(outCall)->SetOutCallIndex(m_outIndexCount);
                    ++m_outIndexCount;

                    in.outs.push_back(AZStd::move(out));
                }
            }

            m_subgraphInterface.AddIn(AZStd::move(in));
        }

        void AbstractCodeModel::AddExecutionMapLatentOut(const Nodes::Core::FunctionDefinitionNode& nodeling, ExecutionTreePtr outCall)
        {
            if (m_processedOuts.contains(&nodeling))
            {
                return;
            }

            m_processedOuts.insert(&nodeling);

            Out out;
            SetDisplayAndParsedName(out, nodeling.GetDisplayName());
            out.sourceID = nodeling.GetIdentifier();

            for (size_t inputIndex = 0; inputIndex < outCall->GetInputCount(); ++inputIndex)
            {
                const auto& inputVariable = outCall->GetInput(inputIndex).m_value;
                out.outputs.push_back
                ({ GetOriginalVariableName(inputVariable, &nodeling)
                , inputVariable->m_name
                , inputVariable->m_datum.GetType()
                , inputVariable->m_sourceVariableId });
            }

            for (size_t returnValueIndex = 0; returnValueIndex < outCall->GetReturnValueCount(); ++returnValueIndex)
            {
                const auto& returnValueVariable = outCall->GetReturnValue(returnValueIndex).second->m_source;
                out.outputs.push_back
                ({ GetOriginalVariableName(returnValueVariable, &nodeling)
                , returnValueVariable->m_name
                , returnValueVariable->m_datum.GetType()
                , returnValueVariable->m_sourceVariableId });
            }

            AZStd::const_pointer_cast<ExecutionTree>(outCall)->SetOutCallIndex(m_outIndexCount);
            ++m_outIndexCount;
            m_subgraphInterface.AddLatent(AZStd::move(out));
        }

        void AbstractCodeModel::AddPreviouslyExecutedScopeVariableToOutputAssignments(VariableConstPtr newInputVariable, const ConnectionsInPreviouslyExecutedScope& connectedInputInPreviouslyExecutedScope)
        {
            for (const auto& connection : connectedInputInPreviouslyExecutedScope.m_connections)
            {
                OutputAssignmentPtr output = AZStd::const_pointer_cast<OutputAssignment>(connection.m_source->GetChild(connection.m_childIndex).m_output[connection.m_outputIndex].second);
                output->m_assignments.push_back(newInputVariable);
            }
        }

        VariablePtr AbstractCodeModel::AddMemberVariable(const Datum& datum, AZStd::string_view rawName)
        {
            auto variable = AddVariable(datum, MakeMemberVariableName(rawName));
            variable->m_isMember = true;
            return variable;
        }

        VariablePtr AbstractCodeModel::AddMemberVariable(const Datum& datum, AZStd::string_view rawName, const AZ::EntityId& sourceNodeId)
        {
            auto variable = AddVariable(datum, MakeMemberVariableName(rawName), sourceNodeId);
            variable->m_isMember = true;
            return variable;
        }

        VariablePtr AbstractCodeModel::AddMemberVariable(const Datum& datum, AZStd::string_view rawName, const VariableId& sourceVariableId)
        {
            auto variable = AddVariable(datum, MakeMemberVariableName(rawName), sourceVariableId);
            variable->m_isMember = true;
            return variable;
        }

        AZStd::string AbstractCodeModel::AddTranslationVariableName(const AZStd::string& name) const
        {
            return AZStd::const_pointer_cast<Scope>(m_graphScope)->AddVariableName(name);
        }

        void AbstractCodeModel::AddUserOutToLeaf(ExecutionTreePtr parent, ExecutionTreeConstPtr root, AZStd::string_view name)
        {
            ExecutionTreePtr out;

            if (parent->GetSymbol() == Symbol::DebugInfoEmptyStatement)
            {
                parent->SetSymbol(Symbol::UserOut);
                parent->SetName(name);
                out = parent;
            }
            else
            {
                out = AZStd::make_shared<ExecutionTree>();
                out->SetSymbol(Symbol::UserOut);
                out->SetName(name);
                out->SetParent(parent);

                if (parent->GetChildrenCount() == 0)
                {
                    parent->AddChild({ nullptr, {}, out });
                }
                else
                {
                    AZ_Assert(parent->GetChildrenCount() == 1, "should only be one child");
                    AZ_Assert(parent->ModChild(0).m_execution == nullptr, "memory leak risk");
                    parent->ModChild(0).m_execution = out;
                }
            }
        }

        void AbstractCodeModel::AddValidation(ValidationConstPtr&& validation) const
        {
            AbstractCodeModel* mutableThis = const_cast<AbstractCodeModel*>(this);
            mutableThis->m_validationEvents.emplace_back(validation);
            ValidationResults results;
            results.AddValidationEvent(validation.get());
        }

        void AbstractCodeModel::AddVariable(VariablePtr variable)
        {
            variable->m_name = m_graphScope->AddVariableName(variable->m_name);
            m_variables.push_back(variable);
        }

        VariablePtr AbstractCodeModel::AddVariable(const Datum& datum, AZStd::string_view rawName)
        {
            auto variable = AZStd::make_shared<Variable>(datum, rawName, TraitsFlags(0));
            AddVariable(variable);
            return variable;
        }

        VariablePtr AbstractCodeModel::AddVariable(const Datum& datum, AZStd::string_view rawName, const AZ::EntityId& sourceNodeId)
        {
            auto variable = AddVariable(datum, rawName);
            variable->m_nodeableNodeId = sourceNodeId;
            return variable;
        }

        VariablePtr AbstractCodeModel::AddVariable(const Datum& datum, AZStd::string_view rawName, const VariableId& sourceVariableId)
        {
            auto variable = AddVariable(datum, rawName);
            variable->m_sourceVariableId = sourceVariableId;
            return variable;
        }

        VariablePtr AbstractCodeModel::AddVariable(const Data::Type& type, AZStd::string_view rawName)
        {
            return AddVariable(Datum(type), rawName);
        }

        void AbstractCodeModel::CheckForKnownNullDereference(ExecutionTreeConstPtr execution, const ExecutionInput& input, const Slot& inputSlot)
        {
            if (Data::IsValueType(inputSlot.GetDataType())
            || !execution->GetId().m_node
            || !execution->GetId().m_slot
            || (input.m_value && !input.m_value->m_datum.Empty()))
            {
                return;
            }

            if (!input.m_value)
            {
                AddError(execution->GetId().m_node->GetEntityId(), nullptr, "Internal Error: CheckForKnownNullDereference called with input with no m_value");
                return;
            }

            if (!execution->GetId().m_node->CanAcceptNullInput(*execution->GetId().m_slot, inputSlot))
            {
                AddError(execution->GetId().m_node->GetEntityId(), nullptr, ParseErrors::NullInputKnown);
            }
        }

        void AbstractCodeModel::CheckConversion(ConversionByIndex& conversion, VariableConstPtr source, size_t index, const Data::Type& targetType)
        {
            const Data::Type& sourceType = source->m_datum.GetType();

            if (!sourceType.IS_A(targetType) && sourceType.IsConvertibleTo(targetType))
            {
                conversion.insert({ index, targetType });
            }
        }

        void AbstractCodeModel::CheckConversions(OutputAssignmentPtr output)
        {
            output->m_sourceConversions.clear();

            for (size_t i = 0; i < output->m_assignments.size(); ++i)
            {
                CheckConversion(output->m_sourceConversions, output->m_source, i, output->m_assignments[i]->m_datum.GetType());
            }
        }

        bool AbstractCodeModel::CheckCreateRoot(const Node& node)
        {
            return CheckCreateNodeableParse(node)
                || CheckCreateUserEventHandling(node)
                || CheckCreateUserFunctionDefinition(node);
        }

        AZStd::string AbstractCodeModel::CheckUniqueInterfaceNames
            ( AZStd::string_view candidate
            , AZStd::string_view defaultName
            , AZStd::unordered_set<AZStd::string>& uniqueNames
            , const AZStd::unordered_set<const ScriptCanvas::Nodes::Core::FunctionDefinitionNode*>& nodelingsOut)
        {
            if (nodelingsOut.size() == 1 && (*nodelingsOut.begin())->GetDisplayName() == candidate)
            {
                return (*nodelingsOut.begin())->GetDisplayName();
            }

            if (!uniqueNames.contains(candidate))
            {
                uniqueNames.insert(candidate);
                return candidate;
            }

            if (!uniqueNames.contains(defaultName))
            {
                uniqueNames.insert(defaultName);
                return defaultName;
            }

            size_t index = uniqueNames.size();
            AZStd::string numberedOut = AZStd::string::format("%s %zu", defaultName.data(), index);

            while (uniqueNames.contains(numberedOut))
            {
                ++index;
                numberedOut = AZStd::string::format("%s %zu", defaultName.data(), index);
            }

            uniqueNames.insert(numberedOut);
            return numberedOut;
        }

        AZStd::string AbstractCodeModel::CheckUniqueOutNames(AZStd::string_view displayName, const AZStd::unordered_set<const ScriptCanvas::Nodes::Core::FunctionDefinitionNode*>& nodelingsOut)
        {
            return CheckUniqueInterfaceNames(displayName, "Out", m_uniqueOutNames, nodelingsOut);
        }

        AZStd::optional<AZStd::pair<size_t, DependencyInfo>> AbstractCodeModel::CheckUserNodeableDependencyConstructionIndex(VariableConstPtr variable) const
        {
            auto iter = m_dependencyByVariable.find(variable);
            if (iter != m_dependencyByVariable.end())
            {
                for (size_t index = 0; index != m_orderedDependencies.orderedAssetIds.size(); ++index)
                {
                    if (iter->second.assetId == m_orderedDependencies.orderedAssetIds[index])
                    {
                        return AZStd::make_pair(index, iter->second);
                    }
                }
            }

            return {};
        }

        AZStd::vector<Grammar::VariableConstPtr> AbstractCodeModel::CombineVariableLists
        (const AZStd::vector<Nodeable*>& constructionNodeables
            , const AZStd::vector<AZStd::pair<VariableId, Datum>>& constructionInputVariables
            , const AZStd::vector<AZStd::pair<VariableId, Data::EntityIDType>>& entityIds) const
        {
            AZStd::vector<Grammar::VariableConstPtr> variables;

            for (const auto& nodeable : constructionNodeables)
            {
                const void* nodeableAsVoidPtr = nodeable;

                auto iter = AZStd::find_if
                (m_nodeablesByNode.begin()
                    , m_nodeablesByNode.end()
                    , [&](const auto& candidate)
                {
                    return candidate.second->m_nodeable->m_datum.GetAsDanger() == nodeableAsVoidPtr;
                });

                if (iter != m_nodeablesByNode.end())
                {
                    variables.push_back(iter->second->m_nodeable);
                }
            }

            auto constructionVariables = ToVariableList(constructionInputVariables);
            variables.insert(variables.end(), constructionVariables.begin(), constructionVariables.end());

            for (const auto& variable : entityIds)
            {
                auto iter = AZStd::find_if(m_variables.begin(), m_variables.end(), [&](const auto& candidate) {
                    if (candidate->m_datum.GetType() == Data::Type::EntityID())
                    {
                        bool isVariableIdMatch = candidate->m_sourceVariableId == variable.first;
                        auto entityId = candidate->m_datum.template GetAs<Data::EntityIDType>();
                        if (entityId)
                        {
                            return isVariableIdMatch && *entityId == variable.second;
                        }
                        else
                        {
                            return isVariableIdMatch;
                        }
                    }
                    else
                    {
                        return false;
                    }
                });

                if (iter != m_variables.end())
                {
                    variables.push_back(*iter);
                }
            }

            return variables;
        }

        void AbstractCodeModel::ConvertNamesToIdentifiers()
        {
            class ConvertListener : public ExecutionTreeTraversalListener
            {
            protected:
                void Evaluate(ExecutionTreeConstPtr node, const Slot*, int) override
                {
                    if (node->GetSymbol() != Symbol::UserOut
                        && azrtti_istypeof<const ScriptCanvas::Nodes::Core::FunctionDefinitionNode*>(node->GetId().m_node))
                    {
                        AZStd::const_pointer_cast<ExecutionTree>(node)->ConvertNameToIdentifier();
                    }
                }
            };

            ConvertListener convertListener;
            TraverseTree(*this, convertListener);
        }

        ExecutionTreePtr AbstractCodeModel::CreateChild(ExecutionTreePtr parent, const Node* node, const Slot* slot) const
        {
            ExecutionTreePtr child = AZStd::make_shared<ExecutionTree>();
            child->SetParent(parent);
            child->SetId({ node, slot });
            child->SetScope(parent ? parent->ModScope() : m_graphScope);
            return child;
        }

        ExecutionTreePtr AbstractCodeModel::CreateChildDebugMarker(ExecutionTreePtr parent) const
        {
            ExecutionTreePtr child = AZStd::make_shared<ExecutionTree>();
            child->SetParent(parent);
            child->SetScope(parent ? parent->ModScope() : m_graphScope);
            child->SetSymbol(Symbol::DebugInfoEmptyStatement);
            return child;
        }

        ExecutionTreePtr AbstractCodeModel::CreateChildPlaceHolder(ExecutionTreePtr parent) const
        {
            ExecutionTreePtr child = AZStd::make_shared<ExecutionTree>();
            child->SetParent(parent);
            child->SetScope(parent ? parent->ModScope() : m_graphScope);
            child->SetSymbol(Symbol::PlaceHolderDuringParsing);
            return child;
        }

        bool AbstractCodeModel::CreateEBusHandling(const Node& node)
        {
            auto ebusHandling = AZStd::make_shared<EBusHandling>();
            ebusHandling->m_ebusName = node.GetEBusName();
            ebusHandling->m_handlerName = m_graphScope->AddVariableName(AZStd::string::format("%sHandler", ebusHandling->m_ebusName.data()));

            auto addressSlot = node.GetEBusConnectAddressSlot();
            VariableConstPtr startingAddressVariable = addressSlot && addressSlot->IsVariableReference() ? FindVariable(addressSlot->GetVariableReference()) : nullptr;

            ebusHandling->m_isAddressed = node.IsEBusAddressed();

            if (ebusHandling->m_isAddressed)
            {
                if (addressSlot == nullptr)
                {
                    AddError(node.GetEntityId(), nullptr, "Missing slot for ebus event");
                    return false;
                }

                if (addressSlot->IsVariableReference())
                {
                    if (startingAddressVariable != nullptr)
                    {
                        ebusHandling->m_startingAdress = startingAddressVariable;
                    }
                    else
                    {
                        AddError(node.GetEntityId(), nullptr, ParseErrors::MissingVariableForEBusHandlerAddress);
                        return false;
                    }
                }
            }

            if (node.IsAutoConnected())
            {
                if (ebusHandling->m_isAddressed)
                {
                    ebusHandling->m_startsConnected = Data::IsValueType(addressSlot->GetDataType()) || startingAddressVariable != nullptr;
                }
                else
                {
                    ebusHandling->m_startsConnected = true;
                }
            }

            ebusHandling->m_isAutoConnected = node.IsAutoConnected();

            if (ebusHandling->m_isAddressed && !addressSlot->IsVariableReference() && (ebusHandling->m_startsConnected || ebusHandling->m_isAutoConnected))
            {
                auto startAddressDatum = node.GetHandlerStartAddress();
                if (!startAddressDatum)
                {
                    AddError(node.GetEntityId(), nullptr, ParseErrors::MissingVariableForEBusHandlerAddressConnected);
                    return false;
                }

                VariablePtr startingAddress = AddMemberVariable(*startAddressDatum, AZStd::string::format("%sAddress", ebusHandling->m_ebusName.data()).data(), node.GetEntityId());
                ebusHandling->m_startingAdress = startingAddress;
            }

            ebusHandling->m_node = &node;
            m_ebusHandlingByNode.emplace(&node, ebusHandling);
            return true;
        }

        bool AbstractCodeModel::CreateEventHandling(const Node& node)
        {
            const auto connectSlot = AzEventHandlerProperty::GetConnectSlot(&node);
            if (!connectSlot)
            {
                AddError(node.GetEntityId(), nullptr, ParseErrors::EventNodeMissingConnectSlot);
                return false;
            }

            const EndpointsResolved connectedEndpoints = node.GetConnectedNodes(*connectSlot);
            if (connectedEndpoints.empty())
            {
                return false;
            }

            if (connectedEndpoints.size() > 1)
            {
                AddError(node.GetEntityId(), nullptr, ParseErrors::EventNodeConnectCallMalformed);
                return false;
            }

            auto azEventNode = azrtti_cast<const ScriptCanvas::Nodes::Core::AzEventHandler*>(&node);
            if (!azEventNode)
            {
                AddError(node.GetEntityId(), nullptr, ParseErrors::BadEventHandlingAccounting);
                return false;
            }

            auto eventInputSlot = azEventNode->GetEventInputSlot();
            if (!eventInputSlot)
            {
                AddError(node.GetEntityId(), nullptr, ParseErrors::EventNodeMissingConnectEventInputSlot);
                return false;
            }

            auto inputHandlerDatum = eventInputSlot->FindDatum();
            if (!inputHandlerDatum)
            {
                AddError(node.GetEntityId(), nullptr, ParseErrors::EventNodeMissingConnectEventInputMissingVariableDatum);
                return false;
            }

            const EndpointResolved& endpoint = connectedEndpoints.front();
            auto eventHandling = AZStd::make_shared<EventHandling>();
            eventHandling->m_eventName = node.GetNodeName();
            eventHandling->m_eventNode = endpoint.first;
            eventHandling->m_eventSlot = endpoint.second;

            auto name = AZStd::string::format("%sHandler", eventHandling->m_eventName.data());
            eventHandling->m_handler = AddMemberVariable(*inputHandlerDatum, name, node.GetEntityId());
            eventHandling->m_handler->m_requiresNullCheck = true;
            eventHandling->m_handler->m_initializeAsNull = true;

            m_eventHandlingByNode.emplace(&node, eventHandling);
            return true;
        }

        bool AbstractCodeModel::CheckCreateNodeableParse(const Node& node)
        {
            if (auto nodeableNode = azrtti_cast<const ScriptCanvas::Nodes::NodeableNode*>(&node))
            {
                if (const Nodeable* nodeable = nodeableNode->GetNodeable())
                {
                    Datum nodeableDatum(Data::Type::BehaviorContextObject(azrtti_typeid(nodeable)), Datum::eOriginality::Copy, reinterpret_cast<const void*>(nodeable), azrtti_typeid(nodeable));
                    auto nodeableVariable = AddMemberVariable(nodeableDatum, nodeable->RTTI_GetTypeName(), node.GetEntityId());
                    auto nodeableParse = AZStd::make_shared<NodeableParse>();
                    nodeableVariable->m_isExposedToConstruction = true;
                    nodeableParse->m_nodeable = nodeableVariable;

                    // iterate through all on variable handlings
                    for (auto executionSlot : node.GetOnVariableHandlingExecutionSlots())
                    {
                        const auto name = m_graphScope->AddFunctionName(AZStd::string::format("On%s", executionSlot->GetName().data()));
                        ExecutionTreePtr onVariableExecution = OpenScope(nullptr, &node, nullptr);
                        onVariableExecution->SetSymbol(Symbol::FunctionDefinition);
                        onVariableExecution->SetName(name);

                        ExecutionTreePtr onInputChangeExecution = CreateChild(onVariableExecution, &node, executionSlot);
                        ParseInputThisPointer(onInputChangeExecution);
                        auto dataInOutcomes = nodeableNode->GetDataInSlotsByExecutionIn(*nodeableNode->GetSlotExecutionMap(), *executionSlot);
                        AZ_Assert(dataInOutcomes.IsSuccess() && dataInOutcomes.GetValue().size() == 1, "Should have only one input per on variable handling.");
                        auto dataSlot = dataInOutcomes.GetValue()[0];
                        auto datum = node.FindDatum(dataSlot->GetId());
                        VariablePtr variable = AddMemberVariable(*datum, dataSlot->GetName(), node.GetEntityId());
                        CreateVariableWriteHandling(*dataSlot, variable, node.IsAutoConnected());
                        onInputChangeExecution->AddInput({ nullptr, variable, DebugDataSource::FromInternal() });

                        FunctionCallDefaultMetaData metaData;
                        metaData.PostParseExecutionTreeBody(*this, onInputChangeExecution);

                        nodeableParse->m_onInputChanges.push_back(onInputChangeExecution);
                        onVariableExecution->AddChild({ nullptr, {}, onInputChangeExecution });

                        VariableWriteHandlingPtr onVariableHandling = AZStd::const_pointer_cast<VariableWriteHandling>(GetVariableHandling(dataSlot));
                        AZ_Assert(onVariableHandling != nullptr, "failure to create variable handling for ebus address");
                        onVariableHandling->m_function = onVariableExecution;
                    }

                    m_nodeablesByNode.emplace(&node, nodeableParse);

                    return true;
                }
                else
                {
                    if (azrtti_istypeof<ScriptCanvas::Nodes::NodeableNodeOverloaded*>(&node))
                    {
                        // todo Add node to these errors
                        AddError(nullptr, ValidationConstPtr(aznew Internal::ParseError(node.GetEntityId(), AZStd::string::format("%s: %s", ParseErrors::NodeableNodeOverloadAmbiguous, node.GetDebugName().data()))));
                    }
                    else
                    {
                        // todo Add node to these errors
                        AddError(nullptr, ValidationConstPtr(aznew Internal::ParseError(node.GetEntityId(), AZStd::string::format("%s: %s", ParseErrors::NodeableNodeDidNotConstructInternalNodeable, node.GetDebugName().data()))));
                    }
                }
            }
            else if (auto functionCallNode = azrtti_cast<const Nodes::Core::FunctionCallNode*>(&node))
            {
                auto subgraphInterface = functionCallNode->GetSubgraphInterface();
                auto requiresCtorParamsForDependencies = subgraphInterface && subgraphInterface->RequiresConstructionParametersForDependencies();
                auto requiresCtorParams = subgraphInterface && subgraphInterface->RequiresConstructionParameters();

                if (requiresCtorParams)
                {
                    m_subgraphInterface.MarkRequiresConstructionParametersForDependencies();
                }

                // #functions2 pure on graph start nodes with dependencies can only by added to the graph as variables
                if (!functionCallNode->IsPure())
                {
                    Datum nodeableDatum(Data::Type::BehaviorContextObject(azrtti_typeid<Nodeable>()), Datum::eOriginality::Copy);

                    auto nodeableVariable = AddMemberVariable(nodeableDatum, functionCallNode->GetInterfaceName(), node.GetEntityId());

                    auto nodeableParse = AZStd::make_shared<NodeableParse>();
                    nodeableVariable->m_isExposedToConstruction = false;
                    nodeableParse->m_nodeable = nodeableVariable;
                    nodeableParse->m_simpleName = subgraphInterface->GetName();

                    m_nodeablesByNode.emplace(&node, nodeableParse);
                    m_userNodeables.insert(nodeableVariable);
                    m_dependencyByVariable.emplace(nodeableVariable, DependencyInfo{ functionCallNode->GetAssetId(), requiresCtorParams, requiresCtorParamsForDependencies });
                    return true;
                }
            }

            return false;
        }

        OutputAssignmentConstPtr AbstractCodeModel::CreateOutputData(ExecutionTreePtr execution, ExecutionChild& out, const Slot& outputSlot)
        {
            /// \note never called on a branch

            if (execution->GetSymbol() == Symbol::FunctionDefinition)
            {
                // Node output is input data to a function definition
                OutputAssignmentPtr output = CreateOutput(execution, outputSlot, {}, "input");

                if (auto variable = FindReferencedVariableChecked(execution, outputSlot))
                {
                    output->m_assignments.push_back(variable);
                    CheckConversions(output);
                }

                return output;
            }

            ReturnValueConnections connections = FindAssignments(execution, outputSlot);

            // get/set methods 
            if (IsVariableSet(execution) && !IsPropertyExtractionSlot(execution, &outputSlot))
            {
                AZ_Assert(out.m_output.size() == 1, "the output for Get/Set should already have been supplied");

                if (!connections.m_returnValuesOrReferences.empty())
                {
                    // return values must get assigned immediately, other inputs will simply read the output
                    OutputAssignmentPtr output = AZStd::const_pointer_cast<OutputAssignment>(out.m_output[0].second);
                    output->m_assignments.insert(output->m_assignments.end(), connections.m_returnValuesOrReferences.begin(), connections.m_returnValuesOrReferences.end());
                    CheckConversions(output);
                }

                // output already created for Set
                return nullptr;
            }

            if (!connections.m_returnValuesOrReferences.empty())
            {
                if (connections.m_hasOtherConnections || connections.m_returnValuesOrReferences.size() > 1)
                {
                    auto outputThread = CreateOutput(execution, outputSlot, GetOutputSlotNameOverride(execution, outputSlot), "output");
                    // this output will be written, and then assigned the assignments
                    outputThread->m_assignments = AZStd::move(connections.m_returnValuesOrReferences);
                    CheckConversions(outputThread);
                    return outputThread;
                }
                else
                {
                    // this output only needs to be directly written to the assignment
                    return CreateOutputAssignment(connections.m_returnValuesOrReferences[0]);
                }
            }
            else if (connections.m_hasOtherConnections)
            {
                // no return variable, but connected output which may be read by other inputs
                return CreateOutput(execution, outputSlot, GetOutputSlotNameOverride(execution, outputSlot), "output");
            }

            return CreateOutput(execution, outputSlot, GetOutputSlotNameOverride(execution, outputSlot), "");
        }

        OutputAssignmentPtr AbstractCodeModel::CreateOutput(ExecutionTreePtr execution, const Slot& outputSlot, AZStd::string_view slotNameOverride, AZStd::string_view suffix)
        {
            auto output = AZStd::make_shared<Variable>();
            output->m_source = execution;
            auto outputSlotDatum = outputSlot.FindDatum();
            // If slot has corresponding datum, use original one
            if (outputSlotDatum && outputSlotDatum->GetType().IsValid())
            {
                output->m_datum = *outputSlotDatum;
            }
            else
            {
                output->m_datum = AZStd::move(Datum(outputSlot.GetDataType(), Datum::eOriginality::Copy));
            }
            output->m_sourceSlotId = outputSlot.GetId();
            output->m_name = execution->ModScope()->AddVariableName(slotNameOverride.empty() ? outputSlot.GetName().c_str() : slotNameOverride.data(), suffix);
            output->m_isUnused = !execution->GetId().m_node->IsConnected(outputSlot); // \todo check for variable read/write names
            return CreateOutputAssignment(output);
        }

        OutputAssignmentPtr AbstractCodeModel::CreateOutputAssignment(VariableConstPtr output)
        {
            auto outputPtr = AZStd::make_shared<OutputAssignment>();
            outputPtr->m_source = output;
            return outputPtr;
        }

        bool AbstractCodeModel::CheckCreateUserEventHandling(const Node& node)
        {
            const EventHandingType handlerType = CheckEventHandlingType(node);
            switch (handlerType)
            {
            case EventHandingType::EBus:
                return CreateEBusHandling(node);

            case EventHandingType::Event:
                return CreateEventHandling(node);

            case EventHandingType::VariableWrite:
                return CreateVariableWriteHandling(node);

            default:
                AZ_Assert(handlerType == EventHandingType::Count, "new event handling type added but not handled");
                return false;
            }
        }

        void AbstractCodeModel::ConvertAllMemberVariablesToLocal(ExecutionTreePtr startNode)
        {
            auto useageIter = m_variableUseByExecution.find(startNode);
            if (useageIter == m_variableUseByExecution.end())
            {
                useageIter = m_variableUseByExecution.emplace(startNode, VariableUseage()).first;
            }

            for (auto constVariable : m_variables)
            {
                auto variable = AZStd::const_pointer_cast<Variable>(constVariable);
                if (variable->m_isMember && !variable->m_nodeableNodeId.IsValid())
                {
                    variable->m_isMember = false;
                    variable->m_source = startNode;
                    useageIter->second.localVariables.insert(constVariable);
                    useageIter->second.memberVariables.erase(constVariable);
                    m_variableUse.localVariables.insert(constVariable);
                    m_variableUse.memberVariables.erase(constVariable);
                }
            }

            auto& variableNamesInStart = ModStaticVariablesNames(startNode);
            for (auto& staticVariable : m_staticVariableNames)
            {
                if (AZStd::find(variableNamesInStart.begin(), variableNamesInStart.end(), staticVariable) == variableNamesInStart.end())
                {
                    variableNamesInStart.push_back(staticVariable);
                }
            }
        }

        void AbstractCodeModel::CreateUserFunctionDefinition(const Node& node, const Slot& entrySlot)
        {
            auto nodeling = azrtti_cast<const ScriptCanvas::Nodes::Core::FunctionDefinitionNode*>(&node);
            auto displayName = nodeling->GetDisplayName();

            if (m_uniqueInNames.contains(displayName))
            {
                AddError
                (nodeling->GetEntityId()
                    , nullptr
                    , AZStd::string::format
                        ( "%s is the name of multiple In Nodelings in a subgraph,\n"
                        "this will result in a difficult or impossible to use Function Node when used in another graph", displayName.c_str()));
                return;
            }
            else
            {
                m_uniqueInNames.insert(displayName);
            }

            ExecutionTreePtr definition = OpenScope(nullptr, nodeling, &entrySlot);
            definition->SetSymbol(Symbol::FunctionDefinition);
            definition->SetName(nodeling->GetDisplayName());

            m_userInsThatRequireTopology.insert({ nodeling, definition });
        }

        bool AbstractCodeModel::CheckCreateUserFunctionDefinition(const Node& node)
        {
            if (auto nodeling = azrtti_cast<const ScriptCanvas::Nodes::Core::FunctionDefinitionNode*>(&node))
            {
                if (auto entrySlot = nodeling->GetEntrySlot())
                {
                    CreateUserFunctionDefinition(node, *entrySlot);
                    return true;
                }
                else if (auto exitSlot = nodeling->GetExitSlot())
                {
                    auto displayName = nodeling->GetDisplayName();
                    if (m_uniqueOutNames.contains(displayName))
                    {
                        AddError(nodeling->GetEntityId()
                            , nullptr
                            , AZStd::string::format
                            ("%s is the name of multiple In Nodelings in a subgraph,\n"
                                "this will result in a difficult or impossible to use Function Node when used in another graph", displayName.data()));
                    }
                    else
                    {
                        m_uniqueOutNames.insert(displayName);
                    }

                    // turn this into a latent in the function node
                    m_userOutsThatRequireTopology.insert({ nodeling, nullptr });
                }
                else
                {
                    AddError(nodeling->GetEntityId(), nullptr, ScriptCanvas::ParseErrors::FunctionDefinitionNodeDidNotReturnSlot);
                }
            }

            return false;
        }

        bool AbstractCodeModel::CreateVariableWriteHandling(const Node& node)
        {
            if (node.IsVariableWriteHandler())
            {
                auto addressSlot = node.GetEBusConnectAddressSlot();
                AZ_Assert(addressSlot, "variable write handling must have address slot");
                AZ_Assert(m_variableWriteHandlingBySlot.find(addressSlot) == m_variableWriteHandlingBySlot.end(), "bad accounting of variable write handling, node has already been parsed");
                AZ_Assert(node.IsEBusAddressed(), "variable write handling bus has no address");

                auto boundVariableId = node.GetHandlerStartAddress();
                if (boundVariableId && boundVariableId->GetAs<GraphScopedVariableId>())
                {
                    if (auto boundVariable = FindBoundVariable(*boundVariableId->GetAs<GraphScopedVariableId>()))
                    {
                        AZ_Assert(boundVariable, "variable write handling gave no bound variable");
                        CreateVariableWriteHandling(*addressSlot, boundVariable, node.IsAutoConnected());
                        return true;
                    }

                }
            }

            return false;
        }

        void AbstractCodeModel::CreateVariableWriteHandling(const Slot& slot, VariableConstPtr boundVariable, bool startsConnected)
        {
            VariableWriteHandlingPtr variableWriteHandler = AZStd::make_shared<VariableWriteHandling>();
            variableWriteHandler->m_variable = boundVariable;
            variableWriteHandler->m_startsConnected = startsConnected;
            variableWriteHandler->m_isEverConnected = variableWriteHandler->m_startsConnected;

            // add to by slot records
            m_variableWriteHandlingBySlot.insert({ &slot, variableWriteHandler });

            // add to by variable records
            auto iter = m_variableWriteHandlingByVariable.find(boundVariable);
            if (iter == m_variableWriteHandlingByVariable.end())
            {
                VariableWriteHandlingSet variableWriteHandlingSet;
                variableWriteHandlingSet.insert(variableWriteHandler);
                m_variableWriteHandlingByVariable.insert({ boundVariable, variableWriteHandlingSet });
            }
            else
            {
                iter->second.insert(variableWriteHandler);
            }
        }

        void AbstractCodeModel::CullUnusedVariables()
        {
            AZStd::erase_if(m_variables, [this](auto variable)
            {
                if (IsManuallyDeclaredUserVariable(variable))
                {
                    if (variable->m_isMember)
                    {
                        if (!this->m_variableUse.memberVariables.contains(variable))
                        {
                            m_variablesUnused.push_back(variable);
                            return true;
                        }
                    }
                    else
                    {
                        if (!this->m_variableUse.localVariables.contains(variable))
                        {
                            m_variablesUnused.push_back(variable);
                            return true;
                        }
                    }
                }

                return false;
            });
        }

        bool AbstractCodeModel::ExecutionContainsCyclesCheck(const Node& node, const Slot& outSlot)
        {
            if (ExecutionContainsCycles(node, outSlot))
            {
                AddError(nullptr, aznew Internal::ParseError(node.GetEntityId(), AZStd::string::format
                    ( "Execution cycle detected (see connections to %s-%s. Use a looping node like While or For"
                    , node.GetDebugName().c_str(), outSlot.GetName().c_str()).c_str()));

                return true;
            }
            else
            {
                return false;
            }
        }

        AbstractCodeModel::ReturnValueConnections AbstractCodeModel::FindAssignments(ExecutionTreeConstPtr execution, const Slot& output)
        {
            ReturnValueConnections connections;

            if (auto variable = FindReferencedVariableChecked(execution, output))
            {
                connections.m_returnValuesOrReferences.push_back(variable);
            }

            auto connectedNodes = execution->GetId().m_node->GetConnectedNodes(output);
            bool isAtLeastOneReturnValueFound = false;

            for (const auto& nodeAndSlot : connectedNodes)
            {
                auto executionAndReturnVar = FindReturnValueOnThread(execution, nodeAndSlot.first, nodeAndSlot.second);

                if (executionAndReturnVar.first)
                {
                    connections.m_returnValuesOrReferences.push_back(executionAndReturnVar.second);
                    isAtLeastOneReturnValueFound = true;
                }

                if (IsAutoConnectedLocalEBusHandler(nodeAndSlot.first) || nodeAndSlot.first->IsNodeableNode())
                {
                    auto dataSlots = nodeAndSlot.first->GetOnVariableHandlingDataSlots();
                    if (AZStd::find(dataSlots.begin(), dataSlots.end(), nodeAndSlot.second) != dataSlots.end())
                    {
                        auto iter = m_variableWriteHandlingBySlot.find(nodeAndSlot.second);
                        if (iter == m_variableWriteHandlingBySlot.end())
                        {
                            AddError(nodeAndSlot.first->GetEntityId(), execution, ScriptCanvas::ParseErrors::VariableHandlingMissing);
                            break;
                        }

                        connections.m_returnValuesOrReferences.push_back(iter->second->m_variable);
                    }
                }
            }

            // if all output is on the thread, and other connections are required, store output, too
            // if there are return values off the thread, add an error
            connections.m_hasOtherConnections = isAtLeastOneReturnValueFound ? (connectedNodes.size() - 1 > 0) : (!connectedNodes.empty());
            return connections;
        }

        VariableConstPtr AbstractCodeModel::FindBoundVariable(GraphScopedVariableId variableId) const
        {
            for (auto variable : m_variables)
            {
                if (variable->m_sourceVariableId == variableId.m_identifier)
                {
                    return variable;
                }
            }

            return nullptr;
        }

        AbstractCodeModel::ConnectionsInPreviouslyExecutedScope AbstractCodeModel::FindConnectedInputInPreviouslyExecutedScope(ExecutionTreePtr executionWithInput, const EndpointsResolved& scriptCanvasNodesConnectedToInput, FirstNode firstNode) const
        {
            ConnectionsInPreviouslyExecutedScope result;
            ExecutionTreePtr outputChild = firstNode == FirstNode::Self ? nullptr : executionWithInput;
            ExecutionTreePtr outputSource = firstNode == FirstNode::Self ? executionWithInput : executionWithInput->ModParent();

            while (outputSource)
            {
                if (IsLooping(outputSource->GetSymbol()))
                {
                    // search loop body, root -> leaves, recursively for output
                    if (FindConnectedInputInPreviouslyExecutedScopeRecurse(result, outputSource->GetChild(0).m_execution, executionWithInput, scriptCanvasNodesConnectedToInput))
                    {
                        result.m_mostParent = outputSource;
                    }
                }
                else if (outputSource->GetSymbol() == Symbol::Sequence)
                {
                    size_t childSequenceIndex = outputSource->FindChildIndex(outputChild);
                    if (childSequenceIndex > 0 && childSequenceIndex < outputSource->GetChildrenCount())
                    {
                        do
                        {
                            // don't search the child that just missed input
                            --childSequenceIndex;
                            // search previous children, root -> leaves, recursively for output
                            if (FindConnectedInputInPreviouslyExecutedScopeRecurse(result, outputSource->GetChild(childSequenceIndex).m_execution, executionWithInput, scriptCanvasNodesConnectedToInput))
                            {
                                result.m_mostParent = outputSource;
                            }
                        }
                        while (childSequenceIndex);
                    }
                }

                outputChild = outputSource;
                outputSource = outputSource->ModParent();
            }

            return result;
        }

        bool AbstractCodeModel::FindConnectedInputInPreviouslyExecutedScopeRecurse(ConnectionsInPreviouslyExecutedScope& result, ExecutionTreeConstPtr outputSource, ExecutionTreePtr executionWithInput, const EndpointsResolved& scriptCanvasNodesConnectedToInput) const
        {
            size_t originalSize = result.m_connections.size();

            auto isConnectedToInput = [&](const Slot* slot)-> EndpointResolved
            {
                auto iter = AZStd::find_if(scriptCanvasNodesConnectedToInput.begin(), scriptCanvasNodesConnectedToInput.end(),
                    [&](const EndpointResolved& candidate)
                {
                    return slot && candidate.second == slot;
                });

                return iter != scriptCanvasNodesConnectedToInput.end() ? *iter : EndpointResolved{ nullptr, nullptr };
            };

            for (size_t childIndex = 0; childIndex < outputSource->GetChildrenCount(); ++childIndex)
            {
                const ExecutionChild& child = outputSource->GetChild(childIndex);

                for (size_t outputIndex = 0; outputIndex < child.m_output.size(); ++outputIndex)
                {
                    const auto& output = child.m_output[outputIndex];

                    EndpointResolved nodeAndSlot = isConnectedToInput(output.first);
                    if (nodeAndSlot.second)
                    {
                        ConnectionInPreviouslyExecutedScope connection;
                        connection.m_childIndex = childIndex;
                        connection.m_outputIndex = outputIndex;
                        connection.m_source = outputSource;
                        result.m_connections.push_back(connection);
                    }
                }

                if (child.m_execution)
                {
                    FindConnectedInputInPreviouslyExecutedScopeRecurse(result, child.m_execution, executionWithInput, scriptCanvasNodesConnectedToInput);
                }
            }

            return result.m_connections.size() > originalSize;
        }

        VariableConstPtr AbstractCodeModel::FindConnectedInputInScope(ExecutionTreePtr executionWithInput, const EndpointsResolved& scriptCanvasNodesConnectedToInput, FirstNode firstNode) const
        {
            ExecutionTreeConstPtr outputChild = executionWithInput;
            ExecutionTreeConstPtr outputSource = firstNode == FirstNode::Self ? outputChild : outputChild->GetParent();

            while (outputSource)
            {
                // check every connected SC Node
                for (const auto& scNodeAndOutputSlot : scriptCanvasNodesConnectedToInput)
                {
                    const auto outputSCNode = scNodeAndOutputSlot.first;
                    const auto outputSlot = scNodeAndOutputSlot.second;
                    const auto mostRecentOutputNodeOnThread = outputSource->GetId().m_node;

                    if (outputSCNode == mostRecentOutputNodeOnThread)
                    {
                        if (!IsPropertyExtractionSlot(outputSource, outputSlot) && (IsVariableGet(outputSource) || IsVariableSet(outputSource)))
                        {
                            return outputSource->GetChild(0).m_output[0].second->m_source;
                        }

                        for (size_t index(0); index < outputSource->GetChildrenCount(); ++index)
                        {
                            auto& child = outputSource->GetChild(index);

                            if (firstNode == FirstNode::Self || child.m_execution == outputChild)
                            {
                                for (const auto& sourceOutputVarPair : child.m_output)
                                {
                                    // this check fails get/set nodes if not the property extraction slot
                                    if (outputSlot->GetId() == sourceOutputVarPair.second->m_source->m_sourceSlotId)
                                    {
                                        for (const auto& otherConnections : scriptCanvasNodesConnectedToInput)
                                        {
                                            if (otherConnections.first == outputSCNode
                                                && otherConnections.second != outputSlot
                                                && InSimultaneousDataPath(*outputSCNode, *otherConnections.second, *outputSlot))
                                            {
                                                AddError(executionWithInput->GetId().m_node->GetEntityId(), executionWithInput, ParseErrors::MultipleSimulaneousInputValues);
                                            }
                                        }

                                        return sourceOutputVarPair.second->m_source;
                                    }
                                }
                            }
                        }
                    }
                }

                // look farther up the execution tree
                firstNode = FirstNode::Parent;
                outputChild = outputSource;
                outputSource = outputSource->GetParent();
            }

            return nullptr;
        }

        VariableConstPtr AbstractCodeModel::FindVariable(const AZ::EntityId& sourceNodeId) const
        {
            auto resultIter = AZStd::find_if
            (m_variables.begin()
                , m_variables.end()
                , [&sourceNodeId](const VariableConstPtr& candidate) { return candidate->m_nodeableNodeId == sourceNodeId; });

            return resultIter != m_variables.end() ? *resultIter : nullptr;
        }

        VariableConstPtr AbstractCodeModel::FindVariable(const VariableId& sourceVariableId) const
        {
            auto resultIter = AZStd::find_if
            (m_variables.begin()
                , m_variables.end()
                , [&sourceVariableId](const VariableConstPtr& candidate) { return candidate->m_sourceVariableId == sourceVariableId; });

            return resultIter != m_variables.end() ? *resultIter : nullptr;
        }

        AZStd::optional<AZStd::string> AbstractCodeModel::FindNodeableSimpleName(VariableConstPtr variable) const
        {
            if (IsUserNodeable(variable))
            {
                auto iter = AZStd::find_if(m_nodeablesByNode.begin(), m_nodeablesByNode.end(), [&variable](auto& candidate) { return candidate.second->m_nodeable == variable; });
                if (iter != m_nodeablesByNode.end())
                {
                    return iter->second->m_simpleName;
                }
            }

            return {};
        }

        const AZStd::pair<VariableConstPtr, AZStd::string>* AbstractCodeModel::FindStaticVariable(VariableConstPtr variable) const
        {
            auto iter = AZStd::find_if
            (m_staticVariableNames.begin()
                , m_staticVariableNames.end()
                , [&](const auto& candidate) { return candidate.first == variable; });

            return (iter != m_staticVariableNames.end()) ? iter : nullptr;
        }

        VariableConstPtr AbstractCodeModel::FindReferencedVariableChecked(ExecutionTreeConstPtr execution, const Slot& slot) const
        {
            if (slot.IsVariableReference())
            {
                if (auto variable = FindVariable(slot.GetVariableReference()))
                {
                    return variable;
                }
                else
                {
                    const_cast<AbstractCodeModel*>(this)->AddError(execution, aznew ParseError(slot.GetNodeId(), AZStd::string::format
                    ("Failed to find member variable for Variable Reference in slot: %s Id: %s"
                        , slot.GetName().data()
                        , slot.GetVariableReference().ToString().data())));
                }
            }

            return nullptr;
        }

        AZStd::pair<ExecutionTreeConstPtr, VariableConstPtr> AbstractCodeModel::FindReturnValueOnThread(ExecutionTreeConstPtr executionNode, const Node* node, const Slot* slot) const
        {
            auto root = executionNode->GetRoot();
            if (root && IsUserFunctionDefinition(root))
            {
                for (size_t index(0); index < root->GetReturnValueCount(); ++index)
                {
                    auto returnValue = root->GetReturnValue(index);

                    if (returnValue.second->m_source->m_sourceSlotId == slot->GetId())
                    {
                        // #functions2 slot<->variable determine if the root or the function call should be passed in here...the slot/node lead to the user call on the thread, but it may not even be created yet
                        return AZStd::make_pair(root, returnValue.second->m_source);
                    }
                }
            }
            else
            {
                auto execution = executionNode;

                while (execution)
                {
                    if (execution->GetId().m_node == node)
                    {
                        for (size_t index(0); index < execution->GetReturnValueCount(); ++index)
                        {
                            auto returnValue = execution->GetReturnValue(index);

                            if (returnValue.second->m_source->m_sourceSlotId == slot->GetId())
                            {
                                return AZStd::make_pair(execution, returnValue.second->m_source);
                            }
                        }
                    }

                    execution = execution->GetParent();
                }

            }

            return {};
        }

        AZStd::vector<VariablePtr> AbstractCodeModel::FindUserImmediateInput(ExecutionTreePtr call) const
        {
            AZStd::vector<VariablePtr> inputsVariables;

            const auto inputs = call->GetId().m_node->GetSlotsByType(CombinedSlotType::DataOut);

            for (const auto& input : inputs)
            {
                auto iter = m_inputVariableByNodelingInSlot.find(input);
                if (iter != m_inputVariableByNodelingInSlot.end())
                {
                    VariablePtr variable = iter->second;
                    const Slot* slot = iter->first;
                    variable->m_name = call->ModScope()->AddVariableName(slot->GetName());
                    variable->m_source = call;
                    inputsVariables.push_back(variable);
                }
            }

            return inputsVariables;
        }

        const AbstractCodeModel::ReturnValueDescription* AbstractCodeModel::FindUserImmediateOutput(ExecutionTreePtr call) const
        {
            auto iter = m_returnValuesByUserFunctionDefinition.find(azrtti_cast<const ScriptCanvas::Nodes::Core::FunctionDefinitionNode*>(call->GetId().m_node));
            return iter != m_returnValuesByUserFunctionDefinition.end() ? &iter->second : nullptr;
        }

        AZStd::vector<VariablePtr> AbstractCodeModel::FindUserLatentOutput(ExecutionTreePtr call) const
        {
            AZStd::vector<VariablePtr> outputVariables;

            const auto outputs = call->GetId().m_node->GetSlotsByType(CombinedSlotType::DataIn);

            for (const auto& output : outputs)
            {
                auto iter = m_outputVariableByNodelingOutSlot.find(output);
                if (iter != m_outputVariableByNodelingOutSlot.end())
                {
                    VariablePtr variable = iter->second;
                    const Slot* slot = iter->first;
                    variable->m_name = call->ModScope()->AddVariableName(slot->GetName());
                    variable->m_source = call;
                    outputVariables.push_back(variable);
                }
            }

            return outputVariables;
        }

        AZStd::vector<VariablePtr> AbstractCodeModel::FindUserLatentReturnValues(ExecutionTreePtr call) const
        {
            AZStd::vector<VariablePtr> returnValues;

            const auto outputs = call->GetId().m_node->GetSlotsByType(CombinedSlotType::DataOut);

            for (const auto& output : outputs)
            {
                auto iter = m_returnVariableByNodelingOutSlot.find(output);
                if (iter != m_returnVariableByNodelingOutSlot.end())
                {
                    VariablePtr variable = iter->second;
                    const Slot* slot = iter->first;
                    variable->m_name = call->ModScope()->AddVariableName(slot->GetName());
                    variable->m_source = call;
                    returnValues.push_back(iter->second);
                }
            }

            return returnValues;
        }

        AZStd::vector<ExecutionTreeConstPtr> AbstractCodeModel::GetAllExecutionRoots() const
        {
            AZStd::vector<ExecutionTreePtr> nonConstRoots = const_cast<AbstractCodeModel*>(this)->ModAllExecutionRoots();
            AZStd::vector<ExecutionTreeConstPtr> constRoots;
            constRoots.reserve(nonConstRoots.size());

            for (auto root : nonConstRoots)
            {
                constRoots.push_back(root);
            }

            return constRoots;
        }

        AZStd::vector<AZStd::pair<VariableConstPtr, AZStd::string>> AbstractCodeModel::GetAllDeactivationVariables() const
        {
            AZStd::vector<AZStd::pair<VariableConstPtr, AZStd::string>> variables;

            for (const auto& nodeable : m_nodeablesByNode)
            {
                variables.push_back(AZStd::make_pair(nodeable.second->m_nodeable, k_DeactivateName));
            }

            for (const auto& ebusHandler : m_ebusHandlingByNode)
            {
                auto variable = AZStd::make_shared<Variable>();
                variable->m_isMember = true;
                variable->m_name = ebusHandler.second->m_handlerName;
                variable->m_datum = Datum(ebusHandler.second->m_handlerName);
                variables.push_back(AZStd::make_pair(variable, k_DeactivateName));
            }

            for (const auto& eventHandler : m_eventHandlingByNode)
            {
                variables.push_back(AZStd::make_pair(eventHandler.second->m_handler, k_AzEventHandlerDisconnectName));
            }

            return variables;
        }

        const size_t* AbstractCodeModel::GetDebugInfoInIndex(ExecutionTreeConstPtr execution) const
        {
            auto iter = m_debugMapReverse.m_in.find(execution);
            return iter != m_debugMapReverse.m_in.end() ? &iter->second : nullptr;
        }

        const size_t* AbstractCodeModel::GetDebugInfoOutIndex(ExecutionTreeConstPtr execution, size_t index) const
        {
            auto iter = m_debugMapReverse.m_out.find(execution);
            return iter != m_debugMapReverse.m_out.end() ? &iter->second[index] : nullptr;
        }

        const size_t* AbstractCodeModel::GetDebugInfoReturnIndex(ExecutionTreeConstPtr execution) const
        {
            auto iter = m_debugMapReverse.m_return.find(execution);
            return iter != m_debugMapReverse.m_return.end() ? &iter->second : nullptr;
        }

        const size_t* AbstractCodeModel::GetDebugInfoVariableAssignmentIndex(OutputAssignmentConstPtr output, size_t assignmentIndex) const
        {
            auto outputIter = m_debugMapReverse.m_assignments.find(output);
            if (outputIter != m_debugMapReverse.m_assignments.end())
            {
                auto indexIter = outputIter->second.find(assignmentIndex);
                if (indexIter != outputIter->second.end())
                {
                    return &indexIter->second;
                }
            }

            return nullptr;
        }

        const size_t* AbstractCodeModel::GetDebugInfoVariableSetIndex(OutputAssignmentConstPtr output) const
        {
            auto iter = m_debugMapReverse.m_variableSets.find(output);
            return iter != m_debugMapReverse.m_variableSets.end() ? &iter->second : nullptr;
        }

        const DebugSymbolMap& AbstractCodeModel::GetDebugMap() const
        {
            return m_debugMap;
        }

        const OrderedDependencies& AbstractCodeModel::GetOrderedDependencies() const
        {
            return m_orderedDependencies;
        }

        EBusHandlingConstPtr AbstractCodeModel::GetEBusEventHandling(const Node* node) const
        {
            auto iter = m_ebusHandlingByNode.find(node);
            return iter != m_ebusHandlingByNode.end() ? iter->second : nullptr;
        }

        AZStd::vector<EBusHandlingConstPtr> AbstractCodeModel::GetEBusHandlings() const
        {
            AZStd::vector<EBusHandlingConstPtr> handlings;

            for (auto handling : m_ebusHandlingByNode)
            {
                handlings.push_back(handling.second);
            }

            return handlings;
        }

        EventHandlingConstPtr AbstractCodeModel::GetEventHandling(const Node* node) const
        {
            auto iter = m_eventHandlingByNode.find(node);
            return iter != m_eventHandlingByNode.end() ? iter->second : nullptr;
        }

        AZStd::vector<EventHandlingConstPtr> AbstractCodeModel::GetEventHandlings() const
        {
            AZStd::vector<EventHandlingConstPtr> handlings;

            for (auto handling : m_eventHandlingByNode)
            {
                handlings.push_back(handling.second);
            }

            return handlings;
        }

        AZStd::vector<ExecutionTreeConstPtr> AbstractCodeModel::GetFunctions() const
        {
            AZStd::vector<ExecutionTreeConstPtr> functions = m_functions;

            for (auto variableHandling : m_variableWriteHandlingBySlot)
            {
                functions.push_back(variableHandling.second->m_function);
            }

            return functions;
        }

        VariableConstPtr AbstractCodeModel::GetImplicitVariable(ExecutionTreeConstPtr execution) const
        {
            auto iter = m_implicitVariablesByNode.find(execution);
            return iter != m_implicitVariablesByNode.end() ? iter->second : nullptr;
        }

        const SubgraphInterface& AbstractCodeModel::GetInterface() const
        {
            return m_subgraphInterface;
        }

        const AZStd::unordered_set<VariableConstPtr>* AbstractCodeModel::GetLocalVariables(ExecutionTreeConstPtr execution) const
        {
            auto iter = m_variableUseByExecution.find(execution);
            return iter != m_variableUseByExecution.end() ? &iter->second.localVariables : nullptr;
        }

        AZStd::vector<NodeableParseConstPtr> AbstractCodeModel::GetNodeableParse() const
        {
            AZStd::vector<NodeableParseConstPtr> nodeableParse;

            for (auto iter : m_nodeablesByNode)
            {
                nodeableParse.push_back(iter.second);
            }

            return nodeableParse;
        }

        AZStd::string AbstractCodeModel::GetOriginalVariableName(VariableConstPtr variable, const Node* node)
        {
            if (variable->m_sourceVariableId.IsValid())
            {
                auto graphVariable = m_source.m_variableData->FindVariable(variable->m_sourceVariableId);
                if (graphVariable)
                {
                    return graphVariable->GetVariableName();
                }
                else
                {
                    AddError(AZ::EntityId(), nullptr, AZStd::string::format("Missing graph variable by source variable id: %s", variable->m_sourceVariableId.ToString().data()).c_str());
                }
            }
            else if (node && variable->m_sourceSlotId.IsValid())
            {
                if (auto slot = node->GetSlot(variable->m_sourceSlotId))
                {
                    return slot->GetName();
                }
                else
                {
                    AddError(AZ::EntityId(), nullptr, AZStd::string::format("Missing graph variable by source slot id: %s", variable->m_sourceSlotId.ToString().data()).c_str());
                }
            }
            else
            {
                AddError(AZ::EntityId(), nullptr, AZStd::string::format("Can't find original slot/variable name for parsed variable"));
            }

            return "";
        }

        AZStd::string AbstractCodeModel::GetOutputSlotNameOverride(ExecutionTreePtr execution, const Slot& outputSlot)
        {
            AZStd::string name;

            if (IsPropertyExtractionSlot(execution, &outputSlot))
            {
                name = outputSlot.GetName();
            }

            if (IsVariableSet(execution))
            {
                return GetWrittenVariable(execution)->m_name + name;
            }
            else if (IsVariableGet(execution))
            {
                return GetReadVariable(execution) ? GetReadVariable(execution)->m_name + name : "UNKNOWN";
            }

            return {};
        }

        AZStd::sys_time_t AbstractCodeModel::GetParseDuration() const
        {
            return m_parseDuration;
        }

        ExecutionCharacteristics AbstractCodeModel::GetExecutionCharacteristics() const
        {
            return m_subgraphInterface.GetExecutionCharacteristics();
        }

        VariableConstPtr AbstractCodeModel::GetReadVariable(ExecutionTreePtr execution)
        {
            VariableId variableId = execution->GetId().m_node->GetVariableIdRead(execution->GetId().m_slot);
            return variableId.IsValid() ? FindVariable(variableId) : nullptr;
        }

        const ParsedRuntimeInputs& AbstractCodeModel::GetRuntimeInputs() const
        {
            return m_runtimeInputs;
        }

        const Source& AbstractCodeModel::GetSource() const
        {
            return m_source;
        }

        const AZStd::string& AbstractCodeModel::GetSourceString() const
        {
            return m_source.m_assetIdString;
        }

        const AZStd::vector<AZStd::pair<VariableConstPtr, AZStd::string>>& AbstractCodeModel::GetStaticVariablesNames() const
        {
            return m_staticVariableNames;
        }

        const AZStd::vector<AZStd::pair<VariableConstPtr, AZStd::string>>& AbstractCodeModel::GetStaticVariablesNames(ExecutionTreeConstPtr functionBlock) const
        {
            return const_cast<AbstractCodeModel*>(this)->ModStaticVariablesNames(functionBlock);
        }

        ExecutionTreeConstPtr AbstractCodeModel::GetStart() const
        {
            return m_start;
        }

        VariableConstPtr AbstractCodeModel::GetWrittenVariable(ExecutionTreePtr execution)
        {
            VariableId variableId = execution->GetId().m_node->GetVariableIdWritten(execution->GetId().m_slot);
            return variableId.IsValid() ? FindVariable(variableId) : nullptr;
        }

        VariableWriteHandlingConstPtr AbstractCodeModel::GetVariableHandling(const Slot* slot) const
        {
            auto iter = m_variableWriteHandlingBySlot.find(slot);
            return iter != m_variableWriteHandlingBySlot.end() ? iter->second : nullptr;
        }

        VariableWriteHandlingConstSet AbstractCodeModel::GetVariableHandling(VariableConstPtr variable) const
        {
            VariableWriteHandlingConstSet constSet;

            auto byVarIter = m_variableWriteHandlingByVariable.find(variable);
            if (byVarIter != m_variableWriteHandlingByVariable.end())
            {
                for (auto iter : byVarIter->second)
                {
                    constSet.insert(iter);
                }
            }

            return constSet;
        }

        const AZStd::vector<VariableConstPtr>& AbstractCodeModel::GetVariables() const
        {
            return m_variables;
        }

        const AZStd::vector<VariableConstPtr>& AbstractCodeModel::GetVariablesUnused() const
        {
            return m_variablesUnused;
        }

        bool AbstractCodeModel::IsActiveGraph() const
        {
            if (!m_nodeablesByNode.empty())
            {
                return true;
            }

            if (m_subgraphInterface.IsActiveDefaultObject())
            {
                return true;
            }

            if (m_subgraphInterface.HasPublicFunctionality())
            {
                return true;
            }

            return false;
        }

        bool AbstractCodeModel::IsAutoConnectedLocalEBusHandler(const Node* node) const
        {
            auto ebusHandlingIter = m_ebusHandlingByNode.find(node);
            return ebusHandlingIter != m_ebusHandlingByNode.end()
                && ebusHandlingIter->second->m_isAutoConnected;
        }

        bool AbstractCodeModel::IsErrorFree() const
        {
            return m_isErrorFree;
        }

        bool AbstractCodeModel::InSimultaneousDataPath(const Node& node, const Slot& reference, const Slot& candidate) const
        {
            AZStd::vector<const Slot*> combinedOutSlots = node.GetSlotsByType(CombinedSlotType::ExecutionOut);
            AZStd::vector<const Slot*> latentSlots = node.GetSlotsByType(CombinedSlotType::LatentOut);
            combinedOutSlots.insert(combinedOutSlots.end(), latentSlots.begin(), latentSlots.end());

            for (auto outSlot : combinedOutSlots)
            {
                ConstSlotsOutcome outcome = node.GetSlotsInExecutionThreadByType(*outSlot, CombinedSlotType::DataOut);

                if (outcome.IsSuccess())
                {
                    const AZStd::vector<const Slot*>& slots = outcome.GetValue();
                    if (AZStd::find(slots.begin(), slots.end(), &reference) != slots.end()
                        && AZStd::find(slots.begin(), slots.end(), &candidate) != slots.end())
                    {
                        return true;
                    }
                }
                else
                {
                    AddError(node.GetEntityId(), nullptr, outcome.GetError().data());
                }
            }

            return false;
        }

        bool AbstractCodeModel::IsPerEntityDataRequired() const
        {
            return !IsPureLibrary();
        }

        bool AbstractCodeModel::IsPureLibrary() const
        {
            return m_subgraphInterface.IsMarkedPure();
        }

        bool AbstractCodeModel::IsSourceInScope(VariableConstPtr variable, VariableFlags::Scope scope) const
        {
            auto& sourceVariables = m_source.m_variableData->GetVariables();

            if (variable->m_sourceVariableId.IsValid())
            {
                auto sourceVariableIter = sourceVariables.find(variable->m_sourceVariableId);
                if (sourceVariableIter != sourceVariables.end())
                {
                    if (sourceVariableIter->second.IsInScope(scope))
                    {
                        return true;
                    }
                }
                else
                {
                    AZ_Assert(false, "bad variable id");
                }
            }

            return false;
        }

        bool AbstractCodeModel::IsUserNodeable() const
        {
            // #functions2 check the subgraph interface for IsUserNodeable vs User variable
            return !IsPureLibrary();
        }

        bool AbstractCodeModel::IsUserNodeable(VariableConstPtr variable) const
        {
            return m_userNodeables.contains(variable);
        }

        void AbstractCodeModel::MarkParseStart()
        {
            m_parseStartTime = AZStd::chrono::system_clock::now();
        }

        void AbstractCodeModel::MarkParseStop()
        {
            m_parseDuration = AZStd::chrono::microseconds(AZStd::chrono::system_clock::now() - m_parseStartTime).count();
        }

        AZStd::vector<ExecutionTreePtr> AbstractCodeModel::ModAllExecutionRoots()
        {
            AZStd::vector<ExecutionTreePtr> roots;

            if (m_start)
            {
                roots.push_back(m_start);
            }

            for (auto& nodeableParse : m_nodeablesByNode)
            {
                for (auto& latent : nodeableParse.second->m_latents)
                {
                    if (latent.second)
                    {
                        roots.push_back(AZStd::const_pointer_cast<ExecutionTree>(latent.second));
                    }
                }
            }

            for (auto& eventHandlerParse : m_ebusHandlingByNode)
            {
                for (auto& event : eventHandlerParse.second->m_events)
                {
                    if (event.second)
                    {
                        roots.push_back(AZStd::const_pointer_cast<ExecutionTree>(event.second));
                    }
                }
            }

            for (auto& eventHandlerParse : m_eventHandlingByNode)
            {
                if (eventHandlerParse.second->m_eventHandlerFunction)
                {
                    roots.push_back(AZStd::const_pointer_cast<ExecutionTree>(eventHandlerParse.second->m_eventHandlerFunction));
                }
            }

            for (auto variableWriteHandling : m_variableWriteHandlingBySlot)
            {
                if (variableWriteHandling.second->m_function)
                {
                    roots.push_back(AZStd::const_pointer_cast<ExecutionTree>(variableWriteHandling.second->m_function));
                }
            }

            for (auto function : m_functions)
            {
                if (function)
                {
                    roots.push_back(AZStd::const_pointer_cast<ExecutionTree>(function));
                }
            }

            return roots;
        }

        AZStd::vector<AZStd::pair<VariableConstPtr, AZStd::string>>& AbstractCodeModel::ModStaticVariablesNames()
        {
            return m_staticVariableNames;
        }

        AZStd::vector<AZStd::pair<VariableConstPtr, AZStd::string>>& AbstractCodeModel::ModStaticVariablesNames(Grammar::ExecutionTreeConstPtr functionBlock)
        {
            auto iter = m_staticVariableNamesByFunctionBlock.find(functionBlock);
            if (iter == m_staticVariableNamesByFunctionBlock.end())
            {
                iter = m_staticVariableNamesByFunctionBlock.insert(AZStd::make_pair(functionBlock, AZStd::vector<AZStd::pair<VariableConstPtr, AZStd::string>>())).first;
            }

            return iter->second;
        }

        ExecutionTreePtr AbstractCodeModel::OpenScope(ExecutionTreePtr parent, const Node* node, const Slot* outSlot) const
        {
            ExecutionTreePtr child = CreateChild(parent, node, outSlot);
            child->SetScope(AZStd::make_shared<Scope>());
            child->ModScope()->m_parent = parent ? parent->ModScope() : m_graphScope;
            return child;
        }

        void AbstractCodeModel::Parse()
        {
            MarkParseStart();

            m_subgraphInterface.SetNamespacePath(m_source.m_namespacePath);

            // The Order Matters: begin

            // add all data to the ACM for easy look up in input/output processing for ACM nodes
            AddAllVariablesPreParse();
            if (!IsErrorFree())
            {
                return;
            }

            // parse basic editor nodes as they may add implicit variables
            for (auto& nodeEntity : m_source.m_graphData->m_nodes)
            {
                if (nodeEntity)
                {
                    if (auto node = AZ::EntityUtils::FindFirstDerivedComponent<Node>(nodeEntity))
                    {
                        if (Parse(*node))
                        {
                            m_possibleExecutionRoots.push_back(node);
                        }
                    }
                    else
                    {
                        AddError(nullptr, ValidationConstPtr(aznew NullNodeInGraph(nodeEntity->GetId(), nodeEntity->GetName())));
                    }
                }
                else
                {
                    AddError(nullptr, ValidationConstPtr(aznew NullEntityInGraph()));
                }

                if (!IsErrorFree())
                {
                    return;
                }
            }

            if (!IsErrorFree())
            {
                return;
            }

            // parse the implicit variables added by ebus handling syntax sugar
            ParseAutoConnectedEBusHandlerVariables();
            // all possible data is available, now parse execution, starting with "main", currently keyed to RuntimeComponent::Activate
            Parse(m_startNodes);
            // parse any function introduced by nodes other than On Graph Start/"main"
            for (auto node : m_possibleExecutionRoots)
            {
                ParseExecutionTreeRoots(*node);
            }
            // parse functions introduced by variable change events
            ParseVariableHandling();
            // parse all user function and function object signatures
            ParseUserFunctionTopology();
            // culls unused variables, and determine whether the the graph defines an object or static functionality
            ParseExecutionCharacteristics();
            // now that variables have been culled, determined what data needs to be initialized by an external source
            ParseConstructionInputVariables();
            // now that externally initialized data has been identified, associate local, static initializers with individual functions
            ParseFunctionLocalStaticUseage();

            // The Order Matters: end

            // from here on, nothing more needs to happen during simple parsing
            // for example, in the editor, to get validation on syntax based effects for the view
            // parsing could stop now

            if (IsErrorFree())
            {
                // the graph could have used several user graphs which required construction, and maybe multiple instances of the same user asset
                // this will create indices for those nodes to be able to pass in the proper entry in the construction argument tree at translation and runtime
                ParseDependenciesAssetIndicies();
                // protect all names against keyword collision and language naming violations
                ConvertNamesToIdentifiers();

                if (m_source.m_addDebugInfo)
                {
                    AddDebugInformation();
                }

                if (!IsActiveGraph())
                {
                    if (m_source.m_graphData->m_nodes.empty())
                    {
                        AddError(AZ::EntityId(), nullptr, ScriptCanvas::ParseErrors::EmptyGraph);
                    }
                    else
                    {
                        AddError(nullptr, ValidationConstPtr(aznew InactiveGraph()));
                    }
                }
                else
                {
                    MarkParseStop();

                    if (m_source.m_printModelToConsole)
                    {
                        AZStd::string pretty;
                        PrettyPrint(pretty, *this);
                        AZ_TracePrintf("ScriptCanvas", "%s", pretty.data());
                        AZ_TracePrintf("ScriptCanvas", "SubgraphInterface:");
                        AZ_TracePrintf("ScriptCanvas", ToString(m_subgraphInterface).data());
                    }

                    AZ_TracePrintf("Script Canvas", "Parse Duration: %8.4f ms\n", m_parseDuration / 1000.0);
                }
            }
        }

        void AbstractCodeModel::ParseAutoConnectedEBusHandlerVariables()
        {
            // *** NOTE *** this means that for all ebus connect calls, the input is broken
            // if it is an auto connected ebus, the input can't be the output of the previous node
            // it has to be the member variable which will then be written to
            // so that will have to get fixed up
            for (auto& nodeAndHandling : m_ebusHandlingByNode)
            {
                EBusHandlingConstPtr ebusHandling = nodeAndHandling.second;

                if (ebusHandling->m_isAutoConnected)
                {
                    m_subgraphInterface.MarkActiveDefaultObject();

                    if (ebusHandling->m_isAddressed)
                    {
                        m_subgraphInterface.MarkActiveDefaultObject();
                        const auto name = m_graphScope->AddFunctionName(AZStd::string::format("On%sAddressChanged", ebusHandling->m_ebusName.c_str()).c_str());
                        ebusHandling->m_startingAdress->m_isMember = true;
                        // add a new variable handling, that is never disconnected, at always controls this ebus connection
                        // make bound variables of their data, if it doesn't exist already
                        // mark all of these variables as handler control addresses
                        auto addressSlot = nodeAndHandling.first->GetEBusConnectAddressSlot();
                        AZ_Assert(addressSlot, "addressed ebus handler node must have address slot.");
                        CreateVariableWriteHandling(*addressSlot, ebusHandling->m_startingAdress, nodeAndHandling.first->IsAutoConnected());
                        VariableWriteHandlingPtr addressChangeHandling = AZStd::const_pointer_cast<VariableWriteHandling>(GetVariableHandling(addressSlot));
                        AZ_Assert(addressChangeHandling != nullptr, "failure to create variable handling for ebus address");
                        ExecutionTreePtr onAddressChange = OpenScope(nullptr, nodeAndHandling.first, nullptr);
                        onAddressChange->SetSymbol(Symbol::FunctionDefinition);
                        onAddressChange->SetName(name);

                        // add the disconnect call
                        ExecutionTreePtr disconnect = CreateChild(onAddressChange, nodeAndHandling.first, nodeAndHandling.first->GetEBusDisconnectSlot());
                        ParseInputThisPointer(disconnect);
                        onAddressChange->AddChild({ nullptr, {}, disconnect });

                        // add the connect call
                        ExecutionTreePtr connect = CreateChild(disconnect, nodeAndHandling.first, nodeAndHandling.first->GetEBusConnectSlot());
                        ParseInputThisPointer(connect);
                        connect->AddInput({ nullptr, ebusHandling->m_startingAdress, DebugDataSource::FromInternal() });
                        disconnect->AddChild({ nullptr, {}, connect });

                        FunctionCallDefaultMetaData metaData;
                        metaData.PostParseExecutionTreeBody(*this, connect);
                        metaData.PostParseExecutionTreeBody(*this, disconnect);

                        addressChangeHandling->m_function = onAddressChange;
                    }
                }
            }
        }

        void AbstractCodeModel::Parse(const AZStd::vector<const Nodes::Core::Start*>& startNodes)
        {
            if (startNodes.empty() && m_subgraphStartCalls.empty())
            {
                return;
            }

            auto startNode = !startNodes.empty() ? startNodes.front()
                : !m_subgraphStartCalls.empty() ? *m_subgraphStartCalls.begin() : nullptr;

            ExecutionTreePtr start = OpenScope(nullptr, startNode, nullptr);
            start->SetSymbol(Symbol::FunctionDefinition);
            m_start = start; // cache the function definition

            if (!m_subgraphStartCalls.empty())
            {
                // call OnGraphStart on all the member nodeables first
                for (auto node : m_subgraphStartCalls)
                {
                    ExecutionTreePtr childStartCall = CreateChild(start, node, nullptr);
                    childStartCall->SetSymbol(Symbol::FunctionCall);
                    childStartCall->SetName(k_OnGraphStartFunctionName);
                    childStartCall->MarkStartCall();

                    auto lexicalScopeOutcome = node->GetFunctionCallLexicalScope(nullptr);

                    if (lexicalScopeOutcome.IsSuccess())
                    {
                        childStartCall->SetNameLexicalScope(lexicalScopeOutcome.GetValue());
                        ParseInputThisPointer(childStartCall);
                        start->AddChild({ nullptr, {}, childStartCall });
                        start = childStartCall;
                    }
                    else
                    {
                        AddError(node->GetEntityId(), nullptr, ScriptCanvas::ParseErrors::SubgraphOnGraphStartFailedToReturnLexicalScope);
                        return;
                    }
                }
            }

            EndpointsResolved outNodes;
            AZStd::vector<const Slot*> outSlots;

            for (auto startNode2 : startNodes)
            {
                auto& node = *startNode2;

                auto outSlot = node.GetSlot(node.GetSlotIdByType("Out", CombinedSlotType::ExecutionOut));
                if (!outSlot)
                {
                    AddError(node.GetEntityId(), nullptr, ScriptCanvas::ParseErrors::NoOutSlotInStart);
                    return;
                }

                if (ExecutionContainsCyclesCheck(node, *outSlot))
                {
                    return;
                }

                auto executionOutNodes = node.GetConnectedNodes(*outSlot);
                if (!executionOutNodes.empty())
                {
                    outNodes.insert(outNodes.end(), executionOutNodes.begin(), executionOutNodes.end());
                    outSlots.insert(outSlots.end(), executionOutNodes.size(), outSlot);
                }
            }

            // ExecutionTreePtr start is now either the last child start() call, or the beginning of the function block,
            // either way, parsing can continue from the ExecutionTreePtr start.

            if (!outSlots.empty())
            {
                start->AddChild({ outSlots[0], {}, nullptr });
                
                ParseExecutionMultipleOutSyntaxSugar(start, outNodes, outSlots);
                PostParseProcess(start);
                PostParseErrorDetect(start);

                if (!IsErrorFree())
                {
                    start->Clear();
                    m_start->Clear();
                    AddError(AZ::EntityId{}, nullptr, ScriptCanvas::ParseErrors::StartNodeFailedToParse);
                    return;
                }
            }

            if (m_start)
            {
                m_start->SetName(k_OnGraphStartFunctionName);
                m_subgraphInterface.MarkOnGraphStart();
            }
        }

        bool AbstractCodeModel::Parse(const Node& node)
        {
            if (!node.IsNodeEnabled())
            {
                return false;
            }

            if (auto functionNode = azrtti_cast<const Nodes::Core::FunctionCallNode*>(&node))
            {
                Nodes::Core::FunctionCallNodeCompareConfig config;

                if (functionNode->IsOutOfDate(config))
                {
                    AZ_Warning("ScriptCanvas", false, "%s node is out-of-date.", node.GetNodeName().c_str());
                    AddError(nullptr, aznew NodeCompatiliblity::NodeOutOfDate(node.GetEntityId(), node.GetNodeName()));
                    return false;
                }
            }
            else if (node.IsOutOfDate(m_source.m_graph->GetVersion()))
            {
                AZ_Warning("ScriptCanvas", false, "%s node is out-of-date.", node.GetNodeName().c_str());
                AddError(nullptr, aznew NodeCompatiliblity::NodeOutOfDate(node.GetEntityId(), node.GetNodeName()));
                return false;
            }

            if (auto start = azrtti_cast<const Nodes::Core::Start*>(&node))
            {
                m_startNodes.push_back(start);
                return false;
            }
            else
            {
                ParseDependencies(node);
                ParseImplicitVariables(node);
                return CheckCreateRoot(node);
            }
        }

        bool AbstractCodeModel::Parse(VariableWriteHandlingPtr variableHandling)
        {
            if (!variableHandling->m_startsConnected && !variableHandling->m_isEverConnected)
            {
                AddError(AZ::EntityId(), variableHandling->m_function, ScriptCanvas::ParseErrors::InfiniteLoopWritingToVariable);
                return false;
            }

            if (IsInfiniteVariableWriteHandlingLoop(*this, variableHandling, variableHandling->m_function, true))
            {
                AddError(variableHandling->m_function, aznew Internal::ParseError(AZ::EntityId(), ScriptCanvas::ParseErrors::InfiniteLoopWritingToVariable));
                return false;
            }

            if (variableHandling->m_connectionVariable && !variableHandling->RequiresConnectionControl())
            {
                variableHandling->m_connectionVariable->m_isMember = false;
                m_variables.erase(AZStd::remove(m_variables.begin(), m_variables.end(), variableHandling->m_connectionVariable), m_variables.end());
                variableHandling->m_connectionVariable = nullptr;
            }

            return true;
        }

        AbstractCodeModelConstPtr AbstractCodeModel::Parse(const Source& source, bool terminateOnError, bool terminateOnInternalError)
        {
            return AZStd::make_shared<AbstractCodeModel>(source, terminateOnError, terminateOnInternalError);
        }

        void AbstractCodeModel::ParseExecutionBreak(ExecutionTreePtr execution)
        {
            if (auto forEach = azrtti_cast<const ScriptCanvas::Nodes::Core::ForEach*>(execution->GetId().m_node))
            {
                if (forEach->GetLoopBreakSlotId() == execution->GetId().m_slot->GetId())
                {
                    auto target = execution->GetParent();
                    while (target)
                    {
                        if (auto forEach2 = azrtti_cast<const ScriptCanvas::Nodes::Core::ForEach*>(target->GetId().m_node))
                        {
                            // This check is to make sure ForEach break slot is connected within correct execution scope
                            // 1. parent execution should be the same foreach node as current execution
                            // 2. parent execution slot should be loop each slot
                            if (forEach2 != execution->GetId().m_node || forEach2->GetLoopSlotId() != target->GetId().m_slot->GetId())
                            {
                                AddError(forEach2->GetEntityId(), execution, ScriptCanvas::ParseErrors::BreakNotInForEachScope);
                            }
                            break;
                        }
                        target = target->GetParent();
                    }
                }
            }

            execution->SetSymbol(Symbol::Break);
        }

        VariableConstPtr AbstractCodeModel::ParseConnectedInputData(const Slot& inputSlot, ExecutionTreePtr executionWithInput, const EndpointsResolved& scriptCanvasNodesConnectedToInput, FirstNode firstNode)
        {
            if (auto inScopeVar = FindConnectedInputInScope(executionWithInput, scriptCanvasNodesConnectedToInput, firstNode))
            {
                return inScopeVar;
            }

            // do this exact thing for multiple out sequence sugar, and change the way those are translated 
            auto inPreviouslyExecutedScopeResult = FindConnectedInputInPreviouslyExecutedScope(executionWithInput, scriptCanvasNodesConnectedToInput, firstNode);

            if (!inPreviouslyExecutedScopeResult.m_connections.empty())
            {
                // get the required scope
                auto mostForLoopParent = inPreviouslyExecutedScopeResult.m_mostParent->ModParent();

                // add a variable to the required scope
                auto newInputResultOfAssignment = AZStd::make_shared<Variable>();
                auto variableName = mostForLoopParent->ModScope()->AddVariableName(inputSlot.GetName(), "input");
                newInputResultOfAssignment->m_name = variableName;
                newInputResultOfAssignment->m_datum = Datum(inputSlot.GetDataType(), Datum::eOriginality::Original);

                // create a variable declaration
                ExecutionTreePtr variableConstruction = CreateChild(inPreviouslyExecutedScopeResult.m_mostParent->ModParent(), nullptr, nullptr);
                variableConstruction->AddInput({ nullptr , newInputResultOfAssignment, DebugDataSource::FromInternal() });
                variableConstruction->SetSymbol(Symbol::VariableDeclaration);

                // splice the variable declaration right before the most parent for loop is executed
                auto positionOutcome = mostForLoopParent->RemoveChild(inPreviouslyExecutedScopeResult.m_mostParent);
                if (!positionOutcome.IsSuccess())
                {
                    AddError(AZ::EntityId(), executionWithInput, ScriptCanvas::ParseErrors::FailedToRemoveChild);
                }

                auto position = positionOutcome.TakeValue();
                position.second.m_execution = variableConstruction;
                mostForLoopParent->InsertChild(position.first, position.second);
                variableConstruction->AddChild({ nullptr, {}, inPreviouslyExecutedScopeResult.m_mostParent });
                inPreviouslyExecutedScopeResult.m_mostParent->SetParent(variableConstruction);

                // add the variable to to the assignments lists of all connections the child had to nodes in parent loop bodies
                AddPreviouslyExecutedScopeVariableToOutputAssignments(newInputResultOfAssignment, inPreviouslyExecutedScopeResult);

                // finally, return the newly created variable to the input of the child SC node that is directly connected
                return newInputResultOfAssignment;
            }

            // \todo add member variable if data has crossed threads, maybe make it opt-in?
            return nullptr;
        }

        void AbstractCodeModel::ParseConstructionInputVariables()
        {
            AZStd::vector<AZStd::pair<AZ::EntityId, Nodeable*>> nodeablesById;
            AZStd::vector<VariableId> inputVariableIds;
            AZStd::unordered_map<VariableId, Grammar::VariableConstPtr> inputVariablesById;

            auto& variables = GetVariables();
            for (auto variable : variables)
            {
                auto constructionRequirement = ParseConstructionRequirement(variable);

                switch (constructionRequirement)
                {
                case VariableConstructionRequirement::None:
                    break;

                case VariableConstructionRequirement::InputEntityId:
                {
                    m_runtimeInputs.m_entityIds.emplace_back(variable->m_sourceVariableId, *variable->m_datum.GetAs<Data::EntityIDType>());
                }
                break;

                case VariableConstructionRequirement::InputNodeable:
                {
                    if (variable->m_datum.Empty())
                    {
                        AddError(nullptr, aznew Internal::ParseError(AZ::EntityId{}, "Empty nodeable datum in variable, probably due to a problem with azrtti declarations"));
                        break;
                    }

                    // I solemnly swear no harm shall come to the nodeable
                    const Nodeable* nodeableSource = reinterpret_cast<const Nodeable*>(variable->m_datum.GetAsDanger());

                    if (!nodeableSource)
                    {
                        AddError(nullptr, aznew Internal::ParseError(AZ::EntityId{}, "No raw nodeable held by variable"));
                        break;
                    }

                    nodeablesById.push_back({ variable->m_nodeableNodeId, const_cast<Nodeable*>(nodeableSource) });
                }
                break;

                case VariableConstructionRequirement::InputVariable:
                {
                    auto variableID = variable->m_sourceVariableId.IsValid() ? variable->m_sourceVariableId : MakeParserGeneratedId(m_generatedIdCount++);
                    inputVariableIds.push_back(variableID);
                    inputVariablesById.insert({ variableID, variable });
                    // sort revealed a datum copy issue: type is not preserved, workaround below
                    // m_runtimeInputs.m_variables.emplace_back(variable->m_sourceVariableId, variable->m_datum);
                }
                break;

                case VariableConstructionRequirement::Static:
                    m_runtimeInputs.m_staticVariables.push_back({ variable->m_sourceVariableId, variable->m_datum.ToAny() });
                    break;

                default:
                    AZ_Assert(constructionRequirement == VariableConstructionRequirement::None, "new requirement added and not handled");
                    break;
                }
            }

            AZStd::sort(nodeablesById.begin(), nodeablesById.end(), [](const auto& lhs, const auto& rhs) { return lhs.first < rhs.first; });

            m_runtimeInputs.m_nodeables.reserve(nodeablesById.size());
            for (auto& idAndNodeable : nodeablesById)
            {
                m_runtimeInputs.m_nodeables.push_back(idAndNodeable.second);
            }

            // sort revealed a datum copy issue: type is not preserved, workaround below
            // AZStd::sort(m_runtimeInputs.m_variables.begin(), m_runtimeInputs.m_variables.end(), [](const auto& lhs, const auto& rhs) { return lhs.first < rhs.first; });
            m_runtimeInputs.m_variables.reserve(inputVariableIds.size());
            AZStd::sort(inputVariableIds.begin(), inputVariableIds.end());
            for (auto variableId : inputVariableIds)
            {
                auto iter = inputVariablesById.find(variableId);
                AZ_Assert(iter != inputVariablesById.end(), "missing variable id from list just constructed");
                m_runtimeInputs.m_variables.emplace_back(iter->first, iter->second->m_datum);
            }

            AZStd::sort(m_runtimeInputs.m_entityIds.begin(), m_runtimeInputs.m_entityIds.end(), [](const auto& lhs, const auto& rhs) { return lhs.first < rhs.first; });
            AZStd::sort(m_runtimeInputs.m_staticVariables.begin(), m_runtimeInputs.m_staticVariables.end(), [](const auto& lhs, const auto& rhs) { return lhs.first < rhs.first; });

            auto allStaticVariables = ToVariableList(m_runtimeInputs.m_staticVariables);
            auto& staticVariableNames = ModStaticVariablesNames();

            for (auto staticVariable : allStaticVariables)
            {
                AZStd::string name = AddTranslationVariableName(AZStd::string::format("s_%sCloneSource", staticVariable->m_name.c_str()));
                staticVariableNames.push_back({ staticVariable, name });

                if (!staticVariable->m_isMember && !staticVariable->m_isFromFunctionDefinitionSlot)
                {
                    if (staticVariable->m_source)
                    {
                        auto& localStatics = ModStaticVariablesNames(staticVariable->m_source);
                        auto iter = AZStd::find_if
                        (localStatics.begin()
                            , localStatics.end()
                            , [&](const auto& candidate) { return candidate.first == staticVariable; });

                        if (iter == localStatics.end())
                        {
                            localStatics.push_back({ staticVariable, name });
                        }
                    }
                }
            }

            if (!(m_runtimeInputs.m_nodeables.empty() && m_runtimeInputs.m_variables.empty() && m_runtimeInputs.m_entityIds.empty()))
            {
                m_subgraphInterface.MarkRequiresConstructionParameters();
            }
        }

        ConstSlotsOutcome AbstractCodeModel::ParseDataOutSlots(ExecutionTreePtr execution, ExecutionChild& executionChild) const
        {
            if (!execution->GetId().m_node)
            {
                return AZ::Failure(AZStd::string("null node in AbstractCodeModel::ParseDataOutSlots"));
            }

            if (!execution->GetId().m_slot)
            {
                return AZ::Failure(AZStd::string("null slot in AbstractCodeModel::ParseDataOutSlots"));
            }

            auto outcome = execution->GetId().m_node->GetSlotsInExecutionThreadByType(*(execution->GetId().m_slot), CombinedSlotType::DataOut, executionChild.m_slot);
            if (!outcome.IsSuccess())
            {
                return outcome;
            }

            return AZ::Success(outcome.GetValue());
        }

        void AbstractCodeModel::ParseDeactivation()
        {
            AZStd::vector<AZStd::pair<VariableConstPtr, AZStd::string>> deactivatables = GetAllDeactivationVariables();

            if (!IsPureLibrary() || !deactivatables.empty())
            {
                ExecutionTreePtr deactivate = OpenScope(nullptr, nullptr, nullptr);
                deactivate->SetSymbol(Symbol::FunctionDefinition);
                deactivate->SetName(k_DeactivateName);

                if (deactivatables.empty())
                {
                    ExecutionTreePtr empty = CreateChild(deactivate, nullptr, nullptr);
                    empty->MarkDebugEmptyStatement();
                    deactivate->AddChild({ nullptr, {}, empty });
                }
                else
                {
                    auto previous = deactivate;

                    for (const auto& variable : deactivatables)
                    {
                        ExecutionTreePtr deactivate2 = CreateChild(previous, nullptr, nullptr);
                        deactivate2->SetSymbol(Symbol::FunctionCall);
                        deactivate2->SetNameLexicalScope(LexicalScope::Variable());
                        deactivate2->SetName(variable.second);
                        deactivate2->AddInput({ nullptr, variable.first, DebugDataSource::FromInternal() });
                        previous->AddChild({ nullptr, {}, deactivate2 });
                        previous = deactivate2;
                    }
                }

                m_functions.push_back(deactivate);
            }
        }

        void AbstractCodeModel::ParseDebugInformation(ExecutionTreePtr execution)
        {
            switch (execution->GetSymbol())
            {
                // nothing required
            case Symbol::PlaceHolderDuringParsing:
                break;

                // out and return value information
            case Symbol::FunctionDefinition:
                AddDebugInformationFunctionDefinition(execution);
                break;

                // add in out everything
            case Symbol::ForEach:
            case Symbol::FunctionCall:
            case Symbol::OperatorAddition:
            case Symbol::OperatorDivision:
            case Symbol::OperatorMultiplication:
            case Symbol::OperatorSubraction:
            case Symbol::RandomSwitch:
            case Symbol::Switch:
            case Symbol::While:
                AddDebugInformationIn(execution);
                AddDebugInformationOut(execution);
                break;

                // add in but not out
            case Symbol::Break:
            case Symbol::LogicalAND:
            case Symbol::LogicalNOT:
            case Symbol::LogicalOR:
            case Symbol::CompareEqual:
            case Symbol::CompareGreater:
            case Symbol::CompareGreaterEqual:
            case Symbol::CompareLess:
            case Symbol::CompareLessEqual:
            case Symbol::CompareNotEqual:
                AddDebugInformationIn(execution);
                break;

                // add in-debug-info if the if condition is NOT prefixed with logic or comparison expression (which will have the in-debug-info)
                // add out-debug-info in all cases including including empty cases
            case Symbol::IfCondition:
                if (!(execution->GetId().m_node->IsIfBranchPrefacedWithBooleanExpression()))
                {
                    AddDebugInformationIn(execution);
                }

                AddDebugInformationOut(execution);
                break;

            default:
                break;
            }
        }

        void AbstractCodeModel::ParseDependencies(const Node& node)
        {
            const auto dependencyOutcome = node.GetDependencies();

            if (dependencyOutcome.IsSuccess())
            {
                const auto& dependencies = dependencyOutcome.GetValue();

                // #functions2 this search needs to recurse, this layer of dependencies will only be one step deep
                // currently this problem is found by the asset processor
                if (dependencies.userSubgraphs.find(m_source.m_namespacePath) != dependencies.userSubgraphs.end())
                {
                    AZStd::string circularDependency = AZStd::string::format
                    (ParseErrors::CircularDependencyFormat
                        , m_source.m_name.data()
                        , node.GetDebugName().data()
                        , m_source.m_name.data());

                    AddError(nullptr, aznew Internal::ParseError(node.GetEntityId(), circularDependency));
                }

                // #functions2 make this use an identifier for the node, for property window display and easier find/replace updates
                // this part must NOT recurse, the dependency tree should remain a tree and not be flattened
                m_orderedDependencies.source.MergeWith(dependencies);
            }
            else
            {
                AddError(nullptr, ValidationConstPtr(aznew DependencyRetrievalFailiure(node.GetEntityId())));
            }

            if (auto subgraphInterface = node.GetSubgraphInterface())
            {
                m_subgraphInterface.MergeExecutionCharacteristics(*subgraphInterface);

                if (subgraphInterface->HasOnGraphStart())
                {
                    m_subgraphStartCalls.insert(&node);
                }

                if (subgraphInterface->IsActiveDefaultObject())
                {
                    m_activeDefaultObject.insert(&node);
                }
            }
        }

        void AbstractCodeModel::ParseDependenciesAssetIndicies()
        {
            for (const auto& subgraphAssetID : m_orderedDependencies.source.userSubgraphAssetIds)
            {
                m_orderedDependencies.orderedAssetIds.push_back(subgraphAssetID);
            }
        }

        void AbstractCodeModel::ParseEntityIdInput(ExecutionTreePtr execution)
        {
            for (size_t index(0); index < execution->GetInputCount(); ++index)
            {
                auto& slotAndVariable = execution->GetInput(index);

                if (auto& input = slotAndVariable.m_value)
                {
                    if (!input->m_sourceVariableId.IsValid() && IsEntityIdThatRequiresRuntimeRemap(input))
                    {
                        input->m_sourceVariableId = MakeParserGeneratedId(m_generatedIdCount++);
                        input->m_source = nullptr;
                        // promote to member variable for at this stage, optimizations on data flow will occur later
                        input->m_isMember = true;

                        AZStd::string entityVariableName;

                        if (slotAndVariable.m_slot)
                        {
                            if (execution->GetId().m_node)
                            {
                                entityVariableName.append(execution->GetId().m_node->GetNodeName());
                                entityVariableName.append(".");
                                entityVariableName.append(slotAndVariable.m_slot->GetName());
                            }
                            else
                            {
                                entityVariableName.append(slotAndVariable.m_slot->GetName());
                            }
                        }
                        else
                        {
                            entityVariableName = input->m_name;
                        }

                        input->m_name = m_graphScope->AddVariableName(entityVariableName);
                        AddVariable(input);
                    }
                }
                else
                {
                    AddError(execution, aznew Internal::ParseError(execution->GetNodeId(), AZStd::string::format("null input in ParseEntityIdInput %zu", index)));
                }
            }

            for (size_t index(0); index < execution->GetChildrenCount(); ++index)
            {
                auto& child = execution->ModChild(index);

                if (child.m_execution)
                {
                    ParseEntityIdInput(child.m_execution);
                }
            }
        }

        void AbstractCodeModel::ParseExecutionCharacteristics()
        {
            // parse each function, individually, but not the start function
            // if all functions are pure, mark the start function pure
            bool allRootsArePure = true;
            auto roots = ModAllExecutionRoots();
            for (auto root : roots)
            {
                if (root != m_start)
                {
                    ParseExecutionCharacteristics(root);
                    const bool rootIsPure = root->IsPure();
                    allRootsArePure = allRootsArePure && rootIsPure;

                    if (rootIsPure)
                    {
                        if (auto functionDefNode = azrtti_cast<const Nodes::Core::FunctionDefinitionNode*>(root->GetId().m_node))
                        {
                            if (auto in = m_subgraphInterface.ModIn(functionDefNode->GetIdentifier()))
                            {
                                in->isPure = true;
                            }
                            else
                            {
                                AddError(root->GetId().m_node->GetEntityId(), root, "Accounting error, missing Execution-In in SubgraphInterface.");
                            }
                        }
                    }
                }
            }

            // The start function gets special handling. If all of the rest of the graph can be considered pure, than no matter what,
            // the start function will only run once. Therefore all the member variables are local to start (if used at all), and
            // the start function itself can be considered pure.
            if (m_start)
            {
                // The start function is only parsed for variable use. This way, if the graph is in all other ways pure, the
                // start function can be pure, even if it has nodes like Cycle or Once. The state of those nodes can now be
                // considered local state.
                ParseVariableUseAndPurity(m_start);
            }

            CullUnusedVariables();

            if (allRootsArePure
                && m_ebusHandlingByNode.empty()
                && m_eventHandlingByNode.empty()
                && m_nodeablesByNode.empty()
                && m_variableWriteHandlingByVariable.empty()
                && m_subgraphInterface.IsParsedPure())
            {
                if (m_start)
                {
                    m_start->MarkPure();
                    ConvertAllMemberVariablesToLocal(m_start);
                }

                m_subgraphInterface.MarkExecutionCharacteristics(ExecutionCharacteristics::Pure);
            }
            else
            {
                m_subgraphInterface.MarkExecutionCharacteristics(ExecutionCharacteristics::Object);
            }

            if (m_subgraphInterface.GetExecutionCharacteristics() == ExecutionCharacteristics::Object)
            {
                ParseDeactivation();
            }
        }

        void AbstractCodeModel::ParseExecutionCharacteristics(ExecutionTreePtr execution)
        {
            if (!ParseVariableUseAndPurity(execution))
            {
                return;
            }

            if (execution->IsOnLatentPath())
            {
                return;
            }

            if (execution->HasExplicitUserOutCalls())
            {
                return;
            }

            execution->MarkPure();
        }

        void AbstractCodeModel::ParseExecutionCycleStatement(ExecutionTreePtr executionCycle)
        {
            auto cycleVariableIter = m_controlVariablesBySourceNode.find(executionCycle->GetId().m_node);
            AZ_Assert(cycleVariableIter != m_controlVariablesBySourceNode.end(), "cycle node didn't add a control variable to graph scope");
            executionCycle->AddInput({ nullptr, cycleVariableIter->second, DebugDataSource::FromInternal() });
            executionCycle->SetSymbol(Symbol::Cycle);
            ParseExecutionSequentialChildren(executionCycle);
        }

        void AbstractCodeModel::ParseExecutionLoop(ExecutionTreePtr executionLoop)
        {
            AddDebugInfiniteLoopDetectionInLoop(executionLoop);

            auto loopSlot = executionLoop->GetId().m_node->GetSlot(executionLoop->GetId().m_node->GetLoopSlotId());
            AZ_Assert(loopSlot, "Node did not return a valid loop slot");
            ExecutionTreePtr executionLoopBody = OpenScope(executionLoop, executionLoop->GetId().m_node, loopSlot);
            executionLoopBody->SetSymbol(Symbol::PlaceHolderDuringParsing);
            executionLoopBody->MarkInputOutputPreprocessed();
            executionLoop->AddChild({ loopSlot, {}, executionLoopBody });

            auto breakSlot = executionLoop->GetId().m_node->GetSlot(executionLoop->GetId().m_node->GetLoopFinishSlotId());
            AZ_Assert(breakSlot, "Node did not return a valid loop break slot");
            ExecutionTreePtr executionBreak = CreateChild(executionLoop, executionLoop->GetId().m_node, breakSlot);
            executionBreak->SetSymbol(Symbol::PlaceHolderDuringParsing);
            executionBreak->MarkInputOutputPreprocessed();
            executionLoop->AddChild({ breakSlot, {}, executionBreak });

            executionLoopBody = ParseExecutionForEachLoop(executionLoopBody, *loopSlot, *breakSlot);
            if (!executionLoopBody)
            {
                return;
            }

            ParseExecutionTreeBody(executionLoopBody, *loopSlot);
            // \todo check if the loop data is ever connected, and whether it can be known that pure iteration has zero side effects
            // otherwise, this optimization cannot be used
            //             if (executionLoopBody->GetChild(0).m_execution->GetSymbol() == Symbol::PlaceHolderDuringParsing)
            //             {
            //                 executionLoopBody->SetSymbol(Symbol::PlaceHolderDuringParsing);
            //             }

            ParseExecutionTreeBody(executionBreak, *breakSlot);

            if (executionBreak->GetChildrenCount() == 1 && executionBreak->GetChild(0).m_execution->GetSymbol() == Symbol::PlaceHolderDuringParsing)
            {
                executionBreak->SetSymbol(Symbol::PlaceHolderDuringParsing);
            }
        }

        ExecutionTreePtr AbstractCodeModel::ParseExecutionForEachLoop(ExecutionTreePtr forEachLoopBody, const Slot& loopSlot, const Slot& /*breakSlot*/)
        {
            auto forEach = forEachLoopBody->ModParent();
            if (forEach->GetSymbol() != Symbol::ForEach)
            {
                return forEachLoopBody;
            }

            if (forEach->GetInputCount() == 0)
            {
                AddError(forEach->GetNodeId(), forEach, ScriptCanvas::ParseErrors::NoInputToForEach);
                return nullptr;
            }

            auto forEachNodeSC = azrtti_cast<const Nodes::Core::ForEach*>(forEach->GetId().m_node);
            AZ_Assert(forEachNodeSC, "null ForEach ScriptCanvas node in for each loop parse");

            ForEachMetaData* forEachMetaData = aznew ForEachMetaData();
            forEach->SetMetaData(MetaDataPtr(forEachMetaData));
            const auto sourceType = forEach->GetInput(0).m_value->m_datum.GetType();
            const char* sourceName = forEach->GetInput(0).m_value->m_name.c_str();

            auto scope = forEach->ModScope();
            forEachMetaData->m_iteratorVariableName = scope->AddVariableName(sourceName, "iter");
            forEachMetaData->m_isNotAtEndFunctionVariableName = scope->AddVariableName(sourceName, "is_not_at_end_func");
            forEachMetaData->m_nextFunctionVariableName = scope->AddVariableName(sourceName, "next_func");
            forEachMetaData->m_valueFunctionVariableName = scope->AddVariableName(sourceName, "get_value_func");

            if (Data::IsMapContainerType(sourceType))
            {
                forEachMetaData->m_isKeyRequired = true;
                forEachMetaData->m_keyFunctionVariableName = scope->AddVariableName(sourceName, "get_key_func");
            }

            /// NOTE: after basic iteration works correctly
            /// \todo when subsequent input (from nodes connected to the break slot) looks up the slot from the node with key or the value,
            /// it is going to have to find the output of the get/key value functions in BOTH the child outs of break and loop
            /// LY-109862 may be required for this

            ExecutionTreePtr lastExecution = forEachLoopBody;

            // create the iterator variable
            auto iteratorVariable = AZStd::make_shared<Variable>();
            iteratorVariable->m_source = forEach;
            iteratorVariable->m_name = forEachMetaData->m_iteratorVariableName;
            // the type here shouldn't matter
            iteratorVariable->m_datum = Datum(Data::Type::Number(), Datum::eOriginality::Original);

            if (forEachMetaData->m_isKeyRequired)
            {
                // add a function call node for the key, use the name, make input output
                auto getKey = lastExecution;
                getKey->SetName(forEachMetaData->m_keyFunctionVariableName);
                getKey->SetSymbol(Symbol::FunctionCall);
                getKey->AddInput({ nullptr, iteratorVariable, DebugDataSource::FromInternal() });
                getKey->MarkInputOutputPreprocessed();

                auto keySlot = forEachNodeSC->GetSlot(forEachNodeSC->GetKeySlotId());
                AZ_Assert(keySlot, "no key slot in for each node");

                lastExecution = CreateChild(getKey, getKey->GetId().m_node, &loopSlot);
                getKey->AddChild({ &loopSlot, {}, lastExecution });
                ExecutionChild& getKeyChild = getKey->ModChild(0);
                getKeyChild.m_output.push_back({ keySlot, CreateOutputData(getKey, getKeyChild, *keySlot) });

                lastExecution->SetSymbol(Symbol::PlaceHolderDuringParsing);
            }

            // create the get value function call node
            auto getValue = lastExecution;
            getValue->SetName(forEachMetaData->m_valueFunctionVariableName);
            getValue->SetSymbol(Symbol::FunctionCall);
            getValue->AddInput({ nullptr, iteratorVariable, DebugDataSource::FromInternal() });
            getValue->MarkInputOutputPreprocessed();

            auto valueSlot = forEachNodeSC->GetSlot(forEachNodeSC->GetValueSlotId());
            AZ_Assert(valueSlot, "no value slot in for each node");

            lastExecution->AddChild({ &loopSlot, {}, nullptr });
            auto outputValue = CreateOutputData(lastExecution, lastExecution->ModChild(0), *valueSlot);
            lastExecution->ModChild(0).m_output.push_back({ valueSlot, outputValue });

            // the former place holder is now a function call to retrieve values from the container
            return lastExecution;
        }

        void AbstractCodeModel::ParseExecutionForEachStatement(ExecutionTreePtr forEach)
        {
            forEach->SetSymbol(Symbol::ForEach);
            ParseInputData(forEach);
            ParseExecutionLoop(forEach);
        }

        void AbstractCodeModel::ParseExecutionFunction(ExecutionTreePtr execution, const Slot& outSlot)
        {
            /**
             * \note This is the most complicated parse, but only due to the nature of our custom nodes.
             * Custom nodes can trigger execution out of a node from an internal C++ class. Outside of
             * SC Grammar flow of control nodes (e.g If nodes), this needs manual parsing of child nodes mildly earlier in the
             * SC Graph traversal process than other nodes. Refactoring this step into a pure, depth-first, recursive
             * function would be complicated and probably not worth the effort. The final output of the parser does
             * yield a model that, when translated, does benefit from a simple depth first traversal of grammar nodes,
             * as is done in the translator(s).
             */

            AccountForEBusConnectionControl(execution);

            if (execution->GetChildrenCount() == 0)
            {
                execution->AddChild({ &outSlot, {}, nullptr });
            }

            ParseMultiExecutionPre(execution);

            if (!execution->IsInputOutputPreprocessed())
            {
                ParseOutputData(execution, execution->ModChild(0));

                // look up the tree for properly routed data, preprocess variable names, and report errors or auto created variables
                ParseInputData(execution);
            }

            ParseOperatorArithmetic(execution);

            // \todo Infinite loop handling both in code and in the graph will have to occur here.
            auto executionOutNodes = execution->GetId().m_node->GetConnectedNodes(outSlot);
            auto numConnections = executionOutNodes.size();

            if (numConnections == 0)
            {
                if (execution->GetChild(0).m_output.empty())
                {
                    execution->ModChild(0).m_execution = CreateChildPlaceHolder(execution);
                }
                else
                {
                    execution->ModChild(0).m_execution = CreateChildDebugMarker(execution);
                }
            }
            else if (numConnections == 1)
            {
                ParseExecutionFunctionRecurse(execution, execution->ModChild(0), outSlot, executionOutNodes[0]);
            }
            else
            {
                AZStd::vector<const Slot*> outSlots;
                outSlots.resize(executionOutNodes.size(), &outSlot);
                ParseExecutionMultipleOutSyntaxSugar(execution, executionOutNodes, outSlots);
            }

            ParseMultiExecutionPost(execution);
        }

        void AbstractCodeModel::ParseExecutionFunctionRecurse(ExecutionTreePtr execution, ExecutionChild& executionChild, const Slot& outSlot, const AZStd::pair<const Node*, const Slot*>& nodeAndSlot)
        {
            // if the node is null, a validation error will be added, or has been already
            if (const auto node = nodeAndSlot.first)
            {
                ExecutionTreePtr child = CreateChild(execution, node, nodeAndSlot.second);
                executionChild.m_execution = child;
                executionChild.m_slot = &outSlot;

                if (IsFlowControl(child))
                {
                    ParseExecutionTreeBody(child, *nodeAndSlot.second);
                }
                else
                {
                    auto childOutSlotsOutcome = child->GetId().m_node->GetSlotsInExecutionThreadByType(*(child->GetId().m_slot), CombinedSlotType::ExecutionOut);
                    if (childOutSlotsOutcome.IsSuccess())
                    {
                        const auto& childOutSlots = childOutSlotsOutcome.GetValue();
                        AZ_Assert(!childOutSlots.empty(), "there must be an immediate out");
                        if (childOutSlots.size() == 1)
                        {
                            ParseExecutionTreeBody(child, *childOutSlots[0]);
                        }
                        else
                        {
                            // Interior node branches: This is required for highly custom or state-ful nodes, namely those that fire different, 
                            // and/or unknown-at-compile-time outs based on the same in.
                            auto iter = m_nodeablesByNode.find(node);
                            if (iter == m_nodeablesByNode.end())
                            {
                                if (azrtti_cast<const Nodes::Core::Method*>(node))
                                {
                                    // Method should have only one execution out, this node is out of date and unsupported by new backend
                                    AddError(nullptr, aznew NodeCompatiliblity::NodeOutOfDate(node->GetEntityId(), node->GetNodeName()));
                                }
                                else
                                {
                                    AddError(node->GetEntityId(), execution, ScriptCanvas::ParseErrors::CustomParsingRequiredForVariable);
                                }
                                return;
                            }

                            child->SetNodeable(iter->second->m_nodeable);

                            for (auto& childOutSlot : childOutSlots)
                            {
                                AZ_Assert(childOutSlot, "null slot in child out slot list");
                                ExecutionTreePtr internalOut = OpenScope(child, node, childOutSlot);

                                const size_t outIndex = node->GetOutIndex(*childOutSlot);
                                if (outIndex == std::numeric_limits<size_t>::max())
                                {
                                    AddError(execution, aznew Internal::ParseError(node->GetEntityId(), AZStd::string::format("Missing internal out key for slot %s", childOutSlot->GetName().c_str())));
                                    return;
                                }

                                internalOut->SetOutCallIndex(outIndex);
                                internalOut->MarkInternalOut();
                                internalOut->SetSymbol(Symbol::FunctionDefinition);
                                auto outNameOutcome = node->GetInternalOutKey(*childOutSlot);
                                AZ_Assert(outNameOutcome.IsSuccess(), "GetInternalOutKey failed");
                                internalOut->SetName(outNameOutcome.TakeValue().data());
                                ParseExecutionTreeBody(internalOut, *childOutSlot);

                                if (internalOut->GetChildrenCount() > 0)
                                {
                                    child->AddChild({ childOutSlot, {}, internalOut });
                                }
                            }

                            ParseInputData(child);
                            ParseMetaData(child);

                            if (auto metaData = child->ModMetaData())
                            {
                                metaData->PostParseExecutionTreeBody(*this, child);
                            }
                        }
                    }
                    else
                    {
                        AddError(execution, aznew Internal::ParseError(execution->GetNodeId(), childOutSlotsOutcome.TakeError()));
                    }
                }
            }
            else
            {
                AZ_Assert(false, "Child out not connected to node");
            }
        }

        void AbstractCodeModel::ParseExecutionIfStatement(ExecutionTreePtr executionIf)
        {
            bool isCheckedOperation = false;
            bool callCheckedOpOnBothBranches = false;

            if (executionIf->GetId().m_node->IsIfBranchPrefacedWithBooleanExpression())
            {
                ExecutionTreePtr booleanExpression;

                {
                    auto removeChildOutcome = RemoveChild(executionIf->ModParent(), executionIf);
                    if (!removeChildOutcome.IsSuccess())
                    {
                        AddError(executionIf->GetNodeId(), executionIf, ScriptCanvas::ParseErrors::FailedToRemoveChild);
                    }

                    if (!IsErrorFree())
                    {
                        return;
                    }

                    const auto indexAndChild = removeChildOutcome.TakeValue();

                    booleanExpression = CreateChild(executionIf->ModParent(), executionIf->GetId().m_node, executionIf->GetId().m_slot);
                    executionIf->ModParent()->InsertChild(indexAndChild.first, { indexAndChild.second.m_slot, indexAndChild.second.m_output, booleanExpression });
                    executionIf->SetParent(booleanExpression);
                }

                // make a condition here
                auto symbol = CheckLogicalExpressionSymbol(booleanExpression);
                if (symbol != Symbol::FunctionCall && symbol != Symbol::Count)
                {
                    ParseExecutionLogicalExpression(booleanExpression, symbol);
                }
                else if (auto methodNode = azrtti_cast<const Nodes::Core::Method*>(executionIf->GetId().m_node))
                {
                    if (methodNode->BranchesOnResult())
                    {
                        const Data::Type methodResultType = methodNode->GetResultType();
                        if (methodResultType == Data::Type::Boolean())
                        {
                            // result type is boolean, parse it as boolean expression directly
                            ParseExecutionIfStatementInternalFunction(booleanExpression);
                        }
                        else
                        {
                            auto removeChildOutcome2 = RemoveChild(booleanExpression->ModParent(), booleanExpression);
                            if (!removeChildOutcome2.IsSuccess())
                            {
                                AddError(methodNode->GetEntityId(), executionIf, ScriptCanvas::ParseErrors::FailedToRemoveChild);
                            }

                            if (!IsErrorFree())
                            {
                                return;
                            }

                            const auto indexAndChild2 = removeChildOutcome2.TakeValue();

                            // parse if statement internal function
                            ExecutionTreePtr internalFunction = CreateChild(booleanExpression->ModParent(), booleanExpression->GetId().m_node, booleanExpression->GetId().m_slot);
                            booleanExpression->ModParent()->InsertChild(indexAndChild2.first, { indexAndChild2.second.m_slot, indexAndChild2.second.m_output, internalFunction });
                            booleanExpression->SetParent(internalFunction);
                            ParseExecutionIfStatementInternalFunction(internalFunction);

                            internalFunction->ModChild(0).m_execution = booleanExpression;
                            booleanExpression->ClearInput();
                            const auto& logicOutput = internalFunction->ModChild(0).m_output;
                            booleanExpression->AddInput({ nullptr, logicOutput[0].second->m_source, DebugDataSource::FromInternal() });

                            // parse if statement boolean expression
                            ParseBranchOnResultFunctionCheck(booleanExpression);
                        }
                    }
                    else if (methodNode->IsCheckedOperation(&callCheckedOpOnBothBranches))
                    {
                        isCheckedOperation = true;
                        ParseCheckedFunctionCheck(booleanExpression);
                    }
                    else
                    {
                        AddError(methodNode->GetEntityId(), executionIf, ScriptCanvas::ParseErrors::FailedToParseIfBranch);
                    }
                }

                booleanExpression->ModChild(0).m_execution = executionIf;

                // route the output of the logical expression to the if condition grammar node
                // and the same output to the output of the if condition grammar node
                executionIf->ClearInput();

                const auto& logicOutput = booleanExpression->ModChild(0).m_output;
                executionIf->AddInput({ nullptr, logicOutput[0].second->m_source, DebugDataSource::FromInternal() });
            }
            else
            {
                ParseInputData(executionIf);
            }

            auto outSlotTrue = executionIf->GetId().m_node->GetIfBranchTrueOutSlot();
            auto outSlotFalse = executionIf->GetId().m_node->GetIfBranchFalseOutSlot();

            executionIf->SetSymbol(Symbol::IfCondition);

            ExecutionTreePtr executionTrue = OpenScope(executionIf, executionIf->GetId().m_node, outSlotTrue);
            executionIf->AddChild({ outSlotTrue, {}, executionTrue });

            if (isCheckedOperation)
            {
                executionTrue->SetSymbol(Symbol::FunctionCall);
                ParseMetaData(executionTrue);
                ParseExecutionFunction(executionTrue, *outSlotTrue);
                if (auto metaData = executionTrue->ModMetaData())
                {
                    metaData->PostParseExecutionTreeBody(*this, executionTrue);
                }
            }
            else
            {
                executionTrue->SetSymbol(Symbol::PlaceHolderDuringParsing);
                executionTrue->MarkInputOutputPreprocessed();

                if (outSlotTrue)
                {
                    ParseExecutionTreeBody(executionTrue, *outSlotTrue);
                }
                else
                {
                    AddError(AZ::EntityId(), executionTrue, ScriptCanvas::ParseErrors::MissingTrueExecutionSlotOnIf);
                }

                executionTrue->MarkDebugEmptyStatement();
            }

            ExecutionTreePtr executionFalse = OpenScope(executionIf, executionIf->GetId().m_node, outSlotFalse);
            executionIf->AddChild({ outSlotFalse, {}, executionFalse });

            if (isCheckedOperation && callCheckedOpOnBothBranches)
            {
                executionFalse->SetSymbol(Symbol::FunctionCall);
                ParseMetaData(executionFalse);
                ParseExecutionFunction(executionFalse, *outSlotFalse);
                if (auto metaData = executionFalse->ModMetaData())
                {
                    metaData->PostParseExecutionTreeBody(*this, executionFalse);
                }
            }
            else
            {
                executionFalse->SetSymbol(Symbol::PlaceHolderDuringParsing);
                executionFalse->MarkInputOutputPreprocessed();

                if (outSlotFalse)
                {
                    ParseExecutionTreeBody(executionFalse, *outSlotFalse);
                }
                else
                {
                    AddError(AZ::EntityId(), executionFalse, ScriptCanvas::ParseErrors::MissingFalseExecutionSlotOnIf);
                }

                executionFalse->MarkDebugEmptyStatement();
            }
        }

        void AbstractCodeModel::ParseExecutionIfStatementBooleanExpression(ExecutionTreePtr booleanExpressionExecution, AZStd::string executionName, LexicalScope lexicalScope)
        {
            booleanExpressionExecution->SetName(executionName);
            booleanExpressionExecution->SetNameLexicalScope(lexicalScope);
            booleanExpressionExecution->AddChild({});

            ExecutionChild& child = booleanExpressionExecution->ModChild(0);
            VariableConstPtr result = AZStd::make_shared<Variable>();
            result->m_name = booleanExpressionExecution->ModScope()->AddVariableName(executionName, "result");
            result->m_datum.SetType(Data::Type::Boolean());
            result->m_source = booleanExpressionExecution;
            auto outputAssignment = CreateOutputAssignment(result);
            child.m_output.push_back(AZStd::make_pair(nullptr, outputAssignment));
            booleanExpressionExecution->SetSymbol(Symbol::FunctionCall);
        }

        void AbstractCodeModel::ParseExecutionIfStatementInternalFunction(ExecutionTreePtr internalFunctionExecution)
        {
            ParseMetaData(internalFunctionExecution);
            internalFunctionExecution->SetSymbol(Symbol::FunctionCall);
            internalFunctionExecution->AddChild({});
            ParseOutputData(internalFunctionExecution, internalFunctionExecution->ModChild(0));
            ParseInputData(internalFunctionExecution);

            if (auto metaData = internalFunctionExecution->ModMetaData())
            {
                metaData->PostParseExecutionTreeBody(*this, internalFunctionExecution);
            }
        }

        void AbstractCodeModel::ParseExecutionLogicalExpression(ExecutionTreePtr execution, Symbol symbol)
        {
            if (!execution->GetId().m_node->IsIfBranchPrefacedWithBooleanExpression())
            {
                AddError(AZ::EntityId(), execution, ScriptCanvas::ParseErrors::AttemptToParseNonExpression);
                return;
            }

            execution->AddChild({});
            ParseOutputData(execution, execution->ModChild(0));

            execution->SetSymbol(symbol);

            if (!IsLogicalExpression(execution))
            {
                AddError(AZ::EntityId(), execution, ScriptCanvas::ParseErrors::FailedToDeduceExpression);
            }

            ParseInputData(execution);
        }

        void AbstractCodeModel::ParseBranchOnResultFunctionCheck(ExecutionTreePtr execution)
        {
            if (auto methodNode = azrtti_cast<const Nodes::Core::Method*>(execution->GetId().m_node))
            {
                AZStd::string checkFunctionName;
                LexicalScope lexicalScope;
                if (methodNode->GetBranchOnResultCheckName(checkFunctionName, lexicalScope))
                {
                    ParseExecutionIfStatementBooleanExpression(execution, checkFunctionName, lexicalScope);
                }
                else
                {
                    AZ_Assert(false, "Unable to fetch branch on result check function name.");
                }
            }
            else
            {
                AZ_Assert(false, "Function check attempted on a node that wasn't a method node.");
            }
        }

        void AbstractCodeModel::ParseCheckedFunctionCheck(ExecutionTreePtr execution)
        {
            if (auto methodNode = azrtti_cast<const Nodes::Core::Method*>(execution->GetId().m_node))
            {
                AZ::CheckedOperationInfo checkedOpInfo;
                AZStd::string checkedOpExposedName;
                LexicalScope lexicalScope;

                // manually process as if the check itself was a method node
                if (methodNode->GetCheckedOperationInfo(checkedOpInfo, checkedOpExposedName, lexicalScope))
                {
                    ParseExecutionIfStatementBooleanExpression(execution, checkedOpExposedName, lexicalScope);
                    ParseInputData(execution);
                    execution->ReduceInputSet(checkedOpInfo.m_inputRestriction);
                    return;
                }
                else
                {
                    AZ_Assert(false, "No checked information operation in execution declared to have one.");
                }
            }
            else
            {
                AZ_Assert(false, "Function check attempted on a node that wasn't a method node.");
            }
        }

        void AbstractCodeModel::ParseExecutionMultipleOutSyntaxSugarOfSequencNode(ExecutionTreePtr execution)
        {
            auto sequentialOutcome = execution->GetId().m_node->GetSlotsInExecutionThreadByType(*(execution->GetId().m_slot), CombinedSlotType::ExecutionOut);
            if (!sequentialOutcome.IsSuccess())
            {
                AddError(execution, aznew Internal::ParseError(execution->GetId().m_node->GetEntityId(), "sequential execution slot mapping failure"));
                return;
            }

            AZStd::vector<const Slot*>& childOutSlots = sequentialOutcome.GetValue();
            if (!childOutSlots.empty())
            {
                // now gather all the connections to sequence node out slots, and treat them like
                // (ordered) connections to the out that connected to the sequence node
                EndpointsResolved outNodes;
                AZStd::vector<const Slot*> outSlots;

                for (auto outSlot : childOutSlots)
                {
                    if (outSlot)
                    {
                        auto executionOutNodes = execution->GetId().m_node->GetConnectedNodes(*outSlot);
                        outNodes.insert(outNodes.end(), executionOutNodes.begin(), executionOutNodes.end());
                        outSlots.insert(outSlots.end(), executionOutNodes.size(), outSlot);
                    }
                }

                ParseExecutionMultipleOutSyntaxSugar(execution, outNodes, outSlots);
            }
        }

        void AbstractCodeModel::ParseExecutionMultipleOutSyntaxSugar(ExecutionTreePtr execution, const EndpointsResolved& executionOutNodes, const AZStd::vector<const Slot*>& outSlots)
        {
            const auto executionNodeId = execution->GetId().m_node ? execution->GetId().m_node->GetEntityId() : AZ::EntityId();

            if (executionOutNodes.size() != outSlots.size())
            {
                AddError(executionNodeId, execution, ParseErrors::ParseExecutionMultipleOutSyntaxSugarMismatchOutSize);
            }

            if (execution->GetSymbol() != Symbol::Sequence)
            {
                // make a new execution node, add it the the child out indicated by the slot
                // set execution equal to the new node
                ExecutionTreePtr sequence = CreateChild(execution, nullptr, outSlots[0]);
                sequence->SetSymbol(Symbol::Sequence);
                ExecutionChild* child = execution->FindChild(outSlots[0]->GetId());

                if (!child)
                {
                    AddError(executionNodeId, execution, ParseErrors::ParseExecutionMultipleOutSyntaxSugarNullChildFound);
                    return;
                }

                if (child->m_execution)
                {
                    AddError(executionNodeId, execution, ParseErrors::ParseExecutionMultipleOutSyntaxSugarNonNullChildExecutionFound);
                    return;
                }

                child->m_execution = sequence;
                execution = sequence;
            }

            for (size_t childIndex = 0; childIndex < executionOutNodes.size(); ++childIndex)
            {
                execution->AddChild({ outSlots[childIndex], {}, nullptr });
            }

            for (size_t childIndex = 0; childIndex < executionOutNodes.size(); ++childIndex)
            {
                ParseExecutionFunctionRecurse(execution, execution->ModChild(childIndex), *outSlots[childIndex], executionOutNodes[childIndex]);

                if (!IsErrorFree())
                {
                    return;
                }

                if (execution->GetChildrenCount() != executionOutNodes.size())
                {
                    AddError(executionNodeId, execution, ParseErrors::ParseExecutionMultipleOutSyntaxSugarChildExecutionRemovedAndNotReplaced);
                    return;
                }
            }
        }

        void AbstractCodeModel::ParseExecutionOnce(ExecutionTreePtr once)
        {
            const auto ID = once->GetId();
            auto onceVariableIter = m_controlVariablesBySourceNode.find(ID.m_node);
            AZ_Assert(onceVariableIter != m_controlVariablesBySourceNode.end(), "Once node didn't add a control variable to graph scope");

            ExecutionTreePtr nextParse;
            const Slot* nextSlot = nullptr;

            if (IsOnceIn(*ID.m_node, ID.m_slot))
            {
                auto outSlotTrue = GetOnceOutSlot(*ID.m_node);
                AZ_Assert(outSlotTrue, "Must be an out slot for a Once node");
                once->SetSymbol(Symbol::IfCondition);
                once->AddInput({ nullptr, onceVariableIter->second, DebugDataSource::FromInternal() });

                // the once branch, first set the control value to false
                ExecutionTreePtr controlValue = OpenScope(once, once->GetId().m_node, outSlotTrue);
                controlValue->SetSymbol(Symbol::VariableAssignment);
                auto falseValue = AZStd::make_shared<Variable>();
                falseValue->m_isMember = false;
                falseValue->m_source = controlValue;
                falseValue->m_datum = Datum(Data::BooleanType(false));
                controlValue->AddInput({ nullptr, falseValue, DebugDataSource::FromInternal() });
                once->AddChild({ outSlotTrue, {}, controlValue });

                // placeholder true branch
                ExecutionTreePtr executionTrue = CreateChild(controlValue, once->GetId().m_node, outSlotTrue);
                executionTrue->SetSymbol(Symbol::PlaceHolderDuringParsing);
                executionTrue->MarkInputOutputPreprocessed();
                controlValue->AddChild({ outSlotTrue, {}, executionTrue });
                controlValue->ModChild(0).m_output.push_back({ nullptr, CreateOutputAssignment(onceVariableIter->second) });

                nextParse = executionTrue;
                nextSlot = outSlotTrue;

                // (unused) placeholder false branch for possible debugging, and for if statement construction consistency
                ExecutionTreePtr neverRuns = OpenScope(once, once->GetId().m_node, nullptr);
                neverRuns->MarkDebugEmptyStatement();
                once->AddChild({ nullptr, {}, neverRuns });
            }
            else
            {
                AZ_Assert(IsOnceReset(*ID.m_node, ID.m_slot), "Once slot not accounted for in grammar");
                auto onceResetSlot = GetOnceOnResetSlot(*ID.m_node);
                AZ_Assert(onceResetSlot, "Must be an On Reset Slot for a Once node");

                once->SetSymbol(Symbol::VariableAssignment);
                auto trueValue = AZStd::make_shared<Variable>();
                trueValue->m_isMember = false;
                trueValue->m_source = once;
                trueValue->m_datum = Datum(Data::BooleanType(true));
                once->AddInput({ nullptr, trueValue, DebugDataSource::FromInternal() });

                ExecutionTreePtr onReset = CreateChild(once, once->GetId().m_node, onceResetSlot);
                onReset->SetSymbol(Symbol::PlaceHolderDuringParsing);
                onReset->MarkInputOutputPreprocessed();
                once->AddChild({ onceResetSlot, {}, onReset });

                once->ModChild(0).m_output.push_back({ nullptr, CreateOutputAssignment(onceVariableIter->second) });

                nextParse = onReset;
                nextSlot = onceResetSlot;
            }

            if (!nextSlot)
            {
                AddError(ID.m_node->GetEntityId(), once, "Once node missing next slot, likely needs replacement");
                return;
            }

            ParseExecutionTreeBody(nextParse, *nextSlot);
            nextParse->MarkDebugEmptyStatement();
        }

        void AbstractCodeModel::ParseExecutionRandomSwitchStatement(ExecutionTreePtr executionRandomSwitch)
        {
            // parse input normally
            ParseInputData(executionRandomSwitch);

            // add the weight names for later summation
            const size_t weightCount = executionRandomSwitch->GetInputCount();
            for (size_t index = 0; index < weightCount; ++index)
            {
                auto weight = AZStd::make_shared<Variable>();
                weight->m_name = executionRandomSwitch->ModScope()->AddVariableName(AZStd::string::format("randomSwitchWeight_%zu", index));
                weight->m_source = executionRandomSwitch;
                executionRandomSwitch->AddInput({ nullptr, weight, DebugDataSource::FromInternal() });
            }

            // add a control variable for later comparison against weight
            auto controlVariable = AZStd::make_shared<Variable>();
            controlVariable->m_datum = Datum(Data::NumberType(0));
            controlVariable->m_name = executionRandomSwitch->ModScope()->AddVariableName("randomSwitchControl");
            controlVariable->m_source = executionRandomSwitch;
            executionRandomSwitch->AddInput({ nullptr, controlVariable, DebugDataSource::FromInternal() });

            // add a running total variable for ease of use
            auto runningTotal = AZStd::make_shared<Variable>();
            runningTotal->m_datum = Datum(Data::NumberType(0));
            runningTotal->m_name = executionRandomSwitch->ModScope()->AddVariableName("randomSwitchRunningTotal");
            runningTotal->m_source = executionRandomSwitch;
            executionRandomSwitch->AddInput({ nullptr, runningTotal, DebugDataSource::FromInternal() });
            executionRandomSwitch->SetSymbol(Symbol::RandomSwitch);

            ParseExecutionSequentialChildren(executionRandomSwitch);
        }

        void AbstractCodeModel::ParseExecutionSequentialChildren(ExecutionTreePtr execution)
        {
            auto sequentialOutcome = execution->GetId().m_node->GetSlotsInExecutionThreadByType(*(execution->GetId().m_slot), CombinedSlotType::ExecutionOut);
            if (!sequentialOutcome.IsSuccess())
            {
                AddError(AZ::EntityId(), execution, ScriptCanvas::ParseErrors::SequentialExecutionMappingFailure);
                return;
            }

            const auto& childOuts = sequentialOutcome.GetValue();
            for (const auto& childOutSlot : childOuts)
            {
                ExecutionTreePtr childOutExecution = OpenScope(execution, execution->GetId().m_node, childOutSlot);
                childOutExecution->SetSymbol(Symbol::PlaceHolderDuringParsing);
                childOutExecution->MarkInputOutputPreprocessed();
                execution->AddChild({ childOutSlot, {}, childOutExecution });
                ParseExecutionTreeBody(childOutExecution, *childOutSlot);
                childOutExecution->MarkDebugEmptyStatement();
            };

            if (execution->GetChildrenCount() == 0)
            {
                execution->SetSymbol(Symbol::PlaceHolderDuringParsing);
            }
        }

        void AbstractCodeModel::ParseExecutionSwitchStatement(ExecutionTreePtr executionSwitch)
        {
            ParseInputData(executionSwitch);
            executionSwitch->SetSymbol(Symbol::Switch);
            ParseExecutionSequentialChildren(executionSwitch);
        }

        // the execution will already have its function definition defined, this will create the body of that function
        void AbstractCodeModel::ParseExecutionTreeBody(ExecutionTreePtr execution, const Slot& outSlot)
        {
            ParseMetaData(execution);

            // \note this grammar check matches IsFlowControl, and needs to be merged with and replace ParseExecutionFunctionRecurse
            if (IsBreak(execution))
            {
                ParseExecutionBreak(execution);
            }
            else if (IsUserOutNode(execution))
            {
                ParseUserOutCall(execution);
            }
            else if (IsIfCondition(execution))
            {
                ParseExecutionIfStatement(execution);
            }
            else if (IsForEach(execution))
            {
                ParseExecutionForEachStatement(execution);
            }
            else if (IsSequenceNode(execution))
            {
                execution->SetSymbol(Symbol::Sequence);
                ParseExecutionMultipleOutSyntaxSugarOfSequencNode(execution);
            }
            else if (IsSwitchStatement(execution))
            {
                ParseExecutionSwitchStatement(execution);
            }
            else if (IsCycle(execution))
            {
                ParseExecutionCycleStatement(execution);
            }
            else if (IsOnce(execution))
            {
                ParseExecutionOnce(execution);
            }
            else if (IsRandomSwitchStatement(execution))
            {
                ParseExecutionRandomSwitchStatement(execution);
            }
            else if (IsWhileLoop(execution))
            {
                ParseExecutionWhileLoop(execution);
            }
            else
            {
                ParseExecutionFunction(execution, outSlot);
                // if is a connect call to an az event handler...mark grammar, because it will require special processing
                // ParseEventConnectionHandling(execution, outSlot); .. add the previously executed function to the second child of the node
                // make a new grammar thing
            }

            if (auto metaData = execution->ModMetaData())
            {
                metaData->PostParseExecutionTreeBody(*this, execution);
            }
        }

        ExecutionTreePtr AbstractCodeModel::ParseExecutionTreeRoot(ExecutionTreePtr root)
        {
            if (auto slot = root->GetId().m_slot)
            {
                ParseMetaData(root);
                ParseExecutionTreeBody(root, *slot);

                if (root->GetChildrenCount() > 0)
                {
                    root->ModChild(0).m_execution->MarkDebugEmptyStatement();
                    PostParseProcess(root);
                    PostParseErrorDetect(root);

                    if (IsErrorFree())
                    {
                        return root;
                    }
                }
                else
                {
                    AddError(slot->GetNodeId(), root, ScriptCanvas::ParseErrors::NoChildrenAfterRoot);
                }
            }
            else
            {
                AddError(AZ::EntityId(), root, ScriptCanvas::ParseErrors::NoOutForExecution);
            }

            root->Clear();
            return nullptr;
        }

        ExecutionTreePtr AbstractCodeModel::ParseExecutionTreeRoot(const Node& node, const Slot& outSlot, MarkLatent markLatent)
        {
            if (ExecutionContainsCyclesCheck(node, outSlot))
            {
                return nullptr;
            }

            ExecutionTreePtr root = OpenScope(nullptr, &node, &outSlot);
            root->SetSymbol(Symbol::FunctionDefinition);

            if (outSlot.IsLatent() || markLatent == MarkLatent::Yes)
            {
                root->MarkRootLatent();
            }

            return ParseExecutionTreeRoot(root);
        }

        void AbstractCodeModel::ParseExecutionTreeRoots(const Node& node)
        {
            auto nodeableParseIter = m_nodeablesByNode.find(&node);
            if (nodeableParseIter != m_nodeablesByNode.end())
            {
                auto latentSlots = node.GetSlotsByType(CombinedSlotType::LatentOut);
                for (const auto slot : latentSlots)
                {
                    if (slot)
                    {
                        if (auto outRoot = ParseExecutionTreeRoot(node, *slot, MarkLatent::Yes))
                        {
                            AddDebugInfiniteLoopDetectionInHandler(outRoot);
                            outRoot->SetNodeable(nodeableParseIter->second->m_nodeable);
                            auto latentOutKeyOutcome = node.GetLatentOutKey(*slot);
                            if (latentOutKeyOutcome.IsSuccess())
                            {
                                const size_t outIndex = node.GetOutIndex(*slot);
                                if (outIndex == std::numeric_limits<size_t>::max())
                                {
                                    AddError(outRoot, aznew Internal::ParseError(node.GetEntityId(), AZStd::string::format("Missing internal out key for slot %s", slot->GetName().c_str())));
                                    return;
                                }

                                outRoot->SetOutCallIndex(outIndex);
                                outRoot->SetName(latentOutKeyOutcome.GetValue().data());
                                AZStd::const_pointer_cast<NodeableParse>(nodeableParseIter->second)->m_latents.emplace_back(outRoot->GetName(), outRoot);
                            }
                            else
                            {
                                AddError(node.GetEntityId(), nullptr, AZStd::string::format("GetLatentOutKey failed for nodeable failed: %s", node.GetDebugName().data()));
                                return;
                            }
                        }
                    }
                    else
                    {
                        AddError(node.GetEntityId(), nullptr, AZStd::string::format("null latent slot returned by node: %s", node.GetDebugName().data()));
                        return;
                    }
                }
                // \todo more work will be required to determine if a nodeable node is wasted or not
            }

            auto ebusEventHandlingIter = m_ebusHandlingByNode.find(&node);
            if (ebusEventHandlingIter != m_ebusHandlingByNode.end())
            {
                const auto eventSlots = node.GetEventSlots();
                for (const auto slot : eventSlots)
                {
                    if (slot)
                    {
                        if (auto eventRoot = ParseExecutionTreeRoot(node, *slot, MarkLatent::Yes))
                        {
                            AddDebugInfiniteLoopDetectionInHandler(eventRoot);

                            auto latentOutKeyOutcome = node.GetInternalOutKey(*slot);
                            if (latentOutKeyOutcome.IsSuccess())
                            {
                                eventRoot->SetName(latentOutKeyOutcome.GetValue().data());
                                AZStd::const_pointer_cast<EBusHandling>(ebusEventHandlingIter->second)->m_events.emplace_back(eventRoot->GetName(), eventRoot);
                            }
                            else
                            {
                                AddError(node.GetEntityId(), nullptr, AZStd::string::format("GetInternalOutKey for ebus handler failed: %s", node.GetDebugName().data()));
                                return;
                            }
                        }
                    }
                    else
                    {
                        AddError(node.GetEntityId(), nullptr, AZStd::string::format("null event slot returned by event handler: %s", node.GetDebugName().data()));
                        return;
                    }
                }

                if (ebusEventHandlingIter->second->m_events.empty())
                {
                    // \todo add a note or warning about empty events, especially if the connection controls are used, or automatically disable.
                    // remove the handler, remove the connection control calls, (for no-ops) or something?
                }
            }

            auto eventHandlingIter = m_eventHandlingByNode.find(&node);
            if (eventHandlingIter != m_eventHandlingByNode.end())
            {
                const auto eventSlots = node.GetEventSlots();
                for (const auto slot : eventSlots)
                {
                    if (slot)
                    {
                        if (auto eventRoot = ParseExecutionTreeRoot(node, *slot, MarkLatent::Yes))
                        {
                            AddDebugInfiniteLoopDetectionInHandler(eventRoot);

                            auto latentOutKeyOutcome = node.GetInternalOutKey(*slot);
                            if (latentOutKeyOutcome.IsSuccess())
                            {
                                eventRoot->SetName(latentOutKeyOutcome.GetValue().data());
                                AZStd::const_pointer_cast<EventHandling>(eventHandlingIter->second)->m_eventHandlerFunction = eventRoot;
                            }
                            else
                            {
                                AddError(node.GetEntityId(), nullptr, AZStd::string::format("GetInternalOutKey for event handler failed: %s", node.GetDebugName().data()));
                                return;
                            }
                        }
                    }
                    else
                    {
                        AddError(node.GetEntityId(), nullptr, AZStd::string::format("null event slot returned by event handler: %s", node.GetDebugName().data()));
                        return;
                    }
                }
            }

            for (auto dataSlot : node.GetOnVariableHandlingDataSlots())
            {
                auto variableWriteHandlingIter = m_variableWriteHandlingBySlot.find(dataSlot);
                if (variableWriteHandlingIter != m_variableWriteHandlingBySlot.end())
                {
                    VariableWriteHandlingPtr variableWriteHandling = AZStd::const_pointer_cast<VariableWriteHandling>(variableWriteHandlingIter->second);

                    if (node.IsVariableWriteHandler())
                    {
                        const auto eventSlots = node.GetEventSlots();
                        AZ_Assert(eventSlots.size() == 1, "no variable change slot");

                        if (auto onVariableWrite = ParseExecutionTreeRoot(node, *eventSlots[0], MarkLatent::No))
                        {
                            auto name = m_graphScope->AddFunctionName(AZStd::string::format("On%sWritten", variableWriteHandling->m_variable->m_name.c_str()).c_str());
                            onVariableWrite->SetName(name);
                            variableWriteHandling->m_function = onVariableWrite;

                            onVariableWrite->MarkInfiniteLoopDetectionPoint();

                            auto variable = AddMemberVariable(Datum(Data::Type::Number(), Datum::eOriginality::Original), "variableChangeIterationCounter");
                            variable->m_isDebugOnly = true;
                            m_implicitVariablesByNode.insert({ onVariableWrite, variable });
                        }
                    }
                }
            }

            if (const Nodes::Core::FunctionDefinitionNode* nodeling = azrtti_cast<const Nodes::Core::FunctionDefinitionNode*>(&node))
            {
                auto userFunctionIter = m_userInsThatRequireTopology.find(nodeling);
                if (userFunctionIter != m_userInsThatRequireTopology.end())
                {
                    auto& userFunctionNode = *userFunctionIter->first;
                    auto outSlots = userFunctionNode.GetSlotsByType(CombinedSlotType::ExecutionOut);

                    if (outSlots.empty() || !outSlots.front())
                    {
                        AddError(userFunctionNode.GetEntityId(), nullptr, ScriptCanvas::ParseErrors::NoOutSlotInFunctionDefinitionStart);
                        return;
                    }

                    if (!ExecutionContainsCyclesCheck(userFunctionNode, *outSlots.front()))
                    {
                        auto definition = userFunctionIter->second;
                        auto entrySlot = definition->GetId().m_slot;
                        AZ_Assert(entrySlot, "Bad accounting in user function definition node");
                        AZStd::vector<VariablePtr> returnValues;
                        UserOutCallCollector userOutCallCollector;
                        TraverseExecutionConnections(userFunctionNode, *entrySlot, userOutCallCollector);

                        const AZStd::unordered_set<const ScriptCanvas::Nodes::Core::FunctionDefinitionNode*>& uniqueNodelingsOut = userOutCallCollector.GetOutCalls();
                        for (const auto& returnCall : uniqueNodelingsOut)
                        {
                            for (const auto returnSlot : returnCall->GetSlotsByType(CombinedSlotType::DataIn))
                            {
                                auto iter = m_outputVariableByNodelingOutSlot.find(returnSlot);
                                if (iter != m_outputVariableByNodelingOutSlot.end())
                                {
                                    VariablePtr variable = iter->second;
                                    variable->m_name = definition->ModScope()->AddVariableName(returnSlot->GetName());
                                    variable->m_source = definition;
                                    variable->m_sourceSlotId = returnSlot->GetId();
                                    returnValues.push_back(variable);
                                }
                            }
                        }

                        m_returnValuesByUserFunctionDefinition.insert(AZStd::make_pair(nodeling, ReturnValueDescription{ returnValues, uniqueNodelingsOut.size() }));

                        if (auto root = ParseExecutionTreeRoot(userFunctionIter->second))
                        {
                            m_functions.push_back(root);
                        }
                        else
                        {
                            m_userInsThatRequireTopology.erase(userFunctionIter);
                        }
                    }
                }
            }
        }

        void AbstractCodeModel::ParseExecutionWhileLoop(ExecutionTreePtr execution)
        {
            execution->SetSymbol(Symbol::While);
            ParseInputData(execution);
            ParseExecutionLoop(execution);
        }

        void AbstractCodeModel::ParseFunctionLocalStaticUseage()
        {
            for (auto execution : ModAllExecutionRoots())
            {
                if (auto localVariables = GetLocalVariables(execution))
                {
                    for (auto variable : *localVariables)
                    {
                        if (const AZStd::pair<VariableConstPtr, AZStd::string>* pair = FindStaticVariable(variable))
                        {
                            auto& localStatics = ModStaticVariablesNames(execution);
                            auto iter = AZStd::find_if
                                ( localStatics.begin()
                                , localStatics.end()
                                , [&](const auto& candidate) { return candidate.first == variable; });

                            if (iter == localStatics.end())
                            {
                                localStatics.push_back(*pair);
                            }
                        }
                    }
                }
            }
        }

        void AbstractCodeModel::ParseImplicitVariables(const Node& node)
        {
            if (IsCycle(node))
            {
                auto cycleVariable = AddMemberVariable(Datum(Data::NumberType(0)), "cycleControl");
                m_controlVariablesBySourceNode.insert({ &node, cycleVariable });
            }
            else if (IsOnce(node))
            {
                auto onceControl = AddMemberVariable(Datum(Data::BooleanType(true)), "onceControl");
                m_controlVariablesBySourceNode.insert({ &node, onceControl });
            }
            else
            {
                const auto nodelingType = CheckNodelingType(node);
                if (nodelingType != NodelingType::None)
                {
                    ParseNodelingVariables(node, nodelingType);
                }
            }
        }

        void AbstractCodeModel::ParseInputData(ExecutionTreePtr execution)
        {
            if (execution->GetSymbol() == Symbol::FunctionDefinition)
            {
                // input data for functions has been handled already
                return;
            }

            // special handling for Extraction nodes
            if (IsExecutedPropertyExtraction(execution))
            {
                // the input will be assigned by the parent node in the extraction
                return;
            }
            // special handling for Get Variable nodes
            else if (IsVariableGet(execution))
            {
                const VariableId assignedFromId = execution->GetId().m_node->GetVariableIdRead(execution->GetId().m_slot);
                auto variableRead = assignedFromId.IsValid() ? FindVariable(assignedFromId) : nullptr;

                if (variableRead)
                {
                    // nullptr is acceptable here
                    execution->AddInput({ nullptr, variableRead, DebugDataSource::FromVariable(SlotId{}, variableRead->m_datum.GetType(), variableRead->m_sourceVariableId) });
                }
                else
                {
                    AddError(execution->GetNodeId(), execution, ScriptCanvas::ParseErrors::MissingVariable);
                }
            }
            else
            {
                auto dataSlotsOutcome = execution->GetId().m_node->GetSlotsInExecutionThreadByType(*(execution->GetId().m_slot), CombinedSlotType::DataIn);
                if (dataSlotsOutcome.IsSuccess())
                {
                    if (ParseInputThisPointer(execution))
                    {
                        const auto& dataSlots = dataSlotsOutcome.GetValue();
                        for (auto dataInSlot : dataSlots)
                        {
                            AZ_Assert(dataInSlot, "data corruption, bad input slot");
                            ParseInputDatum(execution, *dataInSlot);
                        }
                    }
                }
                else
                {
                    AddError(nullptr, aznew Internal::ParseError(execution->GetNodeId(), dataSlotsOutcome.TakeError()));
                }
            }
        }

        void AbstractCodeModel::ParseInputDatum(ExecutionTreePtr execution, const Slot& input)
        {
            AZ_Assert(execution->GetSymbol() != Symbol::FunctionDefinition, "Function definition input is not handled in AbstractCodeModel::ParseInputDatum");

            if (!input.GetDataType().IsValid())
            {
                AddError(nullptr, aznew Internal::ParseError(execution->GetNodeId(), ParseErrors::InvalidDataTypeInInput));
                return;
            }

            auto nodes = execution->GetId().m_node->GetConnectedNodes(input);
            if (nodes.empty())
            {
                if (auto variable = FindReferencedVariableChecked(execution, input))
                {
                    execution->AddInput({ &input, variable, DebugDataSource::FromVariable(input.GetId(), input.GetDataType(), variable->m_sourceVariableId) });
                    CheckConversion(execution->ModConversions(), variable, execution->GetInputCount() - 1, input.GetDataType());
                }
                else
                {
                    auto variableDatum = input.FindDatum();
                    AZ_Assert(variableDatum, "Input datum missing from Slot %s on Node %s", input.GetName().data(), execution->GetId().m_node ? execution->GetId().m_node->GetNodeName().data() : "");

                    auto inputVariable = AZStd::make_shared<Variable>();
                    inputVariable->m_source = execution;
                    inputVariable->m_sourceSlotId = input.GetId();
                    inputVariable->m_name = execution->ModScope()->AddVariableName(input.GetName());

                    if (variableDatum->GetType().IsValid())
                    {
                        inputVariable->m_datum = *variableDatum;
                    }
                    else if (execution->GetId().m_node->ConvertsInputToStrings())
                    {
                        inputVariable->m_datum = Datum(Data::Type::String(), Datum::eOriginality::Original);
                    }
                    else
                    {
                        AddError(execution->GetNodeId(), execution, AZStd::string::format("input type is invalid on Slot %s on Node %s", input.GetName().data(), execution->GetId().m_node ? execution->GetId().m_node->GetNodeName().data() : "").data());
                        return;
                    }

                    execution->AddInput({ &input, inputVariable, DebugDataSource::FromSelfSlot(input, inputVariable->m_datum.GetType()) });
                }

                // Check for known null reads
                CheckForKnownNullDereference(execution, execution->GetInput(execution->GetInputCount() - 1), input);
            }
            else
            {
                if (auto sourceVariable = ParseConnectedInputData(input, execution, nodes, FirstNode::Parent))
                {
                    execution->AddInput({ &input, sourceVariable, DebugDataSource::FromOtherSlot(input.GetId(), input.GetDataType(), sourceVariable->m_sourceSlotId) });
                    CheckConversion(execution->ModConversions(), sourceVariable, execution->GetInputCount() - 1, input.GetDataType());
                }
                else
                {
                    // This isn't supported visually, yet, but technically, it could be.
                    // One could connect both latent execution and immediate execution to the same code execution path,
                    // but only one uses connected output, and the other uses defaults.
                    // It would require us to change the visualization of the shared path, based on which parent was clicked on in the editor.
                    const auto& targetNode = *execution->GetId().m_node;
                    const auto& targetSlot = input;

                    for (auto sourceNodeAndSlot : nodes)
                    {
                        AddError(nullptr, aznew ScopedDataConnectionEvent
                            ( execution->GetNodeId()
                            , targetNode
                            , targetSlot
                            , *sourceNodeAndSlot.first
                            , *sourceNodeAndSlot.second));

                    }

                    return;
                }
            }
        }

        bool AbstractCodeModel::ParseInputThisPointer(ExecutionTreePtr execution)
        {
            auto node = execution->GetId().m_node;

            if (node->IsVariableWriteHandler())
            {
                auto addressSlot = node->GetEBusConnectAddressSlot();
                AZ_Assert(addressSlot, "variable write handler node must have address slot");
                auto writeHandlingBySlot = m_variableWriteHandlingBySlot.find(addressSlot);
                AZ_Assert(writeHandlingBySlot != m_variableWriteHandlingBySlot.end(), "bad variable write handling accounting");

                auto variableHandling = writeHandlingBySlot->second;
                if (!variableHandling->m_connectionVariable)
                {
                    AZStd::string controlName = variableHandling->m_variable->m_name;
                    controlName.append("WriteControl");
                    variableHandling->m_connectionVariable = AddMemberVariable(Datum(variableHandling->m_startsConnected), controlName);
                }

                auto connectionValue = AZStd::make_shared<Variable>();
                connectionValue->m_datum = Datum(node->GetEBusConnectSlot() == execution->GetId().m_slot);
                connectionValue->m_source = execution;

                execution->AddInput({ nullptr, connectionValue, DebugDataSource::FromInternal() });
                execution->ModChild(0).m_output.push_back({ nullptr, CreateOutputAssignment(variableHandling->m_connectionVariable) });
                execution->SetSymbol(Symbol::VariableAssignment);
                return false;
            }
            else if (CheckEventHandlingType(*node) == EventHandingType::Event)
            {
                if (auto variable = FindVariable(execution->GetNodeId()))
                {
                    execution->MarkInputHasThisPointer();
                    execution->AddInput({ nullptr, variable, DebugDataSource::FromInternal() });
                }
                else
                {
                    auto node2 = execution->GetId().m_node;
                    AddError(execution, aznew ParseError(node2->GetEntityId(), AZStd::string::format
                    ("Failed to find member variable for Node: %s Id: %s"
                        , node2->GetNodeName().data()
                        , node2->GetEntityId().ToString().data()).data()));
                }
            }
            else if (node->IsEventHandler())
            {
                if (auto eventHandling = GetEBusEventHandling(node))
                {
                    auto variable = AZStd::make_shared<Variable>();
                    variable->m_isMember = true;
                    variable->m_datum = Datum(eventHandling->m_handlerName);
                    execution->MarkInputHasThisPointer();
                    execution->AddInput({ nullptr, variable, DebugDataSource::FromInternal() });
                }
                else
                {
                    AddError(execution->GetNodeId(), execution, ScriptCanvas::ParseErrors::BadEventHandlingAccounting);
                }
            }
            else if (node->IsNodeableNode())
            {
                if (auto variable = FindVariable(execution->GetNodeId()))
                {
                    execution->MarkInputHasThisPointer();
                    execution->AddInput({ nullptr, variable, DebugDataSource::FromInternal() });
                }
                else
                {
                    auto node2 = execution->GetId().m_node;
                    AddError(execution, aznew ParseError(node2->GetEntityId(), AZStd::string::format
                    ("Failed to find member variable for Node: %s Id: %s"
                        , node2->GetNodeName().data()
                        , node2->GetEntityId().ToString().data()).data()));
                }
            }

            return true;
        }

        void AbstractCodeModel::ParseMetaData(ExecutionTreePtr execution)
        {
            if (!execution->GetMetaData())
            {
                if (auto metaData = CreateMetaData(execution))
                {
                    execution->SetMetaData(metaData);
                }
            }
        }

        void AbstractCodeModel::ParseMultiExecutionPost(ExecutionTreePtr execution)
        {
            ParsePropertyExtractionsPost(execution);
            ParseMultipleFunctionCallPost(execution);
        }

        void AbstractCodeModel::ParseMultiExecutionPre(ExecutionTreePtr execution)
        {
            ParsePropertyExtractionsPre(execution);
        }

        void AbstractCodeModel::ParseMultipleFunctionCallPost(ExecutionTreePtr execution)
        {
            auto& id = execution->GetId();
            MultipleFunctionCallFromSingleSlotInfo info = id.m_node->GetMultipleFunctionCallFromSingleSlotInfo(*id.m_slot);

            if (info.functionCalls.empty())
            {
                return;
            }

            auto parent = execution->ModParent();

            if (!parent)
            {
                AddError(execution, aznew Internal::ParseError(id.m_node->GetEntityId(), "Null parent in MultipleFunctionCall"));
                return;
            }

            size_t indexInParentCall = parent->FindChildIndex(execution);
            if (indexInParentCall >= parent->GetChildrenCount())
            {
                AddError(execution, aznew Internal::ParseError(id.m_node->GetEntityId(), ParseErrors::MultipleFunctionCallFromSingleSlotNoChildren));
                return;
            }

            ExecutionChild* executionChildInParent = &parent->ModChild(indexInParentCall);

            const size_t executionInputCount = execution->GetInputCount();
            const size_t thisInputOffset = execution->InputHasThisPointer() ? 1 : 0;

            // the original index has ALL the input from the slots on the node
            // create multiple calls with separate function call nodes, but ONLY take the inputs required
            // as indicated by the function call info

            AZStd::unordered_set<const Slot*> usedSlots;
            bool variadicIsFound = false;

            auto createChild = [&](auto parentCall, ExecutionChild* childInParent, auto& functionCallInfo)
            {
                auto child = CreateChild(parentCall, id.m_node, id.m_slot);
                child->SetSymbol(Symbol::FunctionCall);
                child->SetName(functionCallInfo.functionName);
                child->SetNameLexicalScope(functionCallInfo.lexicalScope);
                childInParent->m_execution = child;
                return child;
            };

            auto addThisInput = [&](auto functionCall)
            {
                if (thisInputOffset != 0)
                {
                    if (executionInputCount == 0)
                    {
                        AddError(execution, aznew Internal::ParseError(id.m_node->GetEntityId(), ParseErrors::MultipleFunctionCallFromSingleSlotNotEnoughInputForThis));
                        return;
                    }

                    const ExecutionInput& input = execution->GetInput(0);
                    usedSlots.insert(input.m_slot);
                    functionCall->AddInput(input);
                }
            };

            auto addSlotInput = [&](auto functionCall, size_t inputIndex)
            {
                if (inputIndex >= executionInputCount)
                {
                    AddError(execution, aznew Internal::ParseError(id.m_node->GetEntityId(), ParseErrors::MultipleFunctionCallFromSingleSlotNotEnoughInput));
                    return;
                }

                const ExecutionInput& input = execution->GetInput(inputIndex);

                if (usedSlots.contains(input.m_slot))
                {
                    AddError(execution, aznew Internal::ParseError(id.m_node->GetEntityId(), ParseErrors::MultipleFunctionCallFromSingleSlotNotEnoughInput));
                    return;
                }

                usedSlots.insert(input.m_slot);

                if (input.m_value->m_source == execution)
                {
                    input.m_value->m_source = functionCall;
                }

                functionCall->AddInput(input);
            };

            auto addCall = [&](auto& functionCallInfo, auto childInParent, size_t startingIndex, size_t sentinel, size_t variadicOffset = 0)
            {
                auto child = createChild(parent, childInParent, functionCallInfo);
                addThisInput(child);

                for (size_t index = startingIndex; index < sentinel; ++index)
                {
                    const size_t inputIndex = index + thisInputOffset + variadicOffset;
                    addSlotInput(child, inputIndex);
                }

                child->AddChild({});
                childInParent = &child->ModChild(0);
                return AZStd::make_pair(childInParent, child);
            };

            // loop through each call...
            for (auto& functionCallInfo : info.functionCalls)
            {
                // ...first add any pre-variadic calls, using the starting index and the number of args, since they could come in any order, not input slot order...
                if (!functionCallInfo.isVariadic)
                {
                    AZStd::pair<ExecutionChild*, ExecutionTreePtr> childInParentAndParent = addCall(functionCallInfo, executionChildInParent, functionCallInfo.startingIndex, functionCallInfo.startingIndex + functionCallInfo.numArguments);
                    executionChildInParent = childInParentAndParent.first;
                    parent = childInParentAndParent.second;
                }
                else
                {
                    // ...then add only one variadic call if there is one...
                    if (variadicIsFound)
                    {
                        AddError(execution, aznew Internal::ParseError(id.m_node->GetEntityId(), ParseErrors::MultipleFunctionCallFromSingleSlotMultipleVariadic));
                        return;
                    }

                    variadicIsFound = true;
                    const size_t sentinel = executionInputCount == 0 ? 0 : executionInputCount - thisInputOffset;
                    // ... by looping through the remaining slots, striding by functionCallInfo.numArguments, making repeated calls to the function
                    for (size_t slotInputIndex = functionCallInfo.startingIndex; slotInputIndex < sentinel; slotInputIndex += functionCallInfo.numArguments)
                    {
                        AZStd::pair<ExecutionChild*, ExecutionTreePtr> childInParentAndParent = addCall(functionCallInfo, executionChildInParent, 0, functionCallInfo.numArguments, slotInputIndex);
                        executionChildInParent = childInParentAndParent.first;
                        parent = childInParentAndParent.second;
                    }
                }
            }

            if (info.errorOnUnusedSlot && usedSlots.size() != executionInputCount)
            {
                AddError(execution, aznew Internal::ParseError(id.m_node->GetEntityId(), ParseErrors::MultipleFunctionCallFromSingleSlotUnused));
            }

            // parent now refers to the last child call created
            parent->SwapChildren(execution);
            execution->Clear();
        }

        void AbstractCodeModel::ParseNodelingVariables(const Node& node, NodelingType nodelingType)
        {
            // #functions2 slot<->variable adjust once datums are more coordinated
            auto createVariablesSlots = [&](AZStd::unordered_map<const Slot*, VariablePtr>& variablesBySlots, const AZStd::vector<const Slot*>& slots, bool slotHasDatum)
            {
                for (const auto& slot : slots)
                {
                    auto variable = AZStd::make_shared<Variable>();

                    if (slotHasDatum)
                    {
                        auto variableDatum = slot->FindDatum();
                        if (!variableDatum)
                        {
                            AddError(nullptr, aznew Internal::ParseError(node.GetEntityId(), AZStd::string::format("Datum missing from Slot %s on Node %s", slot->GetName().data(), node.GetNodeName().c_str())));
                            return;
                        }

                        // #functions2 slot<->variable consider getting all variables from the UX variable manager, or from the ACM and looking them up in the variable manager for ordering
//                         auto iter = m_sourceVariableByDatum.find(variableDatum);
//                         if (iter == m_sourceVariableByDatum.end())
//                         {
//                             AddError(nullptr, aznew Internal::ParseError(node.GetEntityId(), AZStd::string::format("Datum missing from Slot %s on Node %s", slot->GetName().data(), node.GetNodeName().c_str())));
//                             return;
//                         }
//                      variable->m_sourceVariableId = iter->second->GetVariableId();
                        variable->m_datum = *variableDatum;
                    }
                    else
                    {
                        // make a new datum and a source slot id and all that
                        variable->m_datum.SetType(slot->GetDataType());
                    }

                    // Scope and name are initialized later
                    variable->m_sourceSlotId = slot->GetId();
                    variable->m_isFromFunctionDefinitionSlot = true;
                    variablesBySlots.insert({ slot, variable });
                    m_variables.push_back(variable);
                }
            };

            if (nodelingType == NodelingType::In)
            {
                // get the output slots of the In-Nodeling
                const auto inputs = node.GetSlotsByType(CombinedSlotType::DataOut);
                createVariablesSlots(m_inputVariableByNodelingInSlot, inputs, false);
            }
            else if (nodelingType == NodelingType::Out)
            {
                // get the input slots of the Out-Nodeling
                const auto outputs = node.GetSlotsByType(CombinedSlotType::DataIn);
                createVariablesSlots(m_outputVariableByNodelingOutSlot, outputs, true);
            }
            else if (nodelingType == NodelingType::OutReturn)
            {
                // get the output slots of the Out-Nodeling
                const auto inputs = node.GetSlotsByType(CombinedSlotType::DataOut);
                createVariablesSlots(m_returnVariableByNodelingOutSlot, inputs, false);
            }
        }

        void AbstractCodeModel::ParseOperatorArithmetic(ExecutionTreePtr execution)
        {
            const CheckOperatorResult result = CheckOperatorArithmeticSymbol(execution);

            execution->SetSymbol(result.symbol);

            if (!result.name.empty())
            {
                execution->SetName(result.name);
                execution->SetNameLexicalScope(result.lexicalScope);
            }

            if (execution->GetSymbol() == Symbol::Count)
            {
                AddError(execution, aznew Internal::ParseError(execution->GetNodeId(), ParseErrors::UntranslatedArithmetic));
            }
            else if (IsOperatorArithmetic(execution))
            {
                // \todo check input validity, including for compile time division by zero
                if (execution->GetInputCount() < 2)
                {
                    AddError(execution, aznew Internal::ParseError(execution->GetNodeId(), ParseErrors::NotEnoughArgsForArithmeticOperator));
                }
            }
        }

        void AbstractCodeModel::ParseOutputData(ExecutionTreePtr execution, ExecutionChild& executionChild)
        {
            if (const auto nodeling = azrtti_cast<const Nodes::Core::FunctionDefinitionNode*>(execution->GetId().m_node))
            {
                // this nodeling will always be the Execution-In part of the function defintion
                // since a call to a user out does not enter this path
                AZ_Assert(execution->GetSymbol() != Symbol::UserOut, "User Out data should not be processed here");
                ParseUserInData(execution, executionChild);
                return;
            }

            if (auto writtenVariable = GetWrittenVariable(execution))
            {
                executionChild.m_output.push_back({ execution->GetId().m_node->GetVariableOutputSlot(), CreateOutputAssignment(writtenVariable) });
            }

            // this can never called on a branch
            auto outputSlotsOutcome = ParseDataOutSlots(execution, executionChild);
            if (outputSlotsOutcome.IsSuccess())
            {
                ParseOutputData(execution, executionChild, outputSlotsOutcome.GetValue());

                if (execution->GetSymbol() == Symbol::FunctionDefinition)
                {
                    auto returnSlotsOutcome = execution->GetId().m_node->GetSlotsInExecutionThreadByType(*(execution->GetId().m_slot), CombinedSlotType::DataIn);
                    if (returnSlotsOutcome.IsSuccess())
                    {
                        for (auto outputSlot : returnSlotsOutcome.GetValue())
                        {
                            ParseReturnValue(execution, *outputSlot);
                        }
                    }
                    else
                    {
                        AddError(execution, aznew Internal::ParseError(execution->GetNodeId(), returnSlotsOutcome.TakeError()));
                    }
                }
            }
            else
            {
                AddError(execution, aznew Internal::ParseError(execution->GetNodeId(), outputSlotsOutcome.TakeError()));
            }
        }

        void AbstractCodeModel::ParseOutputData(ExecutionTreePtr execution, ExecutionChild& executionChild, AZStd::vector<const Slot*>& slots)
        {
            for (auto outputSlot : slots)
            {
                ParseOutputData(execution, executionChild, *outputSlot);
            }
        }

        void AbstractCodeModel::ParseOutputData(ExecutionTreePtr execution, ExecutionChild& executionChild, const Slot& output)
        {
            if (auto newOutput = CreateOutputData(execution, executionChild, output))
            {
                executionChild.m_output.push_back({ &output, newOutput });
            }
        }

        void AbstractCodeModel::ParsePropertyExtractionsPost(ExecutionTreePtr execution)
        {
            if (execution->GetSymbol() == Symbol::FunctionDefinition)
            {
                return;
            }

            // every property extraction has to be individually processed, each one is made into a its one node in the execution tree
            ExecutionTreePtr parent = execution;

            const auto& propertyExtractionSources = execution->GetPropertyExtractionSources();

            if (!propertyExtractionSources.empty() && execution->GetChildrenCount() == 0)
            {
                AddError(execution, aznew Internal::ParseError(execution->GetNodeId(), ParseErrors::NoChildrenInExtraction));
                return;
            }

            for (auto propertyExtractionIter : propertyExtractionSources)
            {
                if (auto propertyOutput = RemoveOutput(execution->ModChild(0), propertyExtractionIter.first->GetId()))
                {
                    if (propertyExtractionIter.second)
                    {
                        ExecutionTreePtr extraction = CreateChild(execution, execution->GetId().m_node, execution->GetId().m_slot);

                        if (auto writtenVariable = GetWrittenVariable(execution))
                        {
                            // nullptr is acceptable here
                            extraction->AddInput({ nullptr, writtenVariable, DebugDataSource::FromVariable(SlotId{}, writtenVariable->m_datum.GetType(), writtenVariable->m_sourceVariableId) });
                        }
                        else
                        {
                            extraction->CopyInput(execution, ExecutionTree::RemapVariableSource::No);
                        }

                        extraction->SetExecutedPropertyExtraction(propertyExtractionIter.second);

                        // make sure the correct node is responsible for creating the output
                        if (propertyOutput->m_source->m_source == execution)
                        {
                            propertyOutput->m_source->m_source = extraction;
                        }

                        // the child output is only the property extraction
                        ExecutionChild child;
                        child.m_output.push_back({ propertyExtractionIter.first, propertyOutput });
                        extraction->AddChild(child);

                        // insert the extraction into the tree
                        extraction->SetParent(parent);
                        extraction->ModChild(0).m_execution = parent->GetChild(0).m_execution;
                        parent->ModChild(0).m_execution = extraction;
                        parent = extraction;
                    }
                }
                else
                {
                    AddError(execution, aznew Internal::ParseError(execution->GetNodeId(), ParseErrors::NoOutForExecution));
                }
            }

            execution->ClearProperyExtractionSources();
        }

        void AbstractCodeModel::ParsePropertyExtractionsPre(ExecutionTreePtr execution)
        {
            if (execution->GetSymbol() == Symbol::FunctionDefinition)
            {
                return;
            }

            auto propertyFields = execution->GetId().m_node->GetPropertyFields();

            for (auto propertyField : propertyFields)
            {
                const auto slot = execution->GetId().m_node->GetSlot(propertyField.second);
                AZ_Assert(slot, "not slot by name %s", propertyField.first.data());

                if (slot->IsVariableReference() || !execution->GetId().m_node->GetConnectedNodes(*slot).empty())
                {
                    PropertyExtractionPtr extraction = AZStd::make_shared<PropertyExtraction>();
                    extraction->m_slot = slot;
                    extraction->m_name = AZ::ReplaceCppArtifacts(propertyField.first);
                    execution->AddPropertyExtractionSource(slot, extraction);
                }
                else
                {
                    execution->AddPropertyExtractionSource(slot, nullptr);
                }
            }
        }

        void AbstractCodeModel::ParseReturnValue(ExecutionTreePtr execution, const Slot& returnValueSlot)
        {
            if (auto variable = FindReferencedVariableChecked(execution, returnValueSlot))
            {
                ParseReturnValue(execution, variable, &returnValueSlot);
            }
            else
            {
                auto returnValueOutput = CreateOutput(execution, returnValueSlot, {}, "return");
                ReturnValuePtr returnValue = AZStd::make_shared<ReturnValue>(AZStd::move(*returnValueOutput.get()));

                auto nodes = execution->GetId().m_node->GetConnectedNodes(returnValueSlot);
                if (!nodes.empty())
                {
                    if (auto sourceVariable = ParseConnectedInputData(returnValueSlot, execution, nodes, FirstNode::Self))
                    {
                        returnValue->m_initializationValue = sourceVariable;
                        returnValue->m_sourceDebug = DebugDataSource::FromReturn(returnValueSlot, execution, sourceVariable);
                    }
                }
                else
                {
                    returnValue->m_sourceDebug = DebugDataSource::FromSelfSlot(returnValueSlot);
                }

                execution->AddReturnValue(&returnValueSlot, returnValue);
            }
        }

        void AbstractCodeModel::ParseReturnValue(ExecutionTreePtr execution, VariableConstPtr variable, const Slot* returnValueSlot)
        {
            auto returnValueOutput = CreateOutputAssignment(variable);
            ReturnValuePtr returnValue = AZStd::make_shared<ReturnValue>(AZStd::move(*returnValueOutput.get()));
            returnValue->m_isNewValue = !variable->m_isMember;

            // this will need a refactor in terms of debug info for function graphs
            if (returnValueSlot)
            {
                returnValue->m_sourceDebug = DebugDataSource::FromReturn(*returnValueSlot, execution, variable);
            }

            execution->AddReturnValue(nullptr, returnValue);
        }

        void AbstractCodeModel::ParseUserFunctionTopology()
        {
            for (auto& iter : m_userInsThatRequireTopology)
            {
                ParseUserIn(iter.second, iter.first);
            }

            m_userInsThatRequireTopology.clear();

            ParseUserOuts();

            auto parseOutcome = m_subgraphInterface.Parse();

            if (!parseOutcome.IsSuccess())
            {
                AddError(nullptr, aznew Internal::ParseError(AZ::EntityId(), AZStd::string::format("Subgraph interface failed to parse: %s", parseOutcome.GetError().c_str()).c_str()));
            }
        }

        void AbstractCodeModel::ParseUserIn(ExecutionTreePtr root, const Nodes::Core::FunctionDefinitionNode* nodeling)
        {
            // make sure this name is unique
            size_t defaultAdded = 0;

            // get all the nodelings, all the leaves, and the calls to nodelings out
            NodelingInParserIterationListener listener;
            TraverseTree(root, listener);
            auto& leavesWithoutNodelings = listener.GetLeavesWithoutNodelings();
            AZStd::unordered_set<const ScriptCanvas::Nodes::Core::FunctionDefinitionNode*> uniqueNodelingsOut = listener.GetNodelingsOut();
            // determine the the execution topology can be reduced to single function call with a single return point to graph execution
            const UserInParseTopologyResult result = ParseUserInTolopology(uniqueNodelingsOut.size(), leavesWithoutNodelings.size());
            // determine the name of the default out if one needs to be added to leaf nodes with with no execution out calls
            AZStd::string defaultOutNameCandidate = CheckUniqueOutNames(root->GetName(), uniqueNodelingsOut);
            AZStd::string defaultOutName;

            if (result.addSingleOutToMap)
            {
                // force all names to be unique, make sure the new name is unique, use result of topology query for name
                // try for in name, if that doesn't work, just make a new one based on, "Out"
                defaultOutName = defaultOutNameCandidate;
                defaultAdded = 1;
            }

            if (result.addNewOutToLeavesWithout)
            {
                for (auto& leafWithout : leavesWithoutNodelings)
                {
                    AddUserOutToLeaf(AZStd::const_pointer_cast<ExecutionTree>(leafWithout), root, defaultOutName);
                }
            }

            // this is a sanity check now to verify there are no leaves
            NodelingInParserIterationListener listenerCheck;
            listenerCheck.CountOnlyGrammarCalls();
            TraverseTree(root, listenerCheck);

            if (result.addNewOutToLeavesWithout)
            {
                auto& leavesWithoutNodelingsChecked = listenerCheck.GetLeavesWithoutNodelings();
                if (!leavesWithoutNodelingsChecked.empty())
                {
                    AddError(root, aznew Internal::ParseError(AZ::EntityId(), "In Nodeling didn't parse properly, there were still leaves without nodelings in the execution tree."));
                    return;
                }
            }

            auto& outCallsChecked = listenerCheck.GetOutCalls();
            if (!result.addSingleOutToMap && outCallsChecked.empty())
            {
                AddError(root, aznew Internal::ParseError(AZ::EntityId(), "In Nodeling didn't parse properly, the parser failed to generate an immediate out."));
                return;
            }

            const size_t branches = uniqueNodelingsOut.size() + defaultAdded;

            if (result.addExplicitOutCalls)
            {
                if (branches < 2)
                {
                    AddError(root, aznew Internal::ParseError(AZ::EntityId(), "In Nodeling didn't parse properly, attempting explicit Outs without user defined branches"));
                    return;
                }

                root->MarkHasExplicitUserOutCalls();

                for (auto& outCall : outCallsChecked)
                {
                    AZStd::const_pointer_cast<ExecutionTree>(outCall)->CopyReturnValuesToInputs(root);
                }
            }
            else if (branches > 1)
            {
                AddError(root, aznew Internal::ParseError(AZ::EntityId(), "In Nodeling didn't parse properly, attempting default return even with branches"));
                return;
            }

            if (branches <= 1)
            {
                for (auto outCallChecked : outCallsChecked)
                {
                    auto nodelingOut = AZStd::const_pointer_cast<ExecutionTree>(outCallChecked);
                    nodelingOut->SetSymbol(Symbol::PlaceHolderDuringParsing);
                    nodelingOut->MarkDebugEmptyStatement();
                }
            }

            if (!defaultOutName.empty() && result.addSingleOutToMap)
            {
                uniqueNodelingsOut.insert(nullptr);
            }

            if ((!root->HasExplicitUserOutCalls())
                && root->GetReturnValueCount() > 0
                && branches > 1)
            {
                AddError(root->GetNodeId(), root, ScriptCanvas::ParseErrors::TooManyBranchesForReturn);
                return;
            }

            // ALWAYS MAKE A MAP, send it to the output, regardless
            AddExecutionMapIn(result, root, outCallsChecked, defaultOutName, nodeling, uniqueNodelingsOut);
        }

        void AbstractCodeModel::ParseUserInData(ExecutionTreePtr execution, ExecutionChild& executionChild)
        {
            if (execution->IsOnLatentPath())
            {
                AddError(execution, aznew Internal::ParseError(execution->GetNodeId(), "latent execution parsed data in immediate thread"));
            }
            else
            {
                // inputs to the user function
                const auto input = FindUserImmediateInput(execution);
                for (auto& inputValue : input)
                {
                    auto outputAssignment = CreateOutputAssignment(inputValue);
                    inputValue->m_source = execution;
                    executionChild.m_output.push_back({ nullptr, outputAssignment });
                }

                // (all possible, even variant) output from the function
                if (const auto output = FindUserImmediateOutput(execution))
                {
                    for (auto& outputValue : output->returnValues)
                    {
                        ParseReturnValue(execution, outputValue, nullptr);
                        AZStd::pair<const Slot*, ReturnValueConstPtr> returnValue = execution->GetReturnValue(execution->GetReturnValueCount() - 1);
                        AZStd::const_pointer_cast<ReturnValue>(returnValue.second)->m_isNewValue = true;
                    }
                }
            }
        }

        AbstractCodeModel::UserInParseTopologyResult AbstractCodeModel::ParseUserInTolopology(size_t nodelingsOutCount, size_t leavesWithoutNodelingsCount)
        {
            AbstractCodeModel::UserInParseTopologyResult result;

            if (nodelingsOutCount == 0)
            {
                // easy definition: syntax sugar where we give the user an Out for free
                result.addSingleOutToMap = true;
                result.addNewOutToLeavesWithout = false;
                result.addExplicitOutCalls = false;
                result.isSimpleFunction = true;
            }
            else if (leavesWithoutNodelingsCount == 0)
            {
                // user defined every possible out
                result.addSingleOutToMap = false;
                result.addNewOutToLeavesWithout = false;
                result.addExplicitOutCalls = nodelingsOutCount > 1;
                result.isSimpleFunction = !result.addExplicitOutCalls;
            }
            else
            {
                // user explicitly defined at least 1 Out and there are execution leaves without Outs, so we provide an Out to any missing ones
                result.addSingleOutToMap = true;
                result.addNewOutToLeavesWithout = true;
                result.addExplicitOutCalls = true;
                result.isSimpleFunction = false;
            }

            return result;
        }

        void AbstractCodeModel::ParseUserLatent(ExecutionTreePtr call, const Nodes::Core::FunctionDefinitionNode* nodeling)
        {
            if (call && !call->GetRoot())
            {
                AddError(call, aznew Internal::ParseError(call->GetNodeId(), "User Out call has no parent"));
                return;
            }

            // process data values into the proper call
            ParseUserLatentData(call); // <- do this for very ACM node
            AddExecutionMapLatentOut(*nodeling, call); // <- do this once for the nodeling
            call->MarkHasExplicitUserOutCalls();
            call->ModRoot()->MarkHasExplicitUserOutCalls();
        }

        void AbstractCodeModel::ParseUserLatentData(ExecutionTreePtr execution)
        {
            if (execution->IsOnLatentPath())
            {
                if (execution->GetChildrenCount() == 0)
                {
                    execution->AddChild({ nullptr, {}, nullptr });
                }

                auto& executionChild = execution->ModChild(0);

                // inputs are return values expected from the latent out call
                for (auto& returnValue : FindUserLatentReturnValues(execution))
                {
                    // if there are return values, we can continue execution after
                    // the nodeling out that is in the path (disable the contract)
                    // and we must make sure there's ONLY ONE
                    // and no immediate ins
                    auto outputAssignment = CreateOutputAssignment(returnValue);
                    executionChild.m_output.push_back({ nullptr, outputAssignment });
                }

                auto methodRoot = execution->ModRoot();

                // outputs are inputs to the latent out call
                for (auto& inputValue : FindUserLatentOutput(execution))
                {
                    inputValue->m_source = methodRoot;
                    execution->AddInput({ nullptr, inputValue, DebugDataSource::FromVariable(SlotId{}, inputValue->m_datum.GetType(), inputValue->m_sourceVariableId) });
                }

                methodRoot->CopyInput(execution, ExecutionTree::RemapVariableSource::No);
            }
            else
            {
                AddError(execution, aznew Internal::ParseError(execution->GetNodeId(), "immediate execution parsed data in latent thread"));
            }
        }

        void AbstractCodeModel::ParseUserOutCall(ExecutionTreePtr execution)
        {
            if (execution && !execution->GetRoot())
            {
                AddError(execution, aznew Internal::ParseError(execution->GetNodeId(), "User Out call has no parent"));
                return;
            }

            if (IsInLoop(execution))
            {
                AddError(execution->GetId().m_node->GetEntityId(), execution, ScriptCanvas::ParseErrors::UserOutCallInLoop);
                return;
            }

            if (IsMidSequence(execution))
            {
                AddError(execution->GetId().m_node->GetEntityId(), execution, ScriptCanvas::ParseErrors::UserOutCallMidSequence);
                return;
            }

            execution->SetSymbol(Symbol::UserOut);
            auto nodeling = azrtti_cast<const Nodes::Core::FunctionDefinitionNode*>(execution->GetId().m_node);
            execution->SetName(nodeling->GetDisplayName());

            if (execution->IsOnLatentPath())
            {
                // Data for these calls processed later here: ParseUserOuts
                m_outsMarkedLatent.insert({ nodeling, execution });
            }
            else
            {
                // Data for these calls are processed later here: ParseUserInData
                // since user out calls indicate branches in function definitions, their data is processed when
                // the function is defined, since the return values could be modified at any time starting from the beginning of the function
                m_outsMarkedImmediate.insert(nodeling);
            }
        }

        void AbstractCodeModel::ParseUserOuts()
        {
            using namespace AbstractCodeModelCpp;

            const AZStd::unordered_set< const Nodes::Core::FunctionDefinitionNode*> intersection = Intersection(m_outsMarkedLatent, m_outsMarkedImmediate);
            if (!intersection.empty())
            {
                AZStd::string report("User out(s) used in both immediate and latent out paths, immediate and latent outs cannot be shared");

                bool isFirst = true;

                AZ::EntityId nodeId;
                for (auto doubleOut : intersection)
                {
                    report += isFirst ? ": " : ", ";
                    report += doubleOut->GetDisplayName();
                    isFirst = false;
                }

                // todo may need to send multiple node Ids
                AddError(nullptr, aznew Internal::ParseError(nodeId, report));
                return;
            }

            for (auto nodeling : m_outsMarkedImmediate)
            {
                if (nodeling)
                {
                    if (!IsConnectedToUserIn(nodeling))
                    {
                        const auto report = AZStd::string::format
                        ("Nodeling Out (%s) not connected to Nodeling In, functionality cannot be executed", nodeling->GetDisplayName().data());

                        AddError(nullptr, aznew Internal::ParseError(nodeling->GetEntityId(), report));
                    }
                }
                else
                {
                    const auto report = AZStd::string::format
                    ("null nodeling in immediate out list");

                    AddError(nullptr, aznew Internal::ParseError(nodeling->GetEntityId(), report));
                }
            }

            for (auto userLatentOut : m_outsMarkedLatent)
            {
                ParseUserLatent(userLatentOut.second, userLatentOut.first);
            }
        }

        void AbstractCodeModel::ParseVariableHandling()
        {
            auto isNotActive = [&](auto iter)->bool
            {
                if (!Parse(iter.second))
                {
                    iter.second->Clear();
                    return true;
                }
                else
                {
                    return false;
                }
            };

            AZStd::erase_if(m_variableWriteHandlingBySlot, isNotActive);
        }

        bool AbstractCodeModel::ParseVariableUseAndPurity(ExecutionTreePtr execution)
        {
            // gather a list of all the variables used in the function scope
            // and in the graph scope in this execution.
            PureFunctionListener listener;
            TraverseTree(execution, listener);
            const auto& usage = listener.GetUsedVariables();
            const bool usesOnlyLocalVariables = usage.memberVariables.empty() && usage.implicitMemberVariables.empty();
            m_variableUse.localVariables.insert(usage.localVariables.begin(), usage.localVariables.end());
            m_variableUse.memberVariables.insert(usage.memberVariables.begin(), usage.memberVariables.end());
            m_variableUseByExecution.emplace(execution, listener.MoveUsedVariables());
            return (!usage.usesExternallyInitializedVariables) && usesOnlyLocalVariables && listener.IsPure();
        }

        void AbstractCodeModel::PostParseErrorDetect(ExecutionTreePtr root)
        {
            if (IsInfiniteSelfEntityActivationLoop(*this, root))
            {
                AddError(root->GetId().m_node->GetEntityId(), root, ScriptCanvas::ParseErrors::InfiniteSelfActivationLoop);
            }

            if (HasPostSelfDeactivationActivity(*this, root))
            {
                AddError(root->GetId().m_node->GetEntityId(), root, ScriptCanvas::ParseErrors::ExecutionAfterSelfDeactivation);
            }
        }

        void AbstractCodeModel::PostParseProcess(ExecutionTreePtr root)
        {
            PruneNoOpChildren(root);
            ParseEntityIdInput(root);
        }

        void AbstractCodeModel::PruneNoOpChildren(const ExecutionTreePtr& execution)
        {
            AZStd::vector<ExecutionTreePtr> noOpChildren;

            for (size_t index(0); index < execution->GetChildrenCount(); ++index)
            {
                auto& child = execution->ModChild(index);

                if (child.m_execution)
                {
                    PruneNoOpChildren(child.m_execution);

                    if (IsNoOp(*this, execution, child))
                    {
                        if (child.m_output.empty())
                        {
                            noOpChildren.push_back(child.m_execution);
                        }
                        else
                        {
                            child.m_execution->SetSymbol(Symbol::DebugInfoEmptyStatement);
                        }
                    }
                }
            }

            for (auto noOpChild : noOpChildren)
            {
                RemoveFromTree(noOpChild);
            }
        }

        void AbstractCodeModel::RemoveFromTree(ExecutionTreePtr execution)
        {
            if (!execution->GetParent())
            {
                AddError(execution->GetNodeId(), execution, ScriptCanvas::ParseErrors::MissingParentOfRemovedNode);
            }

            auto removeChildOutcome = RemoveChild(execution->ModParent(), execution);
            if (removeChildOutcome.IsSuccess())
            {
                auto childCount = execution->GetChildrenCount();
                auto indexAndChild = removeChildOutcome.TakeValue();
                ExecutionChild& removedChild = indexAndChild.second;

                if (!removedChild.m_output.empty() && childCount == 0)
                {
                    AddError(execution->GetNodeId(), execution, ScriptCanvas::ParseErrors::RequiredOutputRemoved);
                }

                if (childCount != 0)
                {
                    if (childCount > 1)
                    {
                        AddError(execution->GetNodeId(), execution, ScriptCanvas::ParseErrors::CannotRemoveMoreThanOneChild);
                    }

                    auto& child = execution->ModChild(0);
                    child.m_slot = removedChild.m_slot;
                    child.m_output = removedChild.m_output;
                    execution->ModParent()->InsertChild(indexAndChild.first, child);

                    if (child.m_execution)
                    {
                        child.m_execution->SetParent(execution->ModParent());
                    }

                    execution->ClearChildren();
                }
            }
            else
            {
                AddError(execution->GetNodeId(), execution, ScriptCanvas::ParseErrors::FailedToRemoveChild);
            }

            execution->Clear();
        }

        AZ::Outcome<AZStd::pair<size_t, ExecutionChild>> AbstractCodeModel::RemoveChild(const ExecutionTreePtr& execution, const ExecutionTreeConstPtr& child)
        {
            return execution->RemoveChild(child);
        }

        bool AbstractCodeModel::RequiresCreationFunction(Data::eType type)
        {
            return type == Data::eType::BehaviorContextObject;
        }
    }
}
