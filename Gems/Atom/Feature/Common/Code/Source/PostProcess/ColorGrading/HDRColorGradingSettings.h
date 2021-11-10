/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
 
#pragma once

#include <AzCore/RTTI/ReflectContext.h>

#include <Atom/Feature/PostProcess/ColorGrading/HDRColorGradingSettingsInterface.h>

#include <PostProcess/PostProcessBase.h>
#include <ACES/Aces.h>

namespace AZ
{
    namespace Render
    {
        class PostProcessSettings;
        
        class HDRColorGradingSettings final
            : public HDRColorGradingSettingsInterface
            , public PostProcessBase
        {
            friend class PostProcessSettings;
            friend class PostProcessFeatureProcessor;

        public:
            AZ_RTTI(HDRColorGradingSettings, "{EA8C05D4-66D0-4141-8D4D-68E5D764C2ED}", HDRColorGradingSettingsInterface, PostProcessBase);
            AZ_CLASS_ALLOCATOR(HDRColorGradingSettings, SystemAllocator, 0);

            HDRColorGradingSettings(PostProcessFeatureProcessor* featureProcessor);
            ~HDRColorGradingSettings() = default;

            void OnConfigChanged() override;

            void ApplySettingsTo(HDRColorGradingSettings* target, float alpha) const;

            // Generate all getters and override setters.
            // Declare non-override setters, which will be defined in the .cpp
#define AZ_GFX_COMMON_PARAM(ValueType, Name, MemberName, DefaultValue)                                  \
        ValueType Get##Name() const override { return MemberName; }                                     \
        void Set##Name(ValueType val) override                                                          \
        {                                                                                               \
            MemberName = val;                                                                           \
        }                                                                                               \

#define AZ_GFX_COMMON_OVERRIDE(ValueType, Name, MemberName, OverrideValueType)                          \
        OverrideValueType Get##Name##Override() const override { return MemberName##Override; }         \
        void Set##Name##Override(OverrideValueType val) override { MemberName##Override = val; }        \

#include <Atom/Feature/ParamMacros/MapAllCommon.inl>
#include <Atom/Feature/PostProcess/ColorGrading/HDRColorGradingParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>

        private:
            // Generate members...
#include <Atom/Feature/ParamMacros/StartParamMembers.inl>
#include <Atom/Feature/PostProcess/ColorGrading/HDRColorGradingParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>

            void Simulate(float deltaTime);

            PostProcessSettings* m_parentSettings = nullptr;
        };
    }
}
