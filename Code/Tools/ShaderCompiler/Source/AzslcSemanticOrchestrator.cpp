/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "AzslcSemanticOrchestrator.h"

namespace AZ::ShaderCompiler
{
    namespace  // utility helpers to raise the abstraction level of SemanticOrchestrator's methods.
    {
        Packing::MatrixMajor ExtractMatrixMajorness(TypeQualifiers variableQualificationFlags)
        {
            Packing::MatrixMajor major = Packing::MatrixMajor::Default;
            if (TypeHasStorageFlag(variableQualificationFlags, StorageFlag::RowMajor))
            {
                major = Packing::MatrixMajor::RowMajor;
            }
            else if (TypeHasStorageFlag(variableQualificationFlags, StorageFlag::ColumnMajor))
            {
                major = Packing::MatrixMajor::ColumnMajor;
            }
            return major;
        }

        TypeQualifiers ExtractTypeQualifiers(azslParser::StorageFlagsContext* flags)
        {
            TypeQualifiers qualifiers;
            for (auto* flagCtx : flags->storageFlag())
            {
                StorageFlag newFlag = AsFlag(flagCtx);
                qualifiers.m_flag |= newFlag;
                if (newFlag == StorageFlag::Other)
                {
                    qualifiers.m_others.push_back(flagCtx->getText());
                }
            }
            return qualifiers;
        }

        void CheckFunctionReturnTypeModifierNotOptionNorRootconstant(const TypeQualifiers& qualifier, size_t line)
        {
            auto ngFlags = Modifiers{StorageFlag::Option} | StorageFlag::Rootconstant;
            if (qualifier.m_flag & ngFlags)
            {
                throw AzslcOrchestratorException{ORCHESTRATOR_DISALLOWED_FUNCTION_MODIFIER, line,
                    none, " Functions can't have option or rootconstant qualified return types."};
            }
        }
    }

    SemanticOrchestrator::SemanticOrchestrator(SymbolAggregator* sema, ScopeTracker* scope, azslLexer* lexer)
        : m_symbols{ sema },
          m_scope{ scope },
          m_lexer{ lexer },
          m_anonymousCounter{ 0 }
    {
        assert(sema != nullptr && scope != nullptr);
    }

    IdAndKind& SemanticOrchestrator::GetCurrentScopeIdAndKind()
    {
        auto nameOfScope = m_scope->GetNameOfCurScope();
        auto* idAndKindPtr = m_symbols->GetIdAndKindInfo(nameOfScope);
        if (!idAndKindPtr)
        {
            throw std::logic_error("Internal error: current scope not registered");
        }
        return *idAndKindPtr;
    }

    // Note that to factorize code between above and below, one could pass a runtime (or template integer), N-levels up as an argument.
    IdAndKind& SemanticOrchestrator::GetCurrentParentScopeIdAndKind()
    {
        auto nameOfParentScope = m_scope->GetNameOfCurParentScope();
        auto idAndKindPtr = m_symbols->GetIdAndKindInfo(nameOfParentScope);
        if (!idAndKindPtr)
        {   // even parent scope will invariantly be registered. and the Levelup of '/' is '/'
            throw std::logic_error("Internal error: parent scope not registered");
        }
        return *idAndKindPtr;
    }

    // special high level entry of "RegisterFunction" feature but with pre-treatments related to scope resolution and semantic checks specifics to methods
    IdAndKind& SemanticOrchestrator::RegisterDeportedMethod(UnqualifiedNameView uqName, azslParser::UserDefinedTypeContext* className, AstFuncSig* ctx)
    {
        // extract the type name from the type rule. remember that it can be a reference to an existing UDT, or the inline definition of a new UDT.
        //  note: in the latter case, the syntax is contrived but valid (surprising for C++/java people)
        //        e.g.   bool struct MyS{}::Method() {return true;}
        //        however, the semantics of that expression are not supported. And that's because "struct MyS" is visited
        //        AFTER the method registration, so MyS doesn't exist as a registered symbol yet.
        ExtractedComposedType extracted = ExtractComposedTypeNamesFromAstContext(className);
        verboseCout << ctx->start->getLine() << ": register method: " << className->getText() << "::" << uqName << "\n";
        // lookup for the symbol of the extracted scope
        IdAndKind* scopeIdKind = LookupSymbol(extracted.m_core.m_name);
        if (!scopeIdKind)
        {
            throw AzslcOrchestratorException{ORCHESTRATOR_SCOPE_NOT_FOUND,
                ctx->start, ConcatString("scope ", extracted.m_core.m_name,
                                         " for method ", uqName, " not found")};
        }
        auto [scopeUid, scopeKind] = *scopeIdKind;
        if (!scopeKind.IsKindOneOf(Kind::Class, Kind::ShaderResourceGroup))
        {
            throw AzslcOrchestratorException{ORCHESTRATOR_DEPORTED_METHOD_DEFINITION,
                ctx->start, "Only class and SRG may have deported method definitions"};
        }
        IdentifierUID holdingScope = GetCurrentScopeIdAndKind().first;
        if (! (holdingScope.m_name == "/" || holdingScope == scopeUid))
        {
            throw AzslcOrchestratorException{ORCHESTRATOR_DEFINITION_FOREIGN_SCOPE,
                ctx->start, ConcatString("definition of (", uqName,
                                         ") method of ", scopeUid.m_name, " in foreign scope ", holdingScope.m_name, " is forbidden")};
        }
        // between function name and the entrance into the function scope, the class scope is briefly activated.
        // this way, parameters may be specified as if in the scope of their holding function. e.g `ObjectOfCurrentScope MyClass::MyMethod(ObjectOfMyClass)`
        m_scope->EnterScope(scopeUid.GetName(), ctx->Name->getTokenIndex());
        // now that we're in scope, we can establish the decorated (leaf) identity of this function:
        auto decoratedUqName = UnqualifiedName{ConcatString(uqName, CreateDecorationOfFunction(ctx->functionParams()))};

        bool scopeIsClass = scopeKind.GetKind() == Kind::Class;
        // now let's check if that method was pre-declared in the class/SRG:
        optional<IdentifierUID> optionalIdentifier = scopeIsClass ? scopeKind.GetSubRefAs<ClassInfo>().FindMemberFromLeafName(decoratedUqName)
                                                                  : scopeKind.GetSubRefAs<SRGInfo>().FindMemberFromLeafName(decoratedUqName);
        // note that if the method is not found, we could accept to add it anyway to provide an extension-method feature a la C#.
        // it would be great to require an attribute or a keyword for that though (like [[extends]])
        // for now, it will be forbidden to inject methods in classes from outside.
        if (!optionalIdentifier)
        {
            throw AzslcOrchestratorException{ORCHESTRATOR_NO_DECLERATION, ctx->start,
                ConcatString((scopeIsClass ? "class " : "SRG "), scopeUid.m_name, " doesn't have a declaration for ", decoratedUqName)};
        }
        // verify also the kind of the member
        auto* originalDeclarationAsFunc = m_symbols->GetAsSub<FunctionInfo>(*optionalIdentifier);
        if (!originalDeclarationAsFunc)
        {
            Kind realKindOfOriginallyDeclaredMember = m_symbols->GetIdAndKindInfo(optionalIdentifier->GetName())->second.GetKind();
            throw AzslcOrchestratorException{ORCHESTRATOR_UNEXPECTED_KIND,
                ctx->start, ConcatString((scopeIsClass ? "class " : "SRG "), scopeUid.m_name,
                                         " holds a member ", optionalIdentifier->m_name,
                                         " but it is of kind ", string{ Kind::ToStr(realKindOfOriginallyDeclaredMember) },
                                         " instead of expected ", string{ Kind::ToStr(Kind::Function) })};
        }
        // now we're good.
        // merge the type and the method name and call the classic register. RegisterFunction is going to re-run the decoration so just pass the naked name.
        string nakedJoined = JoinPath(scopeUid.GetName(), uqName, JoinPolicy::EmptyMeansRoot);
        return RegisterFunction(QualifiedNameView{nakedJoined}, ctx, AsFunc::Definition);
    }

    // unqualified-name (UQN) taking version. (expect a relative name to current scope)
    IdAndKind& SemanticOrchestrator::RegisterFunction(UnqualifiedNameView name, AstFuncSig* ctx, AsFunc statementGenre)
    {
        auto fqName = MakeFullyQualified(name);
        return RegisterFunction(fqName, ctx, statementGenre);
    }

    string SemanticOrchestrator::CreateDecorationOfFunction(azslParser::FunctionParamsContext* parametersContext) const
    {
        if (parametersContext == nullptr || parametersContext->Void())
        {
            return "()";
        }
        vector<QualifiedName> typeList;
        auto vectorOfFunctionParams = parametersContext->functionParam();
        typeList.reserve(vectorOfFunctionParams.size());
        // transform the collection into looked-up type names
        for (auto functionParamContext : vectorOfFunctionParams)
        {
            IdentifierUID paramType = LookupType(functionParamContext->type()); // [TODO-GFX][ATOM2627]: change this to CreateExtendedTypeInfo
            typeList.push_back(paramType.GetName());
        }
        return ::AZ::ShaderCompiler::CreateDecorationOfFunction(typeList.begin(), typeList.end());
    }

    QualifiedName SemanticOrchestrator::CreateDecoratedIdentityOfFunction(QualifiedNameView name, azslParser::FunctionParamsContext* parametersContext) const
    {
        return QualifiedName{ConcatString(name, CreateDecorationOfFunction(parametersContext))};
    }

    // qualified-name (FQN) taking version. (pre-resolved scope)
    IdAndKind& SemanticOrchestrator::RegisterFunction(QualifiedNameView fqUndecoratedName, AstFuncSig* ctx, AsFunc statementGenre)
    {
        // parameter validation: check the claim of the caller
        assert((statementGenre == AsFunc::Declaration && Is<azslParser::HlslFunctionDeclarationContext*>(ctx->parent))
            || (statementGenre == AsFunc::Definition  && Is<azslParser::HlslFunctionDefinitionContext*>(ctx->parent)));

        auto line = ctx->Name->getLine();
        verboseCout << line << ": register func: " << fqUndecoratedName;

        // `/f` is undecorated. `/f(?int)` is decorated
        QualifiedName decoratedName = CreateDecoratedIdentityOfFunction(fqUndecoratedName, ctx->functionParams());

        verboseCout << " full identity: " << decoratedName << "\n";

        // validation
        bool isScopeCompositeType = IsScopeStructClassInterface();
        if (statementGenre == AsFunc::Declaration && ctx->ClassName)
        {
            if (isScopeCompositeType)
            {
                throw AzslcOrchestratorException{ORCHESTRATOR_OVERLY_QUALIFIED, ctx->Name,
                    ConcatString(ctx->getText(), " is overly qualified. In-class declarations spawn new identifiers, and don't have to refer to existing symbols.")};
            }
            else
            {
                throw AzslcOrchestratorException{ORCHESTRATOR_DEPORTED_METHOD, ctx->Name,
                    ConcatString(ctx->getText(), "is a deported method declaration, which is considered ill-formed. You can make it a definition (with a body), or delete that statement.")};
            }
        }
        IdAndKind* symbol = m_symbols->GetIdAndKindInfo(decoratedName);
        auto* funcInfo = symbol ? symbol->second.GetSubAs<FunctionInfo>() : nullptr;

        bool alreadyDeclared = !!symbol;
        bool alreadyDefined = funcInfo ? !!funcInfo->m_defNode : false;
        if (!alreadyDeclared)  // brand new function
        {
            symbol = &m_symbols->AddIdentifier(decoratedName, Kind::Function, line);
            // prepare a virgin subinfo
            funcInfo = &symbol->second.GetSubAfterInitAs<Kind::Function>();
        }
        else
        {
            if (statementGenre == AsFunc::Declaration)
            {
                PrintWarning(Warn::W1, ctx->start, "ignored redundant redeclaration of function ", decoratedName,
                             ", ", GetFirstSeenLineMessage(symbol->second));
                funcInfo->m_multiFwds = FMF_FwdDeclRedundancy;
                return *symbol;
            }
            funcInfo->m_multiFwds = FMF_SeenDef;
            auto originalKind = symbol->second.GetKind();
            if (originalKind != Kind::Function)  // verify that we're not transforming a lychee into a melon
            {
                ThrowRedeclarationAsDifferentKind(decoratedName, Kind::Function, symbol->second, line);
            }

            if (alreadyDefined)  // verify that it's not a second definition
            {
                throw AzslcOrchestratorException{ORCHESTRATOR_FUNCTION_ALREADY_DEFINED, ctx->Name,
                    ConcatString("One Definition Rule: function ", symbol->first.m_name,
                                 " is already defined ", GetFirstSeenLineMessage(symbol->second))};
            }

            // this function was already declared before. (like a forward)
            // imagine this scenario:
            //   void f(int i);
            //
            //   void f(int num) {...}
            //
            // If we do nothing, the symbol table will have "f, f/i and f/num" but f/i will be unusable.
            // and worse, f will register two parameters i and num. (because by chance they have different names)
            // Also, recall that
            //   void f(int);
            // Is a legal declaration, arguments don't need to be named, and the declarative functionality is still the same.
            // so for canonicalization we can consider that we never saw the arguments in declarations; but only as soon as we see a definition.
            // if we never store them, we won't be able to emit the declaration in the backend if it is the only thing we ever see.
            for (auto dependent : symbol->second.GetSubAs<FunctionInfo>()->GetParameters(true))
            {   // delete AST-dependents (children) : the arguments.
                if (!dependent.m_varId.IsEmpty())
                {
                    m_symbols->DeleteIdentifier(dependent.m_varId);
                }
            }
            // don't delete the old symbol because it has the seenat table, important to keep track of all occurrences of forward declarations.
            // just delete references to the deleted parameters.
            symbol->second.GetSubRefAs<FunctionInfo>().StashParameters();
            // also some attributes are only parsed at declaration, like override, or static.
            // and inversely, HLSL semantics are only considered at the definition site.

            // push a second apparition record in the ordered elastic symbol list
            // this way, the emitter can emit 2 entities: a declaration at first apparition, and the definition on the second apparition
            m_symbols->m_elastic.m_order.push_back(symbol->first);
        }
        // decompose the Id and Kind of this function
        auto& [newUID, newKind] = *symbol;

        // keep track of original AST node
        if (statementGenre == AsFunc::Definition)
        {
            funcInfo->m_defNode = ctx;
        }
        if (!funcInfo->m_declNode)
        {
            funcInfo->m_declNode = ctx;
        }
        // OR fusion between decl and def sites
        funcInfo->m_mustOverride = funcInfo->m_mustOverride || ctx->Override() != nullptr;
        // return types must match (between redeclaration of this concrete function)
        // at the exception of inconsequential type modifiers (extern, inline, static, volatile, uniform) that we must merge to preserve in case of difference
        ExtendedTypeInfo returnType = CreateExtendedTypeInfo(ctx->type(), {});
        if (alreadyDeclared && funcInfo->m_returnType != returnType)
        {
            throw AzslcOrchestratorException{ORCHESTRATOR_FUNCTION_INCONSISTENT_RETURN_TYPE, ctx->type()->start,
                                             ConcatString("function definition ",  decoratedName, ", ",
                                                          GetFirstSeenLineMessage(symbol->second), ", had a different return type: ",
                                                          funcInfo->m_returnType.GetDisplayName(), ", versus now seen: ", returnType.GetDisplayName())};
        }
        CheckFunctionReturnTypeModifierNotOptionNorRootconstant(returnType.m_qualifiers, line); // throws a diagnostic if needed
        returnType.m_qualifiers.OrMerge(funcInfo->m_returnType.m_qualifiers);
        funcInfo->m_returnType = returnType;
        assert(!funcInfo->m_returnType.IsEmpty());
        if (!funcInfo->m_returnType.IsClassFound())
        {
            PrintWarning(Warn::W2, ctx->type()->start, "return type ", ctx->type()->getText(), " not understood.",
                         " (for function ", decoratedName, ")");
        }

        // try to fetch the overload-set:
        IdAndKind* overloadSetIdKind = m_symbols->GetIdAndKindInfo(fqUndecoratedName);
        OverloadSetInfo* overloadSet = overloadSetIdKind ? overloadSetIdKind->second.GetSubAs<OverloadSetInfo>() : nullptr;
        if (!overloadSet)  // don't exist yet. it must be the first occurrence of this function's core name.
        {
            // create and prepare a brand new overload-set
            overloadSetIdKind = &m_symbols->AddIdentifier(fqUndecoratedName, Kind::OverloadSet, line);
            overloadSet = overloadSetIdKind->second.GetSubAs<OverloadSetInfo>();
            overloadSet->SetSetName(overloadSetIdKind->first); // set own id on it for logical independence (decoupling) in the methods of this object
        }
        // add this concrete function occurrence to the overload-set:
        overloadSet->PushConcreteFunction(newUID, returnType);

        // don't register the parameters here because of two reasons:
        //  - the listener will naturally enter variableDeclarator rule which calls RegisterVar
        //  - the scope needs to be the function scope, and we will enter the scope only after function registration
        if (isScopeCompositeType)    // vocabulary reminder: composite-type = product or sum type = class/struct/union/interface/enum
        {   // we are now in a class-kind scope -> so this function is a method
            funcInfo->m_isMethod = true;
            // access the class kindinfo and add a member:
            auto& classInfo = GetCurrentScopeSubInfoAs<ClassInfo>();
            classInfo.PushMember(newUID, Kind::Function);
            if (classInfo.m_kind == Kind::Interface)
            {
                funcInfo->m_isVirtual = true;
            }
        }
        else if (GetCurrentScopeIdAndKind().second.GetKind() == Kind::ShaderResourceGroup)
        {
            auto& srgInfo = GetCurrentScopeSubInfoAs<SRGInfo>();
            srgInfo.m_functions.push_back(newUID);
        }
        return *symbol;
    }

    IdAndKind& SemanticOrchestrator::RegisterFunctionDeclarationAndAddSeenat(UnqualifiedNameView uqName, AstFuncSig* signature)
    {
        // join scope and uqName (no unqualified lookup here, declarations are authoritative in scope):
        auto qName = MakeFullyQualified(uqName);
        // check existence, because re-declarations are innocent and should not take priority over ones higher up.
        // especially definition, which must be the winning occurrence for registration of identifiers.
        IdAndKind& symbol = RegisterFunction(uqName, signature, AsFunc::Declaration);
        // register a seenat to avoid missing redeclarations as references, because they don't trigger the idExpression rule.
        // (because a new declaration is always a direct Identifier lexer token)
        // a function's reference is incarnated by its name. like its call sites.
        auto location = MakeTokensLocation(signature->Name);
        RegisterSeenat(symbol, location);
        return symbol;
    }

    void SemanticOrchestrator::RegisterEnumerator(azslParser::EnumeratorDeclaratorContext* ctx)
    {
        auto* enumDefinitionCtx = As<azslParser::EnumDefinitionContext*>(ctx->parent->parent);
        bool isScopedEnum = Is<azslParser::ScopedEnumContext*>(enumDefinitionCtx->enumKey());
        UnqualifiedName parentName{(enumDefinitionCtx)->Name->getText()};
        QualifiedName enumQn;
        // reconstruct the identifier of the enumInfo (since current scope may or may not be it, we can't rely on it)
        if (isScopedEnum)
        {
            enumQn = GetCurrentScopeIdAndKind().first.m_name;  // the current scope IS the enum
            assert(enumQn == QualifiedName{JoinPath(m_scope->GetNameOfCurParentScope(), parentName)});  // verify that we can reconstruct it
        }
        else
        {
            enumQn = MakeFullyQualified(parentName); // uses current scope (because the current scope encloses the enum)
        }
        auto& [enumId, parentKindInfo] = *m_symbols->GetIdAndKindInfo(enumQn);
        auto& enumInfo = parentKindInfo.GetSubRefAs<ClassInfo>();

        size_t line            = ctx->Name->getLine();
        auto enumeratorName    = UnqualifiedName{ctx->Name->getText()};
        auto& [uid, var]       = AddIdentifier(enumeratorName, Kind::Variable, line);
        auto& varInfo          = var.GetSubAfterInitAs<Kind::Variable>();
        if (ctx->Value)
        {
            varInfo.m_constVal = FoldEvalStaticConstExprNumericValue(ctx->Value);
        }
        varInfo.m_declNodeEnum = ctx;
        Modifiers modifiers    = Modifiers{StorageFlag::Static} |
            StorageFlag::Const | StorageFlag::Enumerator;

        varInfo.m_typeInfoExt = ExtendedTypeInfo{CreateTypeRefInfo(UnqualifiedNameView{enumQn}, OnNotFoundOrWrongKind::Diagnose), {},
                                                 {modifiers}, {}, {}};
        enumInfo.PushMember(uid, Kind::Variable);
    }

    void SemanticOrchestrator::RegisterSRGSemanticMember(AstSRGSemanticMemberDeclNode* ctx)
    {
        // access SRGSemanticInfo and add a member:
        auto& srgSemanticInfo = GetCurrentScopeSubInfoAs<ClassInfo>();

        if (ctx->Frequency && ctx->FrequencyValue)
        {
            if (auto* intLit = ctx->FrequencyValue->IntegerLiteral())
            {
                size_t line           = ctx->Frequency->getLine();
                auto frequencyId      = ctx->Frequency->getText();
                auto uqNameView       = UnqualifiedNameView{ frequencyId };
                auto& [uid, var]      = AddIdentifier(uqNameView, Kind::Variable, line);
                auto& varInfo         = var.GetSubAfterInitAs<Kind::Variable>();
                varInfo.m_constVal    = FoldEvalStaticConstExprNumericValue(intLit);
                varInfo.m_declNode    = nullptr;
                varInfo.m_typeInfoExt = ExtendedTypeInfo{CreateTypeRefInfo(UnqualifiedNameView{"int"}, OnNotFoundOrWrongKind::Diagnose), {},
                                                         {}, {}, {}, Packing::MatrixMajor::Default };
                srgSemanticInfo.PushMember(uid, Kind::Variable);
            }
        }
        else if (ctx->VariantFallback && ctx->VariantFallbackValue)
        {
            if (auto* intLit = ctx->VariantFallbackValue->IntegerLiteral())
            {
                size_t line           = ctx->VariantFallback->getLine();
                auto variantFallback  = ctx->VariantFallback->getText();
                auto uqNameView       = UnqualifiedNameView{ variantFallback };
                auto& [uid, var]      = AddIdentifier(uqNameView, Kind::Variable, line);
                auto& varInfo         = var.GetSubAfterInitAs<Kind::Variable>();
                varInfo.m_constVal    = FoldEvalStaticConstExprNumericValue(intLit);
                varInfo.m_declNode    = nullptr;
                varInfo.m_typeInfoExt = ExtendedTypeInfo{CreateTypeRefInfo(UnqualifiedNameView{"int"}, OnNotFoundOrWrongKind::Diagnose), {},
                                                         {}, {}, {}, Packing::MatrixMajor::Default };
                srgSemanticInfo.PushMember(uid, Kind::Variable);
            }
        }
    }

    IdAndKind& SemanticOrchestrator::RegisterTypeAlias(string_view newIdentifier, AstType* existingTypeCtx, azslParser::TypeAliasingDefinitionStatementContext* ctx)
    {
        UnqualifiedNameView newId { newIdentifier };
        auto& idKind = AddIdentifier(newId, Kind::TypeAlias, ctx->start->getLine());
        auto& [uid, kinfo]  = idKind;
        TypeAliasInfo& aliasInfo  = kinfo.GetSubAfterInitAs<Kind::TypeAlias>();
        aliasInfo.m_declNode      = ctx;
        aliasInfo.m_canonicalType = CreateExtendedTypeInfo(existingTypeCtx, {});
        if (!aliasInfo.m_canonicalType.IsClassFound())
        {
            throw AzslcOrchestratorException{ORCHESTRATOR_INVALID_TYPEALIAS_TARGET, existingTypeCtx->start,
                ConcatString("target type ", existingTypeCtx->getText() + " not understood in typealias expression")};
        }
        assert(aliasInfo.m_canonicalType.m_coreType.m_typeClass != TypeClass::Alias);
        // further registration in containing scopes
        auto& [curScopeId, curScopeKind] = GetCurrentScopeIdAndKind();
        if (curScopeKind.IsKindOneOf(Kind::Struct, Kind::Class))
        {   // we are now in a struct/class-kind scope -> so this typealias is a member object.
            // access the class kindinfo and add a member:
            // future: check protocol validation with associatedtype
            auto& classInfo = GetCurrentScopeSubInfoAs<ClassInfo>();
            classInfo.PushMember(uid, Kind::TypeAlias);
        }
        return idKind;
    }

    IdAndKind& SemanticOrchestrator::RegisterSRGSemantic(AstSRGSemanticDeclNode* ctx)
    {
        return RegisterStructuredType(ctx, Kind::ShaderResourceGroupSemantic);
    }

    IdAndKind& SemanticOrchestrator::RegisterInterface(AstInterfaceDeclNode* ctx)
    {
        return RegisterStructuredType(ctx, Kind::Interface);
    }

    IdAndKind& SemanticOrchestrator::RegisterClass(AstClassDeclNode* ctx)
    {
        return RegisterStructuredType(ctx, Kind::Class);
    }

    IdAndKind& SemanticOrchestrator::RegisterStruct(AstStructDeclNode* ctx)
    {
        return RegisterStructuredType(ctx, Kind::Struct);
    }

    IdAndKind& SemanticOrchestrator::RegisterEnum(AstEnumDeclNode* ctx)
    {
        return RegisterStructuredType(ctx, Kind::Enum);
    }

