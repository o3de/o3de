/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "AzslcListener.h"

namespace AZ::ShaderCompiler
{
    void SemaCheckListener::enterIdExpression(azslParser::IdExpressionContext* ctx)
    {
        UnqualifiedName currentIdCtxUqName = ExtractNameFromIdExpression(ctx);
        auto* memberAccessExpr = As<AstMemberAccess*>(ctx->parent);
        assert(IsRHSOfMemberAccess(ctx) == !!memberAccessExpr);
        if (memberAccessExpr)  // if direct parent is MAE, it's guaranteed this current idExpression is RHS. (because LHS of MAE is expression, not idExpression)
        {
            // we are in `a.b::c` (which can happen legally c.f. https://stackoverflow.com/q/56253767/893406)
            bool isAbsolute = ctx->qualifiedId() && ctx->qualifiedId()->nestedNameSpecifier()->GlobalSROToken != nullptr;  // qualified and fully.
            // in MAE (MemberAccessExpression) the LHS (left-hand-side) determines what scope we should look into, for this idExpression.
            // in this context (MAE), a member accessed through a dot, `lhs.field` means the scope of interest is typeof(lhs).
            QualifiedName startupScope = isAbsolute ? QualifiedName{"/"}  // no need to do any Join if the RHS is absolute
                                                    : m_ir->m_sema.TypeofExpr(memberAccessExpr->LHSExpr); // relative to the scope of the type of LHS
            if (isAbsolute || m_ir->m_sema.VerifyLHSExprOfMAExprIsValid(memberAccessExpr).first)
            {
                m_ir->m_sema.RegisterSeenat(ctx, startupScope);
            }
        }
        else if (auto* typeofExpression = As<azslParser::TypeofExpressionContext*>(ctx->parent))
        {
            QualifiedName startupScope = typeofExpression->Expr ? m_ir->m_sema.TypeofExpr(typeofExpression->Expr)
                                                                : m_ir->m_sema.TypeofExpr(typeofExpression->type());
            if (m_ir->m_sema.VerifyTypeIsScopeComposable(startupScope).first)
            {
                m_ir->m_sema.RegisterSeenat(ctx, startupScope);
            }
        }
        else
        {   // simple direct reference registration. no outer context needs to be considered that can alter the symbol it refers to.
            m_ir->m_sema.RegisterSeenat(ctx);
        }
    }

    void SemaCheckListener::enterSrgSemantic(azslParser::SrgSemanticContext* ctx)
    {
        m_ir->m_sema.RegisterSRGSemantic(ctx);
        m_ir->m_scope.EnterScope(ctx->Name->getText(), ctx->srgSemanticBodyDeclaration()->LeftBrace()->getSourceInterval().a);
    }

    void SemaCheckListener::enterInterfaceDefinition(azslParser::InterfaceDefinitionContext* ctx)
    {
        m_ir->m_sema.RegisterInterface(ctx);
        m_ir->m_scope.EnterScope(ctx->Name->getText(), ctx->LeftBrace()->getSourceInterval().a);
    }

    void SemaCheckListener::enterStructDefinition(azslParser::StructDefinitionContext* ctx)
    {
        m_ir->m_sema.RegisterStruct(ctx);
        m_ir->m_scope.EnterScope(ctx->Name->getText(), ctx->LeftBrace()->getSourceInterval().a);
    }

    void SemaCheckListener::enterClassDefinition(azslParser::ClassDefinitionContext* ctx)
    {
        m_ir->m_sema.RegisterClass(ctx);
        if (!ctx->baseList())  // if there is a base list, we can't enter the scope that early. it has to be at base list exit.
        {
            m_ir->m_scope.EnterScope(ctx->Name->getText(), ctx->LeftBrace()->getSourceInterval().a);
        }
    }

    void SemaCheckListener::enterEnumDefinition(azslParser::EnumDefinitionContext* ctx)
    {        
        m_ir->m_sema.RegisterEnum(ctx);
        auto* scopedCtx = As<azslParser::ScopedEnumContext*>(ctx->enumKey());
        if (scopedCtx != nullptr)
        {
            m_ir->m_scope.EnterScope(ctx->Name->getText(), ctx->LeftBrace()->getSourceInterval().a);
        }
    }

