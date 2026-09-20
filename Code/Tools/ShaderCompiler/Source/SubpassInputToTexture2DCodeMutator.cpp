/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "SubpassInputToTexture2DCodeMutator.h"


namespace AZ::ShaderCompiler
{
    static constexpr char SubpassFunctionNameLoad[] = "SubpassLoad";

    ///////////////////////////////////////////////////////////////////////
    // azslParserBaseListener Overrides ...
    void SubpassInputToTexture2DCodeMutator::enterFunctionCallExpression(azslParser::FunctionCallExpressionContext* ctx)
    {
        const auto expressionCtx = ctx->expression();
        const std::string functionName = expressionCtx->stop->getText();

        if (functionName == SubpassFunctionNameLoad)
        {
            OnEnterLoad(ctx);
        }
    }
    ///////////////////////////////////////////////////////////////////////


    ///////////////////////////////////////////////////////////////////////
    // ICodeEmissionMutator Overrides ...
    const CodeMutation* SubpassInputToTexture2DCodeMutator::GetMutation(ssize_t tokenId) const
    {
        auto itor = m_mutations.find(tokenId);
        if (itor == m_mutations.end())
        {
            return nullptr;
        }
        return &itor->second;
    }

    bool SubpassInputToTexture2DCodeMutator::RunMiddleEndMutations()
    {
        // Get all variables that are members of something of type Texture2DMS
        // We use this function pointer to find SRGs that have no references.
        auto subpassInputFilterFunc = +[](KindInfo* kindInfo) {
            const auto* varInfo = kindInfo->GetSubAs<VarInfo>();
            return varInfo->m_typeInfoExt.m_coreType.m_typeClass == TypeClass::SubpassInput;
            };

        vector<IdentifierUID> subpassInputVariables = m_ir->GetFilteredSymbolsOfSubType<VarInfo>(subpassInputFilterFunc);
        MutateTypeOfMultiSampleVariables(subpassInputVariables);
        return !subpassInputVariables.empty();
    }

    ///////////////////////////////////////////////////////////////////////

    static UnqualifiedName GetSubpassSymbolName(azslParser::ExpressionContext* expressionCtx)
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

    void SubpassInputToTexture2DCodeMutator::OnEnterLoad(azslParser::FunctionCallExpressionContext* ctx)
    {
        // First we must capture the complete name of the symbol that called <Symbol>.Load(...)
        const auto expressionCtx = ctx->expression();
        const UnqualifiedName uqSymbolName = GetSubpassSymbolName(expressionCtx);
        const SubpassInputType subpassInputType = GetSubpassInputClass(uqSymbolName);
        if (subpassInputType == SubpassInputType::None)
        {
            return;
        }
        
        // Define the mutations.
        if (IsSubpassInputSupported(subpassInputType))
        {
            const auto argumentListCtx = ctx->argumentList();
            if (argumentListCtx)
            {
                const auto argumentsCtx = argumentListCtx->arguments();
                auto vectorOfArguments = argumentsCtx->expression();

                // Found a "SubpassLoad" call and SubpassInputs are supported.
                // We need to remove all parameters.
                // m_subpassInput.SubpassLoad(int3(1, 2, 0)) ----> m_subpassInput.SubpassLoad()
                for (uint32_t i = 0; i < vectorOfArguments.size(); ++i)
                {
                    auto secondArgContext = vectorOfArguments[i];
                    const ssize_t startingTokenIndex = secondArgContext->start->getTokenIndex();
                    const ssize_t stoppingTokenIndex = secondArgContext->stop->getTokenIndex();
                    for (ssize_t tokenIndex = startingTokenIndex; tokenIndex <= stoppingTokenIndex; ++tokenIndex)
                    {
                        CodeMutation codeMutation;
                        codeMutation.m_replace.emplace("");
                        m_mutations.emplace(tokenIndex, codeMutation);
                    }
                }
            }
        }
        else
        {
            // Replace SubpassLoad -> Load and keed the arguments
            // m_subpassInput.SubpassLoad(int3(1, 2, 0)) ----> m_subpassInput.Load(int3(1, 2, 0))
            CodeMutation codeMutation;
            codeMutation.m_replace.emplace("Load");
            m_mutations.emplace(expressionCtx->stop->getTokenIndex(), codeMutation);
        }
    }

    SubpassInputToTexture2DCodeMutator::SubpassInputType SubpassInputToTexture2DCodeMutator::GetSubpassInputClass(const UnqualifiedName& uqSymbolName)
    {
        if (uqSymbolName.empty())
        {
            return SubpassInputType::None;
        }
        // We only care if the symbol that is calling SubpassLoad(...) is of type SubpassInput or SubpassInputMS
        IdAndKind* idkind = m_ir->m_sema.LookupSymbol(uqSymbolName);
        if (!idkind)
        {
            return SubpassInputType::None;
        }
        auto& [uid, kind] = *idkind;
        if (kind.GetKind() != Kind::Variable)
        {
            return SubpassInputType::None;
        }
        auto varInfo = kind.GetSubAs<VarInfo>();
        return GetSubpassInputClass(varInfo);
    }

    SubpassInputToTexture2DCodeMutator::SubpassInputType SubpassInputToTexture2DCodeMutator::GetSubpassInputClass(const VarInfo* varInfo)
    {
        if (varInfo->GetTypeClass() != TypeClass::SubpassInput)
        {
            return SubpassInputType::None;
        }

        if (EndsWith(varInfo->m_typeInfoExt.m_coreType.m_typeId.GetName(), "MS"))
        {
            return EndsWith(varInfo->m_typeInfoExt.m_coreType.m_typeId.GetName(), "DSMS") ? SubpassInputType::SubpassInputDSMS : SubpassInputType::SubpassInputMS;
        }
        return EndsWith(varInfo->m_typeInfoExt.m_coreType.m_typeId.GetName(), "DS") ? SubpassInputType::SubpassInputDS : SubpassInputType::SubpassInput;
    }

    size_t SubpassInputToTexture2DCodeMutator::MutateTypeOfMultiSampleVariables(const vector<IdentifierUID>& subpassInputVariables)
    {
        size_t mutationCount = 0;
        for (const auto& uid : subpassInputVariables)
        {
            auto varInfo = m_ir->GetSymbolSubAs<VarInfo>(uid.GetName());
            auto subpassType = GetSubpassInputClass(varInfo);
            auto& typeId = varInfo->m_typeInfoExt.m_coreType.m_typeId;
            bool isSupported = IsSubpassInputSupported(subpassType);
            switch (subpassType)
            {
            case SubpassInputType::SubpassInput:
            case SubpassInputType::SubpassInputDS:
                typeId.m_name = isSupported ? QualifiedName{ "?SubpassInput" } : QualifiedName{ "?Texture2D" };
                break;
            case SubpassInputType::SubpassInputMS:
            case SubpassInputType::SubpassInputDSMS:
                typeId.m_name = isSupported ? QualifiedName{ "?SubpassInputMS" } : QualifiedName{ "?Texture2DMS" };
                break;
            default:
                break;
            }
            ++mutationCount;
        }

        return mutationCount;
    }
    bool SubpassInputToTexture2DCodeMutator::IsSubpassInputSupported(const SubpassInputType type)
    {
        bool isDepthStencilView = type == SubpassInputType::SubpassInputDS || type == SubpassInputType::SubpassInputDSMS;
        bool subpassInputSupported = (static_cast<uint32_t>(m_subpassInputSupport) & static_cast<uint32_t>(SubpassInputSupportFlag::Color)) && !isDepthStencilView;
        subpassInputSupported |= (static_cast<uint32_t>(m_subpassInputSupport) & static_cast<uint32_t>(SubpassInputSupportFlag::DepthStencil)) && isDepthStencilView;

        return subpassInputSupported;
    }
} //namespace AZ::ShaderCompiler
