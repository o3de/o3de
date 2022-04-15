/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "Primitives.h"
#include "PrimitivesExecution.h"

namespace ScriptCanvas
{
    namespace Nodes::Core
    {
        class FunctionDefinitionNode;
    }

    namespace Grammar
    {
        struct CheckOperatorResult
        {
            Symbol symbol;
            AZStd::string name;
            LexicalScope lexicalScope;

            CheckOperatorResult() = default;
            CheckOperatorResult(Symbol symbol); /* <-- deliberately not explicit */
            CheckOperatorResult(Symbol symbol, AZStd::string_view name, LexicalScopeType scopeType);
        };

        struct VariableUseage
        {
            bool usesExternallyInitializedVariables = false;
            AZStd::unordered_set<VariableConstPtr> localVariables;
            AZStd::unordered_set<VariableConstPtr> memberVariables;
            AZStd::unordered_set<VariableConstPtr> implicitMemberVariables;

            void Clear();

            void Parse(VariableConstPtr variable);
        };

        bool ActivatesSelf(const ExecutionTreeConstPtr& execution);

        EventHandingType CheckEventHandlingType(const ExecutionTreeConstPtr& execution);

        EventHandingType CheckEventHandlingType(const Node& node);

        Symbol CheckLogicalExpressionSymbol(const ExecutionTreeConstPtr& execution);

        NodelingType CheckNodelingType(const Node& node);

        CheckOperatorResult CheckOperatorArithmeticSymbol(const ExecutionTreeConstPtr& execution);

        void DenumberFirstCharacter(AZStd::string& identifier);

        bool ExecutionContainsCycles(const Node& node, const Slot& outSlot);

        ExecutionTraversalResult TraverseExecutionConnections(const Node& node, const Slot& outSlot, GraphExecutionPathTraversalListener& listener);

        bool ExecutionWritesVariable(ExecutionTreeConstPtr execution, VariableConstPtr variable);

        const Slot* GetOnceOnResetSlot(const Node& node);

        const Slot* GetOnceOutSlot(const Node& node);

        bool HasPostSelfDeactivationActivity(const AbstractCodeModel& model, ExecutionTreeConstPtr execution);

        bool HasPostSelfDeactivationActivityRecurse(const AbstractCodeModel& model, ExecutionTreeConstPtr execution, bool& isSelfDeactivationFound);

        bool HasPropertyExtractions(const ExecutionTreeConstPtr& execution);

        bool IsBreak(const ExecutionTreeConstPtr& execution);

        bool IsClassPropertyRead(ExecutionTreeConstPtr execution);

        bool IsClassPropertyWrite(ExecutionTreeConstPtr execution);

        bool IsCodeConstructable(VariableConstPtr value);

        bool IsCycle(const Node& node);

        bool IsCycle(const ExecutionTreeConstPtr& execution);

        bool IsDetectableSelfDeactivation(const ExecutionTreeConstPtr& execution);

        bool IsEventConnectCall(const ExecutionTreeConstPtr& execution);

        bool IsEventDisconnectCall(const ExecutionTreeConstPtr& execution);

        bool IsExecutedPropertyExtraction(const ExecutionTreeConstPtr& execution);

        bool IsExternallyInitialized(VariableConstPtr value);

        bool IsFloatingPointNumberEqualityComparison(ExecutionTreeConstPtr execution);

        bool IsFlowControl(const ExecutionTreeConstPtr& execution);

        bool IsForEach(const ExecutionTreeConstPtr& execution);

        bool IsFunctionCallNullCheckRequired(const ExecutionTreeConstPtr& execution);

        bool IsGlobalPropertyRead(ExecutionTreeConstPtr execution);

        bool IsGlobalPropertyWrite(ExecutionTreeConstPtr execution);

        bool IsIfCondition(const ExecutionTreeConstPtr& execution);

        bool IsInfiniteSelfEntityActivationLoop(const AbstractCodeModel& model, ExecutionTreeConstPtr execution);

        bool IsInfiniteSelfEntityActivationLoopRecurse(const AbstractCodeModel& model, ExecutionTreeConstPtr execution);

        bool IsInfiniteVariableWriteHandlingLoop(const AbstractCodeModel& model, VariableWriteHandlingPtr variableHandling, ExecutionTreeConstPtr execution, bool isConnected);

