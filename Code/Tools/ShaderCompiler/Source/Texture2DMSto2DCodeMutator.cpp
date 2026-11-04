/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Texture2DMSto2DCodeMutator.h"


namespace AZ::ShaderCompiler
{
    static constexpr char TextureFunctionNameLoad[] = "Load";
    static constexpr char TextureFunctionNameGetSamplePosition[] = "GetSamplePosition";
    static constexpr char TextureFunctionNameGetDimensions[] = "GetDimensions";

    ///////////////////////////////////////////////////////////////////////
    // azslParserBaseListener Overrides ...
    void Texture2DMSto2DCodeMutator::enterFunctionCallExpression(azslParser::FunctionCallExpressionContext* ctx)
    {
        const auto expressionCtx = ctx->expression();
        const std::string functionName = expressionCtx->stop->getText();

        if (functionName == TextureFunctionNameLoad)
        {
            OnEnterLoad(ctx);
        }
        else if (functionName == TextureFunctionNameGetSamplePosition)
        {
            OnEnterGetSamplePosition(ctx);
        }
        else if (functionName == TextureFunctionNameGetDimensions)
        {
            OnEnterGetDimensions(ctx);
        }
    }
    ///////////////////////////////////////////////////////////////////////


    ///////////////////////////////////////////////////////////////////////
    // ICodeEmissionMutator Overrides ...
    const CodeMutation* Texture2DMSto2DCodeMutator::GetMutation(ssize_t tokenId) const
    {
        auto itor = m_mutations.find(tokenId);
        if (itor == m_mutations.end())
        {
            return nullptr;
        }
        return &itor->second;
    }
    ///////////////////////////////////////////////////////////////////////

    void Texture2DMSto2DCodeMutator::RunMiddleEndMutations()
    {
        if (MutateTypeOfMultiSampleVariables())
        {
            MutateMultiSampleSystemSemantics();
        }
    }

    //! A helper function that returns the symbol name contained in @expressionCtx.
    static UnqualifiedName GetTextureSymbolName(azslParser::ExpressionContext* expressionCtx)
    {
        const auto& children = expressionCtx->children;
        // We only care for cases with three children:
        // "<Symbol>", ".", "<funcName>"
        if (children.size() == 3)
        {
            string symbolName = Replace(children[0]->getText(), "::", "/");
            return UnqualifiedName{ symbolName };
        }
        return UnqualifiedName();
    }

    Texture2DMSto2DCodeMutator::TextureMSType Texture2DMSto2DCodeMutator::GetMultiSampledTextureClass(const UnqualifiedName& uqSymbolName)
    {
        if (uqSymbolName.empty())
        {
            return TextureMSType::None;
        }
        // We only care if the symbol that is calling Load(...) is of type Texture2DMS or Texture2DMSArray
        IdAndKind* idkind = m_ir->m_sema.LookupSymbol(uqSymbolName);
        if (!idkind)
        {
            return TextureMSType::None;
        }
        auto& [uid, kind] = *idkind;
        if (kind.GetKind() != Kind::Variable)
        {
            return TextureMSType::None;
        }
        auto varInfo = kind.GetSubAs<VarInfo>();
        if (varInfo->GetTypeClass() != TypeClass::MultisampledTexture)
        {
            return TextureMSType::None;
        }
        if (EndsWith(varInfo->m_typeInfoExt.m_coreType.m_typeId.GetName(), "Array"))
        {
            return TextureMSType::Texture2DMSArray;
        }
        return TextureMSType::Texture2DMS;
    }

