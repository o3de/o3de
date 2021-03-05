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

#include <AzCore/std/containers/set.h>
#include <AzCore/std/containers/stack.h>
#include <AzCore/std/containers/vector.h>
#include <ScriptCanvas/Core/Core.h>

#include "ExecutionVisitor.h"

namespace ScriptCanvas
{
    class Graph;
    class Node;

    namespace Grammar
    {
        /**
        This is the effective lexer of the system. It properly identifies which canvas node is the next 'token'
        */
        class ExecutionIterator
        {
            friend class ExecutionVisitor;
        
        public: 
            ExecutionIterator();
            ExecutionIterator(const Graph& graph);
            ExecutionIterator(const Node& block);

            bool Begin();
            bool Begin(const Graph& graph);
            bool Begin(const Node& block);
            AZ_INLINE const Node* GetCurrentToken() const { return *m_current; }
            const Node* operator->() const;
            explicit operator bool() const;
            bool operator!() const;
            ExecutionIterator& operator++();
            void PrettyPrint() const;

        protected:
            static const Node* FindNextStatement(const Node& current);
            
            void PushFunctionCall();
            void PushExpression();
            void PushStatement();
            void BuildNextStatement();
            
            bool BuildQueue(const Graph& graph);
            bool BuildQueue(const Node& node);
            bool BuildQueue();

        private:
            // track whether we're in a expression stack
            bool m_pushingExpressions = false;
            // current canvas node of any type
            const Node* m_currentSourceNode = nullptr;
            // current executable statement that isn't just an expression
            const Node* m_currentSourceStatementNode = nullptr;
            // first statement in the block entry
            const Node* m_currentSourceStatementRoot = nullptr;
            // canvas source
            const Graph* m_source = nullptr;
            
            NodeIdList m_sourceNodes;          
            // "parent-less" executable statements, or parented only to a start node
            NodePtrConstList m_currentSourceInitialStatements;
            // detect infinite loops
            AZStd::set<const Node*> m_iteratedStatements;

            ExecutionVisitor m_visitor;
            // final queue of iterated nodes
            AZStd::vector<const Node*> m_queue;
            // current position in the final queue
            NodePtrConstList::iterator m_current = nullptr;
        };

    } // namespace Parser

} // namepsace ScriptCanvas