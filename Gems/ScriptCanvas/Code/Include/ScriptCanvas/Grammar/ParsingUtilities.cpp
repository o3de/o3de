/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/BehaviorContextUtilities.h>
#include <AzFramework/Entity/GameEntityContextBus.h>
#include <ScriptCanvas/Core/Graph.h>
#include <ScriptCanvas/Core/Node.h>
#include <ScriptCanvas/Core/Slot.h>
#include <ScriptCanvas/Execution/ExecutionState.h>
#include <ScriptCanvas/Libraries/Comparison/EqualTo.h>
#include <ScriptCanvas/Libraries/Comparison/Greater.h>
#include <ScriptCanvas/Libraries/Comparison/GreaterEqual.h>
#include <ScriptCanvas/Libraries/Comparison/Less.h>
#include <ScriptCanvas/Libraries/Comparison/LessEqual.h>
#include <ScriptCanvas/Libraries/Comparison/NotEqualTo.h>
#include <ScriptCanvas/Libraries/Core/AzEventHandler.h>
#include <ScriptCanvas/Libraries/Core/EBusEventHandler.h>
#include <ScriptCanvas/Libraries/Core/FunctionDefinitionNode.h>
#include <ScriptCanvas/Libraries/Core/ExtractProperty.h>
#include <ScriptCanvas/Libraries/Core/ForEach.h>
#include <ScriptCanvas/Libraries/Core/FunctionCallNode.h>
#include <ScriptCanvas/Libraries/Core/GetVariable.h>
#include <ScriptCanvas/Libraries/Core/Method.h>
#include <ScriptCanvas/Libraries/Core/ReceiveScriptEvent.h>
#include <ScriptCanvas/Libraries/Core/SetVariable.h>
#include <ScriptCanvas/Libraries/Core/Start.h>
#include <ScriptCanvas/Libraries/Logic/Break.h>
#include <ScriptCanvas/Libraries/Logic/Cycle.h>
#include <ScriptCanvas/Libraries/Logic/IsNull.h>
#include <ScriptCanvas/Libraries/Logic/Once.h>
#include <ScriptCanvas/Libraries/Logic/OrderedSequencer.h>
#include <ScriptCanvas/Libraries/Logic/TargetedSequencer.h>
#include <ScriptCanvas/Libraries/Logic/WeightedRandomSequencer.h>
#include <ScriptCanvas/Libraries/Logic/While.h>
#include <ScriptCanvas/Libraries/Math/MathExpression.h>
#include <ScriptCanvas/Libraries/Operators/Math/OperatorAdd.h>
#include <ScriptCanvas/Libraries/Operators/Math/OperatorArithmetic.h>
#include <ScriptCanvas/Libraries/Operators/Math/OperatorDiv.h>
#include <ScriptCanvas/Libraries/Operators/Math/OperatorMul.h>
#include <ScriptCanvas/Libraries/Operators/Math/OperatorSub.h>

#include <ScriptCanvas/Translation/Configuration.h>
#include <ScriptCanvas/Variable/VariableData.h>

#include "AbstractCodeModel.h"
#include "ParsingUtilities.h"
#include "ParsingMetaData.h"

namespace ParsingUtilitiesCpp
{
    using namespace ScriptCanvas;
    using namespace ScriptCanvas::Grammar;

    const AZ::u64 k_parserGeneratedMask = 0x7FC0616C94E7465F;
    const size_t k_maskIndex = 0;
    const size_t k_countIndex = 1;

    class PrettyPrinter
        : public ExecutionTreeTraversalListener
    {
    public:
        PrettyPrinter() = default;

        PrettyPrinter(ExecutionTreeConstPtr marker)
            : m_marker(marker)
        {
        }

        void Evaluate(ExecutionTreeConstPtr execution, const Slot* slot, int level)
        {
            for (int i = 0; i < level; ++i)
            {
                m_result += "\t";
            }

            if (slot)
            {
                m_result += slot->GetName();
                m_result += ":";
            }

            AZStd::string executionNodeName = execution->GetName();
            m_result += executionNodeName.empty() ? execution->GetId().m_node ? execution->GetId().m_node->GetNodeName() : executionNodeName : executionNodeName;
            m_result += ":";
            m_result += execution->GetId().m_slot ? execution->GetId().m_slot->GetName() : "<>";
            m_result += "[";
            m_result += GetSymbolName(execution->GetSymbol());
            m_result += "]";

            const size_t childCount = execution->GetChildrenCount();
            if (childCount != 0)
            {
                m_result += AZStd::string::format(" # children: %zu", childCount);
            }

            if (m_marker == execution)
            {
                m_result += " <<<< MARKER <<<< ";
            }

#if defined(ACM_PRINT_INPUT)
            const auto inputCount = execution->GetInputCount();
            if (inputCount != 0)
            {
                for (size_t inputIdx = 0; inputIdx != inputCount; ++inputIdx)
                {
                    m_result += " Input:\n";

                    for (int i = 0; i < level; ++i)
                    {
                        m_result += "\t";
                    }

                    auto& input = execution->GetInput(inputIdx);
                    if (input.m_slot && input.m_value)
                    {
                        m_result += AZStd::string::format
                            ( "%2d: Slot Name: %s, Type: %s, Value: %s"
                            , inputIdx
                            , input.m_slot->GetName().c_str()
                            , Data::GetName(input.m_value->m_datum.GetType()).c_str()
                            , input.m_value->m_datum.ToString().c_str());
                    }
                    else if (input.m_value)
                    {
                        m_result += AZStd::string::format
                            ( "%2d:, Value Name: %s, Type: %s, Value: %s"
                            , inputIdx
                            , input.m_value->m_name.c_str()
                            , Data::GetName(input.m_value->m_datum.GetType()).c_str()
                            , input.m_value->m_datum.ToString().c_str());
                    }
                }
            }
#endif
        }

        void EvaluateChildPre(ExecutionTreeConstPtr, const Slot*, size_t, int)
        {
            m_result += "\n";
        }

        void EvaluateRoot(ExecutionTreeConstPtr node, const Slot*)
        {
            m_result += "\nRoot:\n";
        }

        const AZStd::string& GetResult() const
        {
            return m_result;
        }

        AZStd::string&& TakeResult()
        {
            return AZStd::move(m_result);
        }

    private:
        AZStd::string m_result;
        ExecutionTreeConstPtr m_marker;
    };

