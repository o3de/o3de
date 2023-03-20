/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/limits.h>
#include <AzCore/std/containers/set.h>
#include <AzCore/Outcome/Outcome.h>

#include <ScriptCanvas/Asset/RuntimeInputs.h>
#include <ScriptCanvas/Grammar/PrimitivesDeclarations.h>
#include <ScriptCanvas/Variable/VariableCore.h>
#include <ScriptCanvas/Core/ScriptCanvasBus.h>

#include "GraphToX.h"
#include "TranslationContext.h"
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

            static IsNamed IsInputNamed(Grammar::VariableConstPtr input, Grammar::ExecutionTreeConstPtr execution);
            static AZStd::string SanitizeFunctionCallName(AZStd::string_view name);

            RuntimeInputs m_runtimeInputs;
            BuildConfiguration m_buildConfiguration = BuildConfiguration::Release;
            FunctionBlockConfig m_functionBlockConfig = FunctionBlockConfig::Ignored;
            Context m_context;
            AZStd::string m_tableName;
            Writer m_dotLua;
            SystemComponentConfiguration m_systemConfiguration;
                        
            GraphToLua(const Grammar::AbstractCodeModel& source);

            const AZStd::string& FindAbbreviation(AZStd::string_view dependency) const;
            const AZStd::string& FindLibrary(AZStd::string_view dependency) const;
            AZStd::string_view GetOperatorString(Grammar::ExecutionTreeConstPtr execution);
            bool IsConstructCallRequired() const;
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
            void TranslateBody();
            void TranslateBody(BuildConfiguration configuration);
            void TranslateClassClose();
            void TranslateClassOpen();
            void TranslateConstructMethod();
            void TranslateConstruction();
            void TranslateDependencies();
            void TranslateDestruction();
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
            enum class OutSourceType
            {
                Nodeable,
                InterpretedClass,
            };
            void TranslateExecutionOut(Grammar::VariableConstPtr host, Grammar::ExecutionTreeConstPtr execution, OutSourceType sourceType);
            void TranslateExecutionOuts(Grammar::VariableConstPtr host, Grammar::ExecutionTreeConstPtr execution);
            void TranslateNodeableParse();
            void TranslateStaticInitialization();
            void TranslateVariableInitialization(AZStd::string_view leftValue);
            void WriteClassPropertyRead(Grammar::ExecutionTreeConstPtr);
            void WriteClassPropertyWrite(Grammar::ExecutionTreeConstPtr);
            void WriteConditionalCaseSwitch(Grammar::ExecutionTreeConstPtr execution, Grammar::Symbol symbol, const Grammar::ExecutionChild& child, size_t index);
            enum class IsLeadingCommaRequired { No, Yes };
            void WriteConstructionArgs();
            void WriteConstructionDependencyArgs();
            // currently used for both parameters and arguments, since for Lua, they are the same
            void WriteConstructionInput();
            void WriteConstructMethod();
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
            void WriteGlobalPropertyRead(Grammar::ExecutionTreeConstPtr);
            void WriteHeader();
            void WriteInfiniteLoopCheckPost(Grammar::ExecutionTreeConstPtr execution);
            void WriteInfiniteLoopCheckPre(Grammar::ExecutionTreeConstPtr execution);
            // #scriptcanvas_component_extension
            void WriteInitializeLocalSelfEntityId();
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
            void WriteResolvedScope(Grammar::ExecutionTreeConstPtr execution, const Grammar::LexicalScope& lexicalScope);
            void WriteReturnStatement(Grammar::ExecutionTreeConstPtr execution);
            void WriteReturnValueInitialization(Grammar::ExecutionTreeConstPtr execution);
            void WriteStaticInitializerInput(IsLeadingCommaRequired commaRequired);
            void WriteSwitchEnd(Grammar::Symbol symbol);
            void WriteVariableRead(Grammar::VariableConstPtr variable);
            void WriteVariableReadConvertible(const Grammar::ConversionByIndex& conversions, size_t index, Grammar::VariableConstPtr source);
            void WriteVariableReference(Grammar::VariableConstPtr variable);
            void WriteVariableWrite(Grammar::ExecutionTreeConstPtr execution, const AZStd::vector<AZStd::pair<const Slot*, Grammar::OutputAssignmentConstPtr>>& output);
            void WriteWrittenMathExpression(Grammar::ExecutionTreeConstPtr execution);

        };
               
    } 

} 
