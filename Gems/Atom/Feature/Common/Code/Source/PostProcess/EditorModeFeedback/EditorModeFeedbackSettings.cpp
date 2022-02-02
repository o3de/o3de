/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/Serialization/SerializeContext.h>

#include <PostProcess/EditorModeFeedback/EditorModeFeedbackSettings.h>
#include <PostProcess/PostProcessFeatureProcessor.h>

namespace AZ
{
    namespace Render
    {
        EditorModeFeedbackSettings::EditorModeFeedbackSettings(PostProcessFeatureProcessor* featureProcessor)
            : PostProcessBase(featureProcessor)
        {
        }

        void EditorModeFeedbackSettings::OnConfigChanged()
        {
            m_parentSettings->OnConfigChanged();
        }

        void EditorModeFeedbackSettings::ApplySettingsTo(EditorModeFeedbackSettings* target, [[maybe_unused]] float alpha) const
        {
            AZ_Assert(target != nullptr, "EditorModeFeedbackSettings::ApplySettingsTo called with nullptr as argument.");

            // Auto-gen code to blend individual params based on their override value onto target settings
#define AZ_GFX_FLOAT_PARAM(NAME, MEMBER_NAME, DefaultValue)                                                                                \
    {                                                                                                                                      \
        target->Set##NAME(AZ::Lerp(target->MEMBER_NAME, MEMBER_NAME, alpha));                                                              \
    }

#include <Atom/Feature/PostProcess/EditorModeFeedback/EditorModeFeedbackParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>
        }

        void EditorModeFeedbackSettings::Simulate([[maybe_unused]] float deltaTime)
        {
        }
    } // namespace Render
} // namespace AZ