    void SemaCheckListener::enterEnumeratorDeclarator(azslParser::EnumeratorDeclaratorContext* ctx)
    {
        m_ir->m_sema.RegisterEnumerator(ctx);
    }

    void SemaCheckListener::enterSamplerBodyDeclaration(azslParser::SamplerBodyDeclarationContext* ctx)
    {
        auto name = ExtractVariableNameSamplerBodyDeclaration(ctx);
        m_ir->m_scope.EnterScope(name, ctx->LeftBrace()->getSourceInterval().a);
    }

    void SemaCheckListener::exitSamplerBodyDeclaration(azslParser::SamplerBodyDeclarationContext* ctx)
    {
        m_ir->m_scope.ExitScope(ctx->RightBrace()->getSourceInterval().b);
    }

    void SemaCheckListener::enterSrgDefinition(azslParser::SrgDefinitionContext* ctx)
    {
        m_ir->m_sema.RegisterSRG(ctx);
        m_ir->m_scope.EnterScope(ctx->Name->getText(), ctx->LeftBrace()->getSourceInterval().a);
    }

    void SemaCheckListener::exitSrgDefinition(azslParser::SrgDefinitionContext* ctx)
    {
        // Validate the content on scope exit
        m_ir->m_sema.ValidateSrg(ctx);
        m_ir->m_scope.ExitScope(ctx->RightBrace()->getSourceInterval().b);
    }

    void SemaCheckListener::enterSrgSemanticMemberDeclaration(azslParser::SrgSemanticMemberDeclarationContext* ctx)
    {
        m_ir->m_sema.RegisterSRGSemanticMember(ctx);
    }

    void SemaCheckListener::exitClassDefinition(azslParser::ClassDefinitionContext* ctx)
    {
        m_ir->m_sema.ValidateClass(ctx);
        m_ir->m_scope.ExitScope(ctx->RightBrace()->getSourceInterval().b);
    }

    void SemaCheckListener::exitSrgSemantic(azslParser::SrgSemanticContext* ctx)
    {
        m_ir->m_sema.ValidateSrgSemantic(ctx);
        m_ir->m_scope.ExitScope(ctx->srgSemanticBodyDeclaration()->RightBrace()->getSourceInterval().b);
    }

    void SemaCheckListener::exitInterfaceDefinition(azslParser::InterfaceDefinitionContext* ctx)
    {
        m_ir->m_scope.ExitScope(ctx->RightBrace()->getSourceInterval().b);
    }

    void SemaCheckListener::exitStructDefinition(azslParser::StructDefinitionContext* ctx)
    {
        m_ir->m_scope.ExitScope(ctx->RightBrace()->getSourceInterval().b);
    }

    void SemaCheckListener::exitEnumDefinition(azslParser::EnumDefinitionContext* ctx)
    {
        auto* scopedCtx = As<azslParser::ScopedEnumContext*>(ctx->enumKey());
        if (scopedCtx != nullptr)
        {
            m_ir->m_scope.ExitScope(ctx->RightBrace()->getSourceInterval().b);
        }
    }

    void SemaCheckListener::exitFunctionDefinition(azslParser::FunctionDefinitionContext* ctx)
    {
        m_ir->m_scope.ExitScope(ctx->hlslFunctionDefinition()->block()->RightBrace()->getSourceInterval().b);
        // if it was a scoped definition, we entered a tiny scope to establish the function's identity correctly. exit that too.
        if (ctx->hlslFunctionDefinition()->leadingTypeFunctionSignature()->ClassName)
        {
            m_ir->m_scope.ExitScope(ctx->hlslFunctionDefinition()->leadingTypeFunctionSignature()->Name->getTokenIndex());
        }
    }

