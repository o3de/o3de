/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include "GenericUtils.h"
#include "AzslcUtils.h"
#include "AzslcCodeEmissionMutator.h"
#include "AzslcIntermediateRepresentation.h"
#include "AzslcPlatformEmitter.h"

namespace AZ::ShaderCompiler
{
    //! This is the main class that handles coversion of SubpassInput related
    //! variables, function calls and system semantics to the Texture2D versions.
    //! 
    //!     - As a first step, it implements azslParserBaseListener::enterFunctionCallExpression() so
    //!       the SemaCheckListener can invoke it for the "SubpassLoad" function.
    //!     - After AST parsing is done, RunMiddleEndMutations() is called at the end of IntermediateRepresentation::MiddleEnd() step to
    //!      calculate further code mutations.
    //!     - During code emission, the CodeEmitter invokes the ICodeEmissionMutator interface to see if token
    //!       variables or lines of codes have mutations available.
    struct SubpassInputToTexture2DCodeMutator 
        : public azslParserBaseListener
        , public ICodeEmissionMutator
    {
        SubpassInputToTexture2DCodeMutator() = delete;
        explicit SubpassInputToTexture2DCodeMutator(IntermediateRepresentation* ir, CommonTokenStream* stream, SubpassInputSupportFlag subpassInputSupport)
            : m_ir(ir)
            , m_stream(stream)
            , m_subpassInputSupport(subpassInputSupport) {}
        virtual ~SubpassInputToTexture2DCodeMutator() = default;

        ///////////////////////////////////////////////////////////////////////
        // azslParserBaseListener Overrides ...
        void enterFunctionCallExpression(azslParser::FunctionCallExpressionContext* ctx) override;
        ///////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////
        // ICodeEmissionMutator Overrides ...
        const CodeMutation* GetMutation(ssize_t tokenId) const override;
        ///////////////////////////////////////////////////////////////////////

        //! Should be called at the end of the IntermediateRepresentation::MiddleEnd() transformations.
        bool RunMiddleEndMutations();

    private:
        //! Called when parsing a function call expression of type
        //! SubpassInput.SubpasLoad(...)
        void OnEnterLoad(azslParser::FunctionCallExpressionContext* ctx);

        //! Given an unqualified symbol name, checks within the current parsing scope
        //! if the symbol is a SubpassInput type of variable. 
        enum class SubpassInputType { None, SubpassInput, SubpassInputMS, SubpassInputDS, SubpassInputDSMS };
        SubpassInputType GetSubpassInputClass(const UnqualifiedName& uqSymbolName);
        SubpassInputType GetSubpassInputClass(const VarInfo* varInfo);

        //! Changes the variable types:
        //!     SubpassInput to Texture2D.
        //!     SubpassInputMS to Texture2DMS. 
        //! Returns the number of variables whose type was mutated
        size_t MutateTypeOfMultiSampleVariables(const vector<IdentifierUID>& subpassInputVariables);

        //! Returns true if the SubpassInput is supported by the platform emitter.
        bool IsSubpassInputSupported(const SubpassInputType type);
        
        //! Cached when RunMiddleEndMutations is called.
        IntermediateRepresentation* m_ir = nullptr;

        [[maybe_unused]] CommonTokenStream* m_stream = nullptr;

        //! A map of TokenIndex to Mutation. If a TokenIndex is present,
        //! it means it should produce mutated text during emission.
        unordered_map<ssize_t, CodeMutation > m_mutations;

        //! Subpass Input support.
        SubpassInputSupportFlag m_subpassInputSupport = SubpassInputSupportFlag::None;
    };
} // namespace AZ::ShaderCompiler