    bool IsBehaviorContextProperty(const Node* node, const Slot* slot, bool checkRead, bool checkWrite)
    {
        auto methodNode = azrtti_cast<const Nodes::Core::Method*>(node);
        if (!methodNode)
        {
            return false;
        }

        auto behaviorContext = AZ::GetDefaultBehaviorContext();
        if (!behaviorContext)
        {
            return false;
        }

        auto nameOutcome = methodNode->GetFunctionCallName(slot);
        if (!nameOutcome.IsSuccess())
        {
            return false;
        }

        AZStd::string sanitized(nameOutcome.GetValue());
        AZ::RemovePropertyNameArtifacts(sanitized);

        auto iter = behaviorContext->m_properties.find(sanitized);
        if (iter == behaviorContext->m_properties.end())
        {
            return false;
        }

        if (checkRead && !iter->second->m_getter)
        {
            return false;
        }

        if (checkWrite && !iter->second->m_setter)
        {
            return false;
        }

        return true;
    }

    bool IsBehaviorContextPropertyRead(const Node* node, const Slot* slot)
    {
        return IsBehaviorContextProperty(node, slot, true, false);
    }

    bool IsBehaviorContextPropertyWrite(const Node* node, const Slot* slot)
    {
        return IsBehaviorContextProperty(node, slot, false, true);
    }
}

namespace ScriptCanvas
{
    namespace Grammar
    {
        CheckOperatorResult::CheckOperatorResult(Symbol symbol)
            : symbol(symbol)
        {}

        CheckOperatorResult::CheckOperatorResult(Symbol symbol, AZStd::string_view name, LexicalScopeType scopeType)
            : symbol(symbol)
            , name(name)
            , lexicalScope(scopeType)
        {}

        void VariableUseage::Clear()
        {
            localVariables.clear();
            memberVariables.clear();
        }

        void VariableUseage::Parse(VariableConstPtr variable)
        {
            usesExternallyInitializedVariables = usesExternallyInitializedVariables || IsExternallyInitialized(variable);

            if (IsManuallyDeclaredUserVariable(variable))
            {
                if (variable->m_isMember)
                {
                    memberVariables.insert(variable);
                }
                else
                {
                    localVariables.insert(variable);
                }
            }
            else if (variable->m_isMember)
            {
                implicitMemberVariables.insert(variable);
            }
        }

        bool ActivatesSelf(const ExecutionTreeConstPtr& execution)
        {
            auto methodNode = azrtti_cast<const ScriptCanvas::Nodes::Core::Method*>(execution->GetId().m_node);
            if (!methodNode)
            {
                return false;
            }

            auto bcMethod = methodNode->GetMethod();
            if (!bcMethod)
            {
                return false;
            }

            if (bcMethod->m_name != "ActivateGameEntity")
            {
                return false;
            }

            if (execution->GetInputCount() != 1)
            {
                return false;
            }

            if (execution->GetInput(0).m_slot && execution->GetInput(0).m_slot->IsConnected())
            {
                return false;
            }

            const auto& addressDatum = execution->GetInput(0).m_value->m_datum;

            Data::EntityIDType entityAddress;
            if (auto entityId = addressDatum.GetAs<Data::EntityIDType>())
            {
                entityAddress = *entityId;
            }
            else if (auto namedEntityId = addressDatum.GetAs<Data::NamedEntityIDType>())
            {
                entityAddress = *namedEntityId;
            }

            if (entityAddress != GraphOwnerId)
            {
                return false;
            }

            return true;
        }

        EventHandingType CheckEventHandlingType(const ExecutionTreeConstPtr& execution)
        {
            return execution->GetId().m_node ? CheckEventHandlingType(*execution->GetId().m_node) : EventHandingType::Count;
        }

        EventHandingType CheckEventHandlingType(const Node& node)
        {
            if (azrtti_istypeof<const ScriptCanvas::Nodes::Core::EBusEventHandler*>(&node))
            {
                return node.IsVariableWriteHandler() ? EventHandingType::VariableWrite : EventHandingType::EBus;
            }
            else if (azrtti_istypeof<const ScriptCanvas::Nodes::Core::ReceiveScriptEvent*>(&node))
            {
                return EventHandingType::EBus;
            }
            else if (azrtti_istypeof<const ScriptCanvas::Nodes::Core::AzEventHandler*>(&node))
            {
                return EventHandingType::Event;
            }
            else
            {
                return EventHandingType::Count;
            }
        }

        Symbol CheckLogicalExpressionSymbol(const ExecutionTreeConstPtr& execution)
        {
            if (IsIsNull(execution))
            {
                return Symbol::IsNull;
            }
            else if (execution->GetId().m_node->IsLogicalAND())
            {
                return Symbol::LogicalAND;
            }
            else if (execution->GetId().m_node->IsLogicalNOT())
            {
                return Symbol::LogicalNOT;
            }
            else if (execution->GetId().m_node->IsLogicalOR())
            {
                return Symbol::LogicalOR;
            }
            if (azrtti_istypeof<const ScriptCanvas::Nodes::ComparisonExpression*>(execution->GetId().m_node))
            {
                if (azrtti_istypeof<const ScriptCanvas::Nodes::Comparison::Greater*>(execution->GetId().m_node))
                {
                    return Symbol::CompareGreater;
                }
                else if (azrtti_istypeof<const ScriptCanvas::Nodes::Comparison::GreaterEqual*>(execution->GetId().m_node))
                {
                    return Symbol::CompareGreaterEqual;
                }
                else if (azrtti_istypeof<const ScriptCanvas::Nodes::Comparison::Less*>(execution->GetId().m_node))
                {
                    return Symbol::CompareLess;
                }
                else if (azrtti_istypeof<const ScriptCanvas::Nodes::Comparison::LessEqual*>(execution->GetId().m_node))
                {
                    return Symbol::CompareLessEqual;
                }
                else
                {
                    return Symbol::Count;
                }
            }
            else if (azrtti_istypeof<const ScriptCanvas::Nodes::EqualityExpression*>(execution->GetId().m_node))
            {
                if (azrtti_istypeof<const ScriptCanvas::Nodes::Comparison::EqualTo*>(execution->GetId().m_node))
                {
                    return Symbol::CompareEqual;
                }
                else if (azrtti_istypeof<const ScriptCanvas::Nodes::Comparison::NotEqualTo*>(execution->GetId().m_node))
                {
                    return Symbol::CompareNotEqual;
                }
                else
                {
                    return Symbol::Count;
                }
            }

            return execution->GetSymbol();
        }

        NodelingType CheckNodelingType(const Node& node)
        {
            if (auto nodeling = azrtti_cast<const ScriptCanvas::Nodes::Core::FunctionDefinitionNode*>(&node))
            {
                if (nodeling->IsExecutionEntry())
                {
                    return NodelingType::In;
                }
                else if (nodeling->IsExecutionExit())
                {
                    return NodelingType::Out;
                }
            }

            return NodelingType::None;
        }

