/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include "AzslcScopeTracker.h"
#include "PreprocessorLineDirectiveFinder.h"
#include "AzslcUnboundedArraysValidator.h"
#include "AzslcKindInfo.h"

namespace AZ::ShaderCompiler
{
    //! For argument of RegisterFunction
    enum class AsFunc
    {
        Declaration,
        Definition
    };

    //! Deals with jobs that requires access to both Scope and SymbolTable
    struct SemanticOrchestrator
    {
        SemanticOrchestrator(SymbolAggregator* sema, ScopeTracker* scope, azslLexer* lexer);

        //! Helper shortcut: uses the current scope as a starting location to lookup a symbol.
        //! Returns whatever SymbolAggretator's eponymous returns.
        decltype(auto) LookupSymbol(UnqualifiedNameView name) const
        {
            return m_symbols->LookupSymbol(m_scope->m_currentScopePath, name);
        }
        //! non-const version
        decltype(auto) LookupSymbol(UnqualifiedNameView name)
        {
            return m_symbols->LookupSymbol(m_scope->m_currentScopePath, name);
        }

        //! Does this symbol resolves to an existing ID from the current scope ?
        //! Helper shortcut: will concatenate your symbol name to current scope before passing to SymbolAggregator
        //! Returns whatever SymbolAggretator's eponymous returns.
        decltype(auto) AddIdentifier(UnqualifiedNameView usym, Kind kind, optional<size_t> lineNumber = none)
        {
            return m_symbols->AddIdentifier(MakeFullyQualified(usym), kind, lineNumber);
        }

        auto GetCurrentScopeIdAndKind() -> IdAndKind&;

        auto GetCurrentParentScopeIdAndKind() -> IdAndKind&;

        //! Shorthand for get< AlternativeType > of the current-scope KindInfo's m_subInfo member.
        template< typename AlternativeType >
        auto GetCurrentScopeSubInfoAs() noexcept(false) -> AlternativeType&
        {
            static_assert(KindInfo::isSubAlternative_v< AlternativeType >,
                          "please pass types that belongs to KindInfo::m_subInfo");
            auto& curScope = GetCurrentScopeIdAndKind();
            return curScope.second.GetSubRefAs<AlternativeType>();
        }

        //! Same as above but for the parent scope
        template< typename AlternativeType >
        auto GetCurrentParentScopeSubInfoAs() noexcept(false) -> AlternativeType&
        {
            static_assert(KindInfo::isSubAlternative_v< AlternativeType >,
                          "please pass types that belongs to KindInfo::m_subInfo");
            auto& parentScopeIdKind = GetCurrentParentScopeIdAndKind();
            return parentScopeIdKind.second.GetSubRefAs<AlternativeType>();
        }

        //! Execute a class member function (method) definition
        //! @param name         the function name
        //! @param className    the C in C::Method(){}
        auto RegisterDeportedMethod(UnqualifiedNameView name, azslParser::UserDefinedTypeContext* className, AstFuncSig* ctx) -> IdAndKind&;

        //! Execute a function definition or declaration registration into the intermediate representation.
        //! @param name is UNqualified in this version to allow to simplify the API for callers which don't care about symbols and scopes
        //!             this is notably used by the normal function declaration visitor where only the name is parseable through a simpler Identifier rule.
        auto RegisterFunction(UnqualifiedNameView name, AstFuncSig* ctx, AsFunc statementGenre) -> IdAndKind&;

        //! Execute a function definition or declaration registration into the intermediate representation.
        //! @param name is qualified in this version, to allow callers to do the scope stitching or lookup and resolution themselves.
        //!             this is notably used by deported method definition registration, where classname C in C::M has to be resolved by the caller
        auto RegisterFunction(QualifiedNameView name, AstFuncSig* ctx, AsFunc statementGenre) -> IdAndKind&;

        auto RegisterFunctionDeclarationAndAddSeenat(UnqualifiedNameView name, AstFuncSig* signature) -> IdAndKind&;

