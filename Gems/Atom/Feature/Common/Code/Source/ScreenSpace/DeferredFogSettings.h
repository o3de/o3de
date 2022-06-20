/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>

#include <AzCore/RTTI/ReflectContext.h>

#include <Atom/RHI.Reflect/ShaderResourceGroupLayoutDescriptor.h>

#include <Atom/RPI.Public/Shader/Shader.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>
#include <Atom/RPI.Public/Image/StreamingImage.h>

#include <Atom/Feature/ParamMacros/ParamMacrosHowTo.inl>    // for education purposes
#include <Atom/Feature/ScreenSpace/DeferredFogSettingsInterface.h>
#include <PostProcess/PostProcessBase.h>

namespace AZ
{
    namespace Render
    {
        class PostProcessSettings;

        // The post process sub-settings class for the exposure control feature over
        // full screen deferred fog.
        // The fog is calculated using the linear depth and turbulence texture with two blended
        // octaves that emulate the fog thickness and motion along the view ray direction.
        //
        // [GFX TODO] most methods in this class can be auto generated using a reflection mechanism
        //  based on the PerPass Srg.  Currently we don't have such a reflection system in place
        //  and so, a temporary approach demonstrating partial reflection through preprocessor
        //  macros is used.
        class DeferredFogSettings final :
            public DeferredFogSettingsInterface,
            public PostProcessBase
        {
            friend class DeferredFogPass;
            friend class PostProcessSettings;
            friend class PostProcessFeatureProcessor;

        public:
            AZ_RTTI(DeferredFogSettings, "{FD822CC5-4E7B-4471-AA7D-1FCDF6CAC979}",
                AZ::Render::DeferredFogSettingsInterface, AZ::Render::PostProcessBase);
            AZ_CLASS_ALLOCATOR(AZ::Render::DeferredFogSettings, SystemAllocator, 0);

            DeferredFogSettings(PostProcessFeatureProcessor* featureProcessor);
            DeferredFogSettings();
            ~DeferredFogSettings() = default;

            // DeferredFogSettingsInterface overrides...
            void OnSettingsChanged() override;
            bool GetSettingsNeedUpdate()
            {
                return m_needUpdate;
            }
            void SetSettingsNeedUpdate(bool needUpdate)
            {
                m_needUpdate = needUpdate;
            }

            void SetEnabled(bool value) override;
            bool GetEnabled() const override
            {
                return m_enabled;
            }

            void SetInitialized(bool isInitialized) override
            {
                m_isInitialized = isInitialized;
            }
            bool IsInitialized() override
            {
                return m_isInitialized;
            }

            void SetUseNoiseTextureShaderOption(bool value) override
            {
                m_useNoiseTextureShaderOption = value;
            }
            bool GetUseNoiseTextureShaderOption() override
            {
                return m_useNoiseTextureShaderOption;
            }

            void SetEnableFogLayerShaderOption(bool value) override
            {
                m_enableFogLayerShaderOption = value;
            }
            bool GetEnableFogLayerShaderOption() override
            {
                return m_enableFogLayerShaderOption;
            }

            // Called by the post effects feature processor for doing per frame prep if required
            void Simulate([[maybe_unused]]float deltaTime) {};

            // Applies settings from this onto target using override settings and passed alpha value for blending
            void ApplySettingsTo(DeferredFogSettings* target, float alpha) const;

            Data::Instance<RPI::StreamingImage> LoadStreamingImage(const char* textureFilePath, const char* sampleName);

            //---------------------------------------------------------
            // Generate Get / Set override methods
#define AZ_GFX_COMMON_PARAM(ValueType, Name, MemberName, DefaultValue)                                  \
        ValueType Get##Name() const override;                                                           \
        void Set##Name(ValueType val) override;                                                         \

#include <Atom/Feature/ParamMacros/MapParamCommon.inl>
#include <Atom/Feature/ScreenSpace/DeferredFogParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>

        private:
            //---------------------------------------------------------
            
            // SRG constants
#define AZ_GFX_COMMON_PARAM(ValueType, Name, MemberName, DefaultValue)                                  \
        ValueType MemberName = DefaultValue;                                                            \

#include <Atom/Feature/ParamMacros/MapParamCommon.inl>
#include <Atom/Feature/ScreenSpace/DeferredFogParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>

            //---------------------------------------------------------
            // SRG constants binding indices
#define AZ_GFX_COMMON_PARAM(ValueType, Name, MemberName, DefaultValue)      \
            RHI::ShaderInputConstantIndex MemberName##SrgIndex;             \

#include <Atom/Feature/ParamMacros/MapParamCommon.inl>

            // Change the handling for texture handles
#undef  AZ_GFX_TEXTURE2D_PARAM
#define AZ_GFX_TEXTURE2D_PARAM(Name, MemberName, DefaultValue)       \
            RHI::ShaderInputImageIndex MemberName##SrgIndex;         \

#include <Atom/Feature/ScreenSpace/DeferredFogParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>

            //---------------------------------------------------------
            // Generate all Image members instances
            // Set all macros to be empty, but override the texture for defining image members
#include <Atom/Feature/ParamMacros/MapParamEmpty.inl>

            // Macro to replace only the image macro for defining only image resources
#undef  AZ_GFX_TEXTURE2D_PARAM
#define AZ_GFX_TEXTURE2D_PARAM(Name, MemberName, DefaultValue)                \
            Data::Instance<AZ::RPI::Image> MemberName##Image;                 \

#include <Atom/Feature/ScreenSpace/DeferredFogParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>

            PostProcessSettings* m_parentSettings = nullptr;

            // 'm_enabled' indicates if the pass should be enabled. By default this is set to true since if the
            // pass is data driven enabled, the default settings should not disable it.  If it is driven by
            // an engine component, these settings will be replaced by the component's settings controlled by
            // the user.
            bool m_enabled = true;

            // Indication if the SRG indices were set
            bool m_isInitialized = false;

            // Indicates that the srg indices were set
            bool m_needUpdate = true;

            bool m_useNoiseTextureShaderOption = true;
            bool m_enableFogLayerShaderOption = true;

        };

    }
}
