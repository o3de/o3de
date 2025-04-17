/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Outcome/Outcome.h>
#include <AzCore/std/chrono/chrono.h>
#include <AzCore/std/containers/unordered_map.h>
#include <ScriptCanvas/Core/Core.h>
#include <ScriptCanvas/Core/Datum.h>
#include <ScriptCanvas/Core/SubgraphInterface.h>
#include <ScriptCanvas/Core/Node.h>
#include <ScriptCanvas/Data/Data.h>
#include <ScriptCanvas/Debugger/ValidationEvents/ValidationEvent.h>
#include <ScriptCanvas/Variable/VariableCore.h>

#include "DebugMap.h"
#include "PrimitivesDeclarations.h"
#include "PrimitivesExecution.h"

namespace ScriptCanvas
{
    class Graph;
    class Node;
    class Nodeable;
    class VariableData;

    struct GraphData;

    namespace Nodes
    {
        namespace Core
        {
            class Start;
            class FunctionCallNode;
            class FunctionDefinitionNode;
        }
    }

    namespace Grammar
    {
        class Context;

        /// This class parses a graph into abstract programming concepts for easier translation
        /// into C++, Lua, or whatever else would be needed
        class AbstractCodeModel
            : public AZStd::enable_shared_from_this<AbstractCodeModel>
        {
            friend struct PrintMetaData;

        public:
            static bool RequiresCreationFunction(Data::eType type);

            static AbstractCodeModelConstPtr Parse(const Source& source, bool terminateOnError = false, bool terminateOnInternalError = false);

            AbstractCodeModel() = delete;

            AbstractCodeModel(const AbstractCodeModel&) = delete;

            AbstractCodeModel(const Source& source, bool terminateOnError = false, bool terminateOnInternalError = false);

            ~AbstractCodeModel();

            void AddError(ExecutionTreeConstPtr execution, ValidationConstPtr&& error) const;

            AZStd::string AddTranslationVariableName(const AZStd::string& name) const;

            AZStd::optional<AZStd::pair<size_t, Grammar::DependencyInfo>> CheckUserNodeableDependencyConstructionIndex(VariableConstPtr nodeable) const;

            AZStd::vector<VariableConstPtr> CombineVariableLists
            (const AZStd::vector<Nodeable*>& constructionNodeables
                , const AZStd::vector<AZStd::pair<VariableId, Datum>>& constructionInputVariableIds
                , const AZStd::vector<AZStd::pair<VariableId, Data::EntityIDType>>& entityIds) const;

            AZStd::optional<AZStd::string> FindNodeableSimpleName(VariableConstPtr variable) const;

            const AZStd::pair<Grammar::VariableConstPtr, AZStd::string>* FindStaticVariable(VariableConstPtr) const;

            AZStd::vector<ExecutionTreeConstPtr> GetAllExecutionRoots() const;

            const size_t* GetDebugInfoInIndex(ExecutionTreeConstPtr execution) const;

            const size_t* GetDebugInfoOutIndex(ExecutionTreeConstPtr execution, size_t index) const;

            const size_t* GetDebugInfoReturnIndex(ExecutionTreeConstPtr execution) const;

            const size_t* GetDebugInfoVariableAssignmentIndex(OutputAssignmentConstPtr output, size_t assignmentIndex) const;

            const size_t* GetDebugInfoVariableSetIndex(OutputAssignmentConstPtr output) const;

            const DebugSymbolMap& GetDebugMap() const;

            const OrderedDependencies& GetOrderedDependencies() const;

            EBusHandlingConstPtr GetEBusEventHandling(const Node*) const;

            AZStd::vector<EBusHandlingConstPtr> GetEBusHandlings() const;

            EventHandlingConstPtr GetEventHandling(const Node*) const;

            AZStd::vector<EventHandlingConstPtr> GetEventHandlings() const;

            Grammar::ExecutionCharacteristics GetExecutionCharacteristics() const;

            AZStd::vector<ExecutionTreeConstPtr> GetFunctions() const;

            VariableConstPtr GetImplicitVariable(ExecutionTreeConstPtr execution) const;

            const SubgraphInterface& GetInterface() const;

            const AZStd::unordered_set<VariableConstPtr>* GetLocalVariables(ExecutionTreeConstPtr execution) const;

            AZStd::vector<NodeableParseConstPtr> GetNodeableParse() const;

            AZStd::sys_time_t GetParseDuration() const;

            const ParsedRuntimeInputs& GetRuntimeInputs() const;

            const Source& GetSource() const;

            const AZStd::string& GetSourceString() const;

            ExecutionTreeConstPtr GetStart() const;

            const AZStd::vector<AZStd::pair<Grammar::VariableConstPtr, AZStd::string>>& GetStaticVariablesNames() const;

            const AZStd::vector<AZStd::pair<Grammar::VariableConstPtr, AZStd::string>>& GetStaticVariablesNames(Grammar::ExecutionTreeConstPtr functionBlock) const;

            const ValidationResults::ValidationEventList& GetValidationEvents() const { return m_validationEvents; }

            VariableWriteHandlingConstPtr GetVariableHandling(const Slot* slot) const;

            VariableWriteHandlingConstSet GetVariableHandling(VariableConstPtr variable) const;

            const AZStd::vector<VariableConstPtr>& GetVariables() const;

            const AZStd::vector<VariableConstPtr>& GetVariablesUnused() const;

            bool IsErrorFree() const;

            // has state or handlers
            bool IsClass() const;

            // if only functions, constant data, but no with state operations on state or handlers, etc
            bool IsPureLibrary() const;

            // if any operations on state or handlers, etc
            bool IsUserNodeable() const;

            bool IsUserNodeable(VariableConstPtr variable) const;

            template<typename T>
            AZStd::vector<Grammar::VariableConstPtr> ToVariableList(const AZStd::vector<AZStd::pair<VariableId, T>>& source) const;

        private:
            //////////////////////////////////////////////////////////////////////////
            // Internal parsing
            //////////////////////////////////////////////////////////////////////////
            struct ReturnValueConnections
            {
                bool m_hasOtherConnections = false;
                AZStd::vector<VariableConstPtr> m_returnValuesOrReferences;
            };

            void AddAllVariablesPreParse();

            void AddDebugInformation();

            void AddDebugInformation(ExecutionChild& execution);

            void AddDebugInformationFunctionDefinition(ExecutionTreePtr root);

            void AddDebugInformationIn(ExecutionTreePtr execution);

            void AddDebugInformationOut(ExecutionTreePtr execution);

            void AddDebugInfiniteLoopDetectionInLoop(ExecutionTreePtr execution);

            void AddDebugInfiniteLoopDetectionInHandler(ExecutionTreePtr execution);

            void AddError(const AZ::EntityId& nodeId, ExecutionTreeConstPtr execution, const AZStd::string_view error) const;

            VariablePtr AddMemberVariable(const Datum& datum, AZStd::string_view rawName);

            VariablePtr AddMemberVariable(const Datum& datum, AZStd::string_view rawName, const AZ::EntityId& sourceNodeId);

            VariablePtr AddMemberVariable(const Datum& datum, AZStd::string_view rawName, const VariableId& sourceVariableId);

            void AddValidation(ValidationConstPtr&& validation) const;

            VariablePtr AddVariable(const Datum& datum, AZStd::string_view rawName);

            VariablePtr AddVariable(const Datum& datum, AZStd::string_view rawName, const AZ::EntityId& sourceNodeId);

            VariablePtr AddVariable(const Datum& datum, AZStd::string_view rawName, const VariableId& sourceVariableId);

            VariablePtr AddVariable(const Data::Type& type, AZStd::string_view rawName);

            void AddVariable(VariablePtr variable);

            void CheckConversion(ConversionByIndex& conversion, VariableConstPtr source, size_t index, const Data::Type& targetType);

            void CheckConversions(OutputAssignmentPtr output);

            bool CheckCreateRoot(const Node& node);

            void CheckForKnownNullDereference(ExecutionTreeConstPtr parent, const ExecutionInput& input, const Slot& inputSlot);

            AZStd::string CheckUniqueInterfaceNames
                ( AZStd::string_view candidate
                , AZStd::string_view defaultName
                , AZStd::unordered_set<AZStd::string>& uniqueNames
                , const AZStd::unordered_set<const ScriptCanvas::Nodes::Core::FunctionDefinitionNode*>& nodelingsOut);

            AZStd::string CheckUniqueOutNames(AZStd::string_view outName, const AZStd::unordered_set<const ScriptCanvas::Nodes::Core::FunctionDefinitionNode*>& nodelingsOut);

            ExecutionTreePtr CreateChild(ExecutionTreePtr parent, const Node* node, const Slot* outSlot) const;

            ExecutionTreePtr CreateChildDebugMarker(ExecutionTreePtr parent) const;

            ExecutionTreePtr CreateChildPlaceHolder(ExecutionTreePtr parent) const;

            bool CreateEBusHandling(const Node& node);

            bool CreateEventHandling(const Node& node);

            bool CheckCreateNodeableParse(const Node& node);

            OutputAssignmentPtr CreateOutput(ExecutionTreePtr execution, const Slot& outputSlot, AZStd::string_view slotNameOverride, AZStd::string_view suffix);

            OutputAssignmentPtr CreateOutputAssignment(VariableConstPtr variable);

            bool CheckCreateUserEventHandling(const Node& node);

            bool CheckCreateUserFunctionDefinition(const Node& node);

            void ConvertAllMemberVariablesToLocal(ExecutionTreePtr newSource);

            void CreateUserFunctionDefinition(const Node& node, const Slot& entrySlot);

            bool CreateVariableWriteHandling(const Node& node);

            void CreateVariableWriteHandling(const Slot& slot, VariableConstPtr variable, bool startsConnected);

            OutputAssignmentConstPtr CreateOutputData(ExecutionTreePtr execution, ExecutionChild& executionChild, const Slot& output);

            void CullUnusedVariables();

            // Execution cycle detection is done first, before parsing starts.
            // This way, infinite loops in the parser execution itself are prevented, and input
            // used from previous ACM execution nodes are properly handled.
            // Cycle detection done by performing a purely execution-centric traversal of the source graph,
            // which properly respects both the Ordered Sequencer node, and multiple Execution Outs connections
            // from a single slot.
            // Errors from poorly routed input are NOT detected here, and should not be, those are detected later.
            bool ExecutionContainsCyclesCheck(const Node& node, const Slot& outSlot);

            ReturnValueConnections FindAssignments(ExecutionTreeConstPtr execution, const Slot& output);

            VariableConstPtr FindVariable(const AZ::EntityId& sourceNodeId) const;

            VariableConstPtr FindVariable(const VariableId& sourceVariableId) const;

            VariableConstPtr FindReferencedVariableChecked(ExecutionTreeConstPtr execution, const Slot& slot) const;

            AZStd::pair<ExecutionTreeConstPtr, VariableConstPtr> FindReturnValueOnThread(ExecutionTreeConstPtr executionNode, const Node* node, const Slot* slot) const;

            AZStd::string GetOriginalVariableName(VariableConstPtr variable, const Node* node);

            AZStd::string GetOutputSlotNameOverride(ExecutionTreePtr execution, const Slot& outputSlot);

            VariableConstPtr GetReadVariable(ExecutionTreePtr execution);

            VariableConstPtr GetWrittenVariable(ExecutionTreePtr execution);

            bool IsActiveGraph() const;

            bool IsAutoConnectedLocalEBusHandler(const Node* node) const;

            AZStd::vector<ExecutionTreePtr> ModAllExecutionRoots();

            AZStd::vector<AZStd::pair<Grammar::VariableConstPtr, AZStd::string>>& ModStaticVariablesNames();

            AZStd::vector<AZStd::pair<Grammar::VariableConstPtr, AZStd::string>>& ModStaticVariablesNames(Grammar::ExecutionTreeConstPtr functionBlock);

            ExecutionTreePtr OpenScope(ExecutionTreePtr parent, const Node* node, const Slot* outSlot) const;

            void Parse();

            bool Parse(const Node& node);

            void Parse(const AZStd::vector<const Nodes::Core::Start*>& startNodes);

            bool Parse(VariableWriteHandlingPtr variableHandling);

            void ParseAutoConnectedEBusHandlerVariables();

            void ParseComponentExtension();

            enum class FirstNode
            {
                Self,
                Parent
            };
            VariableConstPtr ParseConnectedInputData(const Slot& inputSlot, ExecutionTreePtr executionWithInput, const EndpointsResolved& scriptCanvasNodesConnectedToInput, FirstNode firstNode);

            void ParseConstructionInputVariables();

            ConstSlotsOutcome ParseDataOutSlots(ExecutionTreePtr execution, ExecutionChild& executionChild) const;

            void ParseDeactivation();

            void ParseDebugInformation(ExecutionTreePtr execution);

            void ParseDependencies(const Node& node);

            void ParseDependenciesAssetIndicies();

            // #scriptcanvas_component_extension
            void ParseEntityIdInput(ExecutionTreePtr execution);

            void ParseExecutionBreak(ExecutionTreePtr execution);

            void ParseExecutionCharacteristics();

            void ParseExecutionCharacteristics(ExecutionTreePtr execution);

            void ParseExecutionCycleStatement(ExecutionTreePtr execution);

            ExecutionTreePtr ParseExecutionForEachLoop(ExecutionTreePtr execution, const Slot& loopSlot, const Slot& breakSlot);

            void ParseExecutionForEachStatement(ExecutionTreePtr execution);

            void ParseExecutionFunction(ExecutionTreePtr execution, const Slot& outSlot);

            void ParseExecutionFunctionRecurse(ExecutionTreePtr execution, ExecutionChild& child, const Slot& outSlot, const AZStd::pair<const Node*, const Slot*>& nodeAndSlot);

            void ParseExecutionIfStatement(ExecutionTreePtr execution);

            void ParseExecutionLogicalExpression(ExecutionTreePtr execution, Symbol symbol);

            void ParseExecutionLoop(ExecutionTreePtr execution);

            void ParseExecutionMultipleOutSyntaxSugar(ExecutionTreePtr execution, const EndpointsResolved& executionOutNodes, const AZStd::vector<const Slot*>& outSlots);

            bool HasUnparsedImplicitConnections(const Slot* outSlot, const Slot* inSlot);

            void ParseExecutionMultipleOutSyntaxSugarOfSequencNode(ExecutionTreePtr sequence);

            void ParseExecutionOnce(ExecutionTreePtr execution);

            void ParseExecutionRandomSwitchStatement(ExecutionTreePtr execution);

            void ParseExecutionSequentialChildren(ExecutionTreePtr execution);

            void ParseExecutionSwitchStatement(ExecutionTreePtr execution);

            void ParseExecutionTreeBody(ExecutionTreePtr execution, const Slot& outSlot);

            ExecutionTreePtr ParseExecutionTreeRoot(ExecutionTreePtr root);

            enum MarkLatent
            {
                No,
                Yes
            };
            ExecutionTreePtr ParseExecutionTreeRoot(const Node& node, const Slot& outSlot, MarkLatent markLatent);

            void ParseExecutionTreeRoots(const Node& node);

            void ParseExecutionWhileLoop(ExecutionTreePtr execution);

            void ParseFunctionLocalStaticUseage();

            void ParseImplicitVariables(const Node& node);

            void ParseInputData(ExecutionTreePtr execution);

            void ParseInputDatum(ExecutionTreePtr execution, const Slot& input);

            bool ParseInputThisPointer(ExecutionTreePtr execution);

            void ParseLocallyDefinedFunctionCalls();

            void ParseMetaData(ExecutionTreePtr execution);

            void ParseMultiExecutionPost(ExecutionTreePtr execution);

            void ParseMultiExecutionPre(ExecutionTreePtr execution);

            void ParseMultipleFunctionCallPost(ExecutionTreePtr execution);

            void ParseNodelingVariables(const Node& node, NodelingType nodelingType);

            void ParseOperatorArithmetic(ExecutionTreePtr execution);

            void ParseOutputData(ExecutionTreePtr execution, ExecutionChild& executionChild);

            void ParseOutputData(ExecutionTreePtr execution, ExecutionChild& executionChild, AZStd::vector<const Slot*>& slots);

            void ParseOutputData(ExecutionTreePtr execution, ExecutionChild& executionChild, const Slot& output);

            void ParsePropertyExtractionsPost(ExecutionTreePtr execution);

            void ParsePropertyExtractionsPre(ExecutionTreePtr execution);

            void ParseReturnValue(ExecutionTreePtr execution, const Slot& returnValueSlot);

            void ParseReturnValue(ExecutionTreePtr execution, VariableConstPtr variable, const Slot* returnValueSlot);

            void ParseSubgraphInterface(const Node& node);

            void ParseUserFunctionTopology();

            void ParseUserIn(ExecutionTreePtr root, const Nodes::Core::FunctionDefinitionNode* nodeling);

            void ParseUserInData(ExecutionTreePtr execution, ExecutionChild& executionChild);

            void ParseUserLatent(ExecutionTreePtr leaf, const Nodes::Core::FunctionDefinitionNode* nodeling);

            void ParseUserLatentData(ExecutionTreePtr execution);

            void ParseUserOutCall(ExecutionTreePtr execution);

            void ParseUserOuts();

            void ParseVariableHandling();

            // tracks all local/member variable use, return true if uses a declared member variable
            bool ParseVariableUseAndPurity(ExecutionTreePtr execution);

            void PostParseErrorDetect(ExecutionTreePtr root);

            void PostParseProcess(ExecutionTreePtr root);

            void PruneNoOpChildren(const ExecutionTreePtr& execution);

            AZ::Outcome<AZStd::pair<size_t, ExecutionChild>> RemoveChild(const ExecutionTreePtr& execution, const ExecutionTreeConstPtr& child);

            void RemoveFromTree(ExecutionTreePtr execution);

            struct ConnectionInPreviouslyExecutedScope
            {
                size_t m_childIndex = 0;
                size_t m_outputIndex = 0;
                ExecutionTreeConstPtr m_source;
            };

            struct ConnectionsInPreviouslyExecutedScope
            {
                AZStd::vector<ConnectionInPreviouslyExecutedScope> m_connections;
                ExecutionTreePtr m_mostParent;
            };

            struct ReturnValueDescription
            {
                AZStd::vector<VariablePtr> returnValues;
                size_t outCallCount = 1;
            };

            struct UserInParseTopologyResult
            {
                bool addSingleOutToMap;
                bool addNewOutToLeavesWithout;
                bool addExplicitOutCalls;
                bool isSimpleFunction;
            };

            void AccountForEBusConnectionControl(ExecutionTreePtr execution);

            template<typename t_Handling>
            void AccountForEBusConnectionControl(ExecutionTreePtr execution, t_Handling handling)
            {
                // check for connection control method, which the slot must be at this point
                // update connection status in handling based on slot
                // later...make methods to track connected status in the thread to prevent infinite loops
                // (works for variables, may not work for ebus)
                const auto id = execution->GetId();
                handling->m_isEverConnected = handling->m_isEverConnected || id.m_slot == id.m_node->GetEBusConnectSlot();
                handling->m_isEverDisconnected = handling->m_isEverDisconnected || id.m_slot == id.m_node->GetEBusDisconnectSlot();
            }

            void AddExecutionMapIn
                ( UserInParseTopologyResult result
                , ExecutionTreeConstPtr root
                , const AZStd::vector<ExecutionTreeConstPtr>& outCalls
                , AZStd::string_view defaultOutName
                , const Nodes::Core::FunctionDefinitionNode* nodelingIn
                , const AZStd::unordered_set<const ScriptCanvas::Nodes::Core::FunctionDefinitionNode*>& uniqueNodelingsOut);

            void AddExecutionMapLatentOut(const Nodes::Core::FunctionDefinitionNode& nodeling, ExecutionTreePtr out);

            void AddUserOutToLeaf(ExecutionTreePtr parent, ExecutionTreeConstPtr root, AZStd::string_view name);

            void AddPreviouslyExecutedScopeVariableToOutputAssignments(VariableConstPtr newInputVariable, const ConnectionsInPreviouslyExecutedScope& connectedInputInPreviouslyExecutedScope);

            void ConvertNamesToIdentifiers();

            VariableConstPtr FindBoundVariable(GraphScopedVariableId variableId) const;

            ConnectionsInPreviouslyExecutedScope FindConnectedInputInPreviouslyExecutedScope(ExecutionTreePtr executionWithInput, const EndpointsResolved& scriptCanvasNodesConnectedToInput, FirstNode firstNode) const;

            bool FindConnectedInputInPreviouslyExecutedScopeRecurse(ConnectionsInPreviouslyExecutedScope& result, ExecutionTreeConstPtr outputSource, ExecutionTreePtr executionWithInput, const EndpointsResolved& scriptCanvasNodesConnectedToInput) const;

            VariableConstPtr FindConnectedInputInScope(ExecutionTreePtr executionWithInput, const EndpointsResolved& scriptCanvasNodesConnectedToInput, FirstNode firstNode) const;

            AZStd::vector<AZStd::pair<VariableConstPtr, AZStd::string>> GetAllDeactivationVariables() const;

            bool InSimultaneousDataPath(const Node& node, const Slot& reference, const Slot& candidate) const;

            static UserInParseTopologyResult ParseUserInTolopology(size_t nodelingsOutCount, size_t leavesWithoutNodelingsCount);

            size_t m_outIndexCount = 0;
            size_t m_generatedIdCount = 0;
            ExecutionTreePtr m_start;
            AZStd::vector<const Nodes::Core::Start*> m_startNodes;
            ScopePtr m_graphScope;
            const Source m_source;
            OrderedDependencies m_orderedDependencies;
            AZStd::unordered_set<VariableConstPtr> m_userNodeables;

            AZStd::unordered_map<VariableConstPtr, DependencyInfo> m_dependencyByVariable;

            AZStd::vector<VariableConstPtr> m_variables;
            AZStd::vector<VariableConstPtr> m_variablesUnused;
            AZStd::vector<const Node*> m_possibleExecutionRoots;

            // true iff there are no internal errors and no error validation events
            bool m_isErrorFree = true;

            // for post parsing validation
            ValidationResults::ValidationEventList m_validationEvents;

            DebugSymbolMap m_debugMap;

            DebugSymbolMapReverse m_debugMapReverse;

            AZStd::sys_time_t m_parseDuration;
            AZStd::chrono::steady_clock::time_point m_parseStartTime;
            EBusHandlingByNode m_ebusHandlingByNode;
            EventHandlingByNode m_eventHandlingByNode;
            ImplicitVariablesByNode m_implicitVariablesByNode;
            ControlVariablesBySourceNode m_controlVariablesBySourceNode;
            NodeableParseByNode m_nodeablesByNode;
            // owns the handling
            VariableHandlingBySlot m_variableWriteHandlingBySlot;
            // references the handling only
            VariableWriteHandlingByVariable m_variableWriteHandlingByVariable;
            // owns nothing
            AZStd::vector<ExecutionTreeConstPtr> m_functions;
            // owns the execution in nodelings
            AZStd::unordered_map<const Nodes::Core::FunctionDefinitionNode*, ExecutionTreePtr> m_userInsThatRequireTopology;
            AZStd::unordered_map<const Nodes::Core::FunctionDefinitionNode*, ExecutionTreePtr> m_userOutsThatRequireTopology;
            AZStd::unordered_multimap<const Nodes::Core::FunctionDefinitionNode*, ExecutionTreePtr> m_outsMarkedLatent;
            AZStd::unordered_set<const Nodes::Core::FunctionDefinitionNode*> m_outsMarkedImmediate;
            AZStd::unordered_set<const Nodes::Core::FunctionDefinitionNode*> m_processedOuts;

            // the output slots of the In-Nodeling
            AZStd::unordered_map<const Slot*, VariablePtr> m_inputVariableByNodelingInSlot;
            // the output slots of the Out-Nodeling
            AZStd::unordered_map<const Slot*, VariablePtr> m_returnVariableByNodelingOutSlot;
            // the input slots of the Out-Nodeling
            AZStd::unordered_map<const Slot*, VariablePtr> m_outputVariableByNodelingOutSlot;

            AZStd::unordered_map<const Nodes::Core::FunctionDefinitionNode*, ReturnValueDescription> m_returnValuesByUserFunctionDefinition;

            AZStd::unordered_map<const Datum*, const GraphVariable*> m_sourceVariableByDatum;
            AZStd::unordered_set<const Node*> m_subgraphStartCalls;
            AZStd::unordered_set<const Node*> m_activeDefaultObject;

            AZStd::vector<const Nodes::Core::FunctionCallNode*> m_locallyDefinedFunctionCallNodes;

            SubgraphInterface m_subgraphInterface;

            AZStd::unordered_set<AZStd::string> m_uniqueOutNames;
            AZStd::unordered_set<AZStd::string> m_uniqueInNames;
            AZStd::vector<AZStd::pair<Grammar::VariableConstPtr, AZStd::string>> m_staticVariableNames;
            AZStd::unordered_map<Grammar::ExecutionTreeConstPtr, AZStd::vector<AZStd::pair<Grammar::VariableConstPtr, AZStd::string>>> m_staticVariableNamesByFunctionBlock;

            AZStd::unordered_map<ExecutionTreeConstPtr, VariableUseage> m_variableUseByExecution;
            VariableUseage m_variableUse;

            ParsedRuntimeInputs m_runtimeInputs;

            AZStd::vector<AZStd::pair<const Slot*, const Slot*>> m_parsedImplicitConnections;

            AZStd::vector<VariablePtr> FindUserImmediateInput(ExecutionTreePtr call) const;

            const ReturnValueDescription* FindUserImmediateOutput(ExecutionTreePtr call) const;

            AZStd::vector<VariablePtr> FindUserLatentOutput(ExecutionTreePtr call) const;

            AZStd::vector<VariablePtr> FindUserLatentReturnValues(ExecutionTreePtr call) const;

            bool IsSourceInScope(VariableConstPtr variable, VariableFlags::Scope scope) const;

            void MarkParseStart();

            void MarkParseStop();

            void ParseBranchOnResultFunctionCheck(ExecutionTreePtr execution);

            void ParseCheckedFunctionCheck(ExecutionTreePtr execution);

            void ParseExecutionIfStatementBooleanExpression(ExecutionTreePtr booleanExpressionExecution, AZStd::string executionName, LexicalScope lexicalScope);

            void ParseExecutionIfStatementInternalFunction(ExecutionTreePtr internalFunctionExecution);
        }; // class AbstractCodeModel

        template<typename T>
        AZStd::vector<Grammar::VariableConstPtr> AbstractCodeModel::ToVariableList(const AZStd::vector<AZStd::pair<VariableId, T>>& source) const
        {
            AZStd::vector<Grammar::VariableConstPtr> variables;

            for (const auto& variable : source)
            {
                auto iter = AZStd::find_if
                (m_variables.begin()
                    , m_variables.end()
                    , [&](const auto& candidate) { return candidate->m_sourceVariableId == variable.first; });

                if (iter != m_variables.end())
                {
                    variables.push_back(*iter);
                }
            }

            return variables;
        }
    }
}
