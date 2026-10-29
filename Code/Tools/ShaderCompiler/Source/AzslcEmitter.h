/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include "AzslcBackend.h"
#include "AzslcSymbolTranslation.h"
#include "AzslcCodeEmissionMutator.h"

namespace Json
{
    class Value;
}

namespace AZ::ShaderCompiler
{
    enum class EmitFunctionAs
    {
        Declaration,
        Definition
    };

    struct CodeEmitter : Backend
    {
        CodeEmitter(IntermediateRepresentation* ir, TokenStream* tokens, std::ostream& out, PreprocessorLineDirectiveFinder* lineFinder)
            :
            Backend(ir, tokens),
            m_out(out),
            m_lineFinder(lineFinder)
        {}

        //! Create a companion database of mutations on the IR, through which the emitter backend can query symbols scope and names.
        //! The state of changes is stored in the AZ::ShaderCompiler::SymbolTranslation class
        //! @param options  user configuration parsed from command line
        void SetupScopeMigrations(const Options& options);

        //! Execute code emission
        //! @param options  user configuration parsed from command line
        void Run(const Options& options);

        //! For scope-migration-aware name emission of symbol names
        string GetTranslatedName(QualifiedNameView mangledName, UsageContext context, ssize_t tokenId = NotOverToken) const;

        string GetTranslatedName(const IdentifierUID& uid, UsageContext context, ssize_t tokenId = NotOverToken) const;

        string GetTranslatedName(const TypeRefInfo& typeRef, UsageContext context, ssize_t tokenId = NotOverToken) const;

        string GetTranslatedName(const ExtendedTypeInfo& extType, UsageContext context, const Options& options, Modifiers banned = {}, ssize_t tokenId = NotOverToken) const;

        //! Write the HLSL formatted shape of an attribute into a stream
        static void EmitAttribute(const AttributeInfo& attrInfo, Streamable& outstream);

        void AddCodeMutator(ICodeEmissionMutator* codeMutator) { m_codeMutators.push_back(codeMutator); }

        //! It would be nice that the clients don't push text through the passed "out" stream since it's not observed by the line counter;
        //! use this API in case of custom client text pushing.
        template< typename Streamable >
        CodeEmitter& operator << (Streamable&& s)
        {
            m_out << s;
            return *this;
        }

    protected:

        //! Emits the closest preprocessor generated "#line <int> <filepath>" directive located before
        //! @originalLineNumber. Keeps track of the best finds so the #line directives are not emitted more than once.
        void EmitPreprocessorLineDirective(size_t originalLineNumber);

        //! Emits the closest preprocessor generated "#line <int> <filepath>" directive located near
        //! @symbolName. See above, EmitPreprocessorLineDirective (size_t), for more details.
        void EmitPreprocessorLineDirective(QualifiedNameView symbolName);

        void EmitStruct(const ClassInfo& classInfo, string_view structName, const Options& options);

        void EmitAttribute(const AttributeInfo& attrInfo) const;

        void EmitFunction(const FunctionInfo& funcSub, const IdentifierUID& id, EmitFunctionAs entityConfiguration, const Options& options);

        void EmitTypeAlias(const IdentifierUID& uid, const TypeAliasInfo& aliasInfo, const Options& options) const;

        void EmitEnum(const IdentifierUID& uid, const ClassInfo& classInfo, const Options& options);

        //! Emits root constants
        void EmitRootConstants(const RootSigDesc& rootSig, const Options& options) const;

        //! Emits get function declarations for root constant members
        void EmitGetterFunctionDeclarationsForRootConstants(const IdentifierUID& uid) const;

        struct Except : std::initializer_list<string>
        {};
        //! Emit all attributes accumulated over a symbol. Omit an optional list of attributes passed as 2nd argument.
        void EmitAllAttachedAttributes(const IdentifierUID& uid, Except = {}) const;

        //! Emits get function definitions for root constants
        void EmitGetFunctionsForRootConstants(const ClassInfo& classInfo, string_view bufferName) const;

        //! That is a list of code elements we possibly want to emit (e.g when we emit a variable declaration)
        MAKE_REFLECTABLE_ENUM_POWER( VarDeclHas,
            InOutModifiers,  // when we emit in the context of function parameters, HLSL in out keywords are important
            HlslSemantics,   //  : TEXCOORD sort of element
            Initializer,     // = val;  sort of element
            OptionDefine,    // an option define is not a code element but an indicator that we're emitting a shader option
            NoType,          // in the usual declaration 'Type varname', omit the type. this is used for enumerators
            NoModifiers      // modifiers are storage flags (const, precise, rowmajor...)
        );

        using VarDeclHasFlag = Flag<VarDeclHas>;

        void EmitVariableDeclaration(const VarInfo&, const IdentifierUID& uid, const Options& options, VarDeclHasFlag declOptions) const;

