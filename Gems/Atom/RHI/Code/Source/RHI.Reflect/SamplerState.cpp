/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI.Reflect/SamplerState.h>
#include <AzCore/Preprocessor/Enum.h>
#include <AzCore/Preprocessor/EnumReflectUtils.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Utils/TypeHash.h>

namespace AZ
{
    namespace RHI
    {
        AZ_ENUM_DEFINE_REFLECT_UTILITIES(FilterMode);
        AZ_ENUM_DEFINE_REFLECT_UTILITIES(ReductionType);
        AZ_ENUM_DEFINE_REFLECT_UTILITIES(AddressMode);
        AZ_ENUM_DEFINE_REFLECT_UTILITIES(ComparisonFunc);
        AZ_ENUM_DEFINE_REFLECT_UTILITIES(BorderColor);

        void ReflectSamplerStateEnums(ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                FilterModeReflect(*serializeContext);
                ReductionTypeReflect(*serializeContext);
                AddressModeReflect(*serializeContext);
                ComparisonFuncReflect(*serializeContext);
                BorderColorReflect(*serializeContext);
            }

            if (auto behaviorContext = azrtti_cast<BehaviorContext*>(context))
            {
                FilterModeReflect(*behaviorContext);
                ReductionTypeReflect(*behaviorContext);
                AddressModeReflect(*behaviorContext);
                ComparisonFuncReflect(*behaviorContext);
                BorderColorReflect(*behaviorContext);
            }
        }

        void SamplerState::Reflect(AZ::ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<SerializeContext*>(context))
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
                    ->Field("m_borderColor", &SamplerState::m_borderColor)
                    ;

                if (auto editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<SamplerState>("SamplerState", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, false)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &SamplerState::m_anisotropyMax, "Anisotropy Max", "")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &SamplerState::m_anisotropyEnable, "Anisotropy Enable", "")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &SamplerState::m_filterMin, "Filter Min", "")
                            ->Attribute(AZ::Edit::Attributes::EnumValues, AZ::Edit::GetEnumConstantsFromTraits<FilterMode>())
                        ->DataElement(AZ::Edit::UIHandlers::Default, &SamplerState::m_filterMag, "Filter Mag", "")
                            ->Attribute(AZ::Edit::Attributes::EnumValues, AZ::Edit::GetEnumConstantsFromTraits<FilterMode>())
                        ->DataElement(AZ::Edit::UIHandlers::Default, &SamplerState::m_filterMip, "Filter Mip", "")
                            ->Attribute(AZ::Edit::Attributes::EnumValues, AZ::Edit::GetEnumConstantsFromTraits<FilterMode>())
                        ->DataElement(AZ::Edit::UIHandlers::Default, &SamplerState::m_reductionType, "Reduction Type", "")
                            ->Attribute(AZ::Edit::Attributes::EnumValues, AZ::Edit::GetEnumConstantsFromTraits<ReductionType>())
                        ->DataElement(AZ::Edit::UIHandlers::Default, &SamplerState::m_comparisonFunc, "Comparison Func", "")
                            ->Attribute(AZ::Edit::Attributes::EnumValues, AZ::Edit::GetEnumConstantsFromTraits<ComparisonFunc>())
                        ->DataElement(AZ::Edit::UIHandlers::Default, &SamplerState::m_addressU, "Address U", "")
                            ->Attribute(AZ::Edit::Attributes::EnumValues, AZ::Edit::GetEnumConstantsFromTraits<AddressMode>())
                        ->DataElement(AZ::Edit::UIHandlers::Default, &SamplerState::m_addressV, "Address V", "")
                            ->Attribute(AZ::Edit::Attributes::EnumValues, AZ::Edit::GetEnumConstantsFromTraits<AddressMode>())
                        ->DataElement(AZ::Edit::UIHandlers::Default, &SamplerState::m_addressW, "Address W", "")
                            ->Attribute(AZ::Edit::Attributes::EnumValues, AZ::Edit::GetEnumConstantsFromTraits<AddressMode>())
                        ->DataElement(AZ::Edit::UIHandlers::Default, &SamplerState::m_mipLodMin, "Mip Lod Min", "")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &SamplerState::m_mipLodMax, "Mip Lod Max", "")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &SamplerState::m_mipLodBias, "Mip Lod Bias", "")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &SamplerState::m_borderColor, "Border Color", "")
                            ->Attribute(AZ::Edit::Attributes::EnumValues, AZ::Edit::GetEnumConstantsFromTraits<BorderColor>())
                        ;
                }
            }

            if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->Class<SamplerState>("SamplerState")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                    ->Attribute(AZ::Script::Attributes::Category, "RHI")
                    ->Attribute(AZ::Script::Attributes::Module, "rhi")
                    ->Constructor()
                    ->Constructor<const SamplerState&>()
                    ->Property("anisotropyMax", BehaviorValueProperty(&SamplerState::m_anisotropyMax))
                    ->Property("anisotropyEnable", BehaviorValueProperty(&SamplerState::m_anisotropyEnable))
                    ->Property("filterMin", BehaviorValueProperty(&SamplerState::m_filterMin))
                    ->Property("filterMag", BehaviorValueProperty(&SamplerState::m_filterMag))
                    ->Property("filterMip", BehaviorValueProperty(&SamplerState::m_filterMip))
                    ->Property("reductionType", BehaviorValueProperty(&SamplerState::m_reductionType))
                    ->Property("comparisonFunc", BehaviorValueProperty(&SamplerState::m_comparisonFunc))
                    ->Property("addressU", BehaviorValueProperty(&SamplerState::m_addressU))
                    ->Property("addressV", BehaviorValueProperty(&SamplerState::m_addressV))
                    ->Property("addressW", BehaviorValueProperty(&SamplerState::m_addressW))
                    ->Property("mipLodMin", BehaviorValueProperty(&SamplerState::m_mipLodMin))
                    ->Property("mipLodMax", BehaviorValueProperty(&SamplerState::m_mipLodMax))
                    ->Property("mipLodBias", BehaviorValueProperty(&SamplerState::m_mipLodBias))
                    ->Property("borderColor", BehaviorValueProperty(&SamplerState::m_borderColor))
                    ;
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
