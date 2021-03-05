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

#if !defined(_RELEASE)

#include <AzCore/std/containers/unordered_map.h>
#include <ScriptCanvas/Core/Core.h>
#include <ScriptCanvas/Variable/VariableCore.h>
#include <ScriptCanvas/Variable/GraphVariable.h>
#include <AzCore/Outcome/Outcome.h>

namespace ScriptCanvas
{
    class Graph;
    class Node;
    class VariableData;

    struct GraphData;

    namespace Grammar
    {
        struct Namespace;

        struct Source
        {
            static AZ::Outcome<Source, AZStd::string> Construct(const Graph& graph, const AZStd::string& name, const AZStd::string& path);

            const Graph& m_graph;
            const GraphData& m_graphData;
            const VariableData& m_variableData;
            const AZStd::string m_name;
            const AZStd::string m_path;

            Source(const Graph& graph, const GraphData& graphData, const VariableData& variableData, const AZStd::string& name, const AZStd::string& path);
        }; // struct Source

        struct Dependency
        {
            const Node* m_source = nullptr;
        }; // struct Dependency

        struct Function
        {
            const Node* m_function = nullptr;
            const Namespace* m_namespace = nullptr;
            bool m_isPublic = false;
            bool m_isStatic = true;
            bool m_isStart = false;
        }; // struct Function

        using Functions = AZStd::unordered_map<AZ::EntityId, Function>;

        struct Namespace
        {
            const AZStd::string m_name;
            const Namespace* m_parent = nullptr;
        }; // struct Namespace

        struct Variable
        {
            const GraphVariable* m_variable = nullptr;
            bool m_isConstant = true;
            bool m_isMember = false;
        }; // struct Variable

        /// This class parses a graph into abstract programming concepts for easier translation
        /// into C++, LLVM-IR, Lua, or whatever else would be needed
        class AbstractCodeModel
        {
        public:
            
            AbstractCodeModel(const Source& source);
            
            const Functions& GetFunctions() const;

            AZ::Outcome<void, AZStd::string> GetOutcome() const;

            const Source& GetSource() const;

            const Node* GetStartNode() const;
            
            bool IsPureLibrary() const;
                                  
        private:
            AZ::Outcome<void, AZStd::string> m_outcome;
            // if only functions, or pure data, but no with state operations or handlers, etc
            bool m_isPureLibrary = true;
            // if start nodes, or auto-connected handlers
            bool m_requiresActivation = false;
            const Source m_source;
            const Node* m_startNode = nullptr;
            AZStd::unordered_map<AZ::EntityId, Dependency> m_dependencies;
            Functions m_functions;
            // not just ebus event handlers, but implicit or time based handlers, like for the delay node, etc
            AZStd::unordered_map<AZ::EntityId, Node*> m_handlers;
            AZStd::unordered_map<AZ::EntityId, Variable> m_variables;

            // add as entry into the function list
            void AddFunction(const Node& node);
            
            AZ::Outcome<void, AZStd::string> Parse();
            AZ::Outcome<void, AZStd::string> Parse(const Node& node);
            // examine slices, or references to functionality exposed only through other graphs
            AZ::Outcome<void, AZStd::string> ProcessDependencies();
            // analyze the scope of all functions, and the variables involved
            // detect latent scope, etc.
            AZ::Outcome<void, AZStd::string> ProcessFunctions();
            AZ::Outcome<void, AZStd::string> ProcessHandlers();
            AZ::Outcome<void, AZStd::string> ProcessVariables();

            bool IsFunction(const Node& node) const;
            
            void ParseLatentExecution(const Node& node);

        public:
            /* for c++-i-ness .. move out of this model, and put in a translator
            /// returns true iff has any handlers (OR ...)
            bool RequiresDestructor() const;

            /// returns true iff has any start nodes OR auto connected handlers
            bool RequiresActivation() const;
            */
        
        }; // class AbstractCodeModel

    } // namespace Grammar

} // namespace ScriptCanvas

#else


#endif