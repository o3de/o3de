/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include "AzslcListener.h"
#include "NewLineCounterStream.h"

#include "jsoncpp/json/json.h"
#include "AzslcRegisters.h"

namespace AZ::ShaderCompiler
{
    struct PlatformEmitter;

    struct DescriptorCountBounds
    {
        int m_descriptorsTotal = -1;   //!< negative values deactivate the check
        int m_spaces = -1;
        int m_samplers = -1;
        int m_textures = -1;
        int m_buffers = -1;
    };

    //! This structure is typically filled from parsed user settings from the command line
    struct Options
    {
        bool m_useUniqueIndices = false;
        bool m_emitConstantBufferBody = false;
        bool m_emitRootSig = false;
        bool m_forceMatrixRowMajor = false;     //!< False by default (HLSL standard)
        bool m_forceEmitMajor = false;   //!< True if either -Zpc or -Zpr was specified
        bool m_padRootConstantCB = false; //!< If True, the emitted root constant CB will padded to 16-byte boundary.
        bool m_skipAlignmentValidation = false; //! < If True, disables validation of a known DXC issue when certain word or 2-words size variables are preceded by some MatrixRxC variables.
        DescriptorCountBounds m_minAvailableDescriptors;   //!< Hint about the targeted graphics API's minimal guaranteed usable descriptors
        int m_maxSpaces = std::numeric_limits<int>::max();   //!< Maximum allocatable register logical space, after which register indexes will accumulate, but spaces will be capped
        int m_rootConstantsMaxSize = std::numeric_limits<int>::max();   //!< Indicates the number of root constants to be allowed, 0 means root constants not enabled
        Packing::Layout m_packConstantBuffers  = Packing::Layout::DirectXPacking; //!< Packing standard for constant buffers (uniform)
        Packing::Layout m_packDataBuffers      = Packing::Layout::CStylePacking;  //!< Packing standard for data buffer views
        bool m_useSpecializationConstantsForOptions = false; //!< Use specialization constants for shader options
        int32_t m_subpassInputsOffset = 0; //< Offset to apply to the subpass index
    };

    struct Binding
    {
        int m_registerIndex = -1;
        int m_logicalSpace = -1;
    };

    //! store 2 sets of binding points: one calculated purely (untainted), and one calculated with space compression (SRG-Merging)
    struct BindingPair
    {
        MAKE_REFLECTABLE_ENUM(Set, Untainted, Merged);
        std::array<Binding, Set::EndEnumeratorSentinel_> m_pair;
    };

    struct RootSigDesc
    {
        struct SrgParamDesc
        {
            IdentifierUID m_uid;
            RootParamType m_type;
            BindingPair m_registerBinding;
            int m_registerRange = -1;
            int m_num32BitConstants = -1;

            // This flag is added so m_registerRange can take the value
            // of 1 and at the same time We do not forget that m_uid refers
            // to an unbounded array.
            bool m_isUnboundedArray = false;
        };

        struct SrgDesc
        {
            IdentifierUID m_uid;
            vector<SrgParamDesc> m_parameters;
        };

        const SrgParamDesc& Get(const IdentifierUID& uid) const
        {
            return m_descriptorMap.at(uid);
        }

        vector<SrgDesc> m_srGroups;
        unordered_map<IdentifierUID, SrgParamDesc> m_descriptorMap;
    };

    //! Stateful tracker that helps to construct binding points, by counting up
    struct SingleBindingLocationTracker
    {
        //! Increments logical space index and nullifies all register indices
        void IncrementSpace();

        //! Equalizes all register indices (important for platforms with no B,T,S,U distinction)
        void UnifyIndices();

        //! Access total burned up resources, e.g for hardware capacity checks
        int GetAccumulated(BindingType r) const;

        int m_space = 0;  //<! logical space
        int m_registerPos[BindingType::EndEnumeratorSentinel_] = {0};   //!< register index, one by type.
        int m_accumulated[BindingType::EndEnumeratorSentinel_] = {0};  //!< Counter for total usage independently from space increments
        int m_accumulatedUnused[BindingType::EndEnumeratorSentinel_] = {0};  //!< Counter for holes created by indices unification
    };

    //! Because of space limitations on some devices, we needed to introduce SRG-merging.
    //! For minimal engine impact, we need to be able to reflect the "untainted" (non-merged) binding scheme,
    //!  and the merged scheme together. For that purpose, we will track 2 binding locations in that maker.
    class MultiBindingLocationMaker
    {
    public:
        MultiBindingLocationMaker(const Options& options)
            : m_options(options)
        {}

        void SignalIncrementSpace(std::function<void(int, int)> warningMessageFunctionForMinDescOvershoot);

        void SignalUnifyIndices();
        
        void SignalIncrementRegister(BindingType regType, int count);

        BindingPair GetCurrent(BindingType regType);

        SingleBindingLocationTracker m_untainted;
        SingleBindingLocationTracker m_merged;
        Options m_options;
    };