        template< typename ContextType >
        auto RegisterStructuredType(ContextType* ctx, Kind kind) -> IdAndKind&
        {
            auto const& idText     = ctx->Name->getText();
            size_t line            = ctx->Name->getLine();
            verboseCout << line << ": " << Kind::ToStr(kind) << " decl: " << idText << "\n";
            auto uqNameView        = UnqualifiedNameView{idText};
            if (auto* param = ExtractSpecificParent<azslParser::FunctionParamContext>(ctx))
            {
                // because arguments participate in the signature of the function,
                // and the scope of arguments is that very function, we have a catch-22.
                throw AzslcOrchestratorException(ORCHESTRATOR_NO_INLINE_UDT_IN_PARAMETERS, param->start,
                                                 ConcatString(" introducing new type (", idText,
                                                              ") within function arguments is ill-formed"));
            }
            // register the new identifier:
            IdAndKind& symbol      = AddIdentifier(uqNameView, kind, line);
            auto& [uid, info]      = symbol;
            // now fillup what we can about the kindinfo:
            auto& classInfo        = info.GetSubRefAs<ClassInfo>();
            classInfo.m_kind       = kind;
            classInfo.m_declNodeVt = ctx;

            if (kind == Kind::ShaderResourceGroupSemantic)
            {
                SRGSemanticInfo semanticInfo;
                classInfo.m_subInfo = semanticInfo;
            }

            if (kind == Kind::Enum)
            {
                EnumerationInfo enumInfo;
                auto* enumCtx   = dynamic_cast<azslParser::EnumDefinitionContext*>(ctx);
                auto* scopedCtx = dynamic_cast<azslParser::ScopedEnumContext*>(enumCtx->enumKey());
                enumInfo.m_isScoped = scopedCtx != nullptr;

                // If we decide to support the enum struct|class name : type syntax, the underlying type will come from there
                // The underlying type can be used in SemanticOrchestrator::GetTypeClass(IdentifierUID typeId) later if we need type casting
                // this int is provisional, because in reality it depends on the collective values of enumerators.
                ExtractedTypeExt enumType = { UnqualifiedNameView("int"), nullptr };
                enumInfo.m_underlyingType = CreateTypeRefInfo(enumType);

                classInfo.m_subInfo = enumInfo;
            }

            // If this is a structure inside an SRG, add it to the map:
            const auto& [scopeId, scopeKind] = GetCurrentScopeIdAndKind();
            if (kind == Kind::Struct && scopeKind.GetKind() == Kind::ShaderResourceGroup)
            {
                // access the class SRGInfo and add a member:
                auto& srgInfo = GetCurrentScopeSubInfoAs<SRGInfo>();
                srgInfo.m_structs.push_back(uid);
            }
            // If nested, register it as a member in its parent
            if (scopeKind.IsKindOneOf(Kind::Struct, Kind::Class, Kind::Interface))
            {
                auto& parentInfo = GetCurrentScopeSubInfoAs<ClassInfo>();
                parentInfo.PushMember(uid, kind);
            }

            return symbol;
        }

        auto RegisterTypeAlias(string_view newIdentifier, AstType* existingTypeCtx, azslParser::TypeAliasingDefinitionStatementContext* ctx) -> IdAndKind&;

        auto RegisterSRGSemantic(AstSRGSemanticDeclNode* ctx) -> IdAndKind&;

        auto RegisterInterface(AstInterfaceDeclNode* ctx) -> IdAndKind&;

        auto RegisterClass(AstClassDeclNode* ctx) -> IdAndKind&;

        auto RegisterStruct(AstStructDeclNode* ctx) -> IdAndKind&;

        auto RegisterEnum(AstEnumDeclNode* ctx)-> IdAndKind&;

        auto RegisterVar(Token* nameIdentifier, AstUnnamedVarDecl* ctx) -> IdAndKind*;

        void RegisterNamelessFunctionParameter(azslParser::FunctionParamContext* ctx);

        void FillOutSrgField(AstNamedVarDecl* ctx, VarInfo &varInfo, IdentifierUID varUid, ArrayDimensions &arrayDims);

        auto ExtractSamplerState(AstVarInitializer* ctx) -> SamplerStateDesc;

        auto RegisterSRG(AstSRGDeclNode* ctx) -> IdAndKind&;

        void RegisterSRGSemanticMember(AstSRGSemanticMemberDeclNode* ctx);

        void RegisterEnumerator(azslParser::EnumeratorDeclaratorContext* ctx);

        void RegisterBases(azslParser::BaseListContext* ctx);

        void RegisterSeenat(IdAndKind& idPair, const TokensLocation& location);

        //! Will register one seenat for each found symbol in the nested name expression,
        //! `Base::Middle::Leaf` will register `Base`, `Base::Middle` and `Base::Middle::Leaf`
        //! It assumes starting scope is current scope.
        void RegisterSeenat(AstIdExpr* ctx);
        //! Same but for a context withing a member-access-expression where the scope is relative to the scope of the type of LHS.
        void RegisterSeenat(AstIdExpr* ctx, QualifiedNameView startupScope);

