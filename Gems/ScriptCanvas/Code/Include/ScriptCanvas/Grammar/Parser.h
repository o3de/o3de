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
#include <ScriptCanvas/AST/AST.h>
#include <ScriptCanvas/AST/Nodes.h>
#include <ScriptCanvas/Core/Core.h>
#include <ScriptCanvas/Core/Node.h>

#include "ExecutionIterator.h"
#include "ParseError.h"
#include "ParserVisitor.h"

namespace ScriptCanvas
{
    class Graph;
    class Node;

    namespace AST
    {
        class Block;
    }

    namespace Grammar
    {
        NodePtrConstList FindInitialStatements(const NodeIdList& nodes);
        NodePtrConstList FindInitialStatements(const Node& node);
        const Node* FindNextStatement(const Node& node);

        class Parser
        {
            friend class ParserVisitor;

        public:
            Parser(const Graph& graph);

            AZ_CLASS_ALLOCATOR(Parser, AZ::SystemAllocator, 0);

            AST::SmartPtrConst<AST::Block> Parse();
            void ConsumeToken();
            const Node* GetToken() const;
            const Graph& GetSource() const;
            const NodeIdList& GetSourceNodeIDs() const;
            AST::NodePtr GetTree() const;
            bool IsValid() const;
            void SetRoot(AST::SmartPtr<AST::Block>&& root);
            void AddChild(AST::NodePtr&& child);
            
            Grammar::eRule GetExpectedRule() const;

        protected:
            void AddError(ParseError&& error);
            void AddStatement(AST::SmartPtrConst<AST::Statement>&& statement);
            void FindInitialStatements();
            void FindNextSourceNode();
            void InitializeGraphBlock();

            AST::SmartPtrConst<AST::BinaryOperation> CreateBinaryOperator(const Node& node, Grammar::eRule operatorType, AST::SmartPtrConst<AST::Expression>&& lhs, AST::SmartPtrConst<AST::Expression>&& rhs);
            AST::SmartPtrConst<AST::BinaryOperation> ParseBinaryOperation(const Node& node, Grammar::eRule operatorType);
            AST::SmartPtrConst<AST::Expression> ParseExpression(const Node& node);
            AST::SmartPtrConst<AST::ExpressionList> ParseExpressionList(const Node& node);
            AST::NodePtrConst ParseFunctionCall(const Node& node, const char* functionName);
            AST::SmartPtrConst<AST::FunctionCallAsStatement> ParseFunctionCallAsStatement(const Node& node, const char* functionName);
            AST::SmartPtrConst<AST::Numeral> ParseNumeral(const Node& node);

        private:
            class ExpressionScoper
            {
            public:
                AZ_INLINE ExpressionScoper(Parser& parser)
                    : m_parser(parser)
                {
                    ++m_parser.m_expressionDepth;
                }

                AZ_INLINE ~ExpressionScoper()
                {
                    --m_parser.m_expressionDepth;
                }

            private:
                Parser& m_parser;
            };

            friend class ExpressionScoper;

            int m_expressionDepth = 0;
            Grammar::ExecutionIterator m_executionIterator;
            AST::SmartPtr<AST::Block> m_currentBlock;
            AST::SmartPtr<AST::Block> m_root;
            AZStd::set<ID> m_consumedSourceNodes;
            AZStd::vector<ParseError> m_errors;
            ParserVisitor m_visitor;
        };
    } // namespace Grammar

} // namespace ScriptCanvas}