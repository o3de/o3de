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

#include <Atom/Feature/PostProcess/DepthOfField/DepthOfFieldSettingsInterface.h>
#include <Atom/RPI.Public/Image/StreamingImage.h>
#include <PostProcess/PostProcessBase.h>

#include <Atom/RPI.Public/View.h>

namespace AZ
{
    namespace Render
    {
        class PostProcessSettings;

        struct DepthOfFieldViewSRG
        {
            // x : viewFar, y : viewNear, z : focusDistance
            AZStd::array<float, 3> m_cameraParameters = { { 0.0f, 0.0f, 0.0f } };

            // scale / offset to convert DofFactor to blend ratio for back buffer.
            AZStd::array<float, 2> m_backBlendFactorDivision2 = { { 0.0f, 0.0f } };
            AZStd::array<float, 2> m_backBlendFactorDivision4 = { { 0.0f, 0.0f } };
            AZStd::array<float, 2> m_backBlendFactorDivision8 = { { 0.0f, 0.0f } };
            // scale / offset to convert DofFactor to blend ratio for front buffer.
            AZStd::array<float, 2> m_frontBlendFactorDivision2 = { { 0.0f, 0.0f } };
            AZStd::array<float, 2> m_frontBlendFactorDivision4 = { { 0.0f, 0.0f } };
            AZStd::array<float, 2> m_frontBlendFactorDivision8 = { { 0.0f, 0.0f } };

            // Used to determine the pencilMap texture coordinates from depth.
            float m_pencilMapTexcoordToCocRadius = 0.0f;
            float m_pencilMapFocusPointTexcoordU = 0.0f;

            // circle of confusion to screen ratio.
            float m_cocToScreenRatio = 0.0f;
        };

        // The post process sub-settings class for the Depth of Field feature
        class DepthOfFieldSettings final
            : public DepthOfFieldSettingsInterface
            , public PostProcessBase
        {
            friend class PostProcessSettings;
            friend class PostProcessFeatureProcessor;
            friend class DepthOfFieldBokehBlurPass;
            friend class DepthOfFieldMaskPass;
            friend class DepthOfFieldReadBackFocusDepthPass;
            friend class DepthOfFieldCompositePass;

        public:
            AZ_RTTI(AZ::Render::DepthOfFieldSettings, "{64782D63-80E0-4935-9E26-47EFC735305D}",
                AZ::Render::DepthOfFieldSettingsInterface, AZ::Render::PostProcessBase);
            AZ_CLASS_ALLOCATOR(DepthOfFieldSettings, SystemAllocator);

            DepthOfFieldSettings(PostProcessFeatureProcessor* featureProcessor);
            ~DepthOfFieldSettings() = default;

            // DepthOfFieldSettingsInterface overrides...
            void OnConfigChanged() override;

            // Applies settings from this onto target using override settings and passed alpha value for blending
            void ApplySettingsTo(DepthOfFieldSettings* target, float alpha) const;

            AZ::RHI::Handle<uint32_t> GetSplitSizeForPass(const Name& passName) const;

            // Set dof related settings to the viewSrg
            void SetValuesToViewSrg(AZ::Data::Instance<RPI::ShaderResourceGroup> viewSrg);

            // Generate all getters and override setters.
            // Declare non-override setters, which will be defined in the .cpp
#define AZ_GFX_COMMON_PARAM(ValueType, Name, MemberName, DefaultValue)                                  \
        ValueType Get##Name() const override { return MemberName; }                                     \
        void Set##Name(ValueType val) override;                                                         \

#define AZ_GFX_COMMON_OVERRIDE(ValueType, Name, MemberName, OverrideValueType)                          \
        OverrideValueType Get##Name##Override() const override { return MemberName##Override; }         \
        void Set##Name##Override(OverrideValueType val) override { MemberName##Override = val; }        \

#include <Atom/Feature/ParamMacros/MapAllCommon.inl>
#include <Atom/Feature/PostProcess/DepthOfField/DepthOfFieldParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>

        private:

            // Generate members...
#include <Atom/Feature/ParamMacros/StartParamMembers.inl>
#include <Atom/Feature/PostProcess/DepthOfField/DepthOfFieldParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>

            void Simulate(float deltaTime);

            void UpdateFNumber();

            PostProcessSettings* m_parentSettings = nullptr;

            // From DepthOfFieldFeatureProcessor...

            void LoadPencilMap();
            void UpdatePencilMapTexture() const;

            AZ::Data::Instance<AZ::RPI::StreamingImage> m_pencilMap;
            RHI::ShaderInputImageIndex m_pencilMapIndex;

            // From DepthOfFieldRenderProxy...

            void UpdateCameraParameters();
            void UpdateBlendFactor();
            void UpdateAutoFocusDepth(bool enabled);

            AZ::RHI::ShaderInputNameIndex m_cameraParametersIndex = "m_dof.m_cameraParameters";
            AZ::RHI::ShaderInputNameIndex m_pencilMapTexcoordToCocRadiusIndex = "m_dof.m_pencilMapTexcoordToCocRadius";
            AZ::RHI::ShaderInputNameIndex m_pencilMapFocusPointTexcoordUIndex = "m_dof.m_pencilMapFocusPointTexcoordU";
            AZ::RHI::ShaderInputNameIndex m_cocToScreenRatioIndex = "m_dof.m_cocToScreenRatio";

            AZ::RHI::NameIdReflectionMap<AZ::RHI::Handle<uint32_t>> m_passListWithHashOfDivisionNumber;

            DepthOfFieldViewSRG m_configurationToViewSRG;
            float m_viewAspectRatio = 0.0f;
            float m_maxBokehRadiusDivision2 = 0.0f;
            float m_minBokehRadiusDivision2 = 0.0f;
            float m_maxBokehRadiusDivision4 = 0.0f;
            float m_minBokehRadiusDivision4 = 0.0f;
            float m_maxBokehRadiusDivision8 = 0.0f;
            float m_minBokehRadiusDivision8 = 0.0f;

            // Radial division count of bokeh blur kernel.
            uint32_t m_sampleRadialDivision2 = 0;
            uint32_t m_sampleRadialDivision4 = 0;
            uint32_t m_sampleRadialDivision8 = 0;

            // camera parameters
            float m_viewFovRadian = 0.0f;
            float m_viewWidth = 0.0f;
            float m_viewHeight = 0.0f;
            float m_viewNear = 0.0f;
            float m_viewFar = 0.0f;

            float m_normalizedFocusDistanceForAutoFocus = 0.0f;
            float m_deltaTime = 0.0f;
        };

    }
}
