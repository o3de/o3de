/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "PrimitivesDeclarations.h"
#include "PrimitivesExecution.h"

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Core
        {
            class FunctionDefinitionNode;
        }
    }

    namespace Grammar
    {
        class NodelingInParserIterationListener
            : public ExecutionTreeTraversalListener
        {
        public:
            void CountOnlyGrammarCalls();

            const AZStd::vector<ExecutionTreeConstPtr>& GetLeavesWithoutNodelings() const;

            const AZStd::unordered_set<const ScriptCanvas::Nodes::Core::FunctionDefinitionNode*>& GetNodelingsOut() const;

            const AZStd::vector<ExecutionTreeConstPtr>& GetOutCalls() const;

            void Reset() override;

        protected:
            void EvaluateLeaf(ExecutionTreeConstPtr node, const Slot*, int) override;

        private:
            bool m_countOnlyGrammarCalls = false;
            AZStd::unordered_set<const ScriptCanvas::Nodes::Core::FunctionDefinitionNode*> m_uniqueNodelings;
            AZStd::vector<ExecutionTreeConstPtr> m_outCalls;
            AZStd::vector<ExecutionTreeConstPtr> m_leavesWithoutNodelings;
        };

        class PureFunctionListener
            : public ExecutionTreeTraversalListener
        {
        public:
            const VariableUseage& GetUsedVariables() const;

            bool IsPure() const;

            VariableUseage&& MoveUsedVariables();

        protected:
            void Evaluate(ExecutionTreeConstPtr node, const Slot* slot, [[maybe_unused]] int) override;

        private:
            bool m_isPure = true;
            VariableUseage m_usedVariables;
        };

        class UserOutCallCollector
            : public GraphExecutionPathTraversalListener
        {
        public:
            const AZStd::unordered_set<const ScriptCanvas::Nodes::Core::FunctionDefinitionNode*>& GetOutCalls() const;

        protected:
            void Evaluate(const EndpointResolved& endpoint) override;

        private:
            AZStd::unordered_set<const ScriptCanvas::Nodes::Core::FunctionDefinitionNode*> m_outCalls;
        };
    }
} 
