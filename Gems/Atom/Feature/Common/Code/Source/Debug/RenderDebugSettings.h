/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/ReflectContext.h>

#include <Atom/RHI.Reflect/ShaderResourceGroupLayoutDescriptor.h>

#include <Atom/Feature/Debug/RenderDebugSettingsInterface.h>
#include <Atom/RPI.Public/Image/StreamingImage.h>
#include <PostProcess/PostProcessBase.h>

#include <Atom/RPI.Public/View.h>

namespace AZ {
    namespace Render {

        class PostProcessSettings;

        // The post process sub-settings class for SSAO (Screen-Space Ambient Occlusion)
        class RenderDebugSettings final
            : public RenderDebugSettingsInterface
            , public PostProcessBase
        {
            friend class PostProcessSettings;
            friend class PostProcessFeatureProcessor;

        public:
            AZ_RTTI(AZ::Render::RenderDebugSettings, "{942CB951-C5D0-4E90-9F55-633DAA561A03}",
                AZ::Render::RenderDebugSettingsInterface, AZ::Render::PostProcessBase);
            AZ_CLASS_ALLOCATOR(RenderDebugSettings, SystemAllocator, 0);

            RenderDebugSettings(PostProcessFeatureProcessor* featureProcessor);
            ~RenderDebugSettings() = default;

            // RenderDebugSettingsInterface overrides...
            void OnConfigChanged() override;

            // Applies settings from this onto target using override settings and passed alpha value for blending
            void ApplySettingsTo(RenderDebugSettings* target, float alpha) const;

            // Generate getters and setters.
#include <Atom/Feature/ParamMacros/StartParamFunctionsOverrideImpl.inl>
#include <Atom/Feature/Debug/RenderDebugParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>

        private:

            // Generate members...
#include <Atom/Feature/ParamMacros/StartParamMembers.inl>
#include <Atom/Feature/Debug/RenderDebugParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>

            void Simulate(float deltaTime);

            PostProcessSettings* m_parentSettings = nullptr;

            float m_deltaTime = 0.0f;
        };

    }
}
