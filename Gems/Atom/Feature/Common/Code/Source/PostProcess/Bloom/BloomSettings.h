/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once

#include <AzCore/RTTI/ReflectContext.h>

#include <Atom/RHI.Reflect/ShaderResourceGroupLayoutDescriptor.h>

#include <Atom/Feature/PostProcess/Bloom/BloomSettingsInterface.h>
#include <Atom/RPI.Public/Image/StreamingImage.h>
#include <PostProcess/PostProcessBase.h>

#include <Atom/RPI.Public/View.h>

namespace AZ
{
    namespace Render
    {
        class PostProcessSettings;

        // The post process sub-settings class for SSAO (Screen-Space Ambient Occlusion)
        class BloomSettings final
            : public BloomSettingsInterface
            , public PostProcessBase
        {
            friend class PostProcessSettings;
            friend class PostProcessFeatureProcessor;

        public:
            AZ_RTTI(AZ::Render::BloomSettings, "{9CDC625A-0545-494E-AB37-552A19741F6A}",
                AZ::Render::BloomSettingsInterface, AZ::Render::PostProcessBase);
            AZ_CLASS_ALLOCATOR(BloomSettings, SystemAllocator, 0);

            BloomSettings(PostProcessFeatureProcessor* featureProcessor);
            ~BloomSettings() = default;

            // BloomSettingsInterface overrides...
            void OnConfigChanged() override;

            // Applies settings from this onto target using override settings and passed alpha value for blending
            void ApplySettingsTo(BloomSettings* target, float alpha) const;

            // Generate getters and setters.
#include <Atom/Feature/ParamMacros/StartParamFunctions.inl>
#include <Atom/Feature/PostProcess/Bloom/BloomParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>

        private:

            // Generate members...
#include <Atom/Feature/ParamMacros/StartParamMembers.inl>
#include <Atom/Feature/PostProcess/Bloom/BloomParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>

            void Simulate(float deltaTime);

            PostProcessSettings* m_parentSettings = nullptr;

            float m_deltaTime = 0.0f;
        };

    }
}