    //! This class intends to be a base umbrella for compiler back-end services.
    //! A back-end service typically provides reflection or code emission.
    //! Since there are re-used commonalities in such systems, it is helpful
    //! to share their common data and methods in this base class.
    class Backend
    {
    public:
        Backend(IntermediateRepresentation* ir, TokenStream* tokens)
        : m_ir(ir), m_tokens(tokens)
        {}

        virtual ~Backend() = default;

        //! Gets the IntermediateRepresentation object
        const IntermediateRepresentation* GetIR() const { return m_ir; }

        //! Make a string that lists all type qualifiers/modifiers in HLSL format
        static string GetTypeModifier(const ExtendedTypeInfo&, const Options& options, Modifiers bannedFlags = {});

        //! Get HLSL form of in/out modifiers
        static const char* GetInputModifier(const TypeQualifiers& typeQualifier);

        //! Get the initialization clause as a string. Returns an empty string if it doesn't have any initialization.
        string GetInitializerClause(const AZ::ShaderCompiler::VarInfo* varInfo) const;

        //! Fabricate a HLSL snippet that represents the type stored in typeInfo. Relevant options relate to matrix qualifiers.
        //! \param banned is the Flag you can setup to list a collection of type qualifiers you don't want to reproduce.
        string GetExtendedTypeInfo(const ExtendedTypeInfo& extTypeInfo, const Options& options, Modifiers banned, std::function<string(const TypeRefInfo&)> translator) const;

        //! Returns the Platform Emitter that is registered to the namespace in the "ir".
        static const PlatformEmitter& GetPlatformEmitter(IntermediateRepresentation* ir);

    protected:
        //! Obtains a supplement emitter which provides per-platform emission functionality.
        const PlatformEmitter& GetPlatformEmitter() const;

        //! Gets the next and increments tokenIndex. TokenIndex must be in the [misc::Interval.a, misc::Interval.b] range. Token cannot be nullptr.
        auto GetNextToken(ssize_t& tokenIndex, size_t channel = Token::DEFAULT_CHANNEL) const -> antlr4::Token*;

        virtual void EmitTranspiledTokens(misc::Interval interval, Streamable& output) const;

        string GetTranspiledTokens(misc::Interval interval) const;

        uint32_t GetNumberOf32BitConstants(const Options& options, const IdentifierUID& uid) const;

        RootSigDesc BuildSignatureDescription(const Options& options, int num32BitConst) const;

        RootSigDesc::SrgParamDesc ReflectOneExternalResource(IdentifierUID id, MultiBindingLocationMaker& bindInfo, RootSigDesc& rootSig) const;

        RootSigDesc::SrgParamDesc ReflectOneExternalResourceAndWrapWithUnifyIndices(IdentifierUID id, MultiBindingLocationMaker& bindInfo, RootSigDesc& rootSig) const;

        void AppendOptionRange(Json::Value& varOption, const IdentifierUID& varUid, const AZ::ShaderCompiler::VarInfo* varInfo, const Options& options) const;

        Json::Value GetVariantList(const Options& options, bool includeEmpty = false) const;

        void SetupOptionsSpecializationId(const Options& options) const;

        IntermediateRepresentation*      m_ir;
        TokenStream*                     m_tokens;
        
        // On some platforms (DX12), descriptor arrays occupy an individual register slot, and spaces are used
        // to prevent overlapping ranges. When an unbounded array is encountered, we immediately assign it to
        // the value of this member variable and increment. This is initialized in the constructor because the
        // space we spill to must not collide with any other SRG declared in the shader.
        mutable SpaceIndex m_unboundedSpillSpace = FirstUnboundedSpace;
    };

    // independent utility functions
    bool IsReadWriteView(string_view viewName);

    QualifiedName MakeSrgConstantsStructName(IdentifierUID srg);

    QualifiedName MakeSrgConstantsCBName(IdentifierUID srg);

    /// don't use for HLSL emission (this doesn't go through translation)
    string UnmangleTrimedName(const QualifiedNameView name);

    string JoinAllNestedNamesWithUnderscore(const QualifiedNameView name);

    string GetGlobalRootConstantVarName(const QualifiedNameView name);

    string GetShaderKeyFunctionName(const IdentifierUID& uid);

    string GetRootConstFunctionName(const IdentifierUID& uid);

    // don't use for HLSL emission (this doesn't go through translation)
    string UnmangleTrimedName(const ExtendedTypeInfo& extTypeInfo);

    Streamable& operator << (Streamable& out, const SamplerStateDesc::AddressMode& addressMode);
    Streamable& operator << (Streamable& out, const SamplerStateDesc::ComparisonFunc& compFunc);
    Streamable& operator << (Streamable& out, const SamplerStateDesc::BorderColor& borderColor);
    Streamable& operator << (Streamable& out, const SamplerStateDesc& samplerDesc);
    Streamable& operator << (Streamable& out, const SamplerStateDesc::ReductionType& redcType);
}
