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

#include <Atom/Feature/PostProcess/FilmGrain/FilmGrainSettingsInterface.h>
#include <Atom/RPI.Public/Image/StreamingImage.h>
#include <PostProcess/PostProcessBase.h>

#include <Atom/RPI.Public/View.h>

namespace AZ
{
    namespace Render
    {
        class PostProcessSettings;

        class FilmGrainSettings final
            : public FilmGrainSettingsInterface
            , public PostProcessBase
        {
            friend class PostProcessSettings;
            friend class PostProcessFeatureProcessor;

        public:
            AZ_RTTI(
                AZ::Render::FilmGrainSettings,
                "{E75BF1A1-2607-4743-A755-8CE2D59E291E}",
                AZ::Render::FilmGrainSettingsInterface,
                AZ::Render::PostProcessBase);
            AZ_CLASS_ALLOCATOR(FilmGrainSettings, SystemAllocator, 0);

            FilmGrainSettings(PostProcessFeatureProcessor* featureProcessor);
            ~FilmGrainSettings() = default;

            // BloomSettingsInterface overrides...
            void OnConfigChanged() override;

            // Applies settings from this onto target using override settings and passed alpha value for blending
            void ApplySettingsTo(FilmGrainSettings* target, float alpha) const;

            Data::Instance<RPI::StreamingImage> LoadStreamingImage(const char* textureFilePath, const char* sampleName);

            // Generate getters and setters.
#include <Atom/Feature/ParamMacros/StartParamFunctionsOverrideImpl.inl>
#include <Atom/Feature/PostProcess/FilmGrain/FilmGrainParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>

        private:
            // Generate members...
#include <Atom/Feature/ParamMacros/StartParamMembers.inl>
#include <Atom/Feature/PostProcess/FilmGrain/FilmGrainParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>


            void Simulate(float deltaTime);

            PostProcessSettings* m_parentSettings = nullptr;

            float m_deltaTime = 0.0f;
        };

    } // namespace Render
} // namespace AZ
