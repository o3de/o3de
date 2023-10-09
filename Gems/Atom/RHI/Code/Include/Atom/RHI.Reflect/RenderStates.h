/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/Limits.h>
#include <Atom/RHI.Reflect/Format.h>
#include <Atom/RHI.Reflect/SamplerState.h>
#include <Atom/RHI.Reflect/MultisampleState.h>
#include <AzCore/Preprocessor/Enum.h>

namespace AZ
{
    class ReflectContext;
}

namespace AZ::RHI
{
    AZ_ENUM_CLASS_WITH_UNDERLYING_TYPE(CullMode, uint32_t,
        None,
        Front,
        Back,
        Invalid
    );

    AZ_ENUM_CLASS_WITH_UNDERLYING_TYPE(FillMode, uint32_t,
        Solid,
        Wireframe,
        Invalid
    );

    AZ_ENUM_CLASS_WITH_UNDERLYING_TYPE(DepthWriteMask, uint32_t,
        Zero,
        All,
        Invalid
    );

    AZ_ENUM_CLASS_WITH_UNDERLYING_TYPE(StencilOp, uint32_t,
        Keep,
        Zero,
        Replace,
        IncrementSaturate,
        DecrementSaturate,
        Invert,
        Increment,
        Decrement,
        Invalid
    );

    AZ_ENUM_CLASS_WITH_UNDERLYING_TYPE(BlendFactor, uint32_t,
        Zero,
        One,
        ColorSource,
        ColorSourceInverse,
        AlphaSource,
        AlphaSourceInverse,
        AlphaDest,
        AlphaDestInverse,
        ColorDest,
        ColorDestInverse,
        AlphaSourceSaturate,
        Factor,
        FactorInverse,
        ColorSource1,
        ColorSource1Inverse,
        AlphaSource1,
        AlphaSource1Inverse,
        Invalid
    );

    AZ_ENUM_CLASS_WITH_UNDERLYING_TYPE(BlendOp, uint32_t,
        Add,               //!< Result = Source + Destination
        Subtract,          //!< Result = Source - Destination
        SubtractReverse,   //!< Result = Destination - Source
        Minimum,           //!< Result = MIN(Source, Destination)
        Maximum,           //!< Result = MAX(Source, Destination)
        Invalid
    );

    void ReflectRenderStateEnums(ReflectContext* context);

    struct RasterState
    {
        AZ_TYPE_INFO(RasterState, "{57D4BE50-EBE2-4ABE-90A4-C99BF2EA43FB}");
        static void Reflect(ReflectContext* context);

        bool operator == (const RasterState& rhs) const;

        FillMode m_fillMode = FillMode::Solid;
        CullMode m_cullMode = CullMode::Back;
        int32_t m_depthBias = 0;
        float m_depthBiasClamp = 0.0f;
        float m_depthBiasSlopeScale = 0.0f;
        uint32_t m_multisampleEnable = 0;
        uint32_t m_depthClipEnable = 1;
        uint32_t m_conservativeRasterEnable = 0;
        uint32_t m_forcedSampleCount = 0;
    };

    struct DepthState
    {
        AZ_TYPE_INFO(DepthState, "{5F321456-052F-41F1-BD35-2D34CB26DD9D}");
        static void Reflect(ReflectContext* context);

        bool operator == (const DepthState& rhs) const;

        uint32_t m_enable = 1;
        DepthWriteMask m_writeMask = DepthWriteMask::All;
        ComparisonFunc m_func = ComparisonFunc::Less;
    };

    struct StencilOpState
    {
        AZ_TYPE_INFO(StencilOpState, "{6B0894AA-7FE9-4EB0-8171-FF0872CB9B7F}");
        static void Reflect(ReflectContext* context);

        bool operator == (const StencilOpState& rhs) const;

        StencilOp m_failOp = StencilOp::Keep;
        StencilOp m_depthFailOp = StencilOp::Keep;
        StencilOp m_passOp = StencilOp::Keep;
        ComparisonFunc m_func = ComparisonFunc::Always;
    };

    struct StencilState
    {
        AZ_TYPE_INFO(StencilState, "{098EAE83-A3F3-4270-B7AC-ACD11366BBB9}");
        static void Reflect(ReflectContext* context);

        bool operator == (const StencilState& rhs) const;

        uint32_t m_enable = 0;
        uint32_t m_readMask = 0xFF;
        uint32_t m_writeMask = 0xFF;
        StencilOpState m_frontFace;
        StencilOpState m_backFace;
    };

    struct DepthStencilState
    {
        AZ_TYPE_INFO(DepthStencilState, "{8AB45110-0727-4923-8098-B9926C1789FE}");
        static void Reflect(ReflectContext* context);

        bool operator == (const DepthStencilState& rhs) const;

        static DepthStencilState CreateDepth();
        static DepthStencilState CreateReverseDepth();
        static DepthStencilState CreateDisabled();

        DepthState m_depth;
        StencilState m_stencil;
    };

