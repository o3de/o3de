/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/View.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/Pass/PassFilter.h>
#include <Atom/RPI.Public/Pass/PassSystem.h>
#include <Atom/RPI.Public/Shader/ShaderVariant.h>

#include <Atom/RHI/FrameScheduler.h>
#include <Atom/RHI/PipelineState.h>

#include <AzCore/Console/Console.h>

#include <PostProcessing/LookModificationCompositePass.h>
#include <PostProcess/PostProcessFeatureProcessor.h>
#include <PostProcess/ExposureControl/ExposureControlSettings.h>

namespace AZ
{
    namespace Render
    {
        AZ_CVAR(uint8_t,
            r_lutSampleQuality,
            0,
            [](const uint8_t& value)
            {
                RPI::PassFilter passFilter = RPI::PassFilter::CreateWithPassClass<LookModificationCompositePass>();
                RPI::PassSystemInterface::Get()->ForEachPass(passFilter, [value](RPI::Pass* pass) -> RPI::PassFilterExecutionFlow
                    {
                        LookModificationCompositePass* lookModPass = azrtti_cast<LookModificationCompositePass*>(pass);
                        lookModPass->SetSampleQuality(LookModificationCompositePass::SampleQuality(value));

                         return RPI::PassFilterExecutionFlow::ContinueVisitingPasses;
                    });
            },
            ConsoleFunctorFlags::Null,
            "This can be increased to deal with particularly tricky luts. Range (0-2). 0 (default) - Standard linear sampling. 1 - 7 tap b-spline sampling. 2 - 19 tap b-spline sampling."
        );
        
        RPI::Ptr<LookModificationCompositePass> LookModificationCompositePass::Create(const RPI::PassDescriptor& descriptor)
        {
            RPI::Ptr<LookModificationCompositePass> pass = aznew LookModificationCompositePass(descriptor);
            return AZStd::move(pass);
        }

        LookModificationCompositePass::LookModificationCompositePass(const RPI::PassDescriptor& descriptor)
            : AZ::RPI::FullscreenTrianglePass(descriptor)
        {
        }

        void LookModificationCompositePass::SetExposureControlEnabled(bool enabled)
        {
            if (m_exposureControlEnabled != enabled)
            {
                m_exposureControlEnabled = enabled;
                m_needToUpdateShaderVariant = true;
            }
        }

        void LookModificationCompositePass::InitializeInternal()
        {
            FullscreenTrianglePass::InitializeInternal();

            m_shaderColorGradingLutImageIndex.Reset();
            m_shaderColorGradingShaperTypeIndex.Reset();
            m_shaderColorGradingShaperBiasIndex.Reset();
            m_shaderColorGradingShaperScaleIndex.Reset();

            InitializeShaderVariant();
        }

        void LookModificationCompositePass::InitializeShaderVariant()
        {
            AZ_Assert(m_shader != nullptr, "LookModificationCompositePass %s has a null shader when calling InitializeShaderVariant.", GetPathName().GetCStr());

            struct OptionSettings
            {
                AZ::Name m_enableExposureControl;
                AZ::Name m_enableColorGrading;
                RPI::ShaderOptionValue m_lutSampleQuality;

                OptionSettings(const char* enableExposureControl, const char* enableColorGrading, SampleQuality sampleQuality)
                    : m_enableExposureControl(Name(enableExposureControl))
                    , m_enableColorGrading(Name(enableColorGrading))
                    , m_lutSampleQuality(RPI::ShaderOptionValue(sampleQuality))
                {}
            };

            AZStd::vector<OptionSettings> options =
            {
                { "false", "false", SampleQuality::Linear },
                { "true", "false", SampleQuality::Linear },
                { "false", "true", SampleQuality::Linear },
                { "false", "true", SampleQuality::BSpline7Tap },
                { "false", "true", SampleQuality::BSpline19Tap },
                { "true", "true", SampleQuality::Linear },
                { "true", "true", SampleQuality::BSpline7Tap },
                { "true", "true", SampleQuality::BSpline19Tap },
            };

            // Caching all pipeline state for each shader variation for performance reason.
            for (auto shaderVariantIndex = 0; shaderVariantIndex < options.size(); ++shaderVariantIndex)
            {
                auto shaderOption = m_shader->CreateShaderOptionGroup();
                shaderOption.SetValue(m_exposureShaderVariantOptionName, options.at(shaderVariantIndex).m_enableExposureControl);
                shaderOption.SetValue(m_colorGradingShaderVariantOptionName, options.at(shaderVariantIndex).m_enableColorGrading);
                shaderOption.SetValue(m_lutSampleQualityShaderVariantOptionName, options.at(shaderVariantIndex).m_lutSampleQuality);
                PreloadShaderVariant(m_shader, shaderOption, GetRenderAttachmentConfiguration(), GetMultisampleState());
            }

            m_needToUpdateShaderVariant = true;
        }

        void LookModificationCompositePass::FrameBeginInternal(FramePrepareParams params)
        {
            UpdateExposureFeatureState();
            UpdateLookModificationFeatureState();
            FullscreenTrianglePass::FrameBeginInternal(params);
        }

        void LookModificationCompositePass::UpdateExposureFeatureState()
        {
            bool exposureControlEnabled = false;
            AZ::RPI::Scene* scene = GetScene();
            if (scene)
            {
                PostProcessFeatureProcessor* fp = scene->GetFeatureProcessor<PostProcessFeatureProcessor>();
                AZ::RPI::ViewPtr view = scene->GetDefaultRenderPipeline()->GetDefaultView();
                if (fp)
                {
                    PostProcessSettings* postProcessSettings = fp->GetLevelSettingsFromView(view);
                    if (postProcessSettings)
                    {
                        ExposureControlSettings* settings = postProcessSettings->GetExposureControlSettings();
                        if (settings)
                        {
                            exposureControlEnabled = settings->GetEnabled();
                        }
                    }
                }
            }

            SetExposureControlEnabled(exposureControlEnabled);
        }

