/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/Feature/PostProcess/PostProcessSettingsInterface.h>
#include <Atom/Feature/PostProcess/PostFxLayerCategoriesConstants.h>

#include <PostProcess/PostProcessBase.h>
#include <PostProcess/Bloom/BloomSettings.h>
#include <PostProcess/DepthOfField/DepthOfFieldSettings.h>
#include <PostProcess/ExposureControl/ExposureControlSettings.h>
#include <PostProcess/Ssao/SsaoSettings.h>
#include <PostProcess/LookModification/LookModificationSettings.h>
#include <PostProcess/ColorGrading/HDRColorGradingSettings.h>
#include <ScreenSpace/DeferredFogSettings.h>

namespace AZ
{
    namespace Render
    {
        //! A collection of post process settings
        //! This class is responsible for managing and blending sub-settings classes
        //! for post effects like depth of field, anti-aliasing, SSAO, etc.
        class PostProcessSettings final
            : public PostProcessSettingsInterface
            , public PostProcessBase
        {
            friend class PostProcessFeatureProcessor;

        public:
            AZ_RTTI(AZ::Render::PostProcessSettings, "{B4DE4B9F-83D2-4FD8-AD58-C0D1D4AEA23F}",
                AZ::Render::PostProcessSettingsInterface, AZ::Render::PostProcessBase);
            AZ_CLASS_ALLOCATOR(PostProcessSettings, SystemAllocator, 0);

            PostProcessSettings(PostProcessFeatureProcessor* featureProcessor);

            // PostProcessSettingsInterface overrides...
            void OnConfigChanged() override;

            // Auto-gen function declarations related to sub-settings members
#define POST_PROCESS_MEMBER(ClassName, MemberName)                                                      \
            ClassName* Get##ClassName() { return MemberName.get(); }                                    \
            ClassName##Interface* GetOrCreate##ClassName##Interface() override;                         \
            void Remove##ClassName##Interface() override;                                               \

#include <Atom/Feature/PostProcess/PostProcessSettings.inl>
#undef POST_PROCESS_MEMBER

            // Auto-gen getter and setter functions for post process members...
#include <Atom/Feature/ParamMacros/StartParamFunctionsOverrideImpl.inl>
#include <Atom/Feature/PostProcess/PostProcessParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>

            // Getter & setter for layer category value
            int GetLayerCategoryValue() const { return m_layerCategoryValue; }
            void SetLayerCategoryValue(int layerCategoryValue) override { m_layerCategoryValue = layerCategoryValue; }

            // camera-to-blendWeight setters and getters
            void CopyViewToBlendWeightSettings(const ViewBlendWeightMap& perViewBlendWeights) override;
            float GetBlendWeightForView(AZ::RPI::View* view) const;

        private:

            // Applies owned sub-settings onto target's settings using providing blendFactor,
            // this class's overrideFactor and per-param override settings for individual sub-settings
            void ApplySettingsTo(PostProcessSettings* targetSettings, float blendFactor = 1.0f) const;

            // Called from the PostProcessFeatureProcessor on aggregated PostProcessSettings
            // (More detail: The PostProcessFeatureProcessor will blend together PostProcessSettings based on application
            // frequency (Level, Volume, Camera) and per-setting override values. Simulate will be called on those combined settings)
            void Simulate(float deltaTime);

            // Auto-gen post process members...
#include <Atom/Feature/ParamMacros/StartParamMembers.inl>
#include <Atom/Feature/PostProcess/PostProcessParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>

            // Auto-gen sub-settings members
#define POST_PROCESS_MEMBER(ClassName, MemberName)                                                      \
            AZStd::unique_ptr<ClassName> MemberName;                                                    \

#include <Atom/Feature/PostProcess/PostProcessSettings.inl>
#undef POST_PROCESS_MEMBER

            // This camera-to-blendWeights setting will be used by feature processor to
            // blend postfxs per camera
            ViewBlendWeightMap m_perViewBlendWeights;

            // Integer representation of PostFx layer
            int m_layerCategoryValue = PostFx::DefaultLayerCategoryValue;
        };
    }
}
