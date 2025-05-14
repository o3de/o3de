/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <PostProcessing/MotionBlurPass.h>
#include <PostProcess/PostProcessFeatureProcessor.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/Pass/PassFilter.h>

#include <AzCore/Console/Console.h>

namespace AZ
{
    namespace Render
    {
        AZ_CVAR(uint8_t,
            r_motionBlurQuality,
            1,
            [](const uint8_t& value)
            {
                RPI::PassFilter passFilter = RPI::PassFilter::CreateWithPassClass<MotionBlurPass>();
                RPI::PassSystemInterface::Get()->ForEachPass(passFilter, [value](RPI::Pass* pass) -> RPI::PassFilterExecutionFlow
                    {
                        MotionBlurPass* motionBlurPass = azrtti_cast<MotionBlurPass*>(pass);
                        motionBlurPass->SetQualityLevel(value);

                        return RPI::PassFilterExecutionFlow::ContinueVisitingPasses;
                    });
            },
            ConsoleFunctorFlags::Null,
            "Quality level for motion blur effect. Range (0-3). 0 - Low, 6 samples per pixel. 1 - Medium, 14 samples per pixel. 2 - High, 24 samples per pixel. 3 - Ultra, 36 samples per pixel. "
        );

        RPI::Ptr<MotionBlurPass> MotionBlurPass::Create(const RPI::PassDescriptor& descriptor)
        {
            RPI::Ptr<MotionBlurPass> pass = aznew MotionBlurPass(descriptor);
            return AZStd::move(pass);
        }

        MotionBlurPass::MotionBlurPass(const RPI::PassDescriptor& descriptor)
            : RPI::ComputePass(descriptor)
            , m_optionName("o_sampleNumber")
            , m_optionValues
            {
                AZ::Name("SampleNumber::Sample6"),
                AZ::Name("SampleNumber::Sample14"),
                AZ::Name("SampleNumber::Sample24"),
                AZ::Name("SampleNumber::Sample36")
            }
        {
        }

        void MotionBlurPass::InitializeInternal()
        {
            ComputePass::InitializeInternal();

            m_constantsIndex.Reset();
            InitializeShaderVariant();
        }

        void MotionBlurPass::InitializeShaderVariant()
        {
            AZ_Assert(m_shader != nullptr, "MotionBlurPass %s has a null shader when calling InitializeShaderVariant.", GetPathName().GetCStr());

            // Caching all pipeline state for each shader variation for performance reason.
            auto optionValueCount = m_optionValues.size();
            for (auto shaderVariantIndex = 0; shaderVariantIndex < optionValueCount; ++shaderVariantIndex)
            {
                auto shaderOption = m_shader->CreateShaderOptionGroup();
                shaderOption.SetValue(m_optionName, m_optionValues[shaderVariantIndex]);
                PreloadShaderVariant(m_shader, shaderOption, GetRenderAttachmentConfiguration(), GetMultisampleState());
            }

            m_needToUpdateShaderVariant = true;
        }

        void MotionBlurPass::UpdateCurrentShaderVariant()
        {
            AZ_Assert(m_shader != nullptr, "MotionBlurPass %s has a null shader when calling UpdateCurrentShaderVariant.", GetPathName().GetCStr());            

            RPI::ShaderOptionGroup shaderOption = m_shader->CreateShaderOptionGroup();
            shaderOption.SetValue(m_optionName, m_optionValues[m_qualityLevel]);
            UpdateShaderVariant(shaderOption);

            m_needToUpdateShaderVariant = false;
        }

        bool MotionBlurPass::IsEnabled() const
        {
            if (!ComputePass::IsEnabled())
            {
                return false;
            }
            const RPI::Scene* scene = GetScene();
            if (!scene)
            {
                return false;
            }
            PostProcessFeatureProcessor* fp = scene->GetFeatureProcessor<PostProcessFeatureProcessor>();
            const RPI::ViewPtr view = GetRenderPipeline()->GetDefaultView();
            if (!fp)
            {
                return false;
            }
            PostProcessSettings* postProcessSettings = fp->GetLevelSettingsFromView(view);
            if (!postProcessSettings)
            {
                return false;
            }
            const MotionBlurSettings* MotionBlurSettings = postProcessSettings->GetMotionBlurSettings();
            if (!MotionBlurSettings)
            {
                return false;
            }
            return MotionBlurSettings->GetEnabled();
        }

        void MotionBlurPass::FrameBeginInternal(FramePrepareParams params)
        {
            // Must match the struct in MotionBlur.azsl
            struct Constants
            {
                AZStd::array<u32, 2> m_outputSize;
                float m_strength = MotionBlur::DefaultStrength;
                AZStd::array<float, 1> m_pad;
            } constants{};

            RPI::Scene* scene = GetScene();
            PostProcessFeatureProcessor* fp = scene->GetFeatureProcessor<PostProcessFeatureProcessor>();
            if (fp)
            {
                RPI::ViewPtr view = scene->GetDefaultRenderPipeline()->GetDefaultView();
                PostProcessSettings* postProcessSettings = fp->GetLevelSettingsFromView(view);
                if (postProcessSettings)
                {
                    MotionBlurSettings* MotionBlurSettings = postProcessSettings->GetMotionBlurSettings();
                    if (MotionBlurSettings)
                    {
                        constants.m_strength = MotionBlurSettings->GetStrength();
                    }
                }
            }

            AZ_Assert(GetOutputCount() > 0, "MotionBlurPass: No output bindings!");
            RPI::PassAttachment* outputAttachment = GetOutputBinding(0).GetAttachment().get();

            AZ_Assert(outputAttachment != nullptr, "MotionBlurPass: Output binding has no attachment!");
            RHI::Size size = outputAttachment->m_descriptor.m_image.m_size;

            constants.m_outputSize[0] = size.m_width;
            constants.m_outputSize[1] = size.m_height;

            m_shaderResourceGroup->SetConstant(m_constantsIndex, constants);

            RPI::ComputePass::FrameBeginInternal(params);
        }

        void MotionBlurPass::CompileResources(const RHI::FrameGraphCompileContext& context)
        {
            AZ_Assert(m_shaderResourceGroup, "MotionBlurPass %s has a null shader resource group when calling FrameBeginInternal.", GetPathName().GetCStr());

            if (m_needToUpdateShaderVariant)
            {
                UpdateCurrentShaderVariant();
            }

            CompileShaderVariant(m_shaderResourceGroup);
            ComputePass::CompileResources(context);
        }

        void MotionBlurPass::SetQualityLevel(uint8_t qualityLevel)
        {
            AZ_Assert(qualityLevel >= 4, "%s : illegal quality level.", GetPathName().GetCStr());
            m_qualityLevel = qualityLevel;
            m_needToUpdateShaderVariant = true;
        }
    } // namespace Render
} // namespace AZ