    void Texture2DMSto2DCodeMutator::OnEnterLoad(azslParser::FunctionCallExpressionContext* ctx)
    {
        // First we must capture the complete name of the symbol that called <Symbol>.Load(...)
        const auto expressionCtx = ctx->expression();
        const UnqualifiedName uqSymbolName = GetTextureSymbolName(expressionCtx);
        const TextureMSType textureMSType = GetMultiSampledTextureClass(uqSymbolName);
        if (textureMSType == TextureMSType::None)
        {
            return;
        }

        // Define the mutations.
        const auto argumentListCtx = ctx->argumentList();
        const auto argumentsCtx = argumentListCtx->arguments();
        auto vectorOfArguments = argumentsCtx->expression();


        // For Texture2DMS Load has two variants:
        // 1- Two arguments: int2 location, int sampleIndex
        //    When mutating this variant to Texture2D the first argument will be prepended
        //    with "int3("<location> and the second argument "sampleIndex" will be replaced with "0)".
        // 2- Three arguments: int2 location, int sampleIndex, int2 offset
        //    When mutating this variant to Texture2D the first argument will be prepended
        //    with "int3("<location> and the second argument "sampleIndex" will be replaced with "0)",
        //    the third argument will remain as is.
        // For Texture2DMSArray it's the same as above, except that the first argument is of type int3.
        //    And it will be wrapped with an int4.
        const string wrapperType = textureMSType == TextureMSType::Texture2DMSArray ? "int4(" : "int3(";
        if (vectorOfArguments.size() >= 2)
        {
            {
                auto firstArgContext = vectorOfArguments[0];
                ssize_t tokenIndex = firstArgContext->start->getTokenIndex();
                CodeMutation firstArgMutation;
                firstArgMutation.m_prepend.emplace(wrapperType);
                m_mutations.emplace(tokenIndex, firstArgMutation);
            }
            {
                // There's already a "," token that will be emitted after the first argument
                // So all we have to do is simply replace the second argument with "0)"
                // and will get in the end: int3( @firstArgContext, 0) or int4( @firstArgContext, 0) for MSArray.
                // Also keep in mind that we are working with ParseRuleContexts, and they are a range of
                // tokens, for the second argument all tokens will be dropped with "" empty strings,
                // and the last token will be dropped with "0)".
                auto secondArgContext = vectorOfArguments[1];
                const ssize_t startingTokenIndex = secondArgContext->start->getTokenIndex();
                const ssize_t stoppingTokenIndex = secondArgContext->stop->getTokenIndex();
                for (ssize_t tokenIndex = startingTokenIndex; tokenIndex < stoppingTokenIndex; ++tokenIndex)
                {
                    CodeMutation codeMutation;
                    codeMutation.m_replace.emplace("");
                    m_mutations.emplace(tokenIndex, codeMutation);
                }
                CodeMutation codeMutation;
                codeMutation.m_replace.emplace("0)");
                m_mutations.emplace(stoppingTokenIndex, codeMutation);
            }
        }
    }

    void Texture2DMSto2DCodeMutator::OnEnterGetSamplePosition(azslParser::FunctionCallExpressionContext* ctx)
    {
        // First we must capture the complete name of the symbol that called <Symbol>.GetSamplePosition(...)
        const auto expressionCtx = ctx->expression();
        const UnqualifiedName uqSymbolName = GetTextureSymbolName(expressionCtx);
        const TextureMSType textureMSType = GetMultiSampledTextureClass(uqSymbolName);
        if (textureMSType == TextureMSType::None)
        {
            return;
        }

        // Because GetSamplePosition() doesn't exist for Texture2D/Texture2DArray, we will replace
        // the whole expresion with float2(0, 0).
        const ssize_t startingTokenIndex = ctx->start->getTokenIndex();
        const ssize_t stoppingTokenIndex = ctx->stop->getTokenIndex();
        for (ssize_t tokenIndex = startingTokenIndex; tokenIndex < stoppingTokenIndex; ++tokenIndex)
        {
            CodeMutation codeMutation;
            codeMutation.m_replace.emplace("");
            m_mutations.emplace(tokenIndex, codeMutation);
        }
        CodeMutation codeMutation;
        codeMutation.m_replace.emplace("float2(0, 0)");
        m_mutations.emplace(stoppingTokenIndex, codeMutation);
    }

