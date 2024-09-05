/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/RTTI.h>
#include <Atom/Feature/PostProcess/Bloom/BloomSettingsInterface.h>
#include <Atom/Feature/PostProcess/ChromaticAberration/ChromaticAberrationSettingsInterface.h>
#include <Atom/Feature/PostProcess/PaniniProjection/PaniniProjectionSettingsInterface.h>
#include <Atom/Feature/PostProcess/FilmGrain/FilmGrainSettingsInterface.h>
#include <Atom/Feature/PostProcess/WhiteBalance/WhiteBalanceSettingsInterface.h>
#include <Atom/Feature/PostProcess/Vignette/VignetteSettingsInterface.h>
#include <Atom/Feature/PostProcess/DepthOfField/DepthOfFieldSettingsInterface.h>
#include <Atom/Feature/PostProcess/ExposureControl/ExposureControlSettingsInterface.h>
#include <Atom/Feature/PostProcess/Ssao/SsaoSettingsInterface.h>
#include <Atom/Feature/PostProcess/LookModification/LookModificationSettingsInterface.h>
#include <Atom/Feature/PostProcess/ColorGrading/HDRColorGradingSettingsInterface.h>
#include <Atom/Feature/ScreenSpace/DeferredFogSettingsInterface.h>
#include <Atom/RPI.Public/View.h>

namespace AZ
{
    namespace Render
    {
        //! Abstract interface for PostProcessSettings so it can be access outside of Atom (for example in AtomLyIntegration)
        class PostProcessSettingsInterface
        {
        public:
            AZ_RTTI(AZ::Render::PostProcessSettingsInterface, "{22949C39-B56B-4920-94FA-6FDFA8273A2C}");

            using ViewBlendWeightMap = AZStd::unordered_map<AZ::RPI::View*, float>;

            static constexpr float defaultBlendWeight = 1.0f;

            //! To be called when config settings have been changed
            virtual void OnConfigChanged() = 0;

            //! Function to import blend weight settings
            virtual void CopyViewToBlendWeightSettings(const ViewBlendWeightMap& perCameraBlendWeights) = 0;

            //! Setter for PostFx setting's layer represented by an integer
            virtual void SetLayerCategoryValue(int layerCategoryValue) = 0;

            // Auto generate virtual GetOrCreate and Remove
#define POST_PROCESS_MEMBER(ClassName, MemberName)                                                      \
            virtual ClassName##Interface* GetOrCreate##ClassName##Interface() = 0;                      \
            virtual void Remove##ClassName##Interface() = 0;                                            \

#include <Atom/Feature/PostProcess/PostProcessSettings.inl>
#undef POST_PROCESS_MEMBER

            // Auto generate virtual getter and setter functions...
#include <Atom/Feature/ParamMacros/StartParamFunctionsVirtual.inl>
#include <Atom/Feature/PostProcess/PostProcessParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>
        };

    }
}