    IdAndKind* SemanticOrchestrator::RegisterVar(Token* nameIdentifier, AstUnnamedVarDecl* ctx)
    {
        azslParser::FunctionParamContext* paramCtx = nullptr;
        auto typeCtx                    = ExtractTypeFromUnnamedVariableDeclarator(ctx, &paramCtx);
        auto&& idText                   = nameIdentifier->getText();
        size_t line                     = nameIdentifier->getLine();
        verboseCout << ConcatString(line, ": var decl: ", idText, "\n");
        auto uqNameView                 = UnqualifiedNameView{idText};
        // Register the variable in the symbol table early:
        IdAndKind& symbolRef            = AddIdentifier(uqNameView, Kind::Variable, line);
        auto& [uid, kindInfo]           = symbolRef;
        VarInfo& varInfo                = kindInfo.GetSubRefAs<VarInfo>();
        // Discover array dimensions:
        ArrayDimensions arrayDims;
        TryFoldArrayDimensions(ctx, arrayDims);
        // Finally make the structure to hold all type information from the type context:
        //  this will perform a lookup/resolve type/typeof, extract the qualifiers and compose the data.
        //
        //  note: this syntax: "A A;" is allowed in C++/HLSL but not in AZSL.
        // Because originally in C you can disambiguate lookup using a repetition of the type tag: "struct A A;"
        // To make it work (I argue that it's undesirable though), it would require:
        //  - an evolution of the grammar to tolerate tags
        //  - an inversion between CreateExtendedTypeInfo call AddIdentifier call
        //  - an evolution in the mangling to be able to store 2 symbols of the same name in the same scope
        //        E.g "/!A" for type ::A (bang mark would mirror built-ins "?float")
        varInfo.m_typeInfoExt           = CreateExtendedTypeInfo(typeCtx, arrayDims);
        assert(!varInfo.m_typeInfoExt.IsEmpty());
        // now fillup what we can, and already know, about that variable in the IR:
        varInfo.m_declNode              = ctx;
        varInfo.m_identifier            = uqNameView;

        if (!varInfo.m_typeInfoExt.IsClassFound())
        {
            PrintWarning(Warn::W2, typeCtx->start, "variable type ", typeCtx->getText(), " not understood.",
                         " (for variable ", idText, ")");
        }
        if (varInfo.GetTypeRefInfo().IsInputAttachment(m_lexer))
        {
            if (!varInfo.GetGenericParameterTypeId().IsEmpty()
                && varInfo.GetGenericParameterTypeId().GetNameLeaf() != "float4")
            {
                PrintWarning(Warn::W1, typeCtx->start, typeCtx->getText(), " only float4 is supported on SubpassInput. Mutated to implicit float4 form.");
            }
            // erase the generic type, until the SubpassInputStub type can be templated:
            varInfo.m_typeInfoExt.m_genericParameter = TypeRefInfo{};
        }
        // get enclosing scope:
        auto& [curScopeId, curScopeKind] = GetCurrentScopeIdAndKind();
        bool enclosedBySRG = curScopeKind.GetKind() == Kind::ShaderResourceGroup;

        assert(!curScopeKind.IsKindOneOf(Kind::Enum)); // should use RegisterEnumerator

        // Some semantic checks
        if (varInfo.CheckHasStorageFlag(StorageFlag::Inline))
        {
            throw AzslcOrchestratorException{ORCHESTRATOR_INVALID_INLINED_QUALIFIER, ctx->start,
                "inline qualification on variables is ill-formed"};
        }
        bool global = curScopeId.GetName() == "/";
        bool isOption = varInfo.CheckHasStorageFlag(StorageFlag::Option);
        bool isRootconstant = varInfo.CheckHasStorageFlag(StorageFlag::Rootconstant);
        bool hasExplicitLocalFlag = varInfo.CheckHasAnyStorageFlags({ StorageFlag::Static, StorageFlag::Groupshared });

        if (isRootconstant || isOption)
        {
            if (arrayDims.IsArray())
            {
                throw AzslcOrchestratorException{ORCHESTRATOR_INVALID_NONGLOBAL_OPTION_OR_ROOTCONSTANT, ctx->start,
                    "arrays can not be declared as rootconstants."};
            }
            if (!global)
            {
                throw AzslcOrchestratorException{ORCHESTRATOR_INVALID_NONGLOBAL_OPTION_OR_ROOTCONSTANT, ctx->start,
                    "rootconstant or option qualifier is only accepted at top-level scope"};
            }
            if (hasExplicitLocalFlag)
            {
                throw AzslcOrchestratorException{ORCHESTRATOR_INVALID_QUALIFIER_MIX, ctx->start,
                    "static, groupshared qualifiers cannot be used with the rootconstant or option qualifier"};
            }
            if (varInfo.CheckHasStorageFlag(StorageFlag::Const))
            {
                PrintWarning(Warn::W2, ctx->start, "const ignored in conjunction with rootconstant or option (because already immutable).");
                varInfo.m_typeInfoExt.m_qualifiers.m_flag &= StorageFlag::EnumType(~StorageFlag::Const);
            }
        }

        if (isOption)
        {
            if (!varInfo.m_typeInfoExt.IsClassFound())
            {
                const string message = FormatString("Unknown type '%s' for shader option '%.*s'",
                    varInfo.m_typeInfoExt.GetDisplayName().c_str(), static_cast<int>(uqNameView.size()), uqNameView.data());
                throw AzslcOrchestratorException{ORCHESTRATOR_UNKNOWN_OPTION_TYPE, ctx->start, message};
            }
            // we'll set that here, an option is better flagged as static const for simplicity during emission
            varInfo.m_typeInfoExt.m_qualifiers.m_flag |= StorageFlag::Static;
            varInfo.m_typeInfoExt.m_qualifiers.m_flag |= StorageFlag::Const;
            // we don't do the same for rootconstant because they exist through a ConstantBuffer<STRUCT>
        }

        TypeClass typeClass = varInfo.GetTypeClass();
        if (typeClass == TypeClass::Sampler)
        {
            // let's extract any potential sampler information
            varInfo.m_samplerState = ExtractSamplerState(ctx->variableInitializer());

            // Emit a warning to the user in case we have upgraded the Sampler to ComparisonSampler implicitly
            bool declaredAsSamplerComparison = TypeIsSamplerComparisonState(ctx);
            if (varInfo.m_samplerState->m_isComparison && !declaredAsSamplerComparison)
            {
                PrintWarning(Warn::W3, ctx->start, "Comparison function found, sampler will be upgraded to SamplerComparisonState.\n",
                             "Please use SamplerComparisonState if the use is intended, or remove the comparison function if not.");
            }
            varInfo.m_samplerState->m_isComparison |= declaredAsSamplerComparison;
        }

        if (ctx->packOffsetNode())
        {
            PrintWarning(Warn::W1, ctx->packOffsetNode()->start, "packoffset information ignored");
        }

        bool parentIsFuncDecl = IsParentRuleAFunctionDeclaration(paramCtx);
        bool parentIsFuncDef  = IsParentRuleAFunctionDefinition(paramCtx);
        if (parentIsFuncDef || parentIsFuncDecl)
        {
            // We need to register each newly registered parameter variable ID, in the list of the function subinfo too:
            auto& funcSub = GetCurrentScopeSubInfoAs<FunctionInfo>();
            funcSub.PushParameter(uid, varInfo.m_typeInfoExt, varInfo.m_declNode);
        }

        bool isExtern = !varInfo.StorageFlagIsLocalLinkage(global || enclosedBySRG);
        bool dxilLibraryFlagType = typeClass == TypeClass::LibrarySubobject;  // No need to check any semantic for subobjects. They just enrich the dxil metadata but don't participate in the program.
        if (isExtern && !dxilLibraryFlagType)
        {
            if (global && !varInfo.CheckHasStorageFlag(StorageFlag::Rootconstant))
            {
                throw AzslcOrchestratorException{ORCHESTRATOR_ILLEGAL_GLOBAL_VARIABLE, nameIdentifier,
                    ConcatString(Decorate("'", idText), " extern global variables are ill-formed in AZSL. You might want an internal variable (static or groupshared), a rootconstant, an option, or to put your resource in a ShaderResourceGroup.") };
            }
            if (HasStandardInitializer(ctx))
            {
                throw AzslcOrchestratorException{ORCHESTRATOR_EXTERNAL_VARIABLE_WITH_INITIALIZER, nameIdentifier,
                    ConcatString(Decorate("'", idText), " extern variables can't be initialized from the shader side, since their values are set by bindings.") };
            }
        }

        bool isStaticConst = varInfo.CheckHasAllStorageFlags({ StorageFlag::Static, StorageFlag::Const });
        if (curScopeKind.IsKindOneOf(Kind::Struct, Kind::Class))
        {   // we are now in a struct/class-kind scope -> so this variable is a member object.
            // access the class kindinfo and add a member:
            auto& classInfo = GetCurrentScopeSubInfoAs<ClassInfo>();
            classInfo.PushMember(uid, Kind::Variable);

            if (HasStandardInitializer(ctx) && !isStaticConst)
            {
                throw AzslcOrchestratorException{ ORCHESTRATOR_MEMBER_VARIABLE_WITH_INITIALIZER, nameIdentifier,
                    ConcatString(idText, " default-member-initializers are not supported.") };
            }
        }
        if (curScopeKind.GetKind() == Kind::Interface)
        {
            // this is an impossible case because the parser doesn't accept these constructs.
            // but let's say one day we have an API that allows constructing AST programmatically.
            throw AzslcOrchestratorException{ORCHESTRATOR_ILLEGAL_MEMBER_VARIABLE_IN_INTERFACE,
                nameIdentifier, "member variables in interfaces are forbidden."};
        }
        if (enclosedBySRG)
        {
            FillOutSrgField(polymorphic_downcast<AstNamedVarDecl*>(ctx->parent), varInfo, uid, arrayDims);
        }
        // attempt some level of constant folding from initializers:
        //  note to maintainers: do NOT try to avoid bloat in the verbose stream, by protecting this in `if (ctx->variableInitializer())`
        //                       it will result in the "static no-init-assignment zero initialization" case being <failed> instead of 0.
        varInfo.m_constVal = FoldEvalStaticConstExprNumericValue(varInfo);
        return &symbolRef;
    }

    void SemanticOrchestrator::RegisterNamelessFunctionParameter(azslParser::FunctionParamContext* ctx)
    {
        ArrayDimensions arrayDims;
        TryFoldArrayDimensions(ctx->unnamedVariableDeclarator(), arrayDims);
        auto paramType = CreateExtendedTypeInfo(ctx->type(), arrayDims);
        GetCurrentScopeSubInfoAs<FunctionInfo>().PushParameter({}, paramType, ctx->unnamedVariableDeclarator());
    }

    // Helper to avoid code redundancy for a message that is used in three different places.
    static string GetNonEasyToFoldMessage(const ArrayDimensions& arrayDims)
    {
        return ConcatString("array dimensions must be an easy-to-fold build time constant ( ",
            arrayDims.ToString(),
            " ) in external resource declaration");
    }

    void SemanticOrchestrator::FillOutSrgField(AstNamedVarDecl* ctx, VarInfo& varInfo, IdentifierUID varUid, ArrayDimensions& arrayDims)
    {
        const bool isUnboundedArray = arrayDims.IsUnbounded();
        if (!isUnboundedArray && !arrayDims.AreAllDimsFullyConstantFolded())
        {
            throw AzslcOrchestratorException{ORCHESTRATOR_ILLEGAL_FOLDABLE_ARRAY_DIMENSIONS, ctx->start,
                GetNonEasyToFoldMessage(arrayDims)};
        }

        auto& [srgUid, srgKind] = GetCurrentScopeIdAndKind();
        auto& srgInfo = srgKind.GetSubRefAs<SRGInfo>();

        TypeClass typeClass = varInfo.GetTypeClass();
        assert(typeClass != TypeClass::Alias);

        string errorMessage;
        if (!m_unboundedArraysValidator.CheckFieldCanBeAddedToSrg(isUnboundedArray, srgUid, varUid, varInfo, typeClass, &errorMessage))
        {
            throw AzslcOrchestratorException{ORCHESTRATOR_UNBOUNDED_RESOURCE_ISSUE, ctx->start, errorMessage};
        }

        if (isUnboundedArray)
        {
            srgInfo.m_unboundedArrays.push_back(varUid);
        }

        if (typeClass == TypeClass::ConstantBuffer)
        {
            srgInfo.m_CBs.push_back(varUid);
        }
        else if (typeClass == TypeClass::Sampler)
        {
            srgInfo.m_samplers.push_back(varUid);
        }
        else if (IsViewType(typeClass))
        {
            srgInfo.m_srViews.push_back(varUid);
        }
        else if (!varInfo.StorageFlagIsLocalLinkage(true))
        {
            if (isUnboundedArray)
            {
                throw AzslcOrchestratorException{ORCHESTRATOR_ILLEGAL_FOLDABLE_ARRAY_DIMENSIONS, ctx->start,
                    GetNonEasyToFoldMessage(arrayDims)};
            }
            if (!varInfo.GetTypeRefInfo().IsPackable())
            {
                throw AzslcOrchestratorException{ORCHESTRATOR_NON_PACKABLE_TYPE_IN_SRG_CONSTANT, ctx->start,
                    ConcatString(varInfo.GetTypeId().m_name,
                                 " is of kind ",
                                 TypeClass::ToStr(varInfo.GetTypeClass()),
                                 " which is a non packable kind of type.")};
            }
            auto& classInfo = srgInfo.m_implicitStruct;
            classInfo.PushMember(varUid, Kind::Variable);
        }
        else
        {
            if (isUnboundedArray)
            {
                throw AzslcOrchestratorException{ORCHESTRATOR_ILLEGAL_FOLDABLE_ARRAY_DIMENSIONS, ctx->start,
                    GetNonEasyToFoldMessage(arrayDims)};
            }
            assert(varInfo.StorageFlagIsLocalLinkage(true));
            srgInfo.m_nonexternVariables.push_back(varUid);
        }
        varInfo.m_srgMember = true;
    }

    SamplerStateDesc SemanticOrchestrator::ExtractSamplerState(AstVarInitializer* ctx)
    {
        if (ctx == nullptr || ctx->standardVariableInitializer())
        {
            SamplerStateDesc defaultState;
            defaultState.m_isDynamic = true;  // no initializer, or variable assignation both denotes a dynamic sampler.
            return defaultState;
        }
        // Antlr auto-generates tons of methods for every rule, so we have to walk and parse them here.
        // We declare a couple of helper methods to simplify the rest of the resolve function:

        auto GetAddressMode = [](azslParser::AddressModeEnumContext* ctx) -> SamplerStateDesc::AddressMode
        {
            return ctx->ADDRESS_MODE_WRAP()   ? SamplerStateDesc::AddressMode::Wrap
                 : ctx->ADDRESS_MODE_CLAMP()  ? SamplerStateDesc::AddressMode::Clamp
                 : ctx->ADDRESS_MODE_MIRROR() ? SamplerStateDesc::AddressMode::Mirror
                 : ctx->ADDRESS_MODE_BORDER() ? SamplerStateDesc::AddressMode::Border
                 :                              SamplerStateDesc::AddressMode::MirrorOnce;
        };

        auto GetCompFunc = [](azslParser::ComparisonFunctionEnumContext* ctx) -> SamplerStateDesc::ComparisonFunc
        {
            return ctx->COMPARISON_FUNCTION_NEVER()         ? SamplerStateDesc::ComparisonFunc::Never
                 : ctx->COMPARISON_FUNCTION_NEVER()         ? SamplerStateDesc::ComparisonFunc::Never
                 : ctx->COMPARISON_FUNCTION_LESS()          ? SamplerStateDesc::ComparisonFunc::Less
                 : ctx->COMPARISON_FUNCTION_EQUAL()         ? SamplerStateDesc::ComparisonFunc::Equal
                 : ctx->COMPARISON_FUNCTION_LESS_EQUAL()    ? SamplerStateDesc::ComparisonFunc::LessEqual
                 : ctx->COMPARISON_FUNCTION_GREATER()       ? SamplerStateDesc::ComparisonFunc::Greater
                 : ctx->COMPARISON_FUNCTION_NOT_EQUAL()     ? SamplerStateDesc::ComparisonFunc::NotEqual
                 : ctx->COMPARISON_FUNCTION_GREATER_EQUAL() ? SamplerStateDesc::ComparisonFunc::GreaterEqual
                 :                                            SamplerStateDesc::ComparisonFunc::Always;
        };

        auto GetRedcType = [](azslParser::ReductionTypeEnumContext* ctx) -> SamplerStateDesc::ReductionType
        {
            return ctx->REDUCTION_TYPE_FILTER()     ? SamplerStateDesc::ReductionType::Filter
                 : ctx->REDUCTION_TYPE_COMPARISON() ? SamplerStateDesc::ReductionType::Comparison
                 : ctx->REDUCTION_TYPE_MINIMUM()    ? SamplerStateDesc::ReductionType::Minimum
                 :                                    SamplerStateDesc::ReductionType::Maximum;
        };

        auto GetFilterType = [](azslParser::FilterModeEnumContext* ctx) -> SamplerStateDesc::FilterMode
        {
            return ctx->FILTER_MODE_LINEAR() ? SamplerStateDesc::FilterMode::Linear : SamplerStateDesc::FilterMode::Point;
        };

        auto GetBorderColor = [](azslParser::BorderColorEnumContext* ctx) -> SamplerStateDesc::BorderColor
        {
            return ctx->BORDER_COLOR_OPAQUE_BLACK()      ? SamplerStateDesc::BorderColor::OpaqueBlack
                 : ctx->BORDER_COLOR_TRANSPARENT_BLACK() ? SamplerStateDesc::BorderColor::TransparentBlack
                 :                                         SamplerStateDesc::BorderColor::OpaqueWhite;
        };

        // Now proceed with resolving the sampler state
        SamplerStateDesc desc;
        auto samplerOpts = ctx->samplerBodyDeclaration()->samplerMemberDeclaration();
        for (auto samplerOption : samplerOpts)
        {
            if (auto maxAnisotropyOption = samplerOption->maxAnisotropyOption())
            {
                auto maxAnisoVal = FoldEvalStaticConstExprNumericValue(maxAnisotropyOption->IntegerLiteral());
                desc.m_anisotropyMax = static_cast<uint32_t>(ExtractValueAsInt64(maxAnisoVal));
                desc.m_anisotropyEnable = true;
            }

            else if (auto minLodOption = samplerOption->minLodOption())
            {
                auto minLODVal = FoldEvalStaticConstExprNumericValue(minLodOption->FloatLiteral(), false);
                desc.m_mipLodMin = ExtractValueAsFloat(minLODVal);
            }

            else if (auto maxLodOption = samplerOption->maxLodOption())
            {
                auto maxLODVal = FoldEvalStaticConstExprNumericValue(maxLodOption->FloatLiteral(), false);
                desc.m_mipLodMax = ExtractValueAsFloat(maxLODVal);
            }

            else if (auto mipLodBiasOption = samplerOption->mipLodBiasOption())
            {
                auto biasVal = FoldEvalStaticConstExprNumericValue(mipLodBiasOption->FloatLiteral(), false);
                desc.m_mipLodBias = ExtractValueAsFloat(biasVal);
            }

            else if (auto minFilterOption = samplerOption->minFilterOption())
            {
                desc.m_filterMin = GetFilterType(minFilterOption->filterModeEnum());
            }

            else if (auto magFilterOption = samplerOption->magFilterOption())
            {
                desc.m_filterMag = GetFilterType(magFilterOption->filterModeEnum());
            }

            else if (auto mipFilterOption = samplerOption->mipFilterOption())
            {
                desc.m_filterMip = GetFilterType(mipFilterOption->filterModeEnum());
            }

            else if (auto reductionTypeOption = samplerOption->reductionTypeOption())
            {
                desc.m_reductionType = GetRedcType(reductionTypeOption->reductionTypeEnum());
            }

            else if (auto comparisonFunctionOption = samplerOption->comparisonFunctionOption())
            {
                desc.m_comparisonFunc = GetCompFunc(comparisonFunctionOption->comparisonFunctionEnum());
                desc.m_isComparison = true;
            }

            else if (auto addressUOption = samplerOption->addressUOption())
            {
                desc.m_addressU = GetAddressMode(addressUOption->addressModeEnum());
            }

            else if (auto addressVOption = samplerOption->addressVOption())
            {
                desc.m_addressV = GetAddressMode(addressVOption->addressModeEnum());
            }

            else if (auto addressWOption = samplerOption->addressWOption())
            {
                desc.m_addressW = GetAddressMode(addressWOption->addressModeEnum());
            }

            else if (auto borderColorOption = samplerOption->borderColorOption())
            {
                desc.m_borderColor = GetBorderColor(borderColorOption->borderColorEnum());
            }
        }
        return desc;
    }

    IdAndKind& SemanticOrchestrator::RegisterSRG(AstSRGDeclNode* ctx)
    {
        auto const& idText = ctx->Name->getText();
        size_t line        = ctx->Name->getLine();
        verboseCout << line << ": srg decl: " << idText << "\n";
        auto uqNameView    = UnqualifiedNameView{ idText };
        IdAndKind* srgSym  = LookupSymbol(uqNameView);
        if (srgSym) // already exists
        {
            if (ctx->Partial())  // case of a syntactically valid extension
            {
                SRGInfo& srgInfo = srgSym->second.GetSubRefAs<SRGInfo>();
                if (!srgInfo.IsPartial())
                {
                    throw AzslcOrchestratorException{ORCHESTRATOR_TRYING_TO_EXTEND_NOT_PARTIAL_SRG,
                        ctx->Partial()->getSymbol(), ConcatString("Cannot extend ShaderResourceGroup ", uqNameView, " ",
                                                                  GetFirstSeenLineMessage(srgSym->second),
                                                                  " because its original declaration isn't 'partial'")};
                }
                return *srgSym;  // valid: both original and current SRG declaration statements carry partial.
            }
            throw AzslcOrchestratorException{ORCHESTRATOR_ODR_VIOLATION,
                ctx->Name, ConcatString("ShaderResourceGroup ", uqNameView, " already exists, ", GetFirstSeenLineMessage(srgSym->second),
                                        ". Consider using the 'partial' keyword (on both declaration sites) to extend a ShaderResourceGroup.")};
        }
        if (!ctx->Partial() && !ctx->Semantic)
        {
            throw AzslcOrchestratorException{ORCHESTRATOR_INVALID_SEMANTIC_DECLARATION,
                ctx->Name, "A semantic is mandatory on the declaration of a non-partial ShaderResourceGroup."};
        }
        auto& symbol       = AddIdentifier(uqNameView, Kind::ShaderResourceGroup, line);
        // now fillup what we can about the kindinfo:
        auto& [uid, info]  = symbol;
        SRGInfo& srgInfo   = info.GetSubAfterInitAs<Kind::ShaderResourceGroup>();
        srgInfo.m_declNode = ctx;
        srgInfo.m_implicitStruct.m_kind = Kind::Struct;
        return symbol;
    }

    void SemanticOrchestrator::RegisterBases(azslParser::BaseListContext* ctx)
    {
        using namespace std::string_literals;

        // reconstruct the "/C" in "class C : b0, b1..." (since the current scope is not yet /C)
        auto* classDefCtx = polymorphic_downcast<AstClassDeclNode*>(ctx->parent);
        IdentifierUID targetClassUid{MakeFullyQualified(UnqualifiedNameView{classDefCtx->Name->getText()})};
        auto* targetClassInfo = m_symbols->GetAsSub<ClassInfo>(targetClassUid);

        for (auto& idexpr : ctx->idExpression())
        {
            UnqualifiedName baseName = ExtractNameFromIdExpression(idexpr);
            auto baseSymbol = LookupSymbol(baseName);
            if (!baseSymbol)
            {
                throw AzslcOrchestratorException{ORCHESTRATOR_UNSPECIFIED_BASE_SYMBOL,
                    ctx->start, ConcatString("Base symbol "s, baseName, " not found")};
            }
            // register base in the class info IR
            targetClassInfo->PushBase(baseSymbol->first);
        }
    }

    void SemanticOrchestrator::RegisterSeenat(IdAndKind& idPair, const TokensLocation& location)
    {
        auto& [uid, info] = idPair;
        Seenat seenat;
        seenat.m_referredDefinition = uid;
        seenat.m_where              = location;
        info.GetSeenats().emplace_back(seenat);

        const string verboseMessage = ConcatString(seenat.m_where.m_line, ": seenat registered for ", uid.m_name, " at col ", seenat.m_where.m_charPos + 1, "\n");
        verboseCout << verboseMessage;
    }

    void SemanticOrchestrator::RegisterSeenat(AstIdExpr* ctx)
    {
        RegisterSeenat(ctx, GetCurrentScopeIdAndKind().first.GetName());
    }

    // e.g. provided "int a; X GetX(int);" then from expression "(a, GetX(2), true)" MangleArgumentList returns "(?int,/X,?bool)"
    string SemanticOrchestrator::MangleArgumentList(azslParser::ArgumentListContext* ctx) const
    {
        // resolve all argument types.
        vector<QualifiedName> resolvedArguments;
        auto vectorOfExpressions = ctx->arguments() ? ctx->arguments()->expression() : vector<AstExpr*>{};
        resolvedArguments.reserve(vectorOfExpressions.size());
        for (AstExpr* expression : vectorOfExpressions)
        {
            QualifiedName typeName = TypeofExpr(expression);
            resolvedArguments.push_back(typeName);
        }
        return ::AZ::ShaderCompiler::CreateDecorationOfFunction(resolvedArguments.begin(), resolvedArguments.end());
    }

    bool SemanticOrchestrator::HasAnyDefaultParameterValue(const IdentifierUID& functionUid) const
    {
        auto* funcInfo = m_symbols->GetAsSub<FunctionInfo>(functionUid);
        return funcInfo ? funcInfo->HasAnyDefaultParameterValue() : false;
    }

    IdAndKind* SemanticOrchestrator::ResolveOverload(IdAndKind* maybeOverloadSet, azslParser::ArgumentListContext* argumentListCtx) const
    {
        IdAndKind* toReturn = maybeOverloadSet;
        if (maybeOverloadSet && maybeOverloadSet->second.GetKind() == Kind::OverloadSet)
        {
            string mangledArgList = argumentListCtx ? MangleArgumentList(argumentListCtx) : "";
            auto& setInfo = maybeOverloadSet->second.GetSubRefAs<OverloadSetInfo>();
            // attempt direct matching or arity matching
            IdentifierUID concrete = setInfo.GetConcreteFunctionThatMatchesArgumentList(mangledArgList);
            if (concrete.IsEmpty())  // failure case
            {
                std::stringstream message;
                message << " unable to match arguments " << mangledArgList << " to a registered overload. candidates are:\n";
                setInfo.ForEach([&](auto&& uid){ message << uid.GetName() << "\n"; });
                if (setInfo.HasHomogeneousReturnType())
                {
                    verboseCout << (argumentListCtx ? std::to_string(argumentListCtx->start->getLine()) : "")
                                << message.str() << " It is not an error since that overload-set has homogeneous return type\n";  // at this point of the source. further declaration can change that.
                }
                else
                {
                    throw AzslcOrchestratorException{ORCHESTRATOR_OVERLOAD_RESOLUTION_HARD_FAILURE, argumentListCtx ? argumentListCtx->start : nullptr,
                                                     ConcatString(message.str(), " This is an error because functions belonging to this overload-set have heterogeneous return types.\n",
                                                                  "Consider using type-casts to help type resolution.")};
                }
            }
            else
            {
                toReturn = m_symbols->GetIdAndKindInfo(concrete.GetName());
            }
        }
        return toReturn;
    }

    void SemanticOrchestrator::RegisterSeenat(AstIdExpr* ctx, QualifiedNameView startupScope)
    {
        auto* argumentList = GetArgumentListIfBelongsToFunctionCall(ctx);  // no need to execute that in the loop body, extracted up-here.

        // an id-expression is made of nested parts: stuff::thing::leaf
        // for each part we need to register a reference to the symbol the nested name refers to.
        // so we loop over the expression, reconstructing by appending part by part so correctly qualify each nested part for lookup.
        // the startupScope is not the first path of the path of each element, but the CONTEXT from which we start the lookup.
        // the lookup mechanism must sill execute. Just not necessarily from the current scope as startup context.
        string partialExpression;
        ForEachIdExpressionPart(ctx, [&](const IdExpressionPart& part)
                                    {   // this is not Schlemiel the painter
                                        partialExpression += part.GetAZIRMangledText();
                                        if (!part.IsScopeToken()) // scope token is the SRO "::"
                                        {
                                            auto* idToKind = ResolveOverload(m_symbols->LookupSymbol(startupScope, UnqualifiedNameView{partialExpression}),
                                                                             argumentList);
                                            if (idToKind)
                                            {
                                                auto tl = MakeTokensLocation(ctx, part.m_token);
                                                RegisterSeenat(*idToKind, tl);
                                            }
                                            else
                                            {
                                                DiagnoseUndeclaredSub(ctx->start, startupScope, partialExpression);
                                            }
                                        }
                                    });
    }

    void SemanticOrchestrator::DiagnoseUndeclaredSub(Token* atToken, QualifiedNameView startupScope, string partialName) const
    {
        // check if we can help the user a bit, that will avoid long pondering when DXC refuses to build something that broke through translation.
        auto parent = GetParentName(partialName);
        bool hasParent = !parent.empty();
        // try to get the parent to see if we can say something special.
        auto* idToKind = m_symbols->LookupSymbol(startupScope, UnqualifiedNameView{parent});
        bool parentFound = hasParent && idToKind != nullptr;
        if (parentFound)
        {
            auto& [id, kind] = *idToKind;
            if (kind.GetKind() == Kind::Enum)
            {
                bool isScopedEnum = kind.GetSubRefAs<ClassInfo>().Get<EnumerationInfo>()->m_isScoped;
                if (!isScopedEnum)
                {
                    PrintWarning(Warn::W1, atToken, "in AZSL, non-class enumeration ", parent, " can't qualify names.");
                    return; // job done
                }
            }
            PrintWarning(Warn::W3, atToken, "undeclared sub-symbol in idexpression: ", parent, " was found, but not ", partialName);
        }
        else
        {   // that warning is probably going to fire a tad too much. repeated for all left-over elements on the right of an expression that failed early.
            // eg  i_did_a_typo_here_omg::not_found::not_found::not_found... you see the point.
            // but being here is our only chance to grab lone identifiers (completely unqualified).
            // so protect it for only that case.
            if (!hasParent)
            {
                PrintWarning(Warn::W3, atToken, "undeclared sub-symbol in idexpression: ", partialName);
            }
        }
    }

    pair<bool, QualifiedName> SemanticOrchestrator::VerifyLHSExprOfMAExprIsValid(azslParser::MemberAccessExpressionContext* ctx) const
    {
        return VerifyTypeIsScopeComposable(ctx->LHSExpr);
    }

    //! Member Access Expression (MAE) such as A.B is a scoped lookup. (A is the scope and B is the composition)
    //! Typeof Expression such as typeof(A)::B is a scoped lookup. (typeof(A) is the scope and B is the composition)
    //! returns the looked up scope
    pair<bool, QualifiedName> SemanticOrchestrator::VerifyTypeIsScopeComposable(azslParser::ExpressionContext* typeScopeAnyExpression) const
    {
         return VerifyTypeIsScopeComposable(TypeofExpr(typeScopeAnyExpression), typeScopeAnyExpression->getText(), typeScopeAnyExpression->start->getLine());
    }

    //! same function as above for already resolved typeof
    pair<bool, QualifiedName> SemanticOrchestrator::VerifyTypeIsScopeComposable(QualifiedNameView lhsTypeName, optional<string> lhsExpressionText/*= none*/, optional<size_t> line/*= none*/) const
    {
        // (generalized) member-access-expressions can only work on types with members:
        // any UDT: struct, enum, class, interface. Any scope; srg, function. Any type-like: typeof, typedef (because they get collapsed)
        auto lhsSymbol = m_symbols->GetIdAndKindInfo(lhsTypeName);
        bool valid = true;
        if (!lhsSymbol)
        {
            PrintWarning(Warn::W2, line, "unresolved member access ",
                         "on undeclared type ", lhsTypeName,
                         (lhsExpressionText ? " (of expression " + *lhsExpressionText + ")" : ""), ")");
            valid = false;
        }
        else // left side symbol exists
        {
            const KindInfo& lhsKindInfo = lhsSymbol->second;
            if (!lhsKindInfo.IsKindOneOf(Kind::Namespace, Kind::Class, Kind::Struct, Kind::Enum, Kind::Interface, Kind::ShaderResourceGroup, Kind::Function))
            {
                auto outputStream = lhsKindInfo.GetKind() == Kind::Type ? verboseCout : warningCout;
                // registered predefined types can have members, but we don't know them -> not important. But anything else is very likely an ill-formed source.
                PrintWarning(outputStream, Warn::W1, line, none, "ill-formed semantics: access of member ",
                             " on an unsupported kind ", Kind::ToStr(lhsKindInfo.GetKind()),
                             " (of believed type ", lhsSymbol->first.GetName(),
                             (lhsExpressionText ? " from expression " + *lhsExpressionText : ""), ")");
                valid = false;
            }
        }
        return {valid, lhsTypeName};
    }

    QualifiedName SemanticOrchestrator::ComposeMemberNameWithScopeAndGetType(QualifiedName scopingType, AstIdExpr* rhsMember) const
    {
        // get the symbol name we want to lookup on the right hand side:
        UnqualifiedName rhsName = ExtractNameFromIdExpression(rhsMember);
        // look it up from the scope of the lhs's type: (this behavior is explained in https://stackoverflow.com/questions/56253767)
        auto fullyResolved = m_symbols->LookupSymbol(scopingType, rhsName);
        if (!fullyResolved)
        {
            PrintWarning(Warn::W3, rhsMember->start, "type tracking fail: ", rhsName, " is not a member of ", scopingType);
        }
        else // rhs symbol exists
        {   // let's extract its type and return that.
            return GetTypeName(fullyResolved);
        }
        return {"<fail>"};
    }

    QualifiedName SemanticOrchestrator::TypeofExpr(azslParser::ExpressionContext* ctx) const
    {
        try
        {
            return VisitFirstNonNull([this](auto* ctx) { return TypeofExpr(ctx); },
                                     As<AstIdExpr*>(ctx),
                                     As<azslParser::IdentifierExpressionContext*>(ctx),
                                     As<azslParser::ParenthesizedExpressionContext*>(ctx),
                                     As<azslParser::CastExpressionContext*>(ctx),
                                     As<azslParser::MemberAccessExpressionContext*>(ctx),
                                     As<azslParser::FunctionCallExpressionContext*>(ctx),
                                     As<azslParser::ArrayAccessExpressionContext*>(ctx),
                                     As<azslParser::ConditionalExpressionContext*>(ctx),
                                     As<azslParser::AssignmentExpressionContext*>(ctx),
                                     As<azslParser::NumericConstructorExpressionContext*>(ctx),
                                     As<azslParser::LiteralExpressionContext*>(ctx),
                                     As<azslParser::LiteralContext*>(ctx),
                                     As<azslParser::PrefixUnaryExpressionContext*>(ctx),
                                     As<azslParser::PostfixUnaryExpressionContext*>(ctx),
                                     As<azslParser::BinaryExpressionContext*>(ctx));
        }
        catch (AllNull&)
        {
            verboseCout << ctx->start->getLine() << ": unsupported expression in typeof: " << typeid(ctx).name() << "\n";
            return {"<fail>"};
        }
    }

    QualifiedName SemanticOrchestrator::TypeofExpr(azslParser::PrefixUnaryExpressionContext* ctx) const
    {
        // among all possibilities Plus|Minus|Not|Tilde|PlusPlus|MinusMinus
        // only "Not" returns a bool, the rest is transparent and returns the same type as rhs
        return ctx->prefixUnaryOperator()->start->getType() == azslLexer::Not ? MangleScalarType("bool")
            : TypeofExpr(ctx->Expr);
    }

    QualifiedName SemanticOrchestrator::TypeofExpr(azslParser::PostfixUnaryExpressionContext* ctx) const
    {
        return TypeofExpr(ctx->Expr); // in case of x++ or x-- the type is the type of x.
    }

    QualifiedName SemanticOrchestrator::TypeofExpr(azslParser::BinaryExpressionContext* ctx) const
    {
        using lex = azslLexer;
        auto boolResultOperators = {lex::Less, lex::Greater, lex::LessEqual, lex::GreaterEqual, lex::NotEqual, lex::AndAnd, lex::OrOr};
        if (IsIn(ctx->binaryOperator()->start->getType(), boolResultOperators))
        {
            return MangleScalarType("bool");
        }
        QualifiedName lhs = TypeofExpr(ctx->Left);
        QualifiedName rhs = TypeofExpr(ctx->Right);
        TypeRefInfo typeInfoLhs = CreateTypeRefInfo(UnqualifiedNameView{lhs});  // We tolerate a cast here because GetTypeRefInfo was designed to lookup types, but TypeofExpr has already looked up the type.
        TypeRefInfo typeInfoRhs = CreateTypeRefInfo(UnqualifiedNameView{rhs});
        if (typeInfoLhs.m_arithmeticInfo.IsEmpty() || typeInfoRhs.m_arithmeticInfo.IsEmpty())
        {   // Case that shouldn't work in AZSL yet (but may work in HLSL2021)
            //  -> UDT operator (would need support of operator overloading).
            // We arbitrarily assume a result "type of left expression".
            // (what we would really need to do is go get the return type of the overloaded operator)
            return lhs;
        } // After this `if`, both sides are arithmetic types (scalar, vector, matrix).
        // "matrix op vector" (or commutated) is a forbidden case,
        //   e.g it won't do Y=MX (m*v->v), nor dotproduct-ing vectors for that matter (v*v->scalar)
        // It will do piecewise `op` and return more or less the same type as the operands.
        // In case of dimension differences, it will truncate to the smaller type
        //   e.g float2 + float3 results in float2 with .z lost in implicit cast
        //      same for float2x3 * float2x2 (results in float2x2)
        // We assume that for all non bool ops: * + - / % ^ | & << >>
        return PromoteTruncateWith({typeInfoLhs.m_arithmeticInfo, typeInfoRhs.m_arithmeticInfo}).GenTypeId();
    }

    QualifiedName SemanticOrchestrator::TypeofExpr(azslParser::ExpressionExtContext* ctx) const
    {
        return VisitFirstNonNull([this](auto* ctx) { return TypeofExpr(ctx); },
                                 As<azslParser::OtherExpressionContext*>(ctx),
                                 As<azslParser::CommaExpressionContext*>(ctx));
    }

    QualifiedName SemanticOrchestrator::TypeofExpr(azslParser::OtherExpressionContext* ctx) const
    {
        return TypeofExpr(ctx->Expr);
    }

    QualifiedName SemanticOrchestrator::TypeofExpr(AstType* ctx) const
    {
        return LookupType(ctx).GetName();
    }

    QualifiedName SemanticOrchestrator::TypeofExpr(AstIdExpr* ctx) const
    {
        // idExpression will represent registered symbol. if not, it's a fail.
        UnqualifiedName uqName = ExtractNameFromIdExpression(ctx);
        auto lookup = ResolveOverload(LookupSymbol(uqName), GetArgumentListIfBelongsToFunctionCall(ctx));
        if (!lookup)
        {
            verboseCout << ctx->start->getLine() << ": can't find typeof " << uqName << "\n";
            return {"<fail>"};
        }
        else
        {
            return GetTypeName(lookup);  // this call is often the leaf of the TypeofExpr call tree.
        }
    }

    QualifiedName SemanticOrchestrator::TypeofExpr(azslParser::IdentifierExpressionContext* ctx) const
    {
        return TypeofExpr(ctx->idExpression());
    }

    QualifiedName SemanticOrchestrator::TypeofExpr(azslParser::MemberAccessExpressionContext* ctx) const
    {
        // "LHS.RHS"
        // typeof(LHS.RHS) is the type of RHS
        // start with a simple case -> rhs is absolute (e.g. "stuff.::A::f()" ::A::f is absolute)
        bool rhsIsAbsolute = ctx->Member->qualifiedId() && ctx->Member->qualifiedId()->nestedNameSpecifier()->GlobalSROToken;
        if (rhsIsAbsolute)
        {
            return TypeofExpr(ctx->Member /*idExpression*/);
        }
        // otherwise, RHS is relative to LHS -> it's a member of typeof(LHS)
        auto [valid, lhsType] = VerifyLHSExprOfMAExprIsValid(ctx);
        return valid ? ComposeMemberNameWithScopeAndGetType(lhsType, ctx->Member)
                     : QualifiedName{"<fail>"};
    }

    QualifiedName SemanticOrchestrator::TypeofExpr(azslParser::FunctionCallExpressionContext* ctx) const
    {
        // Function call grammar is "Expr arglist" so we need to extract the typeof Expr.
        // which must be a function. (a concrete well resolved function, or an overload set under some conditions)
        // then access its return type and we can return that.
        // it can get complicated in case of overloads.
        auto function{TypeofExpr(ctx->Expr)};
        IdAndKind* symbol = m_symbols->GetIdAndKindInfo(function);
        symbol = ResolveOverload(symbol, ctx->argumentList());
        if (!symbol || !symbol->second.IsKindOneOf(Kind::Function, Kind::OverloadSet))
        {
            return {"<fail>"};
        }
        return GetTypeName(symbol, ForFunctionGetType::Returned);
    }

    QualifiedName SemanticOrchestrator::TypeofExpr(azslParser::ArrayAccessExpressionContext* ctx) const
    {   // typeof(myvariable[3]) is typeof(myvariable) minus brackets.  (e.g. int[] -> int)
        // But because of a limitation, today the type of array variable is not described as type[], but only type
        // which means it is enough today to just return typeof(myvariable).
        // Again, this is loose since array access is not going to be semantically validated.
        //  (if you call it on a function for example, it should fail validation; but that's not the mandate of azslc)
        return TypeofExpr(ctx->Expr);
    }

    QualifiedName SemanticOrchestrator::TypeofExpr(azslParser::ParenthesizedExpressionContext* ctx) const
    {   // parenthesis don't change the type, it's just a syntax precedence thing. so just forward to the inner expression.
        return TypeofExpr(ctx->Expr);
    }

    QualifiedName SemanticOrchestrator::TypeofExpr(azslParser::CastExpressionContext* ctx) const
    {   // typeof( (type)expr ) is typeof(type) so just forward to that sub rule.
        // Note that the cast-expression can optionally specify array-ranks, but they are ignored today.
        return TypeofExpr(ctx->type());
    }

    QualifiedName SemanticOrchestrator::TypeofExpr(azslParser::ConditionalExpressionContext* ctx) const
    {   // typeof(a ? b : c) is typeof(b) because b and c are forbidden to differ in type. DXC/clang is going to see to it.
        return TypeofExpr(ctx->TrueExpr);
    }

    QualifiedName SemanticOrchestrator::TypeofExpr(azslParser::AssignmentExpressionContext* ctx) const
    {   // typeof(a = b) is typeof(a) because an assignment returns a reference to the left-hand-side as rvalue for the enclosing expression.
        return TypeofExpr(ctx->Left);
    }

    QualifiedName SemanticOrchestrator::TypeofExpr(azslParser::NumericConstructorExpressionContext* ctx) const
    {   // typeof(float2(0,0)) is float2
        return LookupType(ctx->scalarOrVectorOrMatrixType()).GetName();
    }

    QualifiedName SemanticOrchestrator::TypeofExpr(azslParser::TypeofExpressionContext* ctx) const
    {   // typeof(typeof(..)) is typeof(..) | and typeof(A)::id is type of the symbol composed by `lookup-of-A`/id
        auto leftType = ctx->Expr ? TypeofExpr(ctx->Expr)
                                  : TypeofExpr(ctx->type());
        if (ctx->SubQualification)
        {
            auto [valid, lhsType] = VerifyTypeIsScopeComposable(leftType,
                                                                ctx->Expr ? ctx->Expr->getText()
                                                                          : ctx->type()->getText(),
                                                                ctx->start->getLine());
            return valid ? ComposeMemberNameWithScopeAndGetType(lhsType, ctx->SubQualification)
                         : QualifiedName{"<fail>"};
        }
        return leftType;
    }

    QualifiedName SemanticOrchestrator::TypeofExpr(azslParser::LiteralExpressionContext* ctx) const
    {
        return TypeofExpr(ctx->literal());
    }

    QualifiedName SemanticOrchestrator::TypeofExpr(azslParser::LiteralContext* ctx) const
    {
        // verifies that last or 1-before-last characters are a particular literal suffix. like in "56ul"
        auto hasSuffix = [](auto node, char s){return tolower(node->getText().back()) == s || tolower(Slice(node->getText(), -3, -2) == s);};

        if (ctx->True() || ctx->False())
        {
            return MangleScalarType("bool");
        }
        else if (auto* integerLiteral = ctx->IntegerLiteral())
        {
            return hasSuffix(integerLiteral, 'u') ? MangleScalarType("uint")
                 : MangleScalarType("int");
        }
        else if (auto* floatLiteral = ctx->FloatLiteral())
        {
            return hasSuffix(floatLiteral, 'h') ? MangleScalarType("half")
                 : hasSuffix(floatLiteral, 'l') ? MangleScalarType("double")
                 : MangleScalarType("float");
        }
        return {"<fail>"};
    }

    QualifiedName SemanticOrchestrator::TypeofExpr(azslParser::CommaExpressionContext* ctx) const
    {
        // comma returns whatever is last
        return TypeofExpr(ctx->Right);
    }

    bool SemanticOrchestrator::TryFoldArrayDimensions(AstUnnamedVarDecl* ctx, ArrayDimensions& arrayDimensions)
    {
        for (auto arrayDecl : ctx->ArrayRankSpecifiers)
        {
            if (arrayDecl->Dimension == nullptr)
            {
                arrayDimensions.PushBack(ArrayDimensions::Unbounded);
            }
            else
            {
                ConstNumericVal arrayDimensionVal = FoldEvalStaticConstExprNumericValue(arrayDecl->Dimension);
                int64_t nextDim = ExtractValueAsInt64(arrayDimensionVal, -1);
                if (nextDim < 0)
                {
                    verboseCout << arrayDecl->start->getLine() << ": array rank specifier (" << nextDim << ") invalid or non foldable";
                    arrayDimensions.PushBack(ArrayDimensions::Unknown);
                }
                else
                {
                    int asInt = static_cast<int>(nextDim);
                    arrayDimensions.PushBack(asInt);
                }
            }
        }

        return true;
    }

    void SemanticOrchestrator::ValidateClass(azslParser::ClassDefinitionContext* ctx) noexcept(false)
    {
        using namespace std::string_literals;
        auto curScopeName = m_scope->m_currentScopePath;
        verboseCout << ctx->RightBrace()->getSymbol()->getLine() << ": exit class " << curScopeName << ". verifying compliance...\n";
        // Get iterator into the symbol database from current scope name. (current scope should be the currently closing class)
        // Access the KindInfo from iter->second, and "cast" the `anyInfo` variant to ClassInfo:
        auto& classSubInfo = GetCurrentScopeSubInfoAs<ClassInfo>();
        // this class original AST node:
        auto declNode = get<AstClassDeclNode*>(classSubInfo.m_declNodeVt);
        // Semantic validation. Iterate each base UID as registered in the ClassInfo:
        int concreteBase = 0; // this can only be 0 or 1.
        for (auto b : classSubInfo.GetBases())
        {
            // get line location diagnostic message of its declaration keyword in source:
            verboseCout << "  base: " << b.m_name << "\n";
            // Fetch the base from name in the database
            auto* infoBase = m_symbols->GetIdAndKindInfo(b.m_name);
            assert(infoBase); // can't be undeclared since it already threw an error in RegisterBases.
            // wannabe is "want-to-be-a-base"
            auto& [baseUid, wannabeInfo] = *infoBase;
            if (!wannabeInfo.IsKindOneOf(Kind::Interface, Kind::Class))
            {
                throw AzslcOrchestratorException{ORCHESTRATOR_INVALID_INTERFACE, declNode->Class()->getSymbol(),
                    ConcatString("base ", baseUid.m_name, " is not an interface or class (it is a "s,
                                 Kind::ToStr(wannabeInfo.GetKind()).data(), ")")};
            }
            concreteBase += wannabeInfo.GetKind() == Kind::Class;
            if (concreteBase > 1)
            {
                throw AzslcOrchestratorException{ORCHESTRATOR_INVALID_INTERFACE, declNode->Class()->getSymbol(),
                                                ConcatString("class ", declNode->Name->getText(),
                                                             " has multiple concrete bases. Only 1 concrete base allowed"s)};
            }
            // verify interfaces full implementation
            if (wannabeInfo.GetKind() == Kind::Interface)
            {
                auto& baseInfoAsClass = wannabeInfo.GetSubRefAs<ClassInfo>();
                for (auto basemember : baseInfoAsClass.GetOrderedMembers())
                {  // Check that any member present in base is present in this class
                    if (!classSubInfo.HasMember(basemember.GetNameLeaf()))
                    {
                        throw AzslcOrchestratorException{ORCHESTRATOR_CLASS_REDEFINE, declNode->Class()->getSymbol(),
                                                         ConcatString("class ", m_scope->m_currentScopeUID.m_name, " does not redefine ", basemember.m_name)};
                    }
                }
            }
        }
    }

    void SemanticOrchestrator::ValidateFunctionAndRegisterFamilySeenat(azslParser::LeadingTypeFunctionSignatureContext* ctx)
    {
        using namespace std::string_literals;

        auto& [thisFuncId, thisFuncKind] = GetCurrentScopeIdAndKind();

        // verify whether it overrides
        auto* parentFunction = GetSymbolHiddenInBase(thisFuncId);
        if (parentFunction)
        {
            // == register this function, to its base symbol's overrides seenat list ! ==
            // let's construct access to that list, from the identifier we got out of GetSymbolHiddenInBase
            auto& [baseFuncId, baseFuncKind] = *parentFunction;
            // let's do a bit of sanity check on that symbol
            Kind baseKind = baseFuncKind.GetKind();
            if (baseKind != Kind::Function)
            {
                auto baseKindStr = Kind::ToStr(baseKind).data();
                throw AzslcOrchestratorException{ORCHESTRATOR_HIDING_SYMBOL_BASE, ctx->Identifier()->getSymbol(),
                    ConcatString("function ", thisFuncId.m_name, " is hiding a symbol of a base, that is not of Function kind, but is ", baseKindStr)};
            }
            auto& baseFuncSubInfo = baseFuncKind.GetSubRefAs<FunctionInfo>();
            if (std::find(baseFuncSubInfo.m_overrides.begin(),  baseFuncSubInfo.m_overrides.end(), thisFuncId) == baseFuncSubInfo.m_overrides.end())
            {   // this collection is not a set, and ValidateFunctionAndRegisterFamilySeenat may be called multiple times on the same symbol
                baseFuncSubInfo.m_overrides.emplace_back(thisFuncId);
            }
            // == and register the (one) base in our own data too. two-way 1toN bridging. ==
            auto& thisFuncSubInfo = thisFuncKind.GetSubRefAs<FunctionInfo>();
            thisFuncSubInfo.m_base = baseFuncId;
        }

        // semantic provision of override guarantee:
        if (ctx->Override())
        {
            if (GetCurrentParentScopeIdAndKind().second.GetKind() != Kind::Class)
            {   // free function case (or in interface !)
                throw AzslcOrchestratorException{ORCHESTRATOR_INVALID_OVERRIDE_SPECIFIER_CLASS, ctx->Identifier()->getSymbol(),
                    ConcatString("function ", thisFuncId.m_name, " has override specifier but is not part of a class")};
            }
            else
            {   // in-class case
                if (!parentFunction)
                {
                    throw AzslcOrchestratorException{ORCHESTRATOR_INVALID_OVERRIDE_SPECIFIER_BASE, ctx->Identifier()->getSymbol(),
                        ConcatString("method ", thisFuncId.m_name, " has override specifier but is not found in any base")};
                }
            }
        }

        // verify parameters integrity; and reflow the default values into the first declaration site:
        auto& funcInfo = thisFuncKind.GetSubRefAs<FunctionInfo>();
        funcInfo.MergeDefaultParameters();

        // forbid mixup of overloading & default param values (because it wreaks the resolution technique)
        auto overloadSetId = IdentifierUID{QualifiedNameView{RemoveLastParenthesisGroup(thisFuncId.GetName())}};
        auto* overloadSet = m_symbols->GetAsSub<OverloadSetInfo>(overloadSetId);
        if (overloadSet->HasOverloads())  // (at least 2 concrete functions)
        {
            bool thisFuncIsCulprit = funcInfo.HasAnyDefaultParameterValue();
            bool previousFuncIsCulprit = overloadSet->AnyOf([this](auto&& uid){return this->HasAnyDefaultParameterValue(uid);});
            if (thisFuncIsCulprit || previousFuncIsCulprit)
            {
                throw AzslcOrchestratorException{ORCHESTRATOR_NO_DEFAULT_PARAM_WITH_OVERLOADS, ctx->Identifier()->getSymbol(),
                                                 ConcatString("can't use default arguments in conjunction with function overloading. (function ", thisFuncId.m_name,
                                                              thisFuncIsCulprit ? " has defaults arguments, but overloads exist)"
                                                                                : " overloads a function that has default arguments)")};
            }
        }
    }

