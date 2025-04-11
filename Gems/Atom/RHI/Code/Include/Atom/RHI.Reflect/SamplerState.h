/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/Base.h>
#include <Atom/RHI.Reflect/Limits.h>
#include <Atom/RHI.Reflect/ShaderStages.h>
#include <AzCore/Preprocessor/Enum.h>
#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/Utils/TypeHash.h>

namespace AZ
{
    class ReflectContext;
}

namespace AZ::RHI
{            
    AZ_ENUM_CLASS_WITH_UNDERLYING_TYPE(FilterMode, uint32_t,
        Point,
        Linear
    );

    AZ_ENUM_CLASS_WITH_UNDERLYING_TYPE(ReductionType, uint32_t,
        Filter,      /// Performs filtering on samples.
        Comparison,  /// Performs comparison of samples using the supplied comparison function.
        Minimum,     /// Returns minimum of samples.
        Maximum      /// Returns maximum of samples.
    );

    AZ_ENUM_CLASS_WITH_UNDERLYING_TYPE(AddressMode, uint32_t,
        Wrap,
        Mirror,
        Clamp,
        Border,
        MirrorOnce
    );

    AZ_ENUM_CLASS_WITH_UNDERLYING_TYPE(ComparisonFunc, uint32_t,
        Never,
        Less,
        Equal,
        LessEqual,
        Greater,
        NotEqual,
        GreaterEqual,
        Always,
        Invalid
    );

    AZ_ENUM_CLASS_WITH_UNDERLYING_TYPE(BorderColor, uint32_t,
        OpaqueBlack,
        TransparentBlack,
        OpaqueWhite
    );

    void ReflectSamplerStateEnums(ReflectContext* context);

    class SamplerState
    {
    public:
        AZ_TYPE_INFO(SamplerState, "{03CF3A01-8C2B-4A65-8781-6C25CFF0475F}");
        static void Reflect(AZ::ReflectContext* context);

        SamplerState() = default;

        static SamplerState Create(
            FilterMode filterModeMinMag,
            FilterMode filterModeMip,
            AddressMode addressMode,
            BorderColor borderColor = BorderColor::TransparentBlack);

        static SamplerState CreateAnisotropic(
            uint32_t anisotropyMax,
            AddressMode addressMode);

        HashValue64 GetHash(HashValue64 seed = HashValue64{ 0 }) const;

        uint32_t m_anisotropyMax = 1;
        uint32_t m_anisotropyEnable = 0;
        FilterMode m_filterMin = FilterMode::Point;
        FilterMode m_filterMag = FilterMode::Point;
        FilterMode m_filterMip = FilterMode::Point;
        ReductionType m_reductionType = ReductionType::Filter;
        ComparisonFunc m_comparisonFunc = ComparisonFunc::Always;
        AddressMode m_addressU = AddressMode::Wrap;
        AddressMode m_addressV = AddressMode::Wrap;
        AddressMode m_addressW = AddressMode::Wrap;
        float m_mipLodMin = 0.0f;
        float m_mipLodMax = static_cast<float>(Limits::Image::MipCountMax);
        float m_mipLodBias = 0.0f;
        BorderColor m_borderColor = BorderColor::TransparentBlack;
    };

    AZ_TYPE_INFO_SPECIALIZE(FilterMode, "{CFAE2156-0293-4D71-87D5-68F5C9F98884}");
    AZ_TYPE_INFO_SPECIALIZE(ReductionType, "{4230D40D-9984-4254-B062-2DD1CE4E7042}");
    AZ_TYPE_INFO_SPECIALIZE(AddressMode, "{977F0D2E-4623-4B9F-B35C-328EEA309F73}");
    AZ_TYPE_INFO_SPECIALIZE(ComparisonFunc, "{BF11B672-B9C4-4CFF-8228-EA09C4A36C36}");
    AZ_TYPE_INFO_SPECIALIZE(BorderColor, "{8A6739E8-538D-47FC-9068-45BCA5B7E5C4}");
}

