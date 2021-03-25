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

#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/View.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/Shader/ShaderVariant.h>

#include <Atom/RHI/FrameScheduler.h>
#include <Atom/RHI/PipelineState.h>

#include <PostProcessing/LookModificationCompositePass.h>
#include <PostProcess/PostProcessFeatureProcessor.h>
#include <PostProcess/ExposureControl/ExposureControlSettings.h>

namespace AZ
{
    namespace Render
    {
        RPI::Ptr<LookModificationCompositePass> LookModificationCompositePass::Create(const RPI::PassDescriptor& descriptor)
        {
            RPI::Ptr<LookModificationCompositePass> pass = aznew LookModificationCompositePass(descriptor);
            return AZStd::move(pass);
        }

        LookModificationCompositePass::LookModificationCompositePass(const RPI::PassDescriptor& descriptor)
            : AZ::RPI::FullscreenTrianglePass(descriptor)
            , m_exposureShaderVariantOptionName(ExposureShaderVariantOptionName)
            , m_colorGradingShaderVariantOptionName(ColorGradingShaderVariantOptionName)
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

        void LookModificationCompositePass::Init()
        {
            FullscreenTrianglePass::Init();

            AZ_Assert(m_shaderResourceGroup != nullptr, "LookModificationCompositePass %s has a null shader resource group when calling Init.", GetPathName().GetCStr());

            if (m_shaderResourceGroup != nullptr)
            {
                m_shaderColorGradingLutImageIndex = m_shaderResourceGroup->FindShaderInputImageIndex(Name{ "m_gradingLut" });

                m_shaderColorGradingShaperTypeIndex = m_shaderResourceGroup->FindShaderInputConstantIndex(Name{ "m_shaperType" });
                m_shaderColorGradingShaperBiasIndex = m_shaderResourceGroup->FindShaderInputConstantIndex(Name{ "m_shaperBias" });
                m_shaderColorGradingShaperScaleIndex = m_shaderResourceGroup->FindShaderInputConstantIndex(Name{ "m_shaperScale" });
            }

            InitializeShaderVariant();
        }

        void LookModificationCompositePass::InitializeShaderVariant()
        {
            AZ_Assert(m_shader != nullptr, "LookModificationCompositePass %s has a null shader when calling InitializeShaderVariant.", GetPathName().GetCStr());

            AZStd::vector<AZ::Name> exposureVariationTypes = { AZ::Name("true"), AZ::Name("false") };
            AZStd::vector<AZ::Name> colorGradingVariationTypes = { AZ::Name("true"), AZ::Name("false") };

            auto exposureVariationTypeCount = exposureVariationTypes.size();
            auto totalVariationCount = exposureVariationTypes.size() * colorGradingVariationTypes.size();

            // Caching all pipeline state for each shader variation for performance reason.
            for (auto shaderVariantIndex = 0; shaderVariantIndex < totalVariationCount; ++shaderVariantIndex)
            {
                auto shaderOption = m_shader->CreateShaderOptionGroup();
                shaderOption.SetValue(m_exposureShaderVariantOptionName, exposureVariationTypes[shaderVariantIndex % exposureVariationTypeCount]);
                shaderOption.SetValue(m_colorGradingShaderVariantOptionName, colorGradingVariationTypes[shaderVariantIndex / exposureVariationTypeCount]);

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

                    m_shaderResourceGroup->SetConstant(m_shaderColorGradingShaperTypeIndex, m_colorGradingShaperParams.type);
                    m_shaderResourceGroup->SetConstant(m_shaderColorGradingShaperBiasIndex, m_colorGradingShaperParams.bias);
                    m_shaderResourceGroup->SetConstant(m_shaderColorGradingShaperScaleIndex, m_colorGradingShaperParams.scale);
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
    }   // namespace Render
}   // namespace AZ