        CheckOperatorResult CheckOperatorArithmeticSymbol(const ExecutionTreeConstPtr& execution)
        {
            if (azrtti_istypeof<const ScriptCanvas::Nodes::Operators::OperatorArithmetic*>(execution->GetId().m_node))
            {
                if (azrtti_istypeof<const ScriptCanvas::Nodes::Operators::OperatorAdd*>(execution->GetId().m_node))
                {
                    if (execution->GetInputCount() > 0 && execution->GetInput(0).m_value->m_datum.GetType() == Data::Type::Color())
                    {
                        return { Symbol::FunctionCall, "AddClamped", LexicalScopeType::Variable };
                    }
                    else
                    {
                        return Symbol::OperatorAddition;
                    }
                }
                else if (azrtti_istypeof<const ScriptCanvas::Nodes::Operators::OperatorDiv*>(execution->GetId().m_node))
                {
                    return Symbol::OperatorDivision;
                }
                else if (azrtti_istypeof<const ScriptCanvas::Nodes::Operators::OperatorMul*>(execution->GetId().m_node))
                {
                    return Symbol::OperatorMultiplication;
                }
                else if (azrtti_istypeof<const ScriptCanvas::Nodes::Operators::OperatorSub*>(execution->GetId().m_node))
                {
                    if (execution->GetInputCount() > 0 && execution->GetInput(0).m_value->m_datum.GetType() == Data::Type::Color())
                    {
                        return { Symbol::FunctionCall, "SubtractClamped", LexicalScopeType::Variable };
                    }
                    else
                    {
                        return Symbol::OperatorSubraction;
                    }
                }
                else
                {
                    return Symbol::Count;
                }
            }
            else if (auto methodNode = azrtti_cast<const ScriptCanvas::Nodes::Core::Method*>(execution->GetId().m_node))
            {
                if (auto method = methodNode->GetMethod())
                {
                    AZ::Script::Attributes::OperatorType operatorType;
                    if (AZ::ReadAttribute(operatorType, AZ::ScriptCanvasAttributes::OperatorOverride, method->m_attributes))
                    {
                        switch (operatorType)
                        {
                        case AZ::Script::Attributes::OperatorType::Add:
                            return Symbol::OperatorAddition;
                        case AZ::Script::Attributes::OperatorType::Sub:
                            return Symbol::OperatorSubraction;
                        case AZ::Script::Attributes::OperatorType::Mul:
                            return Symbol::OperatorMultiplication;
                        case AZ::Script::Attributes::OperatorType::Div:
                            return Symbol::OperatorDivision;
                        default:
                            break;
                        }
                    }
                }
            }

            return execution->GetSymbol();
        }

        void DenumberFirstCharacter(AZStd::string& identifier)
        {
            if (!identifier.empty() && isdigit(identifier.at(0)))
            {
                identifier.insert(0, "_");
            }
        }

        bool ExecutionContainsCycles(const Node& node, const Slot& outSlot)
        {
            GraphExecutionPathTraversalListener listener;
            return TraverseExecutionConnections(node, outSlot, listener) != ExecutionTraversalResult::Success;
        }

        bool ExecutionWritesVariable(ExecutionTreeConstPtr execution, VariableConstPtr variable)
        {
            for (size_t i = 0, sentinel = execution->GetChildrenCount(); i < sentinel; ++i)
            {
                auto& child = execution->GetChild(i);

                for (auto& output : child.m_output)
                {
                    if (output.second->m_source == variable)
                    {
                        return true;
                    }

                    for (auto& assignment : output.second->m_assignments)
                    {
                        if (assignment == variable)
                        {
                            return true;
                        }
                    }
                }
            }

            return false;
        }

        const Slot* GetOnceOnResetSlot(const Node& node)
        {
            return OnceProperty::GetOnResetSlot(&node);
        }

        const Slot* GetOnceOutSlot(const Node& node)
        {
            return OnceProperty::GetOutSlot(&node);
        }

        bool HasPostSelfDeactivationActivity(const AbstractCodeModel& model, ExecutionTreeConstPtr execution)
        {
            bool isSelfDeactivationFound = false;
            return HasPostSelfDeactivationActivityRecurse(model, execution, isSelfDeactivationFound);
        }

        bool HasPostSelfDeactivationActivityRecurse(const AbstractCodeModel& model, ExecutionTreeConstPtr execution, bool& isSelfDeactivationFound)
        {
            isSelfDeactivationFound = isSelfDeactivationFound || IsDetectableSelfDeactivation(execution);

            for (size_t index = 0; index < execution->GetChildrenCount(); ++index)
            {
                auto& child = execution->GetChild(index);

                if (child.m_execution)
                {
                    if (!IsNoOp(model, execution, child) && isSelfDeactivationFound)
                    {
                        return true;
                    }

                    if (HasPostSelfDeactivationActivityRecurse(model, child.m_execution, isSelfDeactivationFound))
                    {
                        return true;
                    }
                }
            }

            return false;
        }

        bool HasPropertyExtractions(const ExecutionTreeConstPtr& execution)
        {
            return execution->GetPropertyExtractionSources().empty();
        }

        bool HasReturnValues(const ExecutionTreeConstPtr& execution)
        {
            return execution->HasReturnValues();
        }

        bool IsBreak(const ExecutionTreeConstPtr& execution)
        {
            if (auto forEach = azrtti_cast<const ScriptCanvas::Nodes::Core::ForEach*>(execution->GetId().m_node))
            {
                if (forEach->GetLoopBreakSlotId() == execution->GetId().m_slot->GetId())
                {
                    return true;
                }
            }

            return azrtti_istypeof<const ScriptCanvas::Nodes::Logic::Break*>(execution->GetId().m_node);
        }

        bool IsClassPropertyRead(ExecutionTreeConstPtr execution)
        {
            return execution->GetSymbol() == Symbol::FunctionCall
                && azrtti_istypeof<const ScriptCanvas::Nodes::Core::Method*>(execution->GetId().m_node)
                && azrtti_cast<const ScriptCanvas::Nodes::Core::Method*>(execution->GetId().m_node)->GetPropertyStatus() == PropertyStatus::Getter;
        }

        bool IsClassPropertyWrite(ExecutionTreeConstPtr execution)
        {
            return execution->GetSymbol() == Symbol::FunctionCall
                && azrtti_istypeof<const ScriptCanvas::Nodes::Core::Method*>(execution->GetId().m_node)
                && azrtti_cast<const ScriptCanvas::Nodes::Core::Method*>(execution->GetId().m_node)->GetPropertyStatus() == PropertyStatus::Setter;
        }

