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

#pragma once

#include <AzCore/std/limits.h>
#include <AzCore/std/containers/set.h>
#include <AzCore/Outcome/Outcome.h>

#include <ScriptCanvas/Grammar/PrimitivesDeclarations.h>
#include <ScriptCanvas/Variable/VariableCore.h>
#include <ScriptCanvas/Core/ScriptCanvasBus.h>

#include "GraphToX.h"
#include "TranslationResult.h"
#include "TranslationUtilities.h"

namespace ScriptCanvas
{
    class Graph;

    namespace Translation
    {
        class Context;
        class GraphToLua;

        struct CheckConversion
        {
            Writer& m_writer;
            Grammar::VariableConstPtr m_source;
            const Grammar::ConversionByIndex& m_conversions;
            const size_t m_index;

            CheckConversion(Writer& writer, Grammar::VariableConstPtr source, const Grammar::ConversionByIndex& conversions, size_t index);
            ~CheckConversion();
        };

        enum class FunctionBlockConfig
        {
            Ignored,
            Traced,
        };

        class GraphToLua
            : public GraphToX
        {
        public:
            static AZ::Outcome<TargetResult, ErrorList> Translate(const Grammar::AbstractCodeModel& source);
            
        protected:
            enum class IsNamed { No, Yes };

            // \todo part of the output of the compilation step will have to be the list of variables that must be stored on disk, cloned and loaded,
            // rather than having the regular build step generate this
            enum class VariableConstructionRequirement
            {
                InputEntityId,
                InputNodeable,
                InputVariable,
                None,
                Static,
            };

            static bool IsCodeConstructable(Grammar::VariableConstPtr value);
            static IsNamed IsInputNamed(Grammar::VariableConstPtr input, Grammar::ExecutionTreeConstPtr execution);
            
            RuntimeInputs m_runtimeInputs;
            BuildConfiguration m_executionConfig = BuildConfiguration::Release;
            FunctionBlockConfig m_functionBlockConfig = FunctionBlockConfig::Ignored;
            const Context* m_context = nullptr;
            AZStd::string m_tableName;
            Writer m_dotLua;
            SystemComponentConfiguration m_systemConfiguration;
                        
            GraphToLua(const Grammar::AbstractCodeModel& source);

            const AZStd::string& FindAbbreviation(AZStd::string_view dependency) const;
            const AZStd::string& FindLibrary(AZStd::string_view dependency) const;
            AZStd::string_view GetOperatorString(Grammar::ExecutionTreeConstPtr execution);
            bool IsDebugInfoWritten() const;

            enum class NilCheck
            {
                None,
                Single,
                Multiple,
            };
            AZStd::pair<NilCheck, AZStd::string> IsReturnValueNilCheckRequired(ScriptCanvas::Grammar::ExecutionTreeConstPtr execution);

            TargetResult MoveResult();
            void OpenFunctionBlock(Writer& writer);
            void ParseConstructionInputVariables();
            VariableConstructionRequirement ParseConstructionRequirement(Grammar::VariableConstPtr value);
            void TranslateBody();
            void TranslateBody(BuildConfiguration configuration);
            void TranslateClassClose();
            void TranslateClassOpen();
            void TranslateConstruction();
            void TranslateDependencies();
            void TranslateDestruction();
            // keep the leftValue thing, fix it, and put it in the configuration
            void TranslateEBusEvents(Grammar::EBusHandlingConstPtr ebusHandling, AZStd::string_view leftValue);
            void TranslateEBusHandlerCreation(Grammar::EBusHandlingConstPtr ebusHandling, AZStd::string_view leftValue);
            void TranslateEBusHandling(AZStd::string_view leftValue);
            void TranslateExecutionTreeChildPost(Grammar::ExecutionTreeConstPtr execution, const Grammar::ExecutionChild& child, size_t index, size_t rootIndex);
            void TranslateExecutionTreeChildPre(Grammar::ExecutionTreeConstPtr execution, const Grammar::ExecutionChild& child, size_t index, size_t rootIndex);
            void TranslateExecutionTreeEntry(Grammar::ExecutionTreeConstPtr execution, size_t index);
            void TranslateExecutionTreeEntryPost(Grammar::ExecutionTreeConstPtr execution, size_t index);
            void TranslateExecutionTreeEntryPre(Grammar::ExecutionTreeConstPtr execution, size_t index);
            void TranslateExecutionTreeEntryRecurse(Grammar::ExecutionTreeConstPtr execution, size_t index);
            void TranslateExecutionTreeFunctionCall(Grammar::ExecutionTreeConstPtr execution);
            void TranslateExecutionTreeUserOutCall(Grammar::ExecutionTreeConstPtr execution);
            void TranslateExecutionTrees();
            void TranslateFunction(Grammar::ExecutionTreeConstPtr execution, IsNamed lex);
            void TranslateFunctionBlock(Grammar::ExecutionTreeConstPtr execution, IsNamed lex);
            void TranslateFunctionBlock(Grammar::ExecutionTreeConstPtr execution, FunctionBlockConfig functionBlockConfig, IsNamed lex);
            void TranslateFunctionDefinition(Grammar::ExecutionTreeConstPtr execution, IsNamed lex);
            void TranslateInheritance();
            void TranslateNodeableOut(Grammar::ExecutionTreeConstPtr execution);
            void TranslateNodeableOuts(Grammar::ExecutionTreeConstPtr execution);
            void TranslateNodeableParse(AZStd::string_view leftValue);
            void TranslateNodeableParse(Grammar::ExecutionTreeConstPtr execution, AZStd::string_view leftValue);
            void TranslateStaticInitialization();
            void TranslateVariableInitialization(AZStd::string_view leftValue);
            void WriteConditionalCaseSwitch(Grammar::ExecutionTreeConstPtr execution, Grammar::Symbol symbol, const Grammar::ExecutionChild& child, size_t index);
            enum class IsLeadingCommaRequired { No, Yes };
            void WriteConstructionInput(IsLeadingCommaRequired commaRequired);
            void WriteCycleBegin(Grammar::ExecutionTreeConstPtr execution);
            void WriteDebugInfoIn(Grammar::ExecutionTreeConstPtr execution, AZStd::string_view);
            void WriteDebugInfoIn(Grammar::ExecutionTreeConstPtr execution, AZStd::string_view, size_t inputCountOverride);
            void WriteDebugInfoOut(Grammar::ExecutionTreeConstPtr execution, size_t index, AZStd::string_view);
            void WriteDebugInfoReturn(Grammar::ExecutionTreeConstPtr execution, AZStd::string_view);
            void WriteDebugInfoVariableChange(Grammar::ExecutionTreeConstPtr execution, Grammar::OutputAssignmentConstPtr output);
            void WriteEventConnectCall(Grammar::ExecutionTreeConstPtr execution);
            void WriteEventConnectCaller(Grammar::ExecutionTreeConstPtr execution, Grammar::EventHandlingConstPtr);
            enum class PostDisconnectAction{ None, SetToNil };
            void WriteEventDisconnectCall(Grammar::ExecutionTreeConstPtr execution, PostDisconnectAction postAction);
            void WriteForEachChildPost(Grammar::ExecutionTreeConstPtr execution, size_t index);
            void WriteForEachChildPre(Grammar::ExecutionTreeConstPtr execution);
            // Write, not translate, because this should be less dependent on the contents of the graph
            void WriteFloatingPointErrorNumberEqualityComparision(Grammar::ExecutionTreeConstPtr execution);
            void WriteFunctionCallInput(Grammar::ExecutionTreeConstPtr execution);
            void WriteFunctionCallInput(Grammar::ExecutionTreeConstPtr execution, size_t inputCountOverride);
            enum class IsFormatStringInput { No, Yes };
            void WriteFunctionCallInput(Grammar::ExecutionTreeConstPtr execution, size_t index, IsFormatStringInput convertToStrings);
            size_t WriteFunctionCallInputThisPointer(Grammar::ExecutionTreeConstPtr execution);
            void WriteFunctionCallNamespace(Grammar::ExecutionTreeConstPtr);
            void WriteFunctionCallNullCheckPost(Grammar::ExecutionTreeConstPtr execution);
            void WriteFunctionCallNullCheckPre(Grammar::ExecutionTreeConstPtr execution);
            void WriteFunctionCallOfNode(Grammar::ExecutionTreeConstPtr, AZStd::string nameOverride = "", size_t inputOverride = AZStd::numeric_limits<size_t>::max());
            void WriteHeader();
            void WriteInfiniteLoopCheckPost(Grammar::ExecutionTreeConstPtr execution);
            void WriteInfiniteLoopCheckPre(Grammar::ExecutionTreeConstPtr execution);
            void WriteLocalInputCreation(Grammar::ExecutionTreeConstPtr execution);
            void WriteLocalOutputInitialization(Grammar::ExecutionTreeConstPtr execution);
            void WriteLocalVariableInitializion(Grammar::ExecutionTreeConstPtr execution);
            void WriteLogicalExpression(Grammar::ExecutionTreeConstPtr execution);
            void WritePreFirstCaseSwitch(Grammar::ExecutionTreeConstPtr execution, Grammar::Symbol symbol);
            void WriteOnVariableWritten(Grammar::ExecutionTreeConstPtr execution, const AZStd::vector<AZStd::pair<const Slot*, Grammar::OutputAssignmentConstPtr>>& output);
            bool WriteOnVariableWritten(Grammar::VariableConstPtr variable);
            void WriteOperatorArithmetic(Grammar::ExecutionTreeConstPtr execution);
            void WriteOutputAssignments(Grammar::ExecutionTreeConstPtr execution);
            void WriteOutputAssignments(Grammar::ExecutionTreeConstPtr execution, const AZStd::vector<AZStd::pair<const Slot*, Grammar::OutputAssignmentConstPtr>>& output);
            void WriteReturnStatement(Grammar::ExecutionTreeConstPtr execution);
            void WriteReturnValueInitialization(Grammar::ExecutionTreeConstPtr execution);
            void WriteStaticInitializerInput(IsLeadingCommaRequired commaRequired);
            void WriteSwitchEnd(Grammar::Symbol symbol);
            void WriteVariableRead(Grammar::VariableConstPtr variable);
            void WriteVariableReadConvertible(const Grammar::ConversionByIndex& conversions, size_t index, Grammar::VariableConstPtr source);
            void WriteVariableReference(Grammar::VariableConstPtr variable);
            void WriteVariableWrite(Grammar::ExecutionTreeConstPtr execution, const AZStd::vector<AZStd::pair<const Slot*, Grammar::OutputAssignmentConstPtr>>& output);
            void WriteWrittenMathExpression(Grammar::ExecutionTreeConstPtr execution);

        private:
        };
               
    } 

} 