    void Texture2DMSto2DCodeMutator::OnEnterGetDimensions(azslParser::FunctionCallExpressionContext* ctx)
    {
        const auto expressionCtx = ctx->expression();
        const UnqualifiedName uqSymbolName = GetTextureSymbolName(expressionCtx);
        const TextureMSType textureMSType = GetMultiSampledTextureClass(uqSymbolName);
        if (textureMSType == TextureMSType::None)
        {
            return;
        }

        // For Texture2DMS GetDimensions(...) only has one variant:
        //      GetDimensions (width, height, samples)
        // All we have to do for Texture2D is to drop ", samples" and add "; samples = 1" after the closing parenthesis. We'll get:
        //      GetDimensions (width, height); samples = 1
        // For Texture2DMSArray GetDimensions(...) only has one variant:
        //      GetDimensions (width, height, elems, samples)
        // All we have to do for Texture2DArray is to drop ", samples"  and add "; samples = 1" after the closing parenthesis. We'll get:
        //      GetDimensions (width, height, elems); samples = 1
        // Remark: The last ";" is already present in the original code, this is why we append "; samples = 1" instead
        //         of "; samples = 1;"
        const auto argumentListCtx = ctx->argumentList();
        const auto argumentsCtx = argumentListCtx->arguments();

        // From argumentsCtx we can detect the last "," token and we'll
        // add it to the mutation as a replacement with "".
        auto vectorOfCommas = argumentsCtx->Comma();
        auto lastCommaIndex = vectorOfCommas.size() - 1;
        auto lastCommaToken = vectorOfCommas[lastCommaIndex];
        {
            ssize_t tokenIndex = lastCommaToken->getSymbol()->getTokenIndex();
            CodeMutation codeMutation;
            codeMutation.m_replace.emplace("");
            m_mutations.emplace(tokenIndex, codeMutation);
        }

        // Drop the last argument.
        auto vectorOfArguments = argumentsCtx->expression();
        auto lastArgumentIndex = vectorOfArguments.size() - 1;
        auto lastArgumentCtx = vectorOfArguments[lastArgumentIndex];
        // Capture the name of the variable that gets the number of samples because
        // it will be assigned the value 1.
        string lastArgumentName = lastArgumentCtx->getText();
        {
            const ssize_t startingTokenIndex = lastArgumentCtx->start->getTokenIndex();
            const ssize_t stoppingTokenIndex = lastArgumentCtx->stop->getTokenIndex();
            for (ssize_t tokenIndex = startingTokenIndex; tokenIndex <= stoppingTokenIndex; ++tokenIndex)
            {
                CodeMutation codeMutation;
                codeMutation.m_replace.emplace("");
                m_mutations.emplace(tokenIndex, codeMutation);
            }
        }

        // Get the rule context for the closing right parenthesis ")".
        // "; samples = 1" will be added after it.
        const auto rightParenthesisNode = ctx->argumentList()->RightParen();
        {
            const ssize_t parenthesisTokenIndex = rightParenthesisNode->getSymbol()->getTokenIndex();
            CodeMutation codeMutation;
            string samplesExpression = AZ::FormatString("; %s = 1 ", lastArgumentName.c_str());
            codeMutation.m_append.emplace(samplesExpression);
            m_mutations.emplace(parenthesisTokenIndex, codeMutation);
        }
    }

    size_t Texture2DMSto2DCodeMutator::MutateTypeOfMultiSampleVariables()
    {
        size_t mutationCount = 0;

        // Get all variables that are members of something of type Texture2DMS
        // We use this function pointer to find SRGs that have no references.
        auto texture2DMSFilterFunc = +[](KindInfo* kindInfo) {
            const auto* varInfo = kindInfo->GetSubAs<VarInfo>();
            return varInfo->m_typeInfoExt.m_coreType.m_typeClass == TypeClass::MultisampledTexture;
        };

        vector<IdentifierUID> texture2DMSVariables = m_ir->GetFilteredSymbolsOfSubType<VarInfo>(texture2DMSFilterFunc);
        for (const auto& uid : texture2DMSVariables)
        {
            auto varInfo = m_ir->GetSymbolSubAs<VarInfo>(uid.GetName());
            auto& typeId = varInfo->m_typeInfoExt.m_coreType.m_typeId;
            auto typeName = typeId.GetName();
            if (typeName == "?Texture2DMS")
            {
                typeId.m_name = QualifiedName{ "?Texture2D" };
            }
            else
            {
                typeId.m_name = QualifiedName{ "?Texture2DArray" };
            }
            ++mutationCount;
        }

        return mutationCount;
    }

