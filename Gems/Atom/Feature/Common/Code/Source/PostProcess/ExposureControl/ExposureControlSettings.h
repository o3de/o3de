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

#include <Atom/RPI.Public/View.h>

#include <Atom/Feature/PostProcess/ExposureControl/ExposureControlSettingsInterface.h>

#include <PostProcess/PostProcessBase.h>

namespace AZ
{
    namespace Render
    {
        class PostProcessSettings;

        // Name of the buffer used for the exposure control feature
        static const char* const ExposureControlBufferName = "ExposureControlBuffer";

        // The post process sub-settings class for the exposure control feature
        class ExposureControlSettings final
            : public ExposureControlSettingsInterface
            , public PostProcessBase
        {
            friend class PostProcessSettings;
            friend class PostProcessFeatureProcessor;

        public:
            AZ_RTTI(ExposureControlSettings, "{51DAEA8B-0744-41C4-B494-387D78E7E7C0}", ExposureControlSettingsInterface, PostProcessBase);
            AZ_CLASS_ALLOCATOR(ExposureControlSettings, SystemAllocator);

            ExposureControlSettings(PostProcessFeatureProcessor* featureProcessor);
            ~ExposureControlSettings() = default;

            // ExposureControlSettingsInterface overrides...
            void OnConfigChanged() override;

            // Applies settings from this onto target using override settings and passed alpha value for blending
            void ApplySettingsTo(ExposureControlSettings* target, float alpha) const;

            void UpdateBuffer();
            const RHI::BufferView* GetBufferView() const { return m_buffer->GetBufferView(); }
            
            // Generate all getters and override setters.
            // Declare non-override setters, which will be defined in the .cpp
#define AZ_GFX_COMMON_PARAM(ValueType, Name, MemberName, DefaultValue)                                  \
        ValueType Get##Name() const override { return MemberName; }                                     \
        void Set##Name(ValueType val) override;                                                         \

#define AZ_GFX_COMMON_OVERRIDE(ValueType, Name, MemberName, OverrideValueType)                          \
        OverrideValueType Get##Name##Override() const override { return MemberName##Override; }         \
        void Set##Name##Override(OverrideValueType val) override { MemberName##Override = val; }        \

#include <Atom/Feature/ParamMacros/MapAllCommon.inl>
#include <Atom/Feature/PostProcess/ExposureControl/ExposureControlParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>

        private:

            // Generate members...
#include <Atom/Feature/ParamMacros/StartParamMembers.inl>
#include <Atom/Feature/PostProcess/ExposureControl/ExposureControlParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>

            void Simulate(float deltaTime);

            bool InitCommonBuffer();

            void UpdateShaderParameters();

            void UpdateExposureControlRelatedPassParameters();
            
            void UpdateLuminanceHeatmap();
            
            PostProcessSettings* m_parentSettings = nullptr;
            bool m_shouldUpdatePassParameters = true;

            struct ShaderParameters
            {
                float m_exposureMin;
                float m_exposureMax;
                float m_speedUp;
                float m_speedDown;
                float m_compensationValue = 0.0f;
                uint32_t m_eyeAdaptationEnabled = 0;
                AZStd::array<uint32_t, 2> m_padding = { 0, 0 };
            };

            // The eye adaptation shader paraemters. This structure is same as ViewSrg::ExposureControlParameters.
            ShaderParameters m_shaderParameters;
            bool m_shouldUpdateShaderParameters = true;

            // Cache of the default view of the render pipeline to check change of the default view.
            const RPI::View* m_lastDefaultView = nullptr;

            AZ::Data::Instance<RPI::Buffer> m_buffer;

            const AZ::Name m_eyeAdaptationPassTemplateNameId;
            const AZ::Name m_luminanceHeatmapNameId;
            const AZ::Name m_luminanceHistogramGeneratorNameId;            
        };

    }
}
