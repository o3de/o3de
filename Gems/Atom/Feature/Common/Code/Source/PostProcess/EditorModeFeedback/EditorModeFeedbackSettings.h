/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
 
#pragma once

#include <AzCore/RTTI/ReflectContext.h>

#include <Atom/Feature/PostProcess/EditorModeFeedback/EditorModeFeedbackSettingsInterface.h>

#include <PostProcess/PostProcessBase.h>

namespace AZ
{
    namespace Render
    {
        class PostProcessSettings;
        
        class EditorModeFeedbackSettings final
            : public EditorModeFeedbackSettingsInterface
            , public PostProcessBase
        {
            friend class PostProcessSettings;
            friend class PostProcessFeatureProcessor;

        public:
            AZ_RTTI(EditorModeFeedbackSettings, "{CBD47C20-8F51-4475-ACBE-A2356BCD3867}", EditorModeFeedbackSettingsInterface, PostProcessBase);
            AZ_CLASS_ALLOCATOR(EditorModeFeedbackSettings, SystemAllocator, 0);

            EditorModeFeedbackSettings(PostProcessFeatureProcessor* featureProcessor);
            ~EditorModeFeedbackSettings() = default;

            void OnConfigChanged() override;

            void ApplySettingsTo(EditorModeFeedbackSettings* target, float alpha) const;

            // Generate all getters and override setters.
            // Declare non-override setters, which will be defined in the .cpp
#define AZ_GFX_COMMON_PARAM(ValueType, Name, MemberName, DefaultValue)                                                                     \
    ValueType Get##Name() const override                                                                                                   \
    {                                                                                                                                      \
        return MemberName;                                                                                                                 \
    }                                                                                                                                      \
    void Set##Name(ValueType val) override                                                                                                 \
    {                                                                                                                                      \
        MemberName = val;                                                                                                                  \
    }

#define AZ_GFX_COMMON_OVERRIDE(ValueType, Name, MemberName, OverrideValueType)                                                             \
    OverrideValueType Get##Name##Override() const override                                                                                 \
    {                                                                                                                                      \
        return MemberName##Override;                                                                                                       \
    }                                                                                                                                      \
    void Set##Name##Override(OverrideValueType val) override                                                                               \
    {                                                                                                                                      \
        MemberName##Override = val;                                                                                                        \
    }        \


#include <Atom/Feature/ParamMacros/MapAllCommon.inl>
#include <Atom/Feature/PostProcess/EditorModeFeedback/EditorModeFeedbackParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>

        private:
            // Generate members...
#include <Atom/Feature/ParamMacros/StartParamMembers.inl>
#include <Atom/Feature/PostProcess/EditorModeFeedback/EditorModeFeedbackParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>

            void Simulate(float deltaTime);

            PostProcessSettings* m_parentSettings = nullptr;
        };
    }
}