        bool IsCodeConstructable(Grammar::VariableConstPtr value)
        {
            return Data::IsValueType(value->m_datum.GetType())
                // || value->m_datum.GetType().GetAZType() == azrtti_typeid<Nodeable>() // <--- cut this.. can't be needed or correct
                || value->m_datum.GetAsDanger() == nullptr;
            ;
        }

        bool IsCycle(const Node& node)
        {
            return azrtti_istypeof<const ScriptCanvas::Nodes::Logic::Cycle>(node);
        }

        bool IsCycle(const ExecutionTreeConstPtr& execution)
        {
            return execution->GetId().m_node
                && IsCycle(*execution->GetId().m_node)
                && execution->GetId().m_slot->GetType() == CombinedSlotType::ExecutionIn;
        }

        bool IsDetectableSelfDeactivation(const ExecutionTreeConstPtr& execution)
        {
            if (execution->GetSymbol() == Symbol::FunctionCall)
            {
                if (auto methodNode = azrtti_cast<const Nodes::Core::Method*>(execution->GetId().m_node))
                {
                    if (auto behaviorMethod = methodNode->GetMethod())
                    {
                        if (AZ::FindAttribute(AZ::ScriptCanvasAttributes::DeactivatesInputEntity, behaviorMethod->m_attributes))
                        {
                            if (execution->GetInputCount() == 1 && !execution->GetInput(0).m_slot->IsConnected() && IsSelfInput(execution, 0))
                            {
                                return true;
                            }
                        }
                    }
                }
            }

            return false;
        }

        bool IsEventConnectCall(const ExecutionTreeConstPtr& execution)
        {
            return execution->GetSymbol() == Symbol::FunctionCall
                && CheckEventHandlingType(execution) == EventHandingType::Event
                && execution->GetId().m_slot == AzEventHandlerProperty::GetConnectSlot(execution->GetId().m_node);
        }

        bool IsEventDisconnectCall(const ExecutionTreeConstPtr& execution)
        {
            return execution->GetSymbol() == Symbol::FunctionCall
                && CheckEventHandlingType(execution) == EventHandingType::Event
                && execution->GetId().m_slot == AzEventHandlerProperty::GetDisconnectSlot(execution->GetId().m_node);
        }

        bool IsExecutedPropertyExtraction(const ExecutionTreeConstPtr& execution)
        {
            return execution->GetExecutedPropertyExtraction() != nullptr;
        }


        bool IsExternallyInitialized(VariableConstPtr value)
        {
            const auto requirement = ParseConstructionRequirement(value);
            return !(requirement == VariableConstructionRequirement::None || requirement == VariableConstructionRequirement::Static);
        }

        bool IsFloatingPointNumberEqualityComparison(ExecutionTreeConstPtr execution)
        {
            const auto symbol = execution->GetSymbol();
            return (symbol == Symbol::CompareEqual || symbol == Symbol::CompareEqual)
                && execution->GetInputCount() == 2
                && execution->GetInput(0).m_value->m_datum.GetType() == Data::Type::Number()
                && execution->GetInput(1).m_value->m_datum.GetType() == Data::Type::Number();
        }

        bool IsFlowControl(const ExecutionTreeConstPtr& execution)
        {
            // \note this grammar check matches AbstractCodeModel::ParseExecutionTreeBody, and needs to be merged with and replace ParseExecutionFunctionRecurse
            return IsBreak(execution)
                || IsCycle(execution)
                || IsForEach(execution)
                || IsIfCondition(execution)
                || IsOnce(execution)
                || IsRandomSwitchStatement(execution)
                || IsSequenceNode(execution)
                || IsSwitchStatement(execution)
                || IsUserOutNode(execution)
                || IsWhileLoop(execution);
        }

        bool IsForEach(const ExecutionTreeConstPtr& execution)
        {
            return azrtti_istypeof<const ScriptCanvas::Nodes::Core::ForEach*>(execution->GetId().m_node)
                && execution->GetId().m_slot
                && execution->GetId().m_slot->GetType() == CombinedSlotType::ExecutionIn
                ;
        }

        bool IsFunctionCallNullCheckRequired(const ExecutionTreeConstPtr& execution)
        {
            return execution->GetSymbol() == Symbol::FunctionCall
                && execution->GetInputCount() > 0
                && execution->GetInput(0).m_value->m_requiresNullCheck;
        }

        bool IsGlobalPropertyRead(ExecutionTreeConstPtr execution)
        {
            return execution->GetSymbol() == Symbol::FunctionCall
                && execution->GetInputCount() == 0
                && ParsingUtilitiesCpp::IsBehaviorContextPropertyRead(execution->GetId().m_node, execution->GetId().m_slot);
        }

        bool IsGlobalPropertyWrite(ExecutionTreeConstPtr execution)
        {
            return execution->GetSymbol() == Symbol::FunctionCall
                && execution->GetInputCount() == 0
                && ParsingUtilitiesCpp::IsBehaviorContextPropertyWrite(execution->GetId().m_node, execution->GetId().m_slot);
        }

        bool IsIfCondition(const ExecutionTreeConstPtr& execution)
        {
            return execution->GetId().m_node->IsIfBranch()
                && execution->GetId().m_slot->GetType() == CombinedSlotType::ExecutionIn;
        }

        bool IsInfiniteSelfEntityActivationLoop(const AbstractCodeModel& model, ExecutionTreeConstPtr execution)
        {
            return (azrtti_istypeof<const Nodes::Core::Start*>(execution->GetId().m_node) || IsOnSelfEntityActivated(model, execution))
                && IsInfiniteSelfEntityActivationLoopRecurse(model, execution);
        }

        bool IsInfiniteSelfEntityActivationLoopRecurse(const AbstractCodeModel& model, ExecutionTreeConstPtr execution)
        {
            if (!execution)
            {
                return false;
            }

            if (ActivatesSelf(execution))
            {
                return true;
            }

            for (size_t index = 0, sentinel = execution->GetChildrenCount(); index < sentinel; ++index)
            {
                if (IsInfiniteSelfEntityActivationLoopRecurse(model, execution->GetChild(index).m_execution))
                {
                    return true;
                }
            }

            return false;
        }

        bool IsInfiniteVariableWriteHandlingLoop(const AbstractCodeModel& model, VariableWriteHandlingPtr variableHandling, ExecutionTreeConstPtr execution, bool isConnected)
        {
            if (!execution)
            {
                return false;
            }

            auto id = execution->GetId();

            if (isConnected)
            {
                if (ExecutionWritesVariable(execution, variableHandling->m_variable))
                {
                    return true;
                }
                else if (id.m_node
                    && id.m_slot == id.m_node->GetEBusDisconnectSlot()
                    && model.GetVariableHandling(id.m_node->GetEBusConnectAddressSlot()) == variableHandling)
                {
                    isConnected = false;
                }
            }
            else if (!isConnected
                && id.m_node
                && id.m_slot == id.m_node->GetEBusDisconnectSlot()
                && model.GetVariableHandling(id.m_node->GetEBusConnectAddressSlot()) == variableHandling)
            {
                isConnected = true;
            }

            for (size_t index = 0, sentinel = execution->GetChildrenCount(); index < sentinel; ++index)
            {
                if (IsInfiniteVariableWriteHandlingLoop(model, variableHandling, execution->GetChild(index).m_execution, isConnected))
                {
                    return true;
                }
            }

            return false;
        }

