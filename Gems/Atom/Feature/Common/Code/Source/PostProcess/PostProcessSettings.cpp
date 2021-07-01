/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <PostProcess/PostProcessFeatureProcessor.h>
#include <PostProcess/PostProcessSettings.h>

namespace AZ
{
    namespace Render
    {
        PostProcessSettings::PostProcessSettings(PostProcessFeatureProcessor* featureProcessor)
            : PostProcessBase(featureProcessor)
        { }

        void PostProcessSettings::OnConfigChanged()
        {
            if (m_featureProcessor)
            {
                m_featureProcessor->OnPostProcessSettingsChanged();
            }
        }

        void PostProcessSettings::Simulate(float deltaTime)
        {
            // Auto-gen calls to simulate() function on all sub-setting members
#define POST_PROCESS_MEMBER(ClassName, MemberName)                                                      \
            if (MemberName)                                                                             \
            {                                                                                           \
                MemberName->Simulate(deltaTime);                                                        \
            }                                                                                           \

#include <Atom/Feature/PostProcess/PostProcessSettings.inl>
#undef POST_PROCESS_MEMBER
        }

        void PostProcessSettings::ApplySettingsTo(PostProcessSettings* target, float blendFactor) const
        {
            if (target == nullptr)
            {
                return;
            }

            // Modulate blendFactor by settings' overrideFactor
            blendFactor *= m_overrideFactor;

            // Auto-gen application of sub-settings for all members
#define POST_PROCESS_MEMBER(ClassName, MemberName)                                                      \
            if (MemberName)                                                                             \
            {                                                                                           \
                if (!target->MemberName)                                                                \
                {                                                                                       \
                    target->MemberName = AZStd::make_unique<ClassName>(*MemberName);                    \
                }                                                                                       \
                else                                                                                    \
                {                                                                                       \
                    MemberName->ApplySettingsTo(target->MemberName.get(), blendFactor);                 \
                }                                                                                       \
            }                                                                                           \

#include <Atom/Feature/PostProcess/PostProcessSettings.inl>
#undef POST_PROCESS_MEMBER
        }

        void PostProcessSettings::CopyViewToBlendWeightSettings(const ViewBlendWeightMap& perViewBlendWeights)
        {
            m_perViewBlendWeights = perViewBlendWeights;
        }

        float PostProcessSettings::GetBlendWeightForView(AZ::RPI::View* view) const
        {
            const auto& settingsIterator = m_perViewBlendWeights.find(view);
            return settingsIterator != m_perViewBlendWeights.end() ? settingsIterator->second : defaultBlendWeight;
        }

        // Auto-gen code for getting, creating and removing sub-setting members
#define POST_PROCESS_MEMBER(ClassName, MemberName)                                                      \
                                                                                                        \
        ClassName##Interface* PostProcessSettings::GetOrCreate##ClassName##Interface()                  \
        {                                                                                               \
            if (!MemberName)                                                                            \
            {                                                                                           \
                MemberName = AZStd::make_unique<ClassName>(m_featureProcessor);                         \
                MemberName->m_parentSettings = this;                                                    \
                OnConfigChanged();                                                                      \
            }                                                                                           \
            return MemberName.get();                                                                    \
        }                                                                                               \
                                                                                                        \
        void PostProcessSettings::Remove##ClassName##Interface()                                        \
        {                                                                                               \
            MemberName = nullptr;                                                                       \
            OnConfigChanged();                                                                          \
        }                                                                                               \

#include <Atom/Feature/PostProcess/PostProcessSettings.inl>
#undef POST_PROCESS_MEMBER

    } // namespace Render
} // namespace AZ