        //! Iter must be an iterator over FunctionInfo::Parameter elements
        template <typename Iter>
        void EmitParameters(Iter begin, Iter end, const Options& options, bool withInitializer)
        {
            for (auto it = begin;
                      it != end;
                    ++it)
            {
                const FunctionInfo::Parameter& param = *it;
                auto* varInfo = m_ir->GetSymbolSubAs<VarInfo>(param.m_varId.GetName());
                if (varInfo)
                {
                    auto flag = VarDeclHasFlag(VarDeclHas::InOutModifiers) | VarDeclHas::HlslSemantics;
                    flag |= withInitializer ? VarDeclHas::Initializer : VarDeclHas::EnumType(0);
                    EmitVariableDeclaration(*varInfo, param.m_varId, options, flag);
                }
                else
                {
                    m_out << GetInputModifier(param.m_typeInfo.m_qualifiers) << " ";

                    m_out << GetTranslatedName(param.m_typeInfo, UsageContext::ReferenceSite, options);

                    if (!param.m_arrayRankSpecifiers.empty())
                    {
                        for (auto* rankCtx : param.m_arrayRankSpecifiers)
                        {
                            EmitTranspiledTokens(rankCtx->getSourceInterval());
                        }
                    }

                    if (auto* semantic = param.m_semanticCtx)
                    {
                        m_out << " " + semantic->getText();
                    }

                    if (param.m_defaultValueExpression)
                    {
                        EmitTranspiledTokens(param.m_defaultValueExpression->getSourceInterval());
                    }
                }

                bool lastIteration = it + 1 == end;
                if (!lastIteration)
                {
                    m_out << ", ";
                }
            }
        }

        //! NotOverToken is for procedurally generated code, that doesn't have an original source in terms of token.
        static const ssize_t NotOverToken = -1;

        void EmitSRGCBUnified(const SRGInfo& srgInfo, IdentifierUID srgId, const Options& options, const RootSigDesc& rootSig);

        void EmitSRGCB(const IdentifierUID& cId, const Options& options, const RootSigDesc& rootSig) const;

        void EmitSRGSampler(const IdentifierUID& sId, const Options& options, const RootSigDesc& rootSig) const;

        void EmitSRGDataView(const IdentifierUID& tId, const Options& options, const RootSigDesc& rootSig) const;

        void EmitGetShaderKeyFunctionDeclaration(const IdentifierUID& getterUid, const TypeRefInfo& returnType) const;

        void EmitGetShaderKeyFunction(const IdentifierUID& shaderKeyUid, const IdentifierUID& getterUid, uint32_t size, uint32_t offset, string_view defaultValue, const TypeRefInfo& returnType) const;

        //! Will emit SRG content in the shape of HLSL transformed resource, e.g a constant buffer struct for the SRG variables.
        void EmitSRG(const SRGInfo& srgInfo, const IdentifierUID& srgId, const Options& options, const RootSigDesc& rootSig);

        //! Advanced logic (targeted transpilation transforms included) interval-as-text extractor from source token stream
        //! Will copy function body original tokens, skipping comments, reformatting if possible, and translating variable declarations when needed, as well as mutating reference names of migrated SRG contents.
        void EmitTranspiledTokens(misc::Interval interval, Streamable& output) const override;

        void EmitTranspiledTokens(misc::Interval interval) const { EmitTranspiledTokens(interval, m_out); }

        //! Move a symbol to a different scope. Currently used to strip SRGs of their symbols, so that SRGs are effectively erased.
        void MigrateASTSubTree(const IdentifierUID& azslSymbol, QualifiedNameView landingScope);

        //! Verifies if a symbol is in the global scope (in the IR "viewed" through the translation)
        bool IsTopLevelThroughTranslation(const IdentifierUID& uid) const;

        //! Emits a single shader option variable as a static const
        void EmitShaderVariantOptionVariableDeclaration(const IdentifierUID& symbolUid, const Options& options) const;

        //! Emits all shader option variable fallback getters
        void EmitShaderVariantOptionGetters(const Options& options) const;

        //! Stateful check to normalize redundant declarations
        bool AlreadyEmittedFunctionDeclaration(const IdentifierUID& uid) const;
        bool AlreadyEmittedFunctionDefinition(const IdentifierUID& uid) const;

        //! Helper function used during GetTextInStream().
        //! The function assumes @nodeFromToken comes from @token.
        //! It checks whether the token that is about to be written to the output
        //! is not an undefined SRG field/variable.
        //! Throws an exception if it is an undefined ShaderResourceGroup field/variable.
        void IfIsSrgMemberValidateIsDefined(antlr4::Token* token, TokenToAst::AstNode* nodeFromToken) const;

        SymbolTranslation m_translations;
        unordered_set<IdentifierUID> m_alreadyEmittedFunctionDeclarations;
        unordered_set<IdentifierUID> m_alreadyEmittedFunctionDefinitions;
        map<size_t, size_t> m_alreadyEmittedPreprocessorLineDirectives;

        IdentifierUID m_shaderVariantFallbackUid;
        
        //! If not null it will be used during code emission to produce
        //! the mutations. 
        vector<ICodeEmissionMutator*> m_codeMutators;

        //! We keep track here of the number of lines that have been emitted.
        //! Each symbol has an original line number (virtual and physical) where it appeared,
        //! and emission will also have line directives to remap errors from further tools to the original azsl.
        //! To avoid spamming the output with line directives, we can keep track of whether a deviation
        //! has been introduced since the last emitted line directive and the desired virtual line of the currently emitted code construct.
        mutable NewLineCounterStream m_out;

        PreprocessorLineDirectiveFinder* m_lineFinder;



        //! This is a readability function for class emission code. Serves for HLSL declarator of classes
        string EmitInheritanceList(const ClassInfo& clInfo);

        //! Given an SRG parameter, determines the space it belongs to based on the platform
        int ResolveBindingSpace(const RootSigDesc::SrgParamDesc& bindInfo, BindingPair::Set bindSet) const;

        //! Returns the code mutation for a token index if it exist.
        const CodeMutation* GetCodeMutation(size_t tokenIndex) const;
    };
}