    void SemaCheckListener::enterFunctionDefinition(azslParser::FunctionDefinitionContext* ctx)
    {
        // Forbid function definitions inside "struct"s.
        if (m_ir->m_sema.IsScopeStruct())
        {
            throw AzslcException(ADVANCED_SYNTAX_FUNCTION_IN_STRUCT,
                                 "Syntax",
                                 ctx->start,
                                 "structs cannot have member functions; consider using a class.");
        }

        auto signature = ctx->hlslFunctionDefinition()->leadingTypeFunctionSignature();
        auto uqName = ExtractNameFromAnyContextWithName(signature);

        // extract the class type in case this is a deported method definition
        azslParser::UserDefinedTypeContext* className = ctx->hlslFunctionDefinition()->leadingTypeFunctionSignature()->ClassName;
        IdentifierUID newSymbol;
        if (className)
        {
            newSymbol = m_ir->m_sema.RegisterDeportedMethod(uqName, className, signature).first;
        }
        else
        {
            newSymbol = m_ir->m_sema.RegisterFunction(uqName, signature, AsFunc::Definition).first;
        }
        // a function scope starts at the beginning of its parameter list
        // use a variation of the EnterScope function that takes an absolute path (FQN)
        // because entry into a deported method is not an accumulative scope, but a looked up one,
        // so it can be arbitrarily unrelated to the current scope.
        m_ir->m_scope.EnterScope(newSymbol.GetName(), ctx->hlslFunctionDefinition()->leadingTypeFunctionSignature()->LeftParen()->getSourceInterval().a);
    }

    void SemaCheckListener::exitFunctionDeclaration(azslParser::FunctionDeclarationContext* ctx)
    {
        // The scope interval will cover all trailing keywords (we are not mirroring the LeftParen with RightParen, but with end-of-statement)
        // anyway don't confuse this token precision for actual scope activation. actual scope activation/deactivation is done by this call.
        m_ir->m_scope.ExitScope(ctx->hlslFunctionDeclaration()->leadingTypeFunctionSignature()->stop->getTokenIndex());
    }

    void SemaCheckListener::enterFunctionDeclaration(azslParser::FunctionDeclarationContext* ctx)
    {
        // Forbid function declarations inside "struct"s.
        if (m_ir->m_sema.IsScopeStruct())
        {
            throw AzslcException(ADVANCED_SYNTAX_FUNCTION_IN_STRUCT,
                                 "Syntax",
                                 ctx->start,
                                 "structs cannot have member functions; consider using a class.");
        }

        auto signature = ctx->hlslFunctionDeclaration()->leadingTypeFunctionSignature();
        auto uqName = ExtractNameFromAnyContextWithName(signature);
        auto& [id, _] = m_ir->m_sema.RegisterFunctionDeclarationAndAddSeenat(uqName, signature);
        // we will shortly enter the scope of the function, even for just a declaration, for the sake of canonicalizing validation.
        // it also makes sense anyway since parameters (and any trailings) are defined in the function's scope.
        m_ir->m_scope.EnterScope(id.GetNameLeaf(), signature->LeftParen()->getSourceInterval().a);
    }

    void SemaCheckListener::exitLeadingTypeFunctionSignature(azslParser::LeadingTypeFunctionSignatureContext* ctx)
    {
        // 'will run from function scope, be it declaration or definition.
        m_ir->m_sema.ValidateFunctionAndRegisterFamilySeenat(ctx);
    }

    void SemaCheckListener::exitFunctionParam(azslParser::FunctionParamContext* ctx)
    {   // we use the exit rule to let the time to inlined-UDT-decl to get registered through the type rule visit first.
        auto& funcInfo = m_ir->m_sema.GetCurrentScopeSubInfoAs<FunctionInfo>();
        if (funcInfo.m_multiFwds != FMF_FwdDeclRedundancy)  // when in that state, we don't accept parameter registration
        {
            if (!ctx->Name)
            {
                m_ir->m_sema.RegisterNamelessFunctionParameter(ctx);
            }
            else
            {
                m_ir->m_sema.RegisterVar(ctx->Name, ctx->unnamedVariableDeclarator());
            }
        }
    }

    void SemaCheckListener::enterNamedVariableDeclarator(azslParser::NamedVariableDeclaratorContext* ctx)
    {
        // unlike in versions prior and equal to 1.4.x.x, this rule is now only taken by standalone variable declaration statements, and not function arguments anymore
        m_ir->m_sema.RegisterVar(ctx->Name, ctx->unnamedVariableDeclarator());
    }

