/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

namespace ScriptCanvas
{
    namespace ParseErrors
    {
        constexpr const char* AddOutputNameFailure = "Failure to add output name to scope";
        constexpr const char* AttemptToParseNonExpression = "Attempting to parse an expression on a node that is not an expression";
        constexpr const char* BadEventHandlingAccounting = "Bad event handling accounting";
        constexpr const char* BreakNotInForEachScope = "Break is not in the same ForEach scope.";
        constexpr const char* CannotRemoveMoreThanOneChild = "Cannot remove a node with more than 1 child from the execution tree";
        constexpr const char* CircularDependencyFormat = "%s graph has attempted to user subgraph by Node %s which itself uses %s graph. Try to take the required functionality, and make a new, smaller subgraph that both graphs can share.";
        constexpr const char* CustomParsingRequiredForVariable = "Nodeable variable should have been made for this node, or it needs custom parsing";
        constexpr const char* CycleDetected = "Execution cycle detected. Use a looping node like While or For";
        constexpr const char* DependencyRetrievalFailiure = "A node failed to retrieve its dependencies.";
        constexpr const char* DirectEntityReferencesNotAllowed =
                "Direct Entity References from the ScriptCanvas Editor are not allowed. "
                "Make and EntityId variable, and select 'FromComponent' as the initial Value. "
                "Then choose the required entity from the scene editor after the graph has been assigned to a component.";
        constexpr const char* DuplicateInputProcessed = "Input to the slot at this execution has already been found";
        constexpr const char* EmptyGraph = "Completely Empty Graph";
        constexpr const char* ExecutionAfterSelfDeactivation = "No execution can occur after self deactivation. Such execution has been detected.";
        constexpr const char* EventNodeConnectCallMalformed = "Event Handler node connect slot must be connected to one and only one other node";
        constexpr const char* EventNodeConnectMissingChild = "Event Connect call was missing parent grammar node";
        constexpr const char* EventNodeConnectMissingChildOutput = "Event Connect call parent was missing child";
        constexpr const char* EventNodeConnectMissingChildOutputSourceNotSet = "Event Connect call parent was missing output";
        constexpr const char* EventNodeConnectMissingChildOutputNotLocal = "Event Connect call parent output was marked as a member variable";
        constexpr const char* EventNodeConnectMissingParent = "There should always be a parent of a connection to an AZ::Event";
        constexpr const char* EventNodeMissingConnectSlot = "Event Handler node was missing a connect slot";
        constexpr const char* EventNodeMissingConnectEventInputSlot = "Event Handler node was missing an input slot for the event for its connect slot";
        constexpr const char* EventNodeMissingConnectEventInputMissingVariableDatum = "Event Handler node was missing data for its input slot for the event for its connect slot";
        constexpr const char* FailedToDeduceExpression = "Failed to deduce an expression on a node reported to be an expression";
        constexpr const char* FailedToParseIfBranch = "Failed parse a node that declared itself an if branch";
        constexpr const char* FailedToRemoveChild = "Failed to remove child expected in parent children list";
        constexpr const char* FunctionNodeFailedToReturnInterface = "FunctionCallNode failed to return latest SubgraphInterface";
        constexpr const char* FunctionNodeFailedToReturnUseableName = "FunctionCallNode failed to return a useable name";
        constexpr const char* FunctionDefinitionCannotStart = "Function definition graph has on graph start that can't get called";
        constexpr const char* FunctionDefinitionNodeDidNotReturnSlot = "Function definition node didn't return 1 and only execution in slot";
        constexpr const char* GetOrSetVariableOutputNotSupplied = "the ACM output data for Get/Set should already have been supplied, but is missing";
        constexpr const char* InactiveGraph = "This graph defines no functions, it is never activated, and will never execute. Add a Start node or connect an event handler or define functions.";
        constexpr const char* InfiniteLoopWritingToVariable = "infinite loop when writing to variable";
        constexpr const char* InfiniteSelfActivationLoop = "infinite loop when activating the entity that owns this graph";
        constexpr const char* InvalidDataTypeInInput = "invalid data type in input slot";
        constexpr const char* MetaDataNeedsTupleTypeIdForEventCall = "Meta data needs an AzTypeId for Tuple if an EBus call returns multiple types to ScriptCanvas";
        constexpr const char* MetaDataRequiredForEventCall = "Meta data is required for ACM node that is an EBus call";
        constexpr const char* MissingFalseExecutionSlotOnIf = "A 'False' Execution output slot is required in an If branch node";
        constexpr const char* MissingInfiniteLoopDetectionVariable = "Missing infinite loop detection variable";
        constexpr const char* MissingInputVariableInFunctionDefinition = "Missing input variable in function definition in Subgraph.";
        constexpr const char* MissingParentOfRemovedNode = "There should always be a parent, even in a function graph, the function definition would be the first node";
        constexpr const char* MissingTrueExecutionSlotOnIf = "A 'True' Execution output slot is required in an If branch node";
        constexpr const char* MissingVariable = "Missing variable";
        constexpr const char* MissingAddressSlotForEbusEvent = "missing slot for EBus handler address";
        constexpr const char* MissingVariableForEBusHandlerAddress = "missing variable for ebus handler address";
        constexpr const char* MissingVariableForEBusHandlerAddressConnected = "missing variable for ebus handler address";
        constexpr const char* MultipleExecutionOutConnections = "This node has multiple, unordered execution Out connections";
        constexpr const char* MultipleFunctionCallFromSingleSlotMultipleVariadic = "Only one variadic call (the last one) is supported in the multi-call per single slot.";
        constexpr const char* MultipleFunctionCallFromSingleSlotNoChildren = "Node missing from parent children.";
        constexpr const char* MultipleFunctionCallFromSingleSlotNotEnoughInput = "Not enough input to support multi call input information.";
        constexpr const char* MultipleFunctionCallFromSingleSlotNotEnoughInputForThis = "Node doesn't have enough input for a parsed this pointer.";
        constexpr const char* MultipleFunctionCallFromSingleSlotReused = "Multiple function slot reused an input slot";
        constexpr const char* MultipleFunctionCallFromSingleSlotUnused = "Multiple function slot left an input slot unused.";
        constexpr const char* MultipleSimulaneousInputValues = "Multiple values routed to the same single input with no way to discern which to take.";
        constexpr const char* MultipleStartNodes = "Multiple Start nodes in a single graph. Only one is allowed.";
        constexpr const char* NoChildrenAfterRoot = "No children after parsing function root";
        constexpr const char* NoChildrenInExtraction = "No children found in property extraction node";
        constexpr const char* NoDataPresent = "Could not construct from graph, no graph data was present";
        constexpr const char* NodeableNodeOverloadAmbiguous = "NodeableNodeOverloaded doesn't have enough data connected to select a valid overload";
        constexpr const char* NodeableNodeDidNotConstructInternalNodeable = "NodeableNode did not construct its internal Nodeable";
        constexpr const char* NoInputToForEach = "No Input To For Each Loop";
        constexpr const char* NoOutForExecution = "No out slot for execution root";
        constexpr const char* NoOutSlotInFunctionDefinitionStart = "No 'Out' slot in start of function definition";
        constexpr const char* NoOutSlotInStart = "No 'Out' slot in start node";
        constexpr const char* NoOutputInExtraction = "No output found in property extraction node";
        constexpr const char* NotEnoughArgsForArithmeticOperator = "Less than two valid arguments for arithmetic operator";
        constexpr const char* NotEnoughBranchesForReturn = "Not enough branches for defined out return values.";
        constexpr const char* NotEnoughInputForArithmeticOperator = "Not enough input for arithmetic operator";
        constexpr const char* NullEntityInGraph = "Null entity pointer in graph";
        constexpr const char* NullInputKnown = "The input is known to be null, and the node does not accept it";
        constexpr const char* ParseExecutionMultipleOutSyntaxSugarChildExecutionRemovedAndNotReplaced = "ParseExecutionMultipleOutSyntaxSugar: child execution node was removed, and not replaced.";
        constexpr const char* ParseExecutionMultipleOutSyntaxSugarMismatchOutSize = "ParseExecutionMultipleOutSyntaxSugar: mismatch in connect nodes vs source slots size";
        constexpr const char* ParseExecutionMultipleOutSyntaxSugarNonNullChildExecutionFound = "ParseExecutionMultipleOutSyntaxSugar: non null child execution node";
        constexpr const char* ParseExecutionMultipleOutSyntaxSugarNullChildFound = "ParseExecutionMultipleOutSyntaxSugar: null Grammar::ExecutionChild found";
        constexpr const char* RequiredOutputRemoved = "Required output removed from execution tree.";
        constexpr const char* SequentialExecutionMappingFailure = "Sequential execution slot mapping failure";
        constexpr const char* SourceUpdateRequired = "Source Graph requires a dedicated upgrade. Please run the Script Canvas Editor, and follow the updgrade instructions.";
        constexpr const char* StartNodeFailedToParse = "Start node execution failed to parse";
        constexpr const char* SubgraphOnGraphStartFailedToReturnLexicalScope = "Subgraph On Graph Start call failed to return LexicalScope.";
        constexpr const char* SubgraphReturnValues = "Subgraph with Latent Outs do not support variables with Scope In or Scope InOut";
        constexpr const char* TooManyBranchesForReturn = "Too many branches for standard return values.";
        constexpr const char* UnexpectedSlotTypeFromNodeling = "unexpected slot type from nodeling";
        constexpr const char* UntranslatedArithmetic = "Arithmetic operator added and not yet parsed";
        constexpr const char* UnusableVariableInFunctionDefinition = "Unusable variable in function definition.";
        constexpr const char* UserOutCallInLoop = "A subgraph Execution Out call was found in a loop. Execution Out must always be the last thing to happen in a Subgraph execution path";
        constexpr const char* UserOutCallMidSequence = "A subgraph Execution Out call was in the middle of a sequence. Execution Out must always be the last thing to happen in a Subgraph execution path";
        constexpr const char* VariableHandlingMissing = "Variable handling missing";

    }
}