        bool IsInLoop(const ExecutionTreeConstPtr& execution)
        {
            auto parent = execution->GetParent();
            if (!parent)
            {
                return false;
            }

            auto id = execution->GetId();
            if (IsLooping(parent->GetSymbol())
                && id.m_slot
                && id.m_node
                && id.m_slot->GetId() == id.m_node->GetLoopSlotId())
            {
                return true;
            }

            return IsInLoop(parent);
        }

        bool IsIsNull(const ExecutionTreeConstPtr& execution)
        {
            return azrtti_istypeof<const ScriptCanvas::Nodes::Logic::IsNull>(execution->GetId().m_node)
                && execution->GetId().m_slot
                && execution->GetId().m_slot->GetType() == CombinedSlotType::ExecutionIn
                ;
        }

        bool IsLogicalExpression(const ExecutionTreeConstPtr& execution)
        {
            const auto symbol = execution->GetSymbol();
            return symbol == Symbol::CompareEqual
                || symbol == Symbol::CompareGreater
                || symbol == Symbol::CompareGreaterEqual
                || symbol == Symbol::CompareLess
                || symbol == Symbol::CompareLessEqual
                || symbol == Symbol::CompareNotEqual
                || symbol == Symbol::IsNull
                || symbol == Symbol::LogicalAND
                || symbol == Symbol::LogicalNOT
                || symbol == Symbol::LogicalOR
                ;
        }

        bool IsLooping(Symbol symbol)
        {
            return symbol == Symbol::ForEach
                || symbol == Symbol::While;
        }

        bool IsManuallyDeclaredUserVariable(VariableConstPtr variable)
        {
            return variable
                && variable->m_source == nullptr
                && !variable->m_sourceSlotId.IsValid()
                && variable->m_sourceVariableId.IsValid()
                && !variable->m_nodeableNodeId.IsValid();
        }

        bool IsMidSequence(const ExecutionTreeConstPtr& execution)
        {
            if (!execution)
            {
                return false;
            }

            auto parent = execution->GetParent();
            if (!parent)
            {
                return false;
            }

            const size_t childrenCount = parent->GetChildrenCount();

            if (childrenCount == 0)
            {
                return false;
            }

            if (parent->GetSymbol() == Symbol::Sequence)
            {
                size_t childIndex = parent->FindChildIndex(execution);
                if (childIndex < (childrenCount - 1))
                {
                    return true;
                }
            }

            return IsMidSequence(parent);
        }

        bool IsNoOp(const AbstractCodeModel& model, const ExecutionTreeConstPtr& parent, const Grammar::ExecutionChild& child)
        {
            const ExecutionTreeConstPtr& execution = child.m_execution;

            if (IsLooping(parent->GetSymbol()))
            {
                return false;
            }

            if (child.m_execution->GetSymbol() == Symbol::VariableAssignment)
            {
                auto dataSlots = child.m_execution->GetId().m_node->GetOnVariableHandlingDataSlots();
                if (dataSlots.size() > 0)
                {
                    bool isNoOp = true;
                    for (auto dataSlot : dataSlots)
                    {
                        auto writeHandling = model.GetVariableHandling(dataSlot);
                        if (!writeHandling || writeHandling->RequiresConnectionControl())
                        {
                            isNoOp = false;
                            break;
                        }
                    }

                    if (isNoOp)
                    {
                        return true;
                    }
                }
            }

            if (IsPropertyExtractionNode(execution) && !IsExecutedPropertyExtraction(execution))
            {
                return true;
            }

            if (execution->GetSymbol() == Symbol::PlaceHolderDuringParsing)
            {
                return true;
            }

            if (!execution->GetId().m_node)
            {
                return false;
            }

            if (execution->GetId().m_node->IsNoOp())
            {
                return true;
            }

            return false;
        }

        bool IsOnce(const Node& node)
        {
            return azrtti_istypeof<const ScriptCanvas::Nodes::Logic::Once>(node);
        }

        bool IsOnce(const ExecutionTreeConstPtr& execution)
        {
            return execution->GetId().m_node
                && IsOnce(*execution->GetId().m_node)
                && execution->GetId().m_slot
                && execution->GetId().m_slot->GetType() == CombinedSlotType::ExecutionIn;
        }

        bool IsOnceIn(const Node& node, const Slot* slot)
        {
            return slot == OnceProperty::GetInSlot(&node);
        }

        bool IsOnceReset(const Node& node, const Slot* slot)
        {
            return slot == OnceProperty::GetResetSlot(&node);
        }

        bool IsOperatorArithmetic(const ExecutionTreeConstPtr& execution)
        {
            const auto symbol = execution->GetSymbol();
            return symbol == Symbol::OperatorAddition
                || symbol == Symbol::OperatorDivision
                || symbol == Symbol::OperatorMultiplication
                || symbol == Symbol::OperatorSubraction
                ;
        }

        bool IsOnSelfEntityActivated(const AbstractCodeModel&, ExecutionTreeConstPtr execution)
        {
            auto id = execution->GetId();

            auto eventHandlerNode = azrtti_cast<const Nodes::Core::EBusEventHandler*>(id.m_node);
            if (!eventHandlerNode)
            {
                return false;
            }

            AZ::BehaviorEBus* ebus = eventHandlerNode->GetBus();
            if (ebus->m_name != "EntityBus")
            {
                return false;
            }

            if (!id.m_slot)
            {
                return false;
            }

            const Nodes::Core::EBusEventEntry* entry = eventHandlerNode->FindEventWithSlot(*id.m_slot);
            if (!entry)
            {
                return false;
            }

            if (entry->m_eventName != "OnEntityActivated")
            {
                return false;
            }

            Data::EntityIDType entityAddress;

            if (auto addressDatum = eventHandlerNode->GetHandlerStartAddress())
            {
                if (auto entityId = addressDatum->GetAs<Data::EntityIDType>())
                {
                    entityAddress = *entityId;
                }
                else if (auto namedEntityId = addressDatum->GetAs<Data::NamedEntityIDType>())
                {
                    entityAddress = *namedEntityId;
                }
            }

            if (entityAddress != GraphOwnerId)
            {
                return false;
            }

            return true;
        }