    void SemaCheckListener::enterBaseList(azslParser::BaseListContext* ctx)
    {

        m_ir->m_sema.RegisterBases(ctx);
    }

    void SemaCheckListener::exitBaseList(azslParser::BaseListContext* ctx)
    {
        // if there was a base list, we deferred entering in scope
        auto* classDefCtx = polymorphic_downcast<AstClassDeclNode*>(ctx->parent);  // baseList is only used in that context
        m_ir->m_scope.EnterScope(classDefCtx->Name->getText(),
                                 classDefCtx->LeftBrace()->getSourceInterval().a);
    }

    void SemaCheckListener::enterCompilerExtensionStatement(azslParser::CompilerExtensionStatementContext* ctx)
    {
        if (m_silentPrintExtensions)
        {
            return;
        }

        if (ctx->KW_ext_print_message())
        {
            // [GFX TODO]: use a log channel that makes sense. e.g. INFORMATION ?
            std::cout << Unescape(ctx->Message->getText());
        }
        else if (ctx->KW_ext_print_symbol())
        {
            if (ctx->idExpression())
            {
                auto name = ExtractNameFromIdExpression(ctx->idExpression());
                auto maybeSym = m_ir->m_sema.LookupSymbol(name);
                if (!maybeSym)
                {
                    std::cout << "<not_found>";
                }
                else
                {
                    auto& [symId, sym] = *maybeSym;
                    if (ctx->KW_ext_prtsym_fully_qualified())
                    {
                        std::cout << symId.m_name;
                    }
                    else if (ctx->KW_ext_prtsym_least_qualified())
                    {
                        std::cout << m_ir->m_symbols.FindLeastQualifiedName(m_ir->m_scope.m_currentScopePath, symId);
                    }
                    else if (ctx->KW_ext_prtsym_constint_value())
                    {
                        if (sym.GetKind() != Kind::Variable)
                        {
                            std::cout << "<not_a_var>";
                        }
                        else
                        {
                            auto& var = sym.GetSubRefAs<VarInfo>();
                            try
                            {
                                int64_t asInt = ExtractValueAsInt64(var.m_constVal);
                                std::cout << asInt;
                            }
                            catch (exception e)
                            {
                                std::cout << "<folding_failed>";
                            }
                        }
                    }
                }
            }
            else if (ctx->typeofExpression())
            {
                QualifiedName resolved = m_ir->m_sema.TypeofExpr(ctx->typeofExpression());
                if (ctx->KW_ext_prtsym_fully_qualified())
                {
                    std::cout << resolved;
                }
                else if (ctx->KW_ext_prtsym_least_qualified())
                {
                    const auto* idKind = m_ir->m_symbols.GetIdAndKindInfo(resolved);
                    if (idKind)
                    {
                        std::cout << m_ir->m_symbols.FindLeastQualifiedName(m_ir->m_scope.m_currentScopePath, idKind->first);
                    }
                    else
                    {
                        std::cout << resolved;
                    }
                }
            }
        }
    }

    static AttributeCategory GetAttributeCategory(antlr4::ParserRuleContext* ctx)
    {
        if (Is<azslParser::AttributeSpecifierContext*>(ctx->parent))
        {
            return AttributeCategory::Single;
        }

        if (Is<azslParser::AttributeSpecifierSequenceContext*>(ctx->parent))
        {
            return AttributeCategory::Sequence;
        }

        throw std::runtime_error{"Attribute specifier declared from unhandled parent type!"};
    }

    template< typename AttributeContextT >
    static void DoAttributeRegistration(AttributeContextT* ctx, AttributeScope scope, IntermediateRepresentation* ir)
    {
        // Attributes can be filtered out by namespace.
        // Attributes without a namespace are always valid and attributes in the 'void' namespace are always filtered out
        auto Namespace = (ctx->Namespace) ? ctx->Namespace->getText() : "";
        if (!Namespace.empty() && (Namespace == "void" || !ir->IsAttributeNamespaceActivated(Namespace)))
        {
            return;
        }

        ir->RegisterAttributeSpecifier(scope,
                                       GetAttributeCategory(ctx),
                                       ctx->getStart()->getLine(),
                                       Namespace,
                                       ctx->Name->getText(),
                                       ctx->attributeArgumentList());
    }