        //! Check that the type of LHS expression (in a member access expression context) satisfies well-formed semantics
        //! returns .first==true if is valid, and .first==false if the semantic fails to check.
        //!         .second is the typeof(LHS)
        auto VerifyLHSExprOfMAExprIsValid(azslParser::MemberAccessExpressionContext* ctx) const -> pair<bool, QualifiedName>;

        //! Resolve the type from the expression, look it up, and verify that it is a kind that may hold sub-members.
        auto VerifyTypeIsScopeComposable(azslParser::ExpressionContext* typeScope) const -> pair<bool, QualifiedName>;

        //! look up the type, verify that it exists and is a kind that may hold sub-members.
        //! takes supplementary parameters for better verbose or warning diagnostics.
        auto VerifyTypeIsScopeComposable(QualifiedNameView lhsTypeName, optional<string> lhsExpressionText = none, optional<size_t> line = none) const -> pair<bool, QualifiedName>;

        //! assemble a type (left) and an idexpr (right) to see if it forms a symbol that exists, and extracts its type.
        auto ComposeMemberNameWithScopeAndGetType(QualifiedName scopingType, AstIdExpr* rhsMember) const -> QualifiedName;

        // This family of functions will attempt to evaluate the type of an expression.
        // returns: a typename. take care that unregistered types will probably not be rooted.
        auto TypeofExpr(azslParser::ExpressionContext* ctx) const -> QualifiedName;
        auto TypeofExpr(azslParser::ExpressionExtContext* ctx) const -> QualifiedName;
        auto TypeofExpr(azslParser::OtherExpressionContext* ctx) const -> QualifiedName;
        auto TypeofExpr(AstType* ctx) const -> QualifiedName;
        auto TypeofExpr(AstIdExpr* ctx) const -> QualifiedName;
        auto TypeofExpr(azslParser::IdentifierExpressionContext* ctx) const -> QualifiedName;
        auto TypeofExpr(azslParser::MemberAccessExpressionContext* ctx) const -> QualifiedName;
        auto TypeofExpr(azslParser::FunctionCallExpressionContext* ctx) const -> QualifiedName;
        auto TypeofExpr(azslParser::ArrayAccessExpressionContext* ctx) const -> QualifiedName;
        auto TypeofExpr(azslParser::ParenthesizedExpressionContext* ctx) const -> QualifiedName;
        auto TypeofExpr(azslParser::CastExpressionContext* ctx) const -> QualifiedName;
        auto TypeofExpr(azslParser::ConditionalExpressionContext* ctx) const -> QualifiedName;
        auto TypeofExpr(azslParser::AssignmentExpressionContext* ctx) const -> QualifiedName;
        auto TypeofExpr(azslParser::NumericConstructorExpressionContext* ctx) const -> QualifiedName;
        auto TypeofExpr(azslParser::TypeofExpressionContext* ctx) const -> QualifiedName;
        auto TypeofExpr(azslParser::LiteralExpressionContext* ctx) const -> QualifiedName;
        auto TypeofExpr(azslParser::LiteralContext* ctx) const -> QualifiedName;
        auto TypeofExpr(azslParser::CommaExpressionContext* ctx) const -> QualifiedName;
        auto TypeofExpr(azslParser::PostfixUnaryExpressionContext* ctx) const -> QualifiedName;
        auto TypeofExpr(azslParser::PrefixUnaryExpressionContext* ctx) const -> QualifiedName;
        auto TypeofExpr(azslParser::BinaryExpressionContext* ctx) const -> QualifiedName;

        //! Parse the AST from a variable declaration and attempt to extract array dimensions integer constants [dim1][dim2]...
        //! Return: <true> on success, <false> otherwise
        bool TryFoldArrayDimensions(AstUnnamedVarDecl* ctx, ArrayDimensions& arrayDimensions);

        void ValidateClass(azslParser::ClassDefinitionContext* ctx) noexcept(false);

        void ValidateFunctionAndRegisterFamilySeenat(azslParser::LeadingTypeFunctionSignatureContext* ctx) noexcept(false);

        // Verify if a symbol overrides a symbol in a base (parent) of the current scope.
        // Currently Azsl can only have interface as parent of classes. And interfaces can only have function signatures.
        // Therefore for now, in practice this function can only be used to check if a method overrides a function in a base interface.
        // Returns: the symbol of the overridden function if found. nullopt if hidingCandidate doesn't hide.
        // Will diagnose-throw if multiple bases have the candidate symbol's name.
        auto GetSymbolHiddenInBase(IdentifierUID hidingCandidate) -> IdAndKind*;