        void LookModificationCompositePass::UpdateLookModificationFeatureState()
        {
            bool colorGradingLutEnabled = false;
            AZ::RPI::Scene* scene = GetScene();
            if (scene)
            {
                PostProcessFeatureProcessor* fp = scene->GetFeatureProcessor<PostProcessFeatureProcessor>();
                AZ::RPI::ViewPtr view = scene->GetDefaultRenderPipeline()->GetDefaultView();
                if (fp)
                {
                    PostProcessSettings* postProcessSettings = fp->GetLevelSettingsFromView(view);
                    if (postProcessSettings)
                    {
                        LookModificationSettings* settings = postProcessSettings->GetLookModificationSettings();
                        if (settings)
                        {
                            settings->PrepareLutBlending();
                            colorGradingLutEnabled = settings->GetLutBlendStackSize() > 0;

                            if (colorGradingLutEnabled)
                            {
                                AcesDisplayMapperFeatureProcessor* dmfp = scene->GetFeatureProcessor<AcesDisplayMapperFeatureProcessor>();
                                dmfp->GetOwnedLut(m_blendedColorGradingLut, AZ::Name("ColorGradingBlendedLut"));
                                if (!m_blendedColorGradingLut.m_lutImage)
                                {
                                    AZ_Warning("LookModificationCompositePass", false, "Unable to load blended color grading LUT.");
                                }
                            }
                        }
                    }
                }
            }

            SetColorGradingLutEnabled(colorGradingLutEnabled);
        }

        void LookModificationCompositePass::SetColorGradingLutEnabled(bool enabled)
        {
            if (m_colorGradingLutEnabled != enabled)
            {
                m_colorGradingLutEnabled = enabled;
                m_needToUpdateShaderVariant = true;
            }
        }

        void LookModificationCompositePass::CompileResources(const RHI::FrameGraphCompileContext& context)
        {
            AZ_Assert(m_shaderResourceGroup != nullptr, "LookModificationCompositePass %s has a null shader resource group when calling Compile.", GetPathName().GetCStr());

            if (m_needToUpdateShaderVariant)
            {
                UpdateCurrentShaderVariant();
            }

            CompileShaderVariant(m_shaderResourceGroup);

            if (m_shaderResourceGroup != nullptr)
            {
                if (m_colorGradingLutEnabled == true && m_blendedColorGradingLut.m_lutImage)
                {
                    m_shaderResourceGroup->SetImageView(m_shaderColorGradingLutImageIndex, m_blendedColorGradingLut.m_lutImageView.get());

                    m_shaderResourceGroup->SetConstant(m_shaderColorGradingShaperTypeIndex, m_colorGradingShaperParams.m_type);
                    m_shaderResourceGroup->SetConstant(m_shaderColorGradingShaperBiasIndex, m_colorGradingShaperParams.m_bias);
                    m_shaderResourceGroup->SetConstant(m_shaderColorGradingShaperScaleIndex, m_colorGradingShaperParams.m_scale);
                }
            }

            BindPassSrg(context, m_shaderResourceGroup);
            m_shaderResourceGroup->Compile();
            BindSrg(m_shaderResourceGroup->GetRHIShaderResourceGroup());
        }

        void LookModificationCompositePass::UpdateCurrentShaderVariant()
        {
            AZ_Assert(m_shader != nullptr, "LookModificationCompositePass %s has a null shader when calling UpdateCurrentShaderVariant.", GetPathName().GetCStr());

            auto shaderOption = m_shader->CreateShaderOptionGroup();
            // Decide which shader to use.
            shaderOption.SetValue(m_exposureShaderVariantOptionName, m_exposureControlEnabled ? AZ::Name("true") : AZ::Name("false"));
            shaderOption.SetValue(m_colorGradingShaderVariantOptionName, m_colorGradingLutEnabled ? AZ::Name("true") : AZ::Name("false"));
            shaderOption.SetValue(m_lutSampleQualityShaderVariantOptionName, RPI::ShaderOptionValue(m_sampleQuality));
            
            UpdateShaderVariant(shaderOption);

            m_needToUpdateShaderVariant = false;
        }

        void LookModificationCompositePass::BuildCommandListInternal(const RHI::FrameGraphExecuteContext& context)
        {
            AZ_Assert(m_shaderResourceGroup != nullptr, "LookModificationCompositePass %s has a null shader resource group when calling Execute.", GetPathName().GetCStr());

            RHI::CommandList* commandList = context.GetCommandList();

            commandList->SetViewport(m_viewportState);
            commandList->SetScissor(m_scissorState);

            SetSrgsForDraw(commandList);

            m_item.m_pipelineState = GetPipelineStateFromShaderVariant();

            commandList->Submit(m_item);
        }

        void LookModificationCompositePass::SetShaperParameters(const ShaperParams& shaperParams)
        {
            m_colorGradingShaperParams = shaperParams;
        }
        
        void LookModificationCompositePass::SetSampleQuality(SampleQuality sampleQuality)
        {
            m_sampleQuality = sampleQuality;
            m_needToUpdateShaderVariant = true;
        }

    }   // namespace Render
}   // namespace AZ