    void Texture2DMSto2DCodeMutator::MutateMultiSampleSystemSemantics()
    {
        //Let's find all variables that have a system semantic.
        auto variablesWithSystemSemanticFilterFunc = +[](KindInfo* kindInfo) {
            const auto* varInfo = kindInfo->GetSubAs<VarInfo>();
            if (!varInfo->m_declNode)
            {
                return false;
            }
            auto* semanticOption = varInfo->m_declNode->SemanticOpt;
            if (!semanticOption)
            {
                return false;
            }
            return semanticOption->hlslSemanticName()->HLSLSemanticSystem() != nullptr;
        };

        vector<IdentifierUID> systemSemanticVariables = m_ir->GetFilteredSymbolsOfSubType<VarInfo>(variablesWithSystemSemanticFilterFunc);
        for (const auto& uid : systemSemanticVariables)
        {
            auto varInfo = m_ir->GetSymbolSubAs<VarInfo>(uid.GetName());

            // Get the semantic name.
            auto systemSemanticName = varInfo->m_declNode->SemanticOpt->hlslSemanticName()->HLSLSemanticSystem()->getText();

            static const std::array<string_view, 2> SystemSemanticsNames =
            {
                "SV_SampleIndex",
                "SV_Coverage",
            };

            if (!IsIn(systemSemanticName, SystemSemanticsNames))
            {
                continue;
            }

            //Semantics can be part of a struct, or function parameters.
            if (ParamContextOverUnnamedVariableDeclarator(varInfo->m_declNode))
            {
                // This is a function parameter.
                IdentifierUID functionUid = IdentifierUID{ GetParentName(uid.GetName()) };
                DropMultiSamplingSystemSemanticFromFunction(uid, varInfo, systemSemanticName, functionUid);
            }
            else
            {
                // This is a variable within a struct
                IdentifierUID structUid = IdentifierUID{ GetParentName(uid.GetName()) };
                MutateMultiSamplingSystemSemanticInStruct(uid, varInfo, systemSemanticName, structUid);
            }
        }
    }

    static vector<Token*> NodeTokens(ParserRuleContext* node, TokenStream* stream)
    {
        vector<Token*> tokens;
        misc::Interval src = node->getSourceInterval();
        for (ssize_t i = src.a; i <= src.b; ++i)
        {
            tokens.push_back(stream->get(i));
        }
        return tokens;
    }

    //! A helper method that figures out how a function argument should look like
    //! when mutated into a local variable.
    static string GetLocalVariableStringFromFunctionArgument(TokenStream* stream, const UnqualifiedName& uqName, AstUnnamedVarDecl* ctx, const char * initializationValue)
    {
        azslParser::FunctionParamContext* paramCtx = nullptr;
        auto typeCtx = ExtractTypeFromUnnamedVariableDeclarator(ctx, &paramCtx);
        auto tokens = NodeTokens(typeCtx, stream);
        vector<string> stringlets;
        TransformCopy(tokens, stringlets, [&](Token* t) { return t->getText(); });
        vector<string> filtered;
        std::copy_if(stringlets.begin(), stringlets.end(), std::back_inserter(filtered), [&](auto subtok)
                     { return subtok != "in" && subtok != "out" && subtok != "inout" && !IsAllWhitespaces(subtok); });
        string typeHlsl = Join(filtered, " ");
        if (initializationValue)
        {
            return FormatString("%s %s = (%s)%s;\n", typeHlsl.c_str(), uqName.c_str(), typeHlsl.c_str(), initializationValue);
        }
        return FormatString("%s %s;\n", typeHlsl.c_str(), uqName.c_str());
    }

