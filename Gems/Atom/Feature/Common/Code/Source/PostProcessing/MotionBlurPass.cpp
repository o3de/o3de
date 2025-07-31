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

namespace AZ
{
    namespace Render
    {
        RPI::Ptr<MotionBlurPass> MotionBlurPass::Create(const RPI::PassDescriptor& descriptor)
        {
            RPI::Ptr<MotionBlurPass> pass = aznew MotionBlurPass(descriptor);
            return AZStd::move(pass);
        }

        MotionBlurPass::MotionBlurPass(const RPI::PassDescriptor& descriptor)
            : RPI::ComputePass(descriptor)
        {
        }

        void MotionBlurPass::InitializeInternal()
        {
            ComputePass::InitializeInternal();

            InitializeShaderVariant();
        }

        void MotionBlurPass::InitializeShaderVariant()
        {
            AZ_Assert(m_shader != nullptr, "MotionBlurPass %s has a null shader when calling InitializeShaderVariant.", GetPathName().GetCStr());

            AZStd::vector<RPI::ShaderOptionValue> options =
            {
                RPI::ShaderOptionValue(MotionBlur::SampleQuality::Low),
                RPI::ShaderOptionValue(MotionBlur::SampleQuality::Medium),
                RPI::ShaderOptionValue(MotionBlur::SampleQuality::High),
                RPI::ShaderOptionValue(MotionBlur::SampleQuality::Ultra),
            };

            // Caching all pipeline state for each shader variation for performance reason.
            for (auto shaderVariantIndex = 0; shaderVariantIndex < options.size(); ++shaderVariantIndex)
            {
                auto shaderOption = m_shader->CreateShaderOptionGroup();
                shaderOption.SetValue(m_sampleQualityShaderVariantOptionName, options.at(shaderVariantIndex));
                PreloadShaderVariantForDispatch(m_shader, shaderOption);
            }

            m_needToUpdateShaderVariant = true;
        }

        void MotionBlurPass::SetSampleQuality(MotionBlur::SampleQuality sampleQuality)
        {
            if (sampleQuality != m_sampleQuality)
            {
                m_sampleQuality = sampleQuality;
                m_needToUpdateShaderVariant = true;
            }
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
                AZStd::array<AZ::u32, 2> m_outputSize;
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
                        SetSampleQuality(MotionBlurSettings->GetSampleQuality());
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

            ComputePass::FrameBeginInternal(params);
        }

        void MotionBlurPass::CompileResources(const RHI::FrameGraphCompileContext& context)
        {
            AZ_Assert(m_shaderResourceGroup != nullptr, "MotionBlurPass %s has a null shader resource group when calling Compile.", GetPathName().GetCStr());

            if (m_needToUpdateShaderVariant)
            {
                UpdateCurrentShaderVariant();
            }

            CompileShaderVariant(m_shaderResourceGroup);
            ComputePass::CompileResources(context);
        }

        void MotionBlurPass::UpdateCurrentShaderVariant()
        {
            AZ_Assert(m_shader != nullptr, "MotionBlurPass %s has a null shader when calling UpdateCurrentShaderVariant.", GetPathName().GetCStr());

            auto shaderOption = m_shader->CreateShaderOptionGroup();
            // Decide which shader to use.
            shaderOption.SetValue(m_sampleQualityShaderVariantOptionName, RPI::ShaderOptionValue(m_sampleQuality));

            UpdateShaderVariant(shaderOption);

            m_needToUpdateShaderVariant = false;
        }

        void MotionBlurPass::BuildCommandListInternal(const RHI::FrameGraphExecuteContext& context)
        {
            AZ_Assert(m_shaderResourceGroup != nullptr, "MotionBlurPass %s has a null shader resource group when calling Execute.", GetPathName().GetCStr());

            RHI::CommandList* commandList = context.GetCommandList();

            SetSrgsForDispatch(context);

            m_dispatchItem.SetPipelineState(GetPipelineStateFromShaderVariant());

            commandList->Submit(m_dispatchItem.GetDeviceDispatchItem(context.GetDeviceIndex()));
        }
    } // namespace Render
} // namespace AZ