        bool IsParserGeneratedId(const ScriptCanvas::VariableId& id)
        {
            using namespace ParsingUtilitiesCpp;
            return reinterpret_cast<const AZ::u64*>(AZStd::ranges::data(id.m_id))[k_maskIndex] == k_parserGeneratedMask;
        }

        bool IsPropertyExtractionSlot(const ExecutionTreeConstPtr& execution, const Slot* outputSlot)
        {
            auto iter = AZStd::find_if
            (execution->GetPropertyExtractionSources().begin()
                , execution->GetPropertyExtractionSources().end()
                , [&](const auto& iter) { return iter.first == outputSlot; });

            return iter != execution->GetPropertyExtractionSources().end();
        }

        bool IsPropertyExtractionNode(const ExecutionTreeConstPtr& execution)
        {
            return azrtti_istypeof<const ScriptCanvas::Nodes::Core::ExtractProperty*>(execution->GetId().m_node);
        }

        bool IsPure(Symbol symbol)
        {
            return symbol != Symbol::Cycle;
        }

        bool IsPure(const Node* node, const Slot* slot)
        {
            if (auto userFunctionCall = azrtti_cast<const ScriptCanvas::Nodes::Core::FunctionCallNode*>(node))
            {
                if (!userFunctionCall->IsSlotPure(slot))
                {
                    return false;
                }
            }
            else if (node && IsOnce(*node))
            {
                return false;
            }

            return true;
        }

        bool IsRandomSwitchStatement(const ExecutionTreeConstPtr& execution)
        {
            return azrtti_istypeof<const ScriptCanvas::Nodes::Logic::WeightedRandomSequencer>(execution->GetId().m_node)
                && execution->GetId().m_slot->GetType() == CombinedSlotType::ExecutionIn;
        }

        bool IsSelf(VariableConstPtr variable)
        {
            return variable
                && ((variable->m_datum.GetAs<Data::NamedEntityIDType>()
                    && *variable->m_datum.GetAs<Data::NamedEntityIDType>() == GraphOwnerId
                    && !variable->m_isExposedToConstruction)
                    || (variable->m_datum.GetAs<Data::EntityIDType>()
                        && *variable->m_datum.GetAs<Data::EntityIDType>() == GraphOwnerId
                        && !variable->m_isExposedToConstruction));
        }

        bool IsSelfInput(const ExecutionInput& input)
        {
            return IsSelf(input.m_value);
        }

        bool IsSelfInput(const ExecutionTreeConstPtr& execution, size_t index)
        {
            return execution->GetInputCount() > index && IsSelfInput(execution->GetInput(index));
        }

        bool IsSelfReturnValue(ReturnValueConstPtr returnValue)
        {
            return IsSelf(returnValue->m_source);
        }

        bool IsSequenceNode(const Node* node)
        {
            return azrtti_istypeof<const ScriptCanvas::Nodes::Logic::OrderedSequencer*>(node);
        }

        bool IsSequenceNode(const ExecutionTreeConstPtr& execution)
        {
            return (IsSequenceNode(execution->GetId().m_node)
                && execution->GetId().m_slot->GetType() == CombinedSlotType::ExecutionIn);
        }

        bool IsSwitchStatement(const ExecutionTreeConstPtr& execution)
        {
            return execution->GetId().m_node->IsSwitchStatement()
                && execution->GetId().m_slot->GetType() == CombinedSlotType::ExecutionIn;
        }

        bool IsUserFunctionCall(const ExecutionTreeConstPtr& execution)
        {
            return (execution->GetSymbol() == Symbol::FunctionCall)
                && azrtti_istypeof<const ScriptCanvas::Nodes::Core::FunctionCallNode*>(execution->GetId().m_node);
        }

        bool IsUserFunctionCallPure(const ExecutionTreeConstPtr& execution)
        {
            return (execution->GetSymbol() == Symbol::FunctionCall)
                && azrtti_istypeof<const ScriptCanvas::Nodes::Core::FunctionCallNode*>(execution->GetId().m_node)
                && azrtti_cast<const ScriptCanvas::Nodes::Core::FunctionCallNode*>(execution->GetId().m_node)->IsPure();
        }

        bool IsUserFunctionDefinition(const ExecutionTreeConstPtr& execution)
        {
            auto nodeling = azrtti_cast<const ScriptCanvas::Nodes::Core::FunctionDefinitionNode*>(execution->GetId().m_node);
            return nodeling && nodeling->IsExecutionEntry() && execution->GetSymbol() == Symbol::FunctionDefinition;
        }

        bool IsUserFunctionCallLocallyDefined(const AbstractCodeModel& model, const Node& node)
        {
            auto functionCallNode = azrtti_cast<const ScriptCanvas::Nodes::Core::FunctionCallNode*>(&node);
            if (!functionCallNode)
            {
                return false;
            }

            const auto& source = model.GetSource();

            auto assetId = functionCallNode->GetAssetId();
            if (source.m_assetId.m_guid == assetId.m_guid)
            {
                return true;
            }

            // move check later after testing
            AZ::IO::Path nodeSourcePath = functionCallNode->GetAssetHint();
            nodeSourcePath = nodeSourcePath.MakePreferred().ReplaceExtension();
            AZ::IO::Path sourcePath = source.m_path;
            sourcePath = sourcePath.MakePreferred().ReplaceExtension();

            if (nodeSourcePath.IsRelativeTo(sourcePath) || sourcePath.IsRelativeTo(nodeSourcePath))
            {
                return true;
            }

            return false;
        }

        bool IsUserFunctionCallLocallyDefined(const ExecutionTreeConstPtr& execution)
        {
            auto userFunctionCallMetaData = AZStd::any_cast<const UserFunctionNodeCallMetaData>(&execution->GetMetaDataEx());
            return userFunctionCallMetaData && userFunctionCallMetaData->m_isLocal;
        }

        const ScriptCanvas::Nodes::Core::FunctionDefinitionNode* IsUserOutNode(const Node* node)
        {
            auto nodeling = azrtti_cast<const ScriptCanvas::Nodes::Core::FunctionDefinitionNode*>(node);
            return nodeling && nodeling->IsExecutionExit() ? nodeling : nullptr;
        }

        const ScriptCanvas::Nodes::Core::FunctionDefinitionNode* IsUserOutNode(const ExecutionTreeConstPtr& execution)
        {
            auto nodeling = IsUserOutNode(execution->GetId().m_node);
            return nodeling ? nodeling : nullptr;
        }

        bool IsVariableGet(const ExecutionTreeConstPtr& execution)
        {
            return azrtti_istypeof<const ScriptCanvas::Nodes::Core::GetVariableNode*>(execution->GetId().m_node)
                && !IsExecutedPropertyExtraction(execution);
        }

        bool IsVariableSet(const ExecutionTreeConstPtr& execution)
        {
            return azrtti_istypeof<const ScriptCanvas::Nodes::Core::SetVariableNode*>(execution->GetId().m_node)
                && !IsExecutedPropertyExtraction(execution);
        }

        bool IsWhileLoop(const ExecutionTreeConstPtr& execution)
        {
            return azrtti_istypeof<const ScriptCanvas::Nodes::Logic::While*>(execution->GetId().m_node)
                && execution->GetId().m_slot->GetType() == CombinedSlotType::ExecutionIn;
        }

        bool IsWrittenMathExpression(const ExecutionTreeConstPtr& execution)
        {
            return azrtti_istypeof<const ScriptCanvas::Nodes::Math::MathExpression*>(execution->GetId().m_node);
        }

        AZStd::string MakeMemberVariableName(AZStd::string_view name)
        {
            return AZStd::string::format("%s%s", k_memberNamePrefix, name.data());
        }

        VariableId MakeParserGeneratedId(size_t count)
        {
            using namespace ParsingUtilitiesCpp;

            AZ::Uuid parserGenerated;
            auto parserGeneratedData = reinterpret_cast<AZ::u64*>(AZStd::ranges::data(parserGenerated));
            parserGeneratedData[k_maskIndex] = k_parserGeneratedMask;
            parserGeneratedData[k_countIndex] = count;
            return ScriptCanvas::VariableId(parserGenerated);
        }

        VariableConstructionRequirement ParseConstructionRequirement(VariableConstPtr variable)
        {
            if (IsEntityIdAndValueIsNotUseable(variable))
            {
                return VariableConstructionRequirement::InputEntityId;
            }
            else if (IsSelf(variable))
            {
                return VariableConstructionRequirement::SelfEntityId;
            }
            else if (variable->m_isExposedToConstruction)
            {
                if (variable->m_nodeableNodeId.IsValid())
                {
                    return VariableConstructionRequirement::InputNodeable;
                }
                else if (variable->m_sourceVariableId.IsValid())
                {
                    return VariableConstructionRequirement::InputVariable;
                }
                else
                {
                    AZ_Assert(false, "A member variable in the model has no valid id");
                }
            }
            else if (variable->m_sourceVariableId.IsValid() && !IsCodeConstructable(variable))
            {
                return VariableConstructionRequirement::Static;
            }

            return VariableConstructionRequirement::None;
        }

        void ParseVariableUse(const OutputAssignment& outputAssignment, VariableUseage& variableUse)
        {
            variableUse.Parse(outputAssignment.m_source);

            for (const auto& assignment : outputAssignment.m_assignments)
            {
                variableUse.Parse(assignment);
            }
        }

        void ParseVariableUse(ExecutionTreeConstPtr execution, VariableUseage& variableUse)
        {
            for (size_t inputIdx = 0; inputIdx != execution->GetInputCount(); ++inputIdx)
            {
                variableUse.Parse(execution->GetInput(inputIdx).m_value);
            }

            for (size_t returnIdx = 0; returnIdx != execution->GetReturnValueCount(); ++returnIdx)
            {
                if (auto returnValue = execution->GetReturnValue(returnIdx).second)
                {
                    ParseVariableUse(*returnValue, variableUse);
                }
            }

            if (auto localOutputPtr = execution->GetLocalOutput())
            {
                for (const auto& localOuput : *localOutputPtr)
                {
                    if (localOuput.second)
                    {
                        return ParseVariableUse(*localOuput.second, variableUse);
                    }
                }
            }
            else
            {
                for (size_t childIdx = 0; childIdx != execution->GetChildrenCount(); ++childIdx)
                {
                    const auto& child = execution->GetChild(childIdx);
                    for (const auto& localOuput : child.m_output)
                    {
                        if (localOuput.second)
                        {
                            ParseVariableUse(*localOuput.second, variableUse);
                        }
                    }
                }
            }
        }

        void PrettyPrint(AZStd::string& result, const AbstractCodeModel& model)
        {
            if (model.IsClass())
            {
                result += "* Per entity data required\n";
            }
            else
            {
                result += "* Pure functionality\n";
            }

            for (auto& variable : model.GetVariables())
            {
                result += AZStd::string::format("Variable: %s, Type: %s, Scope: %s, \n"
                    , variable->m_name.data()
                    , Data::GetName(variable->m_datum.GetType()).data()
                    , variable->m_isMember ? "Member" : "Local");
            }

            auto roots = model.GetAllExecutionRoots();
            for (auto& root : roots)
            {
                ParsingUtilitiesCpp::PrettyPrinter printer;
                TraverseTree(root, printer);
                result += printer.TakeResult();
            }
        }

        void PrettyPrint(AZStd::string& result, const ExecutionTreeConstPtr& execution, ExecutionTreeConstPtr marker)
        {
            ParsingUtilitiesCpp::PrettyPrinter printer(marker);
            TraverseTree(execution, printer);
            result = printer.TakeResult();
        }

        void ProtectReservedWords(AZStd::string& name)
        {
            if (!name.ends_with(k_reservedWordProtection))
            {
                name.append(k_reservedWordProtection);
            }
        }

        OutputAssignmentPtr RemoveOutput(ExecutionChild& executionChild, const SlotId& slotId)
        {
            auto iter = AZStd::find_if(executionChild.m_output.begin(), executionChild.m_output.end(), [&](const auto& candidate) { return candidate.first && candidate.first->GetId() == slotId; });

            if (iter != executionChild.m_output.end())
            {
                auto output = iter->second;
                executionChild.m_output.erase(iter);
                return AZStd::const_pointer_cast<OutputAssignment>(output);
            }
            else
            {
                return nullptr;
            }
        }

        AZStd::string SlotNameToIndexString(const Slot& slot)
        {
            auto indexString = slot.GetName();
            indexString.erase(0, strlen("Out "));
            return indexString;
        }

        AZStd::string ToIdentifier(AZStd::string_view name)
        {
            auto identifier = AZ::ReplaceCppArtifacts(name);
            AZ::StringFunc::Replace(identifier, " ", "_", true);
            DenumberFirstCharacter(identifier);
            return identifier;
        }

        AZStd::string ToIdentifierSafe(AZStd::string_view name)
        {
            auto identifier = AZ::ReplaceCppArtifacts(name);
            AZ::StringFunc::Replace(identifier, " ", "_", true);
            DenumberFirstCharacter(identifier);
            ProtectReservedWords(identifier);
            return identifier;
        }