    void SemaCheckListener::enterGlobalAttribute(azslParser::GlobalAttributeContext* ctx)
    {
        DoAttributeRegistration(ctx, AttributeScope::Global, m_ir);
    }

    void SemaCheckListener::enterAttachedAttribute(azslParser::AttachedAttributeContext* ctx)
    {
        DoAttributeRegistration(ctx, AttributeScope::Attached, m_ir);
    }

    void SemaCheckListener::exitTypedefStatement(azslParser::TypedefStatementContext* ctx)
    {
        m_ir->m_sema.RegisterTypeAlias(ctx->NewTypeName->getText(), ctx->ExistingType, polymorphic_downcast<azslParser::TypeAliasingDefinitionStatementContext*>(ctx->parent));
    }

    void SemaCheckListener::exitTypealiasStatement(azslParser::TypealiasStatementContext* ctx)
    {
        m_ir->m_sema.RegisterTypeAlias(ctx->NewTypeName->getText(), ctx->ExistingType, polymorphic_downcast<azslParser::TypeAliasingDefinitionStatementContext*>(ctx->parent));
    }

    void SemaCheckListener::exitTypeofExpression(azslParser::TypeofExpressionContext* ctx)
    {
        if (ctx->SubQualification
            && ctx->SubQualification->qualifiedId()
            && ctx->SubQualification->qualifiedId()->nestedNameSpecifier()->GlobalSROToken)
        {
            throw AzslcException(ADVANCED_SYNTAX_DOUBLE_SCOPE_RESOLUTION, "Syntax", ctx->SubQualification->qualifiedId()->nestedNameSpecifier()->GlobalSROToken,
                                 "double scope resolution operator forbidden at this position");
        }
    }

    void SemaCheckListener::enterForStatement(azslParser::ForStatementContext* ctx)
    {
        // for (a;b;c) block is special, because it has a special power of symbol definition in the 'a' block.
        //             therefore we enter the anonymous scope right before 'a'
        m_ir->m_sema.MakeAndEnterAnonymousScope("for", ctx->LeftParen()->getSymbol());
    }

    void SemaCheckListener::exitForStatement(azslParser::ForStatementContext* ctx)
    {
        m_ir->m_scope.ExitScope(ctx->stop->getTokenIndex());
    }

    void SemaCheckListener::enterBlockStatement(azslParser::BlockStatementContext* ctx)
    {
        if (!Is< azslParser::HlslFunctionDefinitionContext >(ctx->parent)  // not function because we already entered
            && !Is< azslParser::ForStatementContext >(ctx->parent))  // not for statement because we already entered
        {
            m_ir->m_sema.MakeAndEnterAnonymousScope("bk", ctx->start);
        }
    }

    void SemaCheckListener::exitBlockStatement(azslParser::BlockStatementContext* ctx)
    {
        if (!Is< azslParser::HlslFunctionDefinitionContext >(ctx->parent)  // not function, because we already exited
            && !Is< azslParser::ForStatementContext >(ctx->parent))  // not for statement, because we exited in exitForStatement
        {
            m_ir->m_scope.ExitScope(ctx->stop->getTokenIndex());
        }
    }

    void SemaCheckListener::enterSwitchBlock(azslParser::SwitchBlockContext* ctx)
    {
        m_ir->m_sema.MakeAndEnterAnonymousScope("sw", ctx->start);
    }

    void SemaCheckListener::exitSwitchBlock(azslParser::SwitchBlockContext* ctx)
    {
        m_ir->m_scope.ExitScope(ctx->stop->getTokenIndex());
    }

    void SemaCheckListener::enterFunctionCallExpression(azslParser::FunctionCallExpressionContext* ctx)
    {
        for(azslParserBaseListener* mutator : m_functionCallMutators)
        {
            mutator->enterFunctionCallExpression(ctx);
        }
    }

    void SemaCheckListener::visitTerminal(tree::TerminalNode* node)
    {
        m_ir->RegisterTokenToNodeAssociation(node->getSymbol()->getTokenIndex(), polymorphic_downcast<ParserRuleContext*>(node->parent));
    }
}