    void Texture2DMSto2DCodeMutator::DropMultiSamplingSystemSemanticFromFunction(const IdentifierUID& varUid, const VarInfo* varInfo, const string& systemSemanticName, const IdentifierUID& functionUid)
    {
        // Let's get the FunctionInfo object and report this variable, which will be dropped from the
        // input arguments and will be re-emitted as a local variable to avoid compiler errors from other
        // pieces of code that may reference the semantic.
        // Example:
        // PSOutput mainPS(VSOutput IN, in uint sampleIndex : SV_SampleIndex)
        // {
        //     ...
        //     int2 sampleIndexVector = int2(sampleIndex, sampleIndex);
        //     ...
        // }
        // Will look like this when emitted (Which will avoid compilation errors)
        // PSOutput mainPS(VSOutput IN)
        // {
        //     uint sampleIndex = 0;
        //     ...
        //     int2 sampleIndexVector = int2(sampleIndex, sampleIndex);
        //     ...
        // }
        FunctionInfo* functionInfo = m_ir->GetSymbolSubAs<FunctionInfo>(functionUid.GetName());
        functionInfo->DeleteParameter(varUid);

        string initializationValue = "0";
        if (systemSemanticName == "SV_Coverage")
        {
            // SV_Coverage is a mask where each bit represents active sample indices.
            // In this case we initialize to -1, because bitwise it will look like as if
            // all samples are active.
            // Usually code that that uses SV_Coverage loops over this mask (limited by the number of samples,
            // which will be 1 for no-MS) for each sampleIndex.
            // By settings to -1 it will mimic full coverage and the rendering logic will
            // work seamlessly. It could be set to "1", but "-1" would cover all cases.
            initializationValue = "-1";
        }

        auto newCode = GetLocalVariableStringFromFunctionArgument(m_stream, varUid.GetNameLeaf(), varInfo->m_declNode, initializationValue.c_str());

        // The idea is to find the TokenIndex of the opening bracket "{",
        // Once we know that TokenIndex we can add code mutation as an appended
        // string.
        auto hlslFunctionDefinitionContext = ExtractSpecificParent<azslParser::HlslFunctionDefinitionContext>(functionInfo->m_defNode);
        auto blockContext = hlslFunctionDefinitionContext->block();
        auto leftBraceTokenIndex = blockContext->LeftBrace()->getSymbol()->getTokenIndex();
        auto itor = m_mutations.find(leftBraceTokenIndex);
        if (itor == m_mutations.end())
        {
            CodeMutation mutation;
            mutation.m_append.emplace(newCode);
            m_mutations.emplace(leftBraceTokenIndex, mutation);
        }
        else
        {
            CodeMutation& mutation = itor->second;
            string prevCode = mutation.m_append.value();
            mutation.m_append.emplace(prevCode + newCode);
        }
    }

    void Texture2DMSto2DCodeMutator::MutateMultiSamplingSystemSemanticInStruct(const IdentifierUID& varUid, const VarInfo* varInfo, const string& systemSemanticName, [[maybe_unused]] const IdentifierUID& structUid)
    {
        // This is the case of member variable of a struct, but it is a system semantic.
        // Example:
        // struct VSOutput
        // {
        //     float4 m_position : SV_Position;
        //     float2 m_texCoord : TEXCOORD0;
        //     uint   m_sampleIndex : SV_SampleIndex;  <--- This is the variable in question.
        // };
        // Will look like this when emitted (Which will avoid compilation errors)
        // PSOutput mainPS(VSOutput IN)
        // {
        //     float4 m_position : SV_Position;
        //     float2 m_texCoord : TEXCOORD0;
        //     static const uint   m_sampleIndex = 0; <-- Became a static const, and of course, the semantic is removed.
        //     ...
        // }
        string initializationValue = "0";
        if (systemSemanticName == "SV_Coverage")
        {
            initializationValue = "-1";
        }
        auto newCode = GetLocalVariableStringFromFunctionArgument(m_stream, varUid.GetNameLeaf(), varInfo->m_declNode, initializationValue.c_str());
        auto tokenIndex = varInfo->m_declNode->start->getTokenIndex();
        CodeMutation mutation;
        mutation.m_prepend.emplace("static const ");
        mutation.m_replace.emplace(newCode);
        m_mutations.emplace(tokenIndex, mutation);
    }

} //namespace AZ::ShaderCompiler