        void ValidateSrg(azslParser::SrgDefinitionContext* ctx) noexcept(false);

        void ValidateSrgSemantic(azslParser::SrgSemanticContext* ctx) noexcept(false);

        // That function is to get the interpreted numerical value of a literal token.
        // Careful this should be used only on IntegerLiteral tokens (floats will partially parse or throw).
        // Throws: out_of_range in case of overflow/underflow, or invalid_argument for unsupported letters.
        // Hint: The function has no knowledge of upper contexts, so we can hint if we prefer the const resolved as integer or float
        //   (by default it will try to resolve it as integer)
        // Return: either a int32, a uint32 or a float. but not a monostate.
        auto FoldEvalStaticConstExprNumericValue(tree::TerminalNode* numericLiteralToken, bool hintAsInt = true) const noexcept(false) -> ConstNumericVal;

        // Extracted method from the AstIntLiteralOrId* overload for more flexibility.
        auto FoldEvalStaticConstExprNumericValue(AstIdExpr* idExp) const -> ConstNumericVal;

        // Primitive constant folding system. Evaluate integers in static const variables as much as possible.
        // The returned value can hold any of 3 types:
        //  - an empty monostate (no evaluable value detected, could be: unsupported expression, float, overflow, or non static-const..)
        //  - signed int64
        //  - unsigned int64
        auto FoldEvalStaticConstExprNumericValue(VarInfo& varInfo) const -> ConstNumericVal;

        auto FoldEvalStaticConstExprNumericValue(AstExpr* expr) const -> ConstNumericVal;

        // Create an absolute path symbol for a new name, in the context of the current scope.
        auto MakeFullyQualified(UnqualifiedNameView unqualifiedName) const -> QualifiedName;

        // helper to query a type class from an ID
        auto GetTypeClass(IdentifierUID typeId) const -> TypeClass;

        enum class OnNotFoundOrWrongKind { Diagnose, Empty, CopeByCopy };
        //! Shorthand for symbol lookup, but with supplementary checks, specifics to types.
        //! throws if: the found symbol is not a type, or no found symbol and policy is Diagnose.
        auto LookupType(UnqualifiedNameView typeName, OnNotFoundOrWrongKind policy = OnNotFoundOrWrongKind::CopeByCopy, optional<size_t> sourceline = none) const noexcept(false) -> IdentifierUID;

        //! Find and return a registered type from an AST node. Will also resolve typeof expressions.
        //! could work from any sort of context that has an ExtractTypeNameFromAstContext override
        template <typename TypeCtx>
        auto LookupType(TypeCtx* ctx, OnNotFoundOrWrongKind policy = OnNotFoundOrWrongKind::CopeByCopy) const -> IdentifierUID
        {
            IdentifierUID typeRef;
            UnqualifiedName uqName;
            AstTypeofNode* typeofCtx = ExtractTypeofAstNode(ctx);
            if (typeofCtx)
            {
                uqName = UnqualifiedName{TypeofExpr(typeofCtx)};
            }
            else
            {
                auto core = ExtractComposedTypeNamesFromAstContext(ctx).m_core;
                // the concept here, is to activate recursive lookup in case we have nested generics and/or typeof, in any combination.
                // in practice for now, it will never recurse;
                // because only generic parameters may be further AstNodes. And since we ignore them for now (collapsing behavior), we're set.
                uqName = core.m_node ? UnqualifiedName{LookupType(core.m_node, policy).m_name} : core.m_name;
            }
            IdAndKind* idkind = LookupSymbol(uqName);
            if (idkind)
            {
                if (idkind->second.GetKind() == Kind::TypeAlias)
                {
                    // recurse until not an alias!
                    typeRef = LookupType(UnqualifiedNameView{GetTypeName(idkind)}, policy);
                }
                else
                {
                    // found core type (UDT or collapsed predefined)
                    typeRef = idkind->first;
                }
            }
            else
            {   //! otherwise, we need to do damage control.
                // TODO: if it is a matrix<float,3,4> it needs to be canonicalized to float3x4
                if (policy == OnNotFoundOrWrongKind::Diagnose)
                {
                    throw std::runtime_error(DiagLine(ctx->start) + uqName + " is not a supported type expression ?");
                }
                PrintWarning(Warn::W3, ctx->start, "unidentified name ", uqName, " in type expression");
                if (policy == OnNotFoundOrWrongKind::CopeByCopy)
                {
                    typeRef.m_name = QualifiedName{uqName};  // try to shove it, if we defend enough in code downstream this could end up working.
                }
            }
            return typeRef;
        }

        // lookup the symbol database for a type of a given name (or discover the name through an Ast context) and compose a TypeRefInfo
        // ArgumentType maybe UnqualifiedNameView or a AstType
        template< typename ContextOrNameT >
        auto CreateTypeRefInfo(ContextOrNameT typeNameOrCtx, OnNotFoundOrWrongKind policy = OnNotFoundOrWrongKind::CopeByCopy) const -> TypeRefInfo
        {
            auto typeId      = LookupType(typeNameOrCtx, policy);
            auto tClass      = GetTypeClass(typeId);
            auto arithmetic  = IsNonGenericArithmetic(tClass) ? CreateArithmeticTraits(typeId.GetName()) : ArithmeticTraits{}; // TODO: canonicalize generics
            return TypeRefInfo{typeId, arithmetic, tClass};
        }

        // shortcut helper, when we have a node ast, we use it in priority thanks to all the superior access it provides (for typeof)
        auto CreateTypeRefInfo(const ExtractedTypeExt& extractedType) const -> TypeRefInfo
        {
            return extractedType.m_node ? CreateTypeRefInfo(extractedType.m_node) : CreateTypeRefInfo(extractedType.m_name);
        }

        // Just a helper function to compose the bigger version, that contains more data that can't be stored in the core type (TypeRefInfo).
        // Array dimensions are usually stored in VarInfo for example. If you have a custom way to discover them, use this helper to make your own ExtendedTypeInfo
        auto CreateExtendedTypeInfo(AstType* ctx, ArrayDimensions dims) const -> ExtendedTypeInfo;

        // Helper func which folds any possible generic dimensions into the extracted composed type
        bool TryFoldGenericArrayDimensions(ExtractedComposedType& extType, vector<tree::TerminalNode*>& genericDims) const;

        // another helper to streamline what to do directly with the result from ExtractTypeNameFromAstContext function families.
        auto CreateExtendedTypeInfo(const ExtractedComposedType&, const TypeQualifiers&, ArrayDimensions) const -> ExtendedTypeInfo;

        //! check if current scope is a structured user defined type ("struct", "class" or "interface")
        bool IsScopeStructClassInterface() const;

        //! check if current scope is of type "struct".
        bool IsScopeStruct() const;

        //! Find a concrete function from an overload-set and an argument list.
        //! (adjust the shoot of a lookup to something more precise in case that it's possible)
        //! if maybeOverloadSet is not an overload-set the function returns identity
        auto ResolveOverload(IdAndKind* maybeOverloadSet, azslParser::ArgumentListContext* argumentListCtx) const -> IdAndKind*;

        //! Generate a unique name, create a corresponding namespace symbol, and enter its scope
        void MakeAndEnterAnonymousScope(string_view decorationPrefix, Token* scopeFirstToken);

    private:
        //! for internal use when encountering unresolved symbols by lookup.
        void DiagnoseUndeclaredSub(Token* atToken, QualifiedNameView startupScope, string partialName) const;

        auto TryFoldSRGSemantic(azslParser::SrgSemanticContext* ctx, size_t semanticTokenType, bool required = false) -> optional<int64_t>;

        auto CreateDecorationOfFunction(azslParser::FunctionParamsContext* parametersContext) const -> string;

        auto CreateDecoratedIdentityOfFunction(QualifiedNameView name, azslParser::FunctionParamsContext* parametersContext) const -> QualifiedName;

        //! e.g. provided "int a; X GetX(int);" then from expression "(a, GetX(2), true)" MangleArgumentList returns "(?int,/X,?bool)"
        auto MangleArgumentList(azslParser::ArgumentListContext* ctx) const -> string;

        //! queries whether a function has default parameters
        bool HasAnyDefaultParameterValue(const IdentifierUID& functionUid) const;

    public:
        SymbolAggregator* m_symbols;
        ScopeTracker*     m_scope;
        azslLexer*        m_lexer;
        UnboundedArraysValidator m_unboundedArraysValidator;

    private:
        // remember frequencyId to srg association, for semantic validation
        unordered_map<int64_t, IdentifierUID> m_frequencyToSrg;
        int m_anonymousCounter;
    };
}