    enum class WriteChannelMask : uint8_t
    {
        ColorWriteMaskNone = 0,
        ColorWriteMaskRed = AZ_BIT(0),
        ColorWriteMaskGreen = AZ_BIT(1),
        ColorWriteMaskBlue = AZ_BIT(2),
        ColorWriteMaskAlpha = AZ_BIT(3),
        ColorWriteMaskAll = ColorWriteMaskRed | ColorWriteMaskGreen | ColorWriteMaskBlue | ColorWriteMaskAlpha
    };
    
    struct TargetBlendState
    {
        AZ_TYPE_INFO(TargetBlendState, "{2CDF00FE-614D-44FC-929F-E6B50C348578}");
        static void Reflect(ReflectContext* context);

        bool operator == (const TargetBlendState& rhs) const;

        uint32_t m_enable = 0;
        uint32_t m_writeMask = 0xF;
        BlendFactor m_blendSource = BlendFactor::One;
        BlendFactor m_blendDest = BlendFactor::Zero;
        BlendOp m_blendOp = BlendOp::Add;
        BlendFactor m_blendAlphaSource = BlendFactor::One;
        BlendFactor m_blendAlphaDest = BlendFactor::Zero;
        BlendOp m_blendAlphaOp = BlendOp::Add;
    };

    struct BlendState
    {
        AZ_TYPE_INFO(BlendState, "{EDB2333A-EF10-4A98-A157-B204E90FA179}");
        static void Reflect(ReflectContext* context);

        bool operator == (const BlendState& rhs) const;

        uint32_t m_alphaToCoverageEnable = 0;
        uint32_t m_independentBlendEnable = 0;
        AZStd::array<TargetBlendState, Limits::Pipeline::AttachmentColorCountMax> m_targets;
    };

    struct RenderStates
    {
        AZ_TYPE_INFO(RenderStates, "{521D72D5-DD69-4380-B637-9CC3D8479D2B}");
        static void Reflect(ReflectContext* context);

        HashValue64 GetHash(HashValue64 seed = HashValue64{ 0 }) const;

        bool operator == (const RenderStates& rhs) const;

        MultisampleState m_multisampleState;
        RasterState m_rasterState;
        BlendState m_blendState;
        DepthStencilState m_depthStencilState;
    };

    static constexpr uint32_t RenderStates_InvalidBool = std::numeric_limits<uint32_t>::max();
    static constexpr uint32_t RenderStates_InvalidUInt16 = std::numeric_limits<uint16_t>::max();
    static constexpr uint32_t RenderStates_InvalidUInt = std::numeric_limits<uint32_t>::max();
    static constexpr int32_t RenderStates_InvalidInt = std::numeric_limits<int32_t>::max();
    static constexpr float RenderStates_InvalidFloat = std::numeric_limits<float>::max();

    //! Merges any render states in stateToMerge into the result state object. 
    //! The values in stateToMerge are only copied over into the result if they are
    //! not invalid (see also GetInvalidState below).
    void MergeStateInto(const DepthState& stateToMerge, DepthState& result);
    void MergeStateInto(const RasterState& stateToMerge, RasterState& result);
    void MergeStateInto(const StencilOpState& stateToMerge, StencilOpState& result);
    void MergeStateInto(const StencilState& stateToMerge, StencilState& result);
    void MergeStateInto(const DepthStencilState& stateToMerge, DepthStencilState& result);
    void MergeStateInto(const TargetBlendState& stateToMerge, TargetBlendState& result);
    void MergeStateInto(const BlendState& stateToMerge, BlendState& result);
    void MergeStateInto(const MultisampleState& stateToMerge, MultisampleState& result);
    void MergeStateInto(const RenderStates& statesToMerge, RenderStates& result);

    //! Mark each property in the render states to an invalid value.
    //! This set of functions can be typically used by MergeStateInto
    //! to filter out the actual properties that need to be overwritten.
    const RasterState& GetInvalidRasterState();
    const DepthState& GetInvalidDepthState();
    const StencilOpState& GetInvalidStencilOpState();
    const StencilState& GetInvalidStencilState();
    const DepthStencilState& GetInvalidDepthStencilState();
    const TargetBlendState& GetInvalidTargetBlendState();
    const BlendState& GetInvalidBlendState();
    const MultisampleState& GetInvalidMultisampleState();
    const RenderStates& GetInvalidRenderStates();

    AZ_TYPE_INFO_SPECIALIZE(CullMode, "{AABEEE39-9185-4A9C-9BD7-229DAAAE885D}");
    AZ_TYPE_INFO_SPECIALIZE(FillMode, "{A164B54D-0A74-4F7C-89F3-032D6B6BF107}");
    AZ_TYPE_INFO_SPECIALIZE(DepthWriteMask, "{11B00B11-AC7E-4F8C-B2D9-5A09BB4D92B5}");
    AZ_TYPE_INFO_SPECIALIZE(StencilOp, "{FADAFC88-8638-4104-A73D-CA5CF4C16F74}");
    AZ_TYPE_INFO_SPECIALIZE(BlendFactor, "{BD14C7A1-3DC9-4670-8A13-2017B8CEECB6}");
    AZ_TYPE_INFO_SPECIALIZE(BlendOp, "{23DD9B83-875F-43D1-B1BB-5655C6A59739}");
}