        bool IsInLoop(const ExecutionTreeConstPtr& execution);

        // #scriptcanvas_component_extension
        bool IsInputSelf(const ExecutionInput& input);

        bool IsInputSelf(const ExecutionTreeConstPtr& execution, size_t index);

        bool IsIsNull(const ExecutionTreeConstPtr& execution);

        bool IsLogicalExpression(const ExecutionTreeConstPtr& execution);

        bool IsLooping(Symbol symbol);

        bool IsManuallyDeclaredUserVariable(VariableConstPtr variable);

        bool IsMidSequence(const ExecutionTreeConstPtr& execution);

        bool IsNoOp(const AbstractCodeModel& model, const ExecutionTreeConstPtr& parent, const Grammar::ExecutionChild& child);

        bool IsOperatorArithmetic(const ExecutionTreeConstPtr& execution);

        bool IsOnce(const Node& node);

        bool IsOnce(const ExecutionTreeConstPtr& execution);

        bool IsOnceIn(const Node& node, const Slot* slot);

        bool IsOnceReset(const Node& node, const Slot* slot);

        bool IsOnSelfEntityActivated(const AbstractCodeModel& model, ExecutionTreeConstPtr execution);

        bool IsParserGeneratedId(const VariableId& id);

        bool IsPropertyExtractionSlot(const ExecutionTreeConstPtr& execution, const Slot* outputSlot);

        bool IsPropertyExtractionNode(const ExecutionTreeConstPtr& execution);

        bool IsPure(Symbol symbol);

        bool IsPure(const Node* node, const Slot* slot);

        bool IsRandomSwitchStatement(const ExecutionTreeConstPtr& execution);

        bool IsSelf(VariableConstPtr variable);

        bool IsSequenceNode(const Node* node);

        bool IsSequenceNode(const ExecutionTreeConstPtr& execution);

        bool IsSwitchStatement(const ExecutionTreeConstPtr& execution);

        bool IsUserFunctionCall(const ExecutionTreeConstPtr& execution);

        bool IsUserFunctionCallPure(const ExecutionTreeConstPtr& execution);

        bool IsUserFunctionDefinition(const ExecutionTreeConstPtr& execution);

        const ScriptCanvas::Nodes::Core::FunctionDefinitionNode* IsUserOutNode(const Node* node);

        const ScriptCanvas::Nodes::Core::FunctionDefinitionNode* IsUserOutNode(const ExecutionTreeConstPtr& execution);

        bool IsVariableGet(const ExecutionTreeConstPtr& execution);

        bool IsVariableSet(const ExecutionTreeConstPtr& execution);

        bool IsWhileLoop(const ExecutionTreeConstPtr& execution);

        bool IsWrittenMathExpression(const ExecutionTreeConstPtr& execution);

        VariableId MakeParserGeneratedId(size_t count);

        AZStd::string MakeMemberVariableName(AZStd::string_view name);

        VariableConstructionRequirement ParseConstructionRequirement(Grammar::VariableConstPtr value);

        void ParseVariableUse(ExecutionTreeConstPtr execution, VariableUseage& variableUse);

        void PrettyPrint(AZStd::string& result, const AbstractCodeModel& model);

        void PrettyPrint(AZStd::string& result, const ExecutionTreeConstPtr& execution, ExecutionTreeConstPtr marker = nullptr);

        void ProtectReservedWords(AZStd::string& name);

        OutputAssignmentPtr RemoveOutput(ExecutionChild& execution, const SlotId& slotId);

        AZStd::string SlotNameToIndexString(const Slot& slot);

        AZStd::string ToIdentifier(AZStd::string_view name);

        AZStd::string ToIdentifierSafe(AZStd::string_view name);

        void TraverseTree(const AbstractCodeModel& execution, ExecutionTreeTraversalListener& listener);

        void TraverseTree(const ExecutionTreeConstPtr& execution, ExecutionTreeTraversalListener& listener);

        // #scriptcanvas_component_extension
        bool EntityIdValueIsNotUseable(const AZ::EntityId& entityId);

        bool IsEntityIdAndValueIsNotUseable(const VariableConstPtr& variable);
    }
}
