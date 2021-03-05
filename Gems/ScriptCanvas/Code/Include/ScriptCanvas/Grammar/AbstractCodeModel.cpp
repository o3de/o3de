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

#include "AbstractCodeModel.h"

#if !defined(_RELEASE)

#include <ScriptCanvas/Core/Graph.h>
#include <ScriptCanvas/Core/Node.h>
#include <ScriptCanvas/Variable/VariableData.h>
#include <ScriptCanvas/Libraries/Core/EBusEventHandler.h>
#include <ScriptCanvas/Libraries/Core/Start.h>
#include <ScriptCanvas/Translation/TranslationUtilities.h>

namespace ScriptCanvas
{
    namespace Grammar
    {
        AbstractCodeModel::AbstractCodeModel(const Source& source)
            : m_source(source)
            , m_outcome(AZ::Failure(AZStd::string("Not attempted")))
        {
            m_outcome = Parse();
        }

        void AbstractCodeModel::AddFunction([[maybe_unused]] const Node& node)
        {
            // if not in the function list, add it to the function list
        }

        const Functions& AbstractCodeModel::GetFunctions() const
        {
            return m_functions;
        }

        AZ::Outcome<void, AZStd::string> AbstractCodeModel::GetOutcome() const
        {
            return m_outcome;
        }

        const Node* AbstractCodeModel::GetStartNode() const
        {
            return m_startNode;
        }

        const Source& AbstractCodeModel::GetSource() const
        {
            return m_source;
        }
        
        bool AbstractCodeModel::IsFunction(const Node& node) const
        {
            AZ_Assert(!azrtti_cast<const Nodes::Core::Start*>(&node), "The start node should never enter this function!");
            AZ_Assert(!azrtti_cast<const Nodes::Core::EBusEventHandler*>(&node), "The event handler node should never enter this function!");

            if (node.IsEntryPoint())
            {
                return false;
            }

            const auto executionInputConnections = node.FindConnectedNodesByDescriptor(SlotDescriptors::ExecutionIn());
            const auto size = executionInputConnections.size();

            if (size == 0 || size > 1)
            {
                // headless node, or multiple inputs
                return true;
            }

            if (executionInputConnections[0]->IsEntryPoint())
            {   
                // connected to an entry point
                return true;
            }

            // not a function
            return false;
        }

        bool AbstractCodeModel::IsPureLibrary() const
        {
            return m_isPureLibrary;
        }

        void AbstractCodeModel::ParseLatentExecution([[maybe_unused]] const Node& node)
        {
            // add any connections to the function list
            // make sure any connected output (which must be variables) are member variables

            // don't forget to account for output that got pushed across latent execution, especially in the case 
            // where such output was one of multiple inputs into a function node
        }
        
        AZ::Outcome<void, AZStd::string> AbstractCodeModel::Parse()
        {
            return AZ::Success();
        }

        AZ::Outcome<void, AZStd::string> AbstractCodeModel::Parse(const Node& node)
        {
            // the most generic parse, most nodes won't need more than this (and we should strive that none outside of core nodes do)
            if (IsFunction(node))
            {
                AddFunction(node);
            }

            ParseLatentExecution(node);

            return m_outcome;
        }        

        AZ::Outcome<void, AZStd::string> AbstractCodeModel::ProcessDependencies()
        {
            return AZ::Success();
        }
    
        AZ::Outcome<void, AZStd::string> AbstractCodeModel::ProcessFunctions()
        {
            return AZ::Success();
        }
        
        AZ::Outcome<void, AZStd::string> AbstractCodeModel::ProcessHandlers()
        {
            return AZ::Success();
        }

        AZ::Outcome<void, AZStd::string> AbstractCodeModel::ProcessVariables()
        {
            return AZ::Success();
        }
        
        Source::Source(const Graph& graph, const GraphData& graphData, const VariableData& variableData, const AZStd::string& name, const AZStd::string& path)
            : m_graph(graph)
            , m_graphData(graphData)
            , m_variableData(variableData)
            , m_name(name)
            , m_path(path)
        {}
        
        AZ::Outcome<Source, AZStd::string> Source::Construct(const Graph& graph, const AZStd::string& name, const AZStd::string& path)
        {
            const VariableData emptyVardata{};
            auto graphVariableData = graph.GetVariableDataConst();
            const VariableData* sourceVariableData = graphVariableData ? graphVariableData : &emptyVardata;

            if (auto graphData = graph.GetGraphDataConst())
            {
                return AZ::Success(Source(graph, *graphData, *sourceVariableData, name, path));
            }
            else
            {
                return AZ::Failure(AZStd::string("could not construct from graph, no graph data was present"));
            }            
        }

    } // namespace Grammar
} // namespace ScriptCanvas

#endif