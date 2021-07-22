/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ScreenSpace/DeferredFogPass.h>

#include <Atom/RHI/Factory.h>
#include <Atom/RHI/FrameGraphAttachmentInterface.h>
#include <Atom/RHI/FrameGraphInterface.h>
#include <Atom/RHI/PipelineState.h>

#include <Atom/RPI.Public/Base.h>
#include <Atom/RPI.Public/Pass/PassUtils.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RPI.Public/RPIUtils.h>
#include <PostProcess/PostProcessFeatureProcessor.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/View.h>
#include <Atom/RPI.Reflect/Pass/PassTemplate.h>
#include <Atom/RPI.Reflect/Shader/ShaderAsset.h>

namespace AZ
{
    namespace Render
    {
        DeferredFogPass::DeferredFogPass(const RPI::PassDescriptor& descriptor)
            : RPI::FullscreenTrianglePass(descriptor)
        {
        }

        RPI::Ptr<DeferredFogPass> DeferredFogPass::Create(const RPI::PassDescriptor& descriptor)
        {
            RPI::Ptr<DeferredFogPass> pass = aznew DeferredFogPass(descriptor);
            pass->SetSrgBindIndices();

            return AZStd::move(pass);
        }


        void DeferredFogPass::InitializeInternal()
        {
            FullscreenTrianglePass::InitializeInternal();

            // The following will ensure that in the case of data driven pass, the settings will get
            // updated by the pass enable state.
            // When code is involved or editor component comes to action, this value will be overriden
            // in the following frames.
            DeferredFogSettings* fogSettings = GetPassFogSettings();
            bool isEnabled = Pass::IsEnabled();     // retrieves the state from the data driven pass
            fogSettings->SetEnabled(isEnabled);     // Set it and mark for update
        }

        //---------------------------------------------------------------------
        //! Setting and Binding Shader SRG Constants using settings macro reflection

        DeferredFogSettings* DeferredFogPass::GetPassFogSettings()
        {
            RPI::Scene* scene = GetScene();
            if (!scene)
            {
                return &m_fallbackSettings;
            }

            PostProcessFeatureProcessor* fp = scene->GetFeatureProcessor<PostProcessFeatureProcessor>();
            AZ::RPI::ViewPtr view = scene->GetDefaultRenderPipeline()->GetDefaultView();
            if (fp)
            {
                PostProcessSettings* postProcessSettings = fp->GetLevelSettingsFromView(view);
                if (postProcessSettings)
                {
                    DeferredFogSettings* fogSettings = postProcessSettings->GetDeferredFogSettings();
                    if (fogSettings)
                    {   // The following is required as it indicates that the code created a control
                        // component and if/when it is removed, the default settings' fog should not be active.
                        m_fallbackSettings.SetEnabled(false);
                    }
                    return fogSettings ? fogSettings : &m_fallbackSettings;
                }
            }
            return &m_fallbackSettings;
        }


        //! Set the binding indices of all members of the SRG
        void DeferredFogPass::SetSrgBindIndices()
        {
            DeferredFogSettings* fogSettings = GetPassFogSettings();
            Data::Instance<RPI::ShaderResourceGroup> srg = m_shaderResourceGroup.get();
            
            // match and set all SRG constants' indices
#define AZ_GFX_COMMON_PARAM(ValueType, FunctionName, MemberName, DefaultValue)                          \
            fogSettings->MemberName##SrgIndex = srg->FindShaderInputConstantIndex(Name(#MemberName));   \

#include <Atom/Feature/ParamMacros/MapParamCommon.inl>
            // For texture use a different function call
#undef  AZ_GFX_TEXTURE2D_PARAM
#define AZ_GFX_TEXTURE2D_PARAM(FunctionName, MemberName, DefaultValue)                                  \
            fogSettings->MemberName##SrgIndex = srg->FindShaderInputImageIndex(Name(#MemberName));      \

#include <Atom/Feature/ScreenSpace/DeferredFogParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>

            fogSettings->SetInitialized(true);
        }


