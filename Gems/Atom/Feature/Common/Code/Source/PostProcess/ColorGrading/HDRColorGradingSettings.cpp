/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/Serialization/SerializeContext.h>

#include <PostProcess/ColorGrading/HDRColorGradingSettings.h>
#include <PostProcess/PostProcessFeatureProcessor.h>

namespace AZ
{
    namespace Render
    {
        HDRColorGradingSettings::HDRColorGradingSettings(PostProcessFeatureProcessor* featureProcessor)
            : PostProcessBase(featureProcessor)
        {
        }

        void HDRColorGradingSettings::OnConfigChanged()
        {
            m_parentSettings->OnConfigChanged();
        }

        void HDRColorGradingSettings::ApplySettingsTo(HDRColorGradingSettings* target, [[maybe_unused]] float alpha) const
        {
            AZ_Assert(target != nullptr, "HDRColorGradingSettings::ApplySettingsTo called with nullptr as argument.");

            if (GetEnabled())
            {
                target->m_enabled = m_enabled;

#define AZ_GFX_BOOL_PARAM(NAME, MEMBER_NAME, DefaultValue) ;
#define AZ_GFX_COMMON_PARAM(ValueType, Name, MemberName, DefaultValue) ;
#define AZ_GFX_FLOAT_PARAM(NAME, MEMBER_NAME, DefaultValue)                                                  \
    {                                                                                                        \
                target->Set##NAME(AZ::Lerp(target->MEMBER_NAME, MEMBER_NAME, alpha));                        \
    }

#define AZ_GFX_VEC3_PARAM(NAME, MEMBER_NAME, DefaultValue)                                                   \
    {                                                                                                        \
                target->MEMBER_NAME.Set(AZ::Lerp(target->MEMBER_NAME.GetX(), MEMBER_NAME.GetX(), alpha),     \
                    AZ::Lerp(target->MEMBER_NAME.GetY(), MEMBER_NAME.GetY(), alpha),                         \
                    AZ::Lerp(target->MEMBER_NAME.GetZ(), MEMBER_NAME.GetZ(), alpha));                        \
    }

#include <Atom/Feature/PostProcess/ColorGrading/HDRColorGradingParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>
            }
        }

        void HDRColorGradingSettings::Simulate([[maybe_unused]] float deltaTime)
        {
        }
    } // namespace Render
} // namespace AZ


