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

namespace AZ::ShaderCompiler
{
    //! This is the main class that handles coversion of MultiSampling related
    //! variables, function calls and system semantics to their non-MultiSampling
    //! versions.
    //! 
    //! This class is only active if --no-ms compiler flag is specified. Only if the --no-ms
    //! compiler flag is active then the following events are true:
    //!     - As a first step, it implements azslParserBaseListener::enterFunctionCallExpression() so
    //!       the SemaCheckListener can invoke it.
    //!     - After AST parsing is done, RunMiddleEndMutations() is called at the end of IntermediateRepresentation::MiddleEnd() step to
    //!      calculate further code mutations.
    //!     - During code emission, the CodeEmitter invokes the ICodeEmissionMutator interface to see if token
    //!       variables or lines of codes have mutations available.
    struct Texture2DMSto2DCodeMutator 
        : public azslParserBaseListener
        , public ICodeEmissionMutator
    {
        Texture2DMSto2DCodeMutator() = delete;
        explicit Texture2DMSto2DCodeMutator(IntermediateRepresentation* ir, CommonTokenStream* stream) : m_ir(ir), m_stream(stream) {}
        virtual ~Texture2DMSto2DCodeMutator() = default;

        ///////////////////////////////////////////////////////////////////////
        // azslParserBaseListener Overrides ...
        void enterFunctionCallExpression(azslParser::FunctionCallExpressionContext* ctx) override;
        ///////////////////////////////////////////////////////////////////////

        ///////////////////////////////////////////////////////////////////////
        // ICodeEmissionMutator Overrides ...
        const CodeMutation* GetMutation(ssize_t tokenId) const override;
        ///////////////////////////////////////////////////////////////////////

        //! Should be called at the end of the IntermediateRepresentation::MiddleEnd() transformations.
        void RunMiddleEndMutations();

    private:
        //! Called when parsing a function call expression of type
        //! Texture2DMS.Load(...)
        void OnEnterLoad(azslParser::FunctionCallExpressionContext* ctx);
        
        //! Called when parsing a function call expression of type
        //! Texture2DMS.GetSamplePosition(...)
        void OnEnterGetSamplePosition(azslParser::FunctionCallExpressionContext* ctx);

        //! Called when parsing a function call expression of type
        //! Texture2DMS.GetDimensions(...)
        void OnEnterGetDimensions(azslParser::FunctionCallExpressionContext* ctx);

        //! Changes the variable types:
        //!     Texture2DMS to Texture2D.
        //!     Texture2DMSArray to Texture2DArray. 
        //! Returns the number of variables whose type was mutated
        size_t MutateTypeOfMultiSampleVariables();

        //! Mutates or Drops system semantics from struct definitions or
        //! input arguments in entry point shader functions.
        //! See Texture2DMSto2DCodeMutator.cpp for mutation examples.
        void MutateMultiSampleSystemSemantics();

        //! Called when a function argument, in a function definition, is a variable qualified
        //! with the system semantics SV_SampleIndex or SV_Coverage. When converting to no-MultiSampling
        //! the variable must be removed from the input arguments of the function.
        //! See Texture2DMSto2DCodeMutator.cpp for mutation examples.
        void DropMultiSamplingSystemSemanticFromFunction(const IdentifierUID& varUid, const VarInfo* varInfo, const string& systemSemanticName, const IdentifierUID& functionUid);

        //! Called when a member variable, in a struct definition, is a variable qualified
        //! with the system semantics SV_SampleIndex or SV_Coverage. When converting to no-MultiSampling
        //! the variable must be mutated into an initialized "static const" of the same type as
        //! the semantic.
        //! See Texture2DMSto2DCodeMutator.cpp for mutation examples. 
        void MutateMultiSamplingSystemSemanticInStruct(const IdentifierUID& varUid, const VarInfo* varInfo, const string& systemSemanticName, const IdentifierUID& structUid);

        //! Given an unqualified symbol name, checks within the current parsing scope
        //! if the symbol is a MultiSampling type of variable. 
        enum class TextureMSType { None, Texture2DMS, Texture2DMSArray };
        TextureMSType GetMultiSampledTextureClass(const UnqualifiedName& uqSymbolName);

        //! Cached when RunMiddleEndMutations is called.
        IntermediateRepresentation* m_ir = nullptr;

        CommonTokenStream* m_stream = nullptr;

        //! A map of TokenIndex to Mutation. If a TokenIndex is present,
        //! it means it should produce mutated text during emission.
        unordered_map< ssize_t, CodeMutation > m_mutations;
    };
} // namespace AZ::ShaderCompiler