    void SemanticOrchestrator::ValidateSrg(azslParser::SrgDefinitionContext* ctx) noexcept(false)
    {
        auto& srgInfo = GetCurrentScopeSubInfoAs<SRGInfo>();

        if (ctx->Semantic)
        {
            string semanticName = ctx->Semantic->getText();
            if (srgInfo.m_semantic)
            {
                // We only care the specified semantic is the same as the currently defined semantic for the srg.
                if (srgInfo.m_semantic->GetNameLeaf() != semanticName)
                {
                    const LineDirectiveInfo* originalSrglineInfo = AzslcException::s_lineFinder->GetNearestPreprocessorLineDirective(srgInfo.m_declNode->Semantic->getLine());
                    string errorMsg = FormatString("'partial' extension of ShaderResourceGroup [%s] with semantic [%s] shall not bind a different semantic than [%s] found in line %u of %s",
                        ctx->Name->getText().c_str(), semanticName.c_str(), srgInfo.m_semantic->GetNameLeaf().c_str(),
                        originalSrglineInfo->m_forcedLineNumber, originalSrglineInfo->m_containingFilename.c_str());
                    throw AzslcOrchestratorException{ORCHESTRATOR_SRG_EXTENSION_HAS_DIFFERENT_SEMANTIC, ctx->Semantic, errorMsg};
                }
                // All is good.
                return;
            }

            // Make sure the SRG is referencing a registered srgSemantic (and of the correct kind)
            auto uqName = UnqualifiedNameView{ semanticName };
            auto semanticSymbol = LookupSymbol(uqName);
            if (!semanticSymbol)
            {
                throw AzslcOrchestratorException{ORCHESTRATOR_INVALID_SEMANTIC_DECLARATION, ctx->ShaderResourceGroup()->getSymbol(),
                                                 ConcatString("Declaration for semantic ", semanticName, " used in SRG ", ctx->Name->getText(), " was not found")};
            }

            auto& [semanticSymId, semanticSymKind] = *semanticSymbol;
            Kind kind = semanticSymKind.GetKind();
            if (kind != Kind::ShaderResourceGroupSemantic)
            {
                throw AzslcOrchestratorException{ORCHESTRATOR_INVALID_SEMANTIC_DECLARATION_TYPE, ctx->ShaderResourceGroup()->getSymbol(),
                                                 ConcatString("Declaration for ", semanticName, " used in SRG ", ctx->Name->getText(),
                                                              " is a ", Kind::ToStr(kind).data(),
                                                              " but expected a ", Kind::ToStr(Kind::ShaderResourceGroupSemantic).data())};
            }
            const IdentifierUID& srgId = GetCurrentScopeIdAndKind().first;
            auto* srgSemanticInfo = semanticSymKind.GetSubRefAs<ClassInfo>().Get<SRGSemanticInfo>();
            auto userSrgIterator = m_frequencyToSrg.find(*srgSemanticInfo->m_frequencyId);
            if (userSrgIterator == m_frequencyToSrg.end())
            {
                m_frequencyToSrg[*srgSemanticInfo->m_frequencyId] = srgId;
            }
            else if (userSrgIterator->second != srgId)
            {
                throw AzslcOrchestratorException{ORCHESTRATOR_SRG_REUSES_A_FREQUENCY, ctx->ShaderResourceGroup()->getSymbol(),
                                                 ConcatString("SRG ", ctx->Name->getText(), " reuses frequencyId "
                                                              , userSrgIterator->first, " already used by ",
                                                              userSrgIterator->second)};
            }
            // Good, the SRGSemantic is valid, remember a reference to its ID on the SRG:
            srgInfo.m_semantic = semanticSymId;
            if (srgSemanticInfo->m_variantFallback)
            {
                int variantFallbackValue = static_cast<int>(*(srgSemanticInfo->m_variantFallback));

                // We pay the price in 16-byte (128-bit) chunks
                int keyLength = ((variantFallbackValue + kShaderVariantKeyRegisterSize - 1) / kShaderVariantKeyRegisterSize) * kShaderVariantKeyRegisterSize;
                if (keyLength > variantFallbackValue)
                {
                    PrintWarning(Warn::W1, ctx->ShaderResourceGroup()->getSymbol(),
                                 "ShaderVariantFallback requires ", variantFallbackValue,
                                 " bits, but will be bumped up to ", keyLength, " bits for padding.");
                }

                // Find a name that doesn't collide with previous declarations
                string svkName = kShaderVariantKeyFallbackVarName;
                auto existingSymbol = LookupSymbol(UnqualifiedNameView{ svkName });
                while (existingSymbol)
                {
                    //There is a name collision, add a suffix to the name and try again.
                    svkName = + "z";
                    existingSymbol = LookupSymbol(UnqualifiedNameView{ svkName });
                }

                size_t line = 0;
                auto& [uid, var] = AddIdentifier(UnqualifiedNameView{ svkName }, Kind::Variable, line);
                auto& varInfo = var.GetSubAfterInitAs<Kind::Variable>();
                varInfo.m_srgMember = true;

                ArrayDimensions arrayDims;
                arrayDims.PushBack(keyLength / kShaderVariantKeyRegisterSize);
                varInfo.m_typeInfoExt = ExtendedTypeInfo{CreateTypeRefInfo(UnqualifiedNameView{"uint4"}, OnNotFoundOrWrongKind::Diagnose), {},
                                                         {}, arrayDims, {}, Packing::MatrixMajor::Default };

                srgInfo.m_implicitStruct.PushMember(uid, Kind::Variable);
                srgInfo.m_shaderVariantFallback = uid;
            }
        }

        for (const IdentifierUID& viewId : srgInfo.m_srViews)
        {
            auto& [_, viewKindInfo] = *m_symbols->GetIdAndKindInfo(viewId.m_name);
            auto& memberInfo = viewKindInfo.GetSubRefAs<VarInfo>();

            // in case of core type: structured buffer
            if (memberInfo.GetTypeClass() == TypeClass::StructuredBuffer)
            {
                // check that generic type is not a view or anything nonsensical
                TypeClass genericClass = memberInfo.GetGenericParameterTypeClass();
                bool genericTypeLooksGood = IsFundamental(genericClass) || IsUserDefined(genericClass);
                if (!genericTypeLooksGood)
                {
                    throw AzslcOrchestratorException{ORCHESTRATOR_INVALID_EXTERNAL_BOUND_RESOURCE_VIEW, memberInfo.m_declNode->start,
                                                     "externally bound resources can't be type-parameterized on view-types"};
                }
            }
        }

        for (const IdentifierUID& cbId : srgInfo.m_CBs)
        {
            auto& [_, kindInfo] = *m_symbols->GetIdAndKindInfo(cbId.m_name);
            auto& memberInfo = kindInfo.GetSubRefAs<VarInfo>();
            assert(memberInfo.IsConstantBuffer());
            const auto& genericName = memberInfo.GetGenericParameterTypeId().m_name;
            auto genericClass = memberInfo.GetGenericParameterTypeClass();
            if (!IsUserDefined(genericClass))
            {
                throw AzslcOrchestratorException{ORCHESTRATOR_INVALID_GENERIC_TYPE_CONSTANTBUFFER, memberInfo.m_declNode->start,
                    ConcatString("ConstantBuffer<T>'s generic type ", genericName,
                                 " must be user defined, but seen as ", TypeClass::ToStr(genericClass).data())};
            }
            // further checks by actually fetching the symbol
            IdAndKind* idkind = m_symbols->GetIdAndKindInfo(genericName);
            if (!idkind)
            {
                throw AzslcOrchestratorException{ORCHESTRATOR_UNDECLARED_GENERIC_TYPE_CONSTANTBUFFER, memberInfo.m_declNode->start,
                    ConcatString("ConstantBuffer<T>'s generic type ", genericName,
                                 " is not declared!")};
            }
            auto& [id, kind] = *idkind;
            if (kind.GetKind() != Kind::Struct)
            {
                throw AzslcOrchestratorException{ORCHESTRATOR_INVALID_GENERIC_TYPE_CONSTANTBUFFER_STRUCT, memberInfo.m_declNode->start,
                    ConcatString("ConstantBuffer<T>'s generic type ", genericName,
                                 " must be a struct, but seen as ", Kind::ToStr(kind.GetKind()).data())};
            }
        }
    }

    optional<int64_t> SemanticOrchestrator::TryFoldSRGSemantic(azslParser::SrgSemanticContext* ctx, size_t semanticTokenType, bool required)
    {
        // const ref used, to extend the returned object's temporary life
        string_view intrinsicVarNameFromLexer = m_lexer->getVocabulary().getLiteralName(semanticTokenType);
        string_view intrinsicVarName = Trim(intrinsicVarNameFromLexer, "\'");

        auto semanticSymbol = LookupSymbol(UnqualifiedNameView{ intrinsicVarName });
        if (!semanticSymbol)
        {
            if (!required)
            {
                return none;
            }

            throw AzslcOrchestratorException{ORCHESTRATOR_LITERAL_REQUIRED_SRG_SEMANTIC, ctx->start,
                                             ConcatString(intrinsicVarName, " is required in SRG semantic")};
        }
        auto&[sId, sKind] = *semanticSymbol;
        auto& srgSubInfo = GetCurrentScopeSubInfoAs<ClassInfo>();
        if (!srgSubInfo.HasMember(sId))
        {
            assert(false); // impossible since already verified above. if we fail here, it's a broken program invariant. (incomplete registration)
        }

        auto& varInfo = sKind.GetSubRefAs<VarInfo>();

        int64_t retValue = 0;
        if (!TryGetConstExprValueAsInt64(varInfo.m_constVal, retValue))
        {
            throw AzslcOrchestratorException{ORCHESTRATOR_INVALID_INTEGER_CONSTANT,
                                             ConcatString("Semantic pass error: couldn't get a meaningful integer constant for ", intrinsicVarName)};
        }

        return retValue;
    }

    void SemanticOrchestrator::ValidateSrgSemantic(azslParser::SrgSemanticContext* ctx)
    {
        auto semanticInfo = GetCurrentScopeSubInfoAs<ClassInfo>().Get<SRGSemanticInfo>();

        (*semanticInfo).m_frequencyId = TryFoldSRGSemantic(ctx, azslParser::FrequencyId, true);
        if (*((*semanticInfo).m_frequencyId) > SRGSemanticInfo_MaxAllowedFrequency)
        {
            throw AzslcOrchestratorException{ORCHESTRATOR_INVALID_RANGE_FREQUENCY_ID, ctx->Identifier()->getSymbol(),
                                             ConcatString("ShaderResourceGroupSemantic must define a FrequencyId with value between 0 and ", SRGSemanticInfo_MaxAllowedFrequency)};
        }

        (*semanticInfo).m_variantFallback = TryFoldSRGSemantic(ctx, azslParser::ShaderVariantFallback);
    }

    ConstNumericVal SemanticOrchestrator::FoldEvalStaticConstExprNumericValue(tree::TerminalNode* numericLiteralToken, bool hintAsInt) const noexcept(false)
    {
        string text = numericLiteralToken->getText();
        if (hintAsInt)
        {
            if (EndsWith(text, "u") || EndsWith(text, "U"))
            {
                return static_cast<uint32_t> (std::stoul(text, nullptr, 0/*auto base to support 0#,0x# prefixes*/));
            }

            return int32_t{std::stoi(text, nullptr, 0)}; // stoi(...) resolves floats as integers, use hintAsInt = false if you need a float
        }
        else
        {
            return float{ std::stof(text, nullptr) };
        }
    }

    ConstNumericVal SemanticOrchestrator::FoldEvalStaticConstExprNumericValue(AstIdExpr* idExp) const
    {
        auto uqName = ExtractNameFromIdExpression(idExp);
        auto maybeSymbol = LookupSymbol(uqName);
        if (!maybeSymbol)
        {
            throw AzslcOrchestratorException{ORCHESTRATOR_CONSTANT_FOLDING_FAULT, idExp->start,
                                             ConcatString("in expected constant expression: identifier ", uqName, " not found")};
        }
        auto& [id, symbol] = *maybeSymbol;
        auto what = symbol.GetKind();
        if (what != Kind::Variable)
        {
            throw AzslcOrchestratorException{ORCHESTRATOR_CONSTANT_FOLDING_FAULT, idExp->start,
                                             ConcatString("in expected constant expression: identifier ", uqName, " did not refer to a variable, but a ", Kind::ToStr(what).data())};
        }
        auto const& var = symbol.GetSubRefAs<VarInfo>();
        if (holds_alternative<monostate>(var.m_constVal))
        {
            throw AzslcOrchestratorException{ORCHESTRATOR_CONSTANT_FOLDING_FAULT, idExp->start,
                                             ConcatString("in expected constant expression: variable ", id.m_name, " couldn't be folded to a constant (tip: use --semantic --verbose to diagnose why)")};
        }
        return var.m_constVal;
    }

    namespace
    {
        DiagnosticStream FoldFailedCommonMessage(Token* tok, optional<string_view> identifier = none)
        {
            verboseCout << tok->getLine();
            verboseCout << ": constant folding failed for " << (identifier ? *identifier : tok->getText());
            return verboseCout;
        };
    }

    ConstNumericVal SemanticOrchestrator::FoldEvalStaticConstExprNumericValue(VarInfo& varInfo) const
    {
        auto* declNode = varInfo.m_declNode;
        // first thing: constraint storage-class and type modifiers:
        bool isStaticConst = varInfo.CheckHasAllStorageFlags({StorageFlag::Static, StorageFlag::Const});
        if (!isStaticConst)
        {
            FoldFailedCommonMessage(varInfo.m_declNode->start, string_view{varInfo.m_identifier}) << ": static & const storage flags not set\n";
            return monostate{};
        }

        // second: check initializer syntax nodes
        bool hasInitializer = HasStandardInitializer(declNode);
        bool isStandardExpr = hasInitializer && declNode->variableInitializer()->standardVariableInitializer()->Expr;
        if (isStandardExpr)
        {
            // we support only the expression sub-variant (not arrayElementInitializers and not samplerStateProperty).
            AstExpr* expr = declNode->variableInitializer()->standardVariableInitializer()->Expr;
            return FoldEvalStaticConstExprNumericValue(expr);
        }
        else
        {
            if (!hasInitializer && isStaticConst)
            {
                return monostate{}; // non-initialized static constants will be zero-initialized appropriately by DXC.
            }
            else
            {
                FoldFailedCommonMessage(varInfo.m_declNode->start, string_view{varInfo.m_identifier}) << ": no standard variable initializer expression\n";
            }
        }
        return monostate{};
    }

    ConstNumericVal SemanticOrchestrator::FoldEvalStaticConstExprNumericValue(AstExpr* expr) const
    {
        if (auto* litExpr = As<azslParser::LiteralExpressionContext*>(expr))
        {
            // we are on a literal so we need to restrict down to integers only
            if (auto* intLit = litExpr->literal()->IntegerLiteral())
            {   // easy case. we have the literal right there on a plate.
                return FoldEvalStaticConstExprNumericValue(intLit);
            }
            else if (auto* floatLit = litExpr->literal()->FloatLiteral())
            {   // easy case. we have the literal right there on a plate.
                return FoldEvalStaticConstExprNumericValue(floatLit, false);
            }
            else
            {
                FoldFailedCommonMessage(expr->start) << ": initializer is not an integer literal\n";
            }
        }
        else if (auto* idExpr = As<azslParser::IdentifierExpressionContext*>(expr))
        {
            // in case of initializers that refers to id, if they are variable they will be already parsed and folded.
            auto uqName = ExtractNameFromIdExpression(idExpr->idExpression());
            auto maybeSymbol = LookupSymbol(uqName);
            if (!maybeSymbol)
            {
                FoldFailedCommonMessage(expr->start) << ": initializer referred to an undeclared symbol " << uqName << "\n";
                return monostate{};
            }
            auto& [id, symbol] = *maybeSymbol;
            auto what = symbol.GetKind();
            if (what != Kind::Variable)
            {
                FoldFailedCommonMessage(expr->start) << ": initializer identifier " + uqName + " did not refer to a variable, but a " + Kind::ToStr(what).data();
                return monostate{};
            }
            auto const& var = symbol.GetSubRefAs<VarInfo>();
            return var.m_constVal;
        }
        else
        {
            FoldFailedCommonMessage(expr->start) << ": variable initializer is not a supported expression\n";
        }
        return monostate{};
    }

    QualifiedName SemanticOrchestrator::MakeFullyQualified(UnqualifiedNameView unqualifiedName) const
    {
        return ::AZ::ShaderCompiler::MakeFullyQualified(m_scope->m_currentScopePath, unqualifiedName);
    }

    TypeClass SemanticOrchestrator::GetTypeClass(IdentifierUID typeId) const
    {
        TypeClass toReturn = TypeClass::IsNotType;
        auto* idKind = m_symbols->GetIdAndKindInfo(typeId.GetName());
        if (idKind)  // found
        {
            auto& [_, kind] = *idKind;
            switch (kind.GetKind())
            {
            case Kind::Struct: toReturn = TypeClass::Struct;
                break;
            case Kind::Class: toReturn = TypeClass::Class;
                break;
            case Kind::Enum: toReturn = TypeClass::Enum;
                break;
            case Kind::Interface: toReturn = TypeClass::Interface;
                break;
            case Kind::TypeAlias: toReturn = TypeClass::Alias;
                break;
            default:
                if (auto* asType = kind.GetSubAs<TypeRefInfo>())
                {
                    toReturn = asType->m_typeClass;
                }
                break;
            }
        }
        return toReturn;
    }

    IdentifierUID SemanticOrchestrator::LookupType(UnqualifiedNameView typeName, OnNotFoundOrWrongKind policy, optional<size_t> sourceline /*=none*/) const
    {
        auto getErrorIUID = [policy, typeName](){return policy == OnNotFoundOrWrongKind::Empty ? IdentifierUID{} : IdentifierUID{QualifiedName{typeName}};};
        IdAndKind* type = LookupSymbol(UnqualifiedNameView{typeName});
        if (!type)
        {
            if (policy == OnNotFoundOrWrongKind::Diagnose)
            {
                throw AzslcOrchestratorException{ORCHESTRATOR_TYPE_LOOKUP_FAULT,
                                                 sourceline, none, ConcatString(" type ", string{ typeName }, " requested but not found.")};
            }
            else
            {
                return getErrorIUID();
            }
        }
        // found..
        const auto& [uid, kind] = *type;
        // ..is it of correct Kind ?
        bool isType = IsKindOneOfTypeRelated(kind.GetKind());
        if (!isType)
        {
            if (policy == OnNotFoundOrWrongKind::Diagnose)
            {
                throw AzslcOrchestratorException{ORCHESTRATOR_TYPE_LOOKUP_FAULT,
                    sourceline, none, ConcatString(" type ", typeName.data(),
                                                   " requested but found as ", Kind::ToStr(kind.GetKind()).data())};
            }
            else
            {
                return getErrorIUID();
            }
        }
        return {type->first};
    }

    bool SemanticOrchestrator::TryFoldGenericArrayDimensions(ExtractedComposedType& extType, vector<tree::TerminalNode*>& genericDims) const
    {
        for (auto dim : genericDims)
        {
            auto cVal = FoldEvalStaticConstExprNumericValue(dim);
            int64_t nextDim = ExtractValueAsInt64(cVal, -1);
            if (nextDim < 0)
            {
                verboseCout << "SemanticOrchestrator::TryFoldGenericArrayDimensions could not fold the next dimension (" << nextDim << ")!";
                return false;
                // If we might want to allow such cases use this instead:
                // extType.m_genericDimensions.PushBack(ArrayDimensions::unknown);
            }
            else
            {
                int asInt = static_cast<int>(nextDim);
                extType.m_genericDimensions.PushBack(asInt);
            }
        }

        return true;
    }

    ExtendedTypeInfo SemanticOrchestrator::CreateExtendedTypeInfo(AstType* ctx, ArrayDimensions dims) const
    {
        vector<tree::TerminalNode*> genericDims;
        auto extType = ExtractComposedTypeNamesFromAstContext(ctx, &genericDims);
        if (!TryFoldGenericArrayDimensions(extType, genericDims))
        {
            throw AzslcOrchestratorException{ORCHESTRATOR_ILLEGAL_FOLDABLE_ARRAY_DIMENSIONS, ctx->start,
                                             ConcatString("SemanticOrchestrator::CreateExtendedTypeInfo failed for type (", ctx->getText(), ")")};
        }
        return CreateExtendedTypeInfo(extType, ExtractTypeQualifiers(ctx->storageFlags()), dims);
    }

    ExtendedTypeInfo SemanticOrchestrator::CreateExtendedTypeInfo(const ExtractedComposedType& extractedComposed,
                                                                  const TypeQualifiers& qualifiers,
                                                                  ArrayDimensions dims) const
    {
        TypeRefInfo core = CreateTypeRefInfo(extractedComposed.m_core);
        TypeRefInfo generic = CreateTypeRefInfo(extractedComposed.m_genericParam);
        if (core.m_typeClass == TypeClass::Alias)
        {
            // if we're referring to a type alias, follow the trail and collapse the canonical type to its real target.
            // it's unnecessary to loop on this, because the previously registered typealiases are necessarily collapsed.
            // and the first registration is necessarily of an existing type (non alias). by recurrence there can only be a distance of 1 to resolve.
            const TypeAliasInfo* targetAlias = m_symbols->GetAsSub<TypeAliasInfo>(core.m_typeId);
            return targetAlias->m_canonicalType;
        }
        Packing::MatrixMajor mtxMajor = ExtractMatrixMajorness(qualifiers);
        return ExtendedTypeInfo{core, generic, qualifiers, dims, extractedComposed.m_genericDimensions, mtxMajor};
    }

    IdAndKind* SemanticOrchestrator::GetSymbolHiddenInBase(IdentifierUID hidingCandidate)
    {
        IdAndKind* found = nullptr;
        auto& [containingScopeId, containingScopeKind] = GetCurrentParentScopeIdAndKind();
        if (containingScopeKind.GetKind() == Kind::Class)  // only class can have bases in AZSL
        {
            // get currently parsed class info:
            auto& curClassInfo = GetCurrentParentScopeSubInfoAs<ClassInfo>();
            // look for a match in any base
            for (const IdentifierUID& base : curClassInfo.GetBases())
            {
                auto maybeBaseClass = m_symbols->GetIdAndKindInfo(base.m_name);
                if (maybeBaseClass
                    && maybeBaseClass->second.IsKindOneOf(Kind::Interface, Kind::Class))  // bases must be interfaces or classes, but we don't assume it's enforced prior to calls to this function
                {
                    const auto& baseClassInfo = maybeBaseClass->second.GetSubRefAs<ClassInfo>();
                    bool baseHasSameNameMember = baseClassInfo.HasMember(hidingCandidate.GetNameLeaf());
                    if (baseHasSameNameMember)
                    {
                        if (found)
                        {
                            throw AzslcOrchestratorException{ORCHESTRATOR_MULTIPLE_HIDDEN_SYMBOLS,
                                ConcatString("Found multiple symbols hidden by ", hidingCandidate.m_name,
                                    " in bases of ", containingScopeId.m_name,
                                    ". First was ", found->first.m_name,
                                    ", now also found in ", base.m_name, ".")};
                        }
                        // reconstruct the UID found, and return that.
                        string reconstructedPath = JoinPath(base.m_name, hidingCandidate.GetNameLeaf());
                        found = m_symbols->GetIdAndKindInfo(QualifiedName{reconstructedPath});
                    }
                }
            }
        }
        return found;
    }

    bool SemanticOrchestrator::IsScopeStruct() const
    {
        const KindInfo& scopeKind = m_symbols->GetIdAndKindInfo(m_scope->GetNameOfCurScope())->second;
        return scopeKind.IsKindOneOf(Kind::Struct);
    }

    bool SemanticOrchestrator::IsScopeStructClassInterface() const
    {
        const KindInfo& scopeKind = m_symbols->GetIdAndKindInfo(m_scope->GetNameOfCurScope())->second;
        return scopeKind.IsKindOneOf(Kind::Struct, Kind::Class, Kind::Interface);
    }

    void SemanticOrchestrator::MakeAndEnterAnonymousScope(string_view decorationPrefix, Token* scopeFirstToken)
    {
        UnqualifiedName unnamedBlockCode{ConcatString("$", decorationPrefix, m_anonymousCounter)};
        AddIdentifier(unnamedBlockCode, Kind::Namespace, scopeFirstToken->getLine());
        m_scope->EnterScope(unnamedBlockCode, scopeFirstToken->getTokenIndex());
        ++m_anonymousCounter;
    }
}
