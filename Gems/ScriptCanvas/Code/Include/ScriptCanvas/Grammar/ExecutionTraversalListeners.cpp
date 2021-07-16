/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

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
#include <ScriptCanvas/Libraries/Core/Method.h>
#include <ScriptCanvas/Libraries/Core/Start.h>
#include <ScriptCanvas/Translation/TranslationUtilities.h>
#include <ScriptCanvas/Variable/VariableData.h>

#include "ExecutionTraversalListeners.h"
#include "ParsingUtilities.h"
#include "Primitives.h"

namespace ScriptCanvas
{
    namespace Grammar
    {
        void NodelingInParserIterationListener::CountOnlyGrammarCalls()
        {
            m_countOnlyGrammarCalls = true;
        }

        void NodelingInParserIterationListener::EvaluateLeaf(ExecutionTreeConstPtr node, const Slot*, int)
        {
            bool without = true;
            bool addedToOutCall = false;

            if (node->GetSymbol() == Symbol::UserOut)
            {
                m_outCalls.push_back(node);
                without = false;
                addedToOutCall = true;
            }

            if (auto nodeling = azrtti_cast<const ScriptCanvas::Nodes::Core::FunctionDefinitionNode*>(node->GetId().m_node))
            {
                if (!m_countOnlyGrammarCalls && !addedToOutCall)
                {
                    m_outCalls.push_back(node);
                }

                m_uniqueNodelings.insert(nodeling);
                without = false;
            }

            if (without)
            {
                m_leavesWithoutNodelings.push_back(node);
            }
        }

        const AZStd::vector<ExecutionTreeConstPtr>& NodelingInParserIterationListener::GetLeavesWithoutNodelings() const
        {
            return m_leavesWithoutNodelings;
        }

        const AZStd::unordered_set<const ScriptCanvas::Nodes::Core::FunctionDefinitionNode*>& NodelingInParserIterationListener::GetNodelingsOut() const
        {
            return m_uniqueNodelings;
        }

        const AZStd::vector<ExecutionTreeConstPtr>& NodelingInParserIterationListener::GetOutCalls() const
        {
            return m_outCalls;
        }

        void NodelingInParserIterationListener::Reset()
        {
            m_uniqueNodelings.clear();
            m_outCalls.clear();
            m_leavesWithoutNodelings.clear();
        }

        void PureFunctionListener::Evaluate(ExecutionTreeConstPtr node, [[maybe_unused]] const Slot* slot, [[maybe_unused]] int level)
        {
            ParseVariableUse(node, m_usedVariables);

            m_isPure = m_isPure
                && Grammar::IsPure(node->GetSymbol())
                && Grammar::IsPure(node->GetId().m_node, node->GetId().m_slot);
        }

        const VariableUseage& PureFunctionListener::GetUsedVariables() const
        {
            return m_usedVariables;
        }

        bool PureFunctionListener::IsPure() const
        {
            return m_isPure && m_usedVariables.memberVariables.empty();
        }

        VariableUseage&& PureFunctionListener::MoveUsedVariables()
        {
            return AZStd::move(m_usedVariables);
        }

        void UserOutCallCollector::Evaluate(const EndpointResolved& endpoint)
        {
            if (auto nodeling = IsUserOutNode(endpoint.first))
            {
                m_outCalls.insert(nodeling);
            }
        }

        const AZStd::unordered_set<const ScriptCanvas::Nodes::Core::FunctionDefinitionNode*>& UserOutCallCollector::GetOutCalls() const
        {
            return m_outCalls;
        }
    }
} 
