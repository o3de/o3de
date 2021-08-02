/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI.Reflect/SamplerState.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Utils/TypeHash.h>

namespace AZ
{
    namespace RHI
    {
        const char* ToString(FilterMode filterMode)
        {
            switch (filterMode)
            {
            case FilterMode::Point:  return "Point";
            case FilterMode::Linear:  return "Linear";
            default:
                AZ_Assert(false, "Unhandled type");
                return "<Unknown>";
            }
        }

        const char* ToString(ReductionType reductionType)
        {
            switch (reductionType)
            {
            case ReductionType::Filter:  return "Filter";
            case ReductionType::Comparison:  return "Comparison";
            case ReductionType::Minimum:  return "Minimum";
            case ReductionType::Maximum:  return "Maximum";
            default:
                AZ_Assert(false, "Unhandled type");
                return "<Unknown>";
            }
        }

        const char* ToString(AddressMode addressMode)
        {
            switch (addressMode)
            {
            case AddressMode::Wrap:  return "Wrap";
            case AddressMode::Mirror:  return "Mirror";
            case AddressMode::Clamp:  return "Clamp";
            case AddressMode::Border:  return "Border";
            case AddressMode::MirrorOnce:  return "MirrorOnce";
            default:
                AZ_Assert(false, "Unhandled type");
                return "<Unknown>";
            }
        }

        const char* ToString(ComparisonFunc comparisonFunc)
        {
            switch (comparisonFunc)
            {
            case ComparisonFunc::Never:  return "Never";
            case ComparisonFunc::Less:  return "Less";
            case ComparisonFunc::Equal:  return "Equal";
            case ComparisonFunc::LessEqual:  return "LessEqual";
            case ComparisonFunc::Greater:  return "Greater";
            case ComparisonFunc::NotEqual:  return "NotEqual";
            case ComparisonFunc::GreaterEqual:  return "GreaterEqual";
            case ComparisonFunc::Always:  return "Always";
            default:
                AZ_Assert(false, "Unhandled type");
                return "<Unknown>";
            }
        }

        const char* ToString(BorderColor borderColor)
        {
            switch (borderColor)
            {
            case BorderColor::OpaqueBlack:  return "OpaqueBlack";
            case BorderColor::TransparentBlack:  return "TransparentBlack";
            case BorderColor::OpaqueWhite:  return "OpaqueWhite";
            default:
                AZ_Assert(false, "Unhandled type");
                return "<Unknown>";
            }
        }


        void ReflectSamplerStateEnums(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Enum<FilterMode>()
                    ->Value(ToString(FilterMode::Point), FilterMode::Point)
                    ->Value(ToString(FilterMode::Linear), FilterMode::Linear)
                    ;

                serializeContext->Enum<ReductionType>()
                    ->Value(ToString(ReductionType::Filter), ReductionType::Filter)
                    ->Value(ToString(ReductionType::Comparison), ReductionType::Comparison)
                    ->Value(ToString(ReductionType::Minimum), ReductionType::Minimum)
                    ->Value(ToString(ReductionType::Maximum), ReductionType::Maximum)
                    ;

                serializeContext->Enum<AddressMode>()
                    ->Value(ToString(AddressMode::Wrap), AddressMode::Wrap)
                    ->Value(ToString(AddressMode::Mirror), AddressMode::Mirror)
                    ->Value(ToString(AddressMode::Clamp), AddressMode::Clamp)
                    ->Value(ToString(AddressMode::Border), AddressMode::Border)
                    ->Value(ToString(AddressMode::MirrorOnce), AddressMode::MirrorOnce)
                    ;

                serializeContext->Enum<ComparisonFunc>()
                    ->Value(ToString(ComparisonFunc::Never), ComparisonFunc::Never)
                    ->Value(ToString(ComparisonFunc::Less), ComparisonFunc::Less)
                    ->Value(ToString(ComparisonFunc::Equal), ComparisonFunc::Equal)
                    ->Value(ToString(ComparisonFunc::LessEqual), ComparisonFunc::LessEqual)
                    ->Value(ToString(ComparisonFunc::Greater), ComparisonFunc::Greater)
                    ->Value(ToString(ComparisonFunc::NotEqual), ComparisonFunc::NotEqual)
                    ->Value(ToString(ComparisonFunc::GreaterEqual), ComparisonFunc::GreaterEqual)
                    ->Value(ToString(ComparisonFunc::Always), ComparisonFunc::Always)
                    ;

                serializeContext->Enum<BorderColor>()
                    ->Value(ToString(BorderColor::OpaqueBlack), BorderColor::OpaqueBlack)
                    ->Value(ToString(BorderColor::TransparentBlack), BorderColor::TransparentBlack)
                    ->Value(ToString(BorderColor::OpaqueWhite), BorderColor::OpaqueWhite)
                    ;

            }
        }

        void SamplerState::Reflect(AZ::ReflectContext* context)
        {
            if (SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<SamplerState>()
                    ->Version(3)
                    ->Field("m_anisotropyMax", &SamplerState::m_anisotropyMax)
                    ->Field("m_anisotropyEnable", &SamplerState::m_anisotropyEnable)
                    ->Field("m_filterMin", &SamplerState::m_filterMin)
                    ->Field("m_filterMag", &SamplerState::m_filterMag)
                    ->Field("m_filterMip", &SamplerState::m_filterMip)
                    ->Field("m_reductionType", &SamplerState::m_reductionType)
                    ->Field("m_comparisonFunc", &SamplerState::m_comparisonFunc)
                    ->Field("m_addressU", &SamplerState::m_addressU)
                    ->Field("m_addressV", &SamplerState::m_addressV)
                    ->Field("m_addressW", &SamplerState::m_addressW)
                    ->Field("m_mipLodMin", &SamplerState::m_mipLodMin)
                    ->Field("m_mipLodMax", &SamplerState::m_mipLodMax)
                    ->Field("m_mipLodBias", &SamplerState::m_mipLodBias)
                    ->Field("m_borderColor", &SamplerState::m_borderColor);
            }
        }

        SamplerState SamplerState::Create(
            FilterMode filterModeMinMag,
            FilterMode filterModeMip,
            AddressMode addressMode,
            BorderColor borderColor)
        {
            SamplerState descriptor;
            descriptor.m_filterMin = descriptor.m_filterMag = filterModeMinMag;
            descriptor.m_filterMip = filterModeMip;
            descriptor.m_addressU = descriptor.m_addressV = descriptor.m_addressW = addressMode;
            descriptor.m_borderColor = borderColor;
            return descriptor;
        }

        SamplerState SamplerState::CreateAnisotropic(
            uint32_t anisotropyMax,
            AddressMode addressMode)
        {
            SamplerState descriptor;
            descriptor.m_anisotropyEnable = 1;
            descriptor.m_anisotropyMax = anisotropyMax;
            descriptor.m_addressU = descriptor.m_addressV = descriptor.m_addressW = addressMode;
            return descriptor;
        }

        HashValue64 SamplerState::GetHash(HashValue64 seed /*= 0*/) const
        {
            return TypeHash64(*this, seed);
        }
    }
}
