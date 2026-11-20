/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include "AzslcIntermediateRepresentation.h"

namespace AZ::ShaderCompiler
{
    struct SemaCheckListener : azslParserBaseListener
    {
        using Self = SemaCheckListener;
        using Base = azslParserBaseListener;

        SemaCheckListener(IntermediateRepresentation* ir)
            : m_ir(ir)
        {}

        void enterIdExpression(azslParser::IdExpressionContext* ctx) override;
        void enterSrgSemantic(azslParser::SrgSemanticContext* ctx) override;
        void enterInterfaceDefinition(azslParser::InterfaceDefinitionContext* ctx) override;
        void enterStructDefinition(azslParser::StructDefinitionContext* ctx) override;
        void enterClassDefinition(azslParser::ClassDefinitionContext* ctx) override;
        void enterEnumDefinition(azslParser::EnumDefinitionContext* ctx) override;
        void enterEnumeratorDeclarator(azslParser::EnumeratorDeclaratorContext* ctx) override;
        void enterSamplerBodyDeclaration(azslParser::SamplerBodyDeclarationContext* ctx) override;
        void exitSamplerBodyDeclaration(azslParser::SamplerBodyDeclarationContext* ctx) override;
        void enterSrgDefinition(azslParser::SrgDefinitionContext* ctx) override;
        void exitSrgDefinition(azslParser::SrgDefinitionContext* ctx) override;
        void enterSrgSemanticMemberDeclaration(azslParser::SrgSemanticMemberDeclarationContext* ctx) override;
        void exitClassDefinition(azslParser::ClassDefinitionContext* ctx) override;
        void exitSrgSemantic(azslParser::SrgSemanticContext* ctx) override;
        void exitInterfaceDefinition(azslParser::InterfaceDefinitionContext* ctx) override;
        void exitStructDefinition(azslParser::StructDefinitionContext* ctx) override;
        void exitEnumDefinition(azslParser::EnumDefinitionContext* ctx) override;
        void exitFunctionDefinition(azslParser::FunctionDefinitionContext* ctx) override;
        void enterFunctionDefinition(azslParser::FunctionDefinitionContext* ctx) override;
        void exitFunctionDeclaration(azslParser::FunctionDeclarationContext* ctx) override;
        void enterFunctionDeclaration(azslParser::FunctionDeclarationContext* ctx) override;
        void exitLeadingTypeFunctionSignature(azslParser::LeadingTypeFunctionSignatureContext* ctx) override;
        void exitFunctionParam(azslParser::FunctionParamContext* ctx) override;
        void enterNamedVariableDeclarator(azslParser::NamedVariableDeclaratorContext* ctx) override;
        void enterBaseList(azslParser::BaseListContext* ctx) override;
        void exitBaseList(azslParser::BaseListContext* ctx) override;
        void enterCompilerExtensionStatement(azslParser::CompilerExtensionStatementContext* ctx) override;
        void enterGlobalAttribute(azslParser::GlobalAttributeContext* ctx) override;
        void enterAttachedAttribute(azslParser::AttachedAttributeContext* ctx) override;
        void exitTypedefStatement(azslParser::TypedefStatementContext* ctx) override;
        void exitTypealiasStatement(azslParser::TypealiasStatementContext* ctx) override;
        void exitTypeofExpression(azslParser::TypeofExpressionContext* ctx) override;
        void enterForStatement(azslParser::ForStatementContext* ctx) override;
        void exitForStatement(azslParser::ForStatementContext* ctx) override;
        void enterBlockStatement(azslParser::BlockStatementContext* ctx) override;
        void exitBlockStatement(azslParser::BlockStatementContext* ctx) override;
        void enterSwitchBlock(azslParser::SwitchBlockContext* ctx) override;
        void exitSwitchBlock(azslParser::SwitchBlockContext* ctx) override;
        void enterFunctionCallExpression(azslParser::FunctionCallExpressionContext* ctx) override;

        void visitTerminal(tree::TerminalNode* node) override;

        IntermediateRepresentation* m_ir;
        bool m_silentPrintExtensions = false;

        //! If not empty, these other parser listeners will be used during
        //! enterFunctionCallExpression(), its data type is azslParserBaseListener because
        //! in the future it can be convenient to do mutations at different stages of parsing
        //! the AST.
        vector<azslParserBaseListener*> m_functionCallMutators;
    };
}
