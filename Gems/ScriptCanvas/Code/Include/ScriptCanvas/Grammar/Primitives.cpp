/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Primitives.h"

#include <ScriptCanvas/Core/EBusHandler.h>
#include <ScriptCanvas/Core/Graph.h>
#include <ScriptCanvas/Core/Node.h>
#include <ScriptCanvas/Core/Slot.h>
#include <ScriptCanvas/Execution/ExecutionState.h>
#include <ScriptCanvas/Translation/Configuration.h>
#include <ScriptCanvas/Variable/VariableData.h>

#include "ParsingUtilities.h"
#include "PrimitivesExecution.h"

namespace ScriptCanvas
{
    namespace Grammar
    {
        const char* GetSymbolName(Symbol nodeType)
        {
            return g_SymbolNames[(int)nodeType];
        }

        NamespacePath ToNamespacePath(AZStd::string_view path, AZStd::string_view name)
        {
            ScriptCanvas::NamespacePath namespaces;

            if (path.empty())
            {
                namespaces.push_back(name);
            }
            else
            {
                AZ::StringFunc::Tokenize(path, namespaces, "\\//");

                auto& moduleName = namespaces.back();
                if (!AZ::StringFunc::EndsWith(moduleName, ScriptCanvas::Grammar::k_internalRuntimeSuffix))
                {
                    namespaces.back() = AZStd::string::format("%s%s", namespaces.back().data(), ScriptCanvas::Grammar::k_internalRuntimeSuffix);
                }
            }

            return namespaces;
        }

        AZStd::string ToSafeName(const AZStd::string& name)
        {
            AZStd::vector<AZStd::string> tokens;
            AzFramework::StringFunc::Tokenize(name, tokens, Grammar::k_luaSpecialCharacters);
            AZStd::string joinResult;
            for (auto& token : tokens)
            {
                joinResult.append(token);
            }
            return joinResult;
        }

        AZStd::string ToTypeSafeEBusResultName(const Data::Type& type)
        {
            AZ_Assert(IsValueType(type), "This function is required for value types, and should never befused for reference types");
            return AZStd::string::format("%s%s", Grammar::k_TypeSafeEBusResultName, Data::GetName(type).data());
        }

        bool EBusBase::RequiresConnectionControl() const
        {
            return m_isEverDisconnected;
        }

        void EBusHandling::Clear()
        {
            for (auto& iter : m_events)
            {
                if (auto event = AZStd::const_pointer_cast<ExecutionTree>(iter.second))
                {
                    event->Clear();
                }
            }
        }

        void EventHandling::Clear()
        {
            m_eventNode = nullptr;
            m_eventSlot = nullptr;
            if (auto function = AZStd::const_pointer_cast<ExecutionTree>(m_eventHandlerFunction))
            {
                function->Clear();
            }
        }

        void FunctionPrototype::Clear()
        {
            m_inputs.clear();
            m_outputs.clear();
        }

        bool FunctionPrototype::IsVoid() const
        {
            return m_outputs.empty();
        }

        bool FunctionPrototype::operator==(const FunctionPrototype& other) const
        {
            const bool inputEqual = m_inputs.size() == other.m_inputs.size()
                && AZStd::equal(m_inputs.begin(), m_inputs.end(), other.m_inputs.begin(), [](auto lhsVar, auto rhsVar) { return lhsVar->m_datum.GetType() == rhsVar->m_datum.GetType(); });

            const bool outputCheckedAndEqual = inputEqual && m_outputs.size() == other.m_outputs.size()
                && AZStd::equal(m_outputs.begin(), m_outputs.end(), other.m_outputs.begin(), [](auto lhsVar, auto rhsVar) { return lhsVar->m_datum.GetType() == rhsVar->m_datum.GetType(); });

            return inputEqual && outputCheckedAndEqual;
        }

        void FunctionPrototype::Reflect(AZ::ReflectContext* reflectContext)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext))
            {
                serializeContext->Class<FunctionPrototype>()
                    ->Version(0)
                    ->Field("inputs", &FunctionPrototype::m_inputs)
                    ->Field("outputs", &FunctionPrototype::m_outputs)
                    ;
            }
        }

        LexicalScope::LexicalScope(LexicalScopeType type)
            : m_type(type)
        {}

        LexicalScope::LexicalScope(LexicalScopeType type, const AZStd::vector<AZStd::string>& namespaces)
            : m_type(type)
            , m_namespaces(namespaces)
        {}

        LexicalScope LexicalScope::Global()
        {
            return LexicalScope(LexicalScopeType::Namespace);
        }

        LexicalScope LexicalScope::Variable()
        {
            return LexicalScope(LexicalScopeType::Variable);
        }

        void NodeableParse::Clear()
        {
            m_nodeable = nullptr;

            for (auto& iter : m_latents)
            {
                if (auto latent = AZStd::const_pointer_cast<ExecutionTree>(iter.second))
                {
                    latent->Clear();
                }
            }

            m_latents.clear();
        }

        void OutputAssignment::Clear()
        {
            m_source = nullptr;
            m_assignments.clear();
            m_sourceConversions.clear();
        }

        ReturnValue::ReturnValue(OutputAssignment&& source)
            : OutputAssignment(AZStd::move(source))
        {}

        void ReturnValue::Clear()
        {
            OutputAssignment::Clear();
            m_initializationValue = nullptr;
        }

        AZStd::string Scope::AddFunctionName(AZStd::string_view name)
        {
            const auto baseName = Grammar::ToIdentifierSafe(name);
            const AZ::s32 nameCount = AddNameCount(baseName);
            return nameCount == 0 ? baseName : AZStd::string::format("%s_%d", baseName.data(), nameCount);
        }

        AZStd::string Scope::AddVariableName(AZStd::string_view name)
        {
            const auto baseName = Grammar::ToIdentifierSafe(name);
            const AZ::s32 nameCount = AddNameCount(baseName);
            return nameCount == 0 ? baseName : AZStd::string::format("%s_%d", baseName.data(), nameCount);
        }

        AZStd::string Scope::AddVariableName(AZStd::string_view name, AZStd::string_view suffix)
        {
            AZStd::string baseName = name;
            baseName.append("_");
            baseName.append(suffix);
            return AddVariableName(baseName);
        }

        AZ::s32 Scope::AddNameCount(AZStd::string_view name)
        {
            AZ::s32 count = -1;
            ScopePtr ns = shared_from_this();

            do
            {
                auto iter = ns->m_baseNameToCount.find(name);

                if (iter != ns->m_baseNameToCount.end())
                {
                    // a basename has been found in current or parent scope, get the latest count
                    count = iter->second;
                    break;
                }
            }             while ((ns = AZStd::const_pointer_cast<Scope>(ns->m_parent)));

            auto iter = m_baseNameToCount.find(name);
            if (iter == m_baseNameToCount.end())
            {
                iter = m_baseNameToCount.insert({ name, count }).first;
            }

            // increment the latest count
            count = ++iter->second;

            AZ_Assert(count >= 0, "There must at least be the zero indexed instance of the base name scope");
            return count;
        }

        const VariableData Source::k_emptyVardata{};

        Source::Source
            ( const Graph& graph
            , const AZ::Data::AssetId& id
            , const GraphData& graphData
            , const VariableData& variableData
            , AZStd::string_view name
            , AZStd::string_view path
            , NamespacePath&& namespacePath
            , bool addDebugInfo
            , bool printModelToConsole)
            : m_graph(&graph)
            , m_assetId(id)
            , m_graphData(&graphData)
            , m_variableData(&variableData)
            , m_name(name)
            , m_path(path)
            , m_namespacePath(AZStd::move(namespacePath))
            , m_addDebugInfo(addDebugInfo)
            , m_printModelToConsole(printModelToConsole)
            , m_assetIdString(id.ToString<AZStd::string>())
        {
        }

        AZ::Outcome<Source, AZStd::string> Source::Construct(const Grammar::Request& request)
        {
            if (!request.graph)
            {
                return AZ::Failure(AZStd::string("The request has no editor graph on it!"));
            }

            auto graphVariableData = request.graph->GetVariableDataConst();
            const VariableData* sourceVariableData = graphVariableData ? graphVariableData : &k_emptyVardata;

            if (auto graphData = request.graph->GetGraphDataConst())
            {
                AZStd::string name = request.name;
                AzFramework::StringFunc::Path::StripExtension(name);
                name = ToSafeName(name);

                AZStd::string namespacePath = request.namespacePath;
                AzFramework::StringFunc::Path::StripExtension(namespacePath);

                return AZ::Success(Source
                    (*request.graph
                    , request.scriptAssetId
                    , *graphData
                    , *sourceVariableData
                    , name
                    , request.path
                    , ToNamespacePath(namespacePath, name)
                    , request.addDebugInformation
                    , request.printModelToConsole));
            }
            else
            {
                return AZ::Failure(AZStd::string("could not construct from graph, no graph data was present"));
            }
        }

        Variable::Variable(Datum&& datum)
            : m_datum(datum)
        {}

        Variable::Variable(const Datum& datum, const AZStd::string& name, TraitsFlags traitsFlags)
            : m_datum(datum)
            , m_name(name)
            , m_isConst(traitsFlags& TraitsFlags::Const)
            , m_isMember(traitsFlags& TraitsFlags::Member)
        {}

        Variable::Variable(Datum&& datum, AZStd::string&& name, TraitsFlags&& traitsFlags)
            : m_datum(datum)
            , m_name(name)
            , m_isConst(traitsFlags& TraitsFlags::Const)
            , m_isMember(traitsFlags& TraitsFlags::Member)
        {}

        void Variable::Reflect(AZ::ReflectContext* reflectContext)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext))
            {
                serializeContext->Class<Variable>()
                    ->Version(0)
                    ->Field("sourceSlotId", &Variable::m_sourceSlotId)
                    ->Field("sourceVariableId", &Variable::m_sourceVariableId)
                    ->Field("nodeableNodeId", &Variable::m_nodeableNodeId)
                    ->Field("datum", &Variable::m_datum)
                    ->Field("name", &Variable::m_name)
                    ->Field("isConst", &Variable::m_isConst)
                    ->Field("isMember", &Variable::m_isMember)
                    ->Field("isDebugOnly", &Variable::m_isDebugOnly)
                    ;
            }
        }

        void VariableWriteHandling::Clear()
        {
            if (m_function)
            {
                AZStd::const_pointer_cast<ExecutionTree>(m_function)->Clear();
            }

            m_variable = nullptr;
            m_connectionVariable = nullptr;
        }
    }
}