        ExecutionTraversalResult TraverseExecutionConnectionsRecurse(const EndpointsResolved& nextEndpoints, AZStd::unordered_set<const Slot*>& previousIns, GraphExecutionPathTraversalListener& listener);

        ExecutionTraversalResult TraverseExecutionConnectionsRecurse(const EndpointResolved& in, AZStd::unordered_set<const Slot*>& previousIns, GraphExecutionPathTraversalListener& listener);

        ExecutionTraversalResult TraverseExecutionConnections(const Node& node, const Slot& outSlot, GraphExecutionPathTraversalListener& listener)
        {
            AZStd::unordered_set<const Slot*> path;
            return TraverseExecutionConnectionsRecurse({ &node, &outSlot }, path, listener);
        }

        ExecutionTraversalResult TraverseExecutionConnectionsRecurse(const EndpointsResolved& nextEndpoints, AZStd::unordered_set<const Slot*>& previousPath, GraphExecutionPathTraversalListener& listener)
        {
            if (listener.CancelledTraversal())
            {
                return ExecutionTraversalResult::Success;
            }

            if (!nextEndpoints.empty())
            {
                if (nextEndpoints.size() == 1)
                {
                    auto status = TraverseExecutionConnectionsRecurse(nextEndpoints[0], previousPath, listener);
                    if (status != ExecutionTraversalResult::Success)
                    {
                        return status;
                    }
                }
                else
                {
                    // Subsequent connections after an the multiple Execution Out connections syntax sugar
                    // only have to check for loops up to the sequence point.
                    // Duplicate endpoints after the sequence are not necessarily loops, but are likely just the normal
                    // ScriptCanvas way allowing users to use the same visual path (thus preventing "duplicate code").
                    for (auto nextEndpoint : nextEndpoints)
                    {
                        AZStd::unordered_set<const Slot*> pathUpToSequenceSyntaxSugar(previousPath);

                        auto status = TraverseExecutionConnectionsRecurse(nextEndpoint, pathUpToSequenceSyntaxSugar, listener);
                        if (status != ExecutionTraversalResult::Success)
                        {
                            return status;
                        }
                    }
                }
            }

            return ExecutionTraversalResult::Success;
        }

        ExecutionTraversalResult TraverseExecutionConnectionsRecurse(const EndpointResolved& in, AZStd::unordered_set<const Slot*>& previousPath, GraphExecutionPathTraversalListener& listener)
        {
            if (previousPath.contains(in.second))
            {
                return ExecutionTraversalResult::ContainsCycle;
            }

            if (!in.second)
            {
                return ExecutionTraversalResult::NullSlot;
            }

            listener.Evaluate(in);

            if (listener.CancelledTraversal())
            {
                return ExecutionTraversalResult::Success;
            }

            AZStd::vector<const Slot*> outSlots;

            if (in.second->IsLatent())
            {
                outSlots.push_back(in.second);
            }
            else
            {
                previousPath.insert(in.second);

                auto outSlotsOutcome = in.first->GetSlotsInExecutionThreadByType(*in.second, CombinedSlotType::ExecutionOut);
                if (!outSlotsOutcome.IsSuccess())
                {
                    return ExecutionTraversalResult::GetSlotError;
                }

                outSlots = outSlotsOutcome.GetValue();
            }

            for (auto branch : outSlots)
            {
                auto nextEndpoints = in.first->GetConnectedNodes(*branch);
                AZStd::unordered_set<const Slot*> pathUpToBranchOrSyntaxSugar(previousPath);

                const auto status = TraverseExecutionConnectionsRecurse(nextEndpoints, pathUpToBranchOrSyntaxSugar, listener);
                if (status != ExecutionTraversalResult::Success)
                {
                    return status;
                }
            }

            return ExecutionTraversalResult::Success;
        }

        void TraverseTreeRecurse(const ExecutionTreeConstPtr& execution, ExecutionTreeTraversalListener& listener, const Slot* slot, int depth = 0);

        void TraverseTree(const AbstractCodeModel& model, ExecutionTreeTraversalListener& listener)
        {
            for (auto root : model.GetAllExecutionRoots())
            {
                TraverseTree(root, listener);
            }
        }

        void TraverseTree(const ExecutionTreeConstPtr& execution, ExecutionTreeTraversalListener& listener)
        {
            listener.Reset();
            TraverseTreeRecurse(execution, listener, nullptr, 0);
        }

        void TraverseTreeRecurse(const ExecutionTreeConstPtr& execution, ExecutionTreeTraversalListener& listener, const Slot* slot, int level)
        {
            if (listener.CancelledTraversal())
            {
                return;
            }

            if (!execution->GetParent())
            {
                listener.EvaluateRoot(execution, slot);
            }

            listener.Evaluate(execution, slot, level);

            if (execution->GetChildrenCount() == 0 && !IsInLoop(execution) && !IsMidSequence(execution))
            {
                listener.EvaluateLeaf(execution, slot, level);
            }
            else
            {
                for (size_t i(0); i < execution->GetChildrenCount(); ++i)
                {
                    auto& childIter = execution->GetChild(i);

                    if (childIter.m_execution)
                    {
                        listener.EvaluateChildPre(execution, childIter.m_slot, i, level + 1);
                        TraverseTreeRecurse(childIter.m_execution, listener, childIter.m_slot, level + 1);
                        listener.EvaluateChildPost(execution, childIter.m_slot, i, level + 1);
                    }
                    else if (!IsInLoop(execution) && !IsMidSequence(execution))
                    {
                        listener.EvaluateNullChildLeaf(execution, childIter.m_slot, i, level + 1);
                    }
                }
            }
        }

        bool EntityIdValueIsNotUseable(const AZ::EntityId& entityId)
        {
            return entityId.IsValid()
                && entityId != UniqueId
                && entityId != GraphOwnerId;
        }

        bool IsEntityIdAndValueIsNotUseable(const VariableConstPtr& variable)
        {
            if (auto candidate = variable->m_datum.GetAs<Data::EntityIDType>())
            {
                return (!variable->m_isExposedToConstruction) && EntityIdValueIsNotUseable(*candidate);
            }
            else if (auto candidate2 = variable->m_datum.GetAs<Data::NamedEntityIDType>())
            {
                return (!variable->m_isExposedToConstruction) && EntityIdValueIsNotUseable(*candidate2);
            }
            else
            {
                return false;
            }
        }
    }
}

#include <ScriptCanvas/Libraries/Time/RepeaterNodeable.h>
#include <ScriptCanvas/Libraries/Time/DelayNodeable.h>
#include <ScriptCanvas/Libraries/Time/TimeDelayNodeable.h>
#include <ScriptCanvas/Libraries/Logic/Gate.h>