        //! Bind SRG constants - done via macro reflection
        void DeferredFogPass::SetSrgConstants()
        {
            DeferredFogSettings* fogSettings = GetPassFogSettings();
            Data::Instance<RPI::ShaderResourceGroup> srg = m_shaderResourceGroup.get();

            if (!fogSettings->IsInitialized())
            {   // Should be initialize before, but if not - this is a fail safe that will apply it once
                SetSrgBindIndices();
            }

            if (fogSettings->GetSettingsNeedUpdate())
            {   // SRG constants are up to date and will be bound as they are.
                // First time around they will be dirty to ensure properly set. 

                // Load all texture resources:
                //  first set all macros to be empty, but override the texture for setting images.
#include <Atom/Feature/ParamMacros/MapParamEmpty.inl>

#undef  AZ_GFX_TEXTURE2D_PARAM
#define AZ_GFX_TEXTURE2D_PARAM(Name, MemberName, DefaultValue)                  \
                fogSettings->MemberName##Image =                                \
                    fogSettings->LoadStreamingImage( fogSettings->MemberName.c_str(), "DeferredFogSettings" );  \

#include <Atom/Feature/ScreenSpace/DeferredFogParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>

                fogSettings->SetSettingsNeedUpdate(false);   // Avoid doing this unless data change is required
            }

            // The Srg constants value settings
#define AZ_GFX_COMMON_PARAM(ValueType, Name, MemberName, DefaultValue)                          \
            srg->SetConstant( fogSettings->MemberName##SrgIndex, fogSettings->MemberName );     \

#include <Atom/Feature/ParamMacros/MapParamCommon.inl>

            // The following macro overrides the regular macro defined above, loads an image and bind it
#undef AZ_GFX_TEXTURE2D_PARAM
#define AZ_GFX_TEXTURE2D_PARAM(Name, MemberName, DefaultValue)                      \
            if (!srg->SetImage(fogSettings->MemberName##SrgIndex, fogSettings->MemberName##Image ))           \
            {                                                                       \
                AZ_Error( "DeferredFogPass::SetSrgConstants", false, "Failed to bind SRG image for %s = %s",  \
                    #MemberName, fogSettings->MemberName.c_str() );                                      \
            }                                                                       \

#include <Atom/Feature/ScreenSpace/DeferredFogParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>
        }
        //---------------------------------------------------------------------


        void DeferredFogPass::UpdateEnable(DeferredFogSettings* fogSettings)
        {
            if (!m_pipeline || !fogSettings)
            {
                SetEnabled(false);
                return;
            }

            AZ_Assert(m_pipeline->GetScene(), "Scene shouldn't nullptr");

            if (IsEnabled() == fogSettings->GetEnabled())
            {
                return;
            }

            SetEnabled( fogSettings->GetEnabled() );
        }

        bool DeferredFogPass::IsEnabled() const 
        {
            const DeferredFogSettings* constFogSettings = const_cast<DeferredFogPass*>(this)->GetPassFogSettings();
            return constFogSettings->GetEnabled();
        }

        void DeferredFogPass::UpdateShaderOptions()
        {
            RPI::ShaderOptionGroup shaderOption = m_shader->CreateShaderOptionGroup();
            DeferredFogSettings* fogSettings = GetPassFogSettings();

            // [TODO][ATOM-13659] - AZ::Name all over our code base should use init with string and
            // hash key for the iterations themselves.
            shaderOption.SetValue(AZ::Name("o_enableFogLayer"),
                fogSettings->GetEnableFogLayerShaderOption() ? AZ::Name("true") : AZ::Name("false"));
            shaderOption.SetValue(AZ::Name("o_useNoiseTexture"),
                fogSettings->GetUseNoiseTextureShaderOption() ? AZ::Name("true") : AZ::Name("false"));

            // The following method returns the specified options, as well as fall back values for all 
            // non-specified options.  If all were set you can use the method GetShaderVariantKey that is 
            // cheaper but will not make sure the populated values has the default fall back for any unset bit.
            m_ShaderOptions = shaderOption.GetShaderVariantKeyFallbackValue();
        }

        void DeferredFogPass::SetupFrameGraphDependencies(RHI::FrameGraphInterface frameGraph)
        {
            FullscreenTrianglePass::SetupFrameGraphDependencies(frameGraph);

            // If any change was made, make sure to bind it.
            DeferredFogSettings* fogSettings = GetPassFogSettings();

            UpdateEnable(fogSettings);

            // Update and set the per pass shader options - this will update the current required
            // shader variant and if doesn't exist, it will be created via the compile stage
            if (m_shaderResourceGroup->HasShaderVariantKeyFallbackEntry())
            {
                UpdateShaderOptions();
            }

            SetSrgConstants();
        }
  
        void DeferredFogPass::CompileResources(const RHI::FrameGraphCompileContext& context)
        {
            if (m_shaderResourceGroup->HasShaderVariantKeyFallbackEntry())
            {
                m_shaderResourceGroup->SetShaderVariantKeyFallbackValue(m_ShaderOptions);
            }

            FullscreenTrianglePass::CompileResources(context);
        }
    }   // namespace Render
}   // namespace AZ

