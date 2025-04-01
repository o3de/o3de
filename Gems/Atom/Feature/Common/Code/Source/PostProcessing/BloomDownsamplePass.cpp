/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <PostProcessing/BloomDownsamplePass.h>
#include <PostProcess/Bloom/BloomSettings.h>
#include <PostProcess/PostProcessFeatureProcessor.h>

#include <Atom/RHI/CommandList.h>
#include <Atom/RHI/Factory.h>
#include <Atom/RHI/FrameScheduler.h>

#include <Atom/RPI.Reflect/Pass/PassTemplate.h>

#include <Atom/RPI.Public/Pass/PassUtils.h>
#include <Atom/RPI.Public/Pass/PassAttachment.h>
#include <Atom/RPI.Public/RPIUtils.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/View.h>
#include <Atom/RPI.Public/RPISystemInterface.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>
#include <Atom/RPI.Public/Scene.h>

#include <Atom/RPI.Public/Image/AttachmentImagePool.h>
#include <Atom/RPI.Public/Image/ImageSystemInterface.h>


namespace AZ
{
    namespace Render
    {
        RPI::Ptr<BloomDownsamplePass> BloomDownsamplePass::Create(const RPI::PassDescriptor& descriptor)
        {
            RPI::Ptr<BloomDownsamplePass> pass = aznew BloomDownsamplePass(descriptor);
            return AZStd::move(pass);
        }

        BloomDownsamplePass::BloomDownsamplePass(const RPI::PassDescriptor& descriptor)
            : ComputePass(descriptor)
        { }

        void BloomDownsamplePass::BuildOutAttachmentBinding()
        {
            RPI::Ptr<RPI::PassAttachment> outAttachment = m_ownedAttachments[0];
            RPI::PassAttachmentBinding* outAttachmentBinding = FindAttachmentBinding(AZ::Name("Output"));
            if (outAttachmentBinding)
            {
                // We use the owned attachment as mip level 0, because we can't have overlapping attachments
                // with write access.
                RHI::ImageViewDescriptor attachmentViewDesc;
                attachmentViewDesc.m_mipSliceMin = 0;
                attachmentViewDesc.m_mipSliceMax = 0;
                outAttachmentBinding->m_shaderInputName = Name{ "m_targetMipLevel0" };
                outAttachmentBinding->m_unifiedScopeDesc.SetAsImage(attachmentViewDesc);
            }

            // Create the rest of mip level attachments
            for (uint16_t i = 1; i < Render::Bloom::MaxStageCount; ++i)
            {
                // Create bindings

                // Set pass slot
                RPI::PassAttachmentBinding outBinding;
                outBinding.m_name = Name{ AZStd::string::format("Downsampled%d", i) };
                outBinding.m_shaderInputName = Name{ AZStd::string::format("m_targetMipLevel%d", i) };
                outBinding.m_slotType = RPI::PassSlotType::Output;
                outBinding.m_scopeAttachmentUsage = RHI::ScopeAttachmentUsage::Shader;
                
                // Set image view descriptor
                RHI::ImageViewDescriptor outViewDesc;
                outViewDesc.m_mipSliceMin = i;
                outViewDesc.m_mipSliceMax = i;
                outBinding.m_unifiedScopeDesc.SetAsImage(outViewDesc);
                outBinding.SetAttachment(outAttachment);

                AddAttachmentBinding(outBinding);
            }

            ComputePass::BuildInternal();
        }

        void BloomDownsamplePass::BuildInternal()
        {
            BuildOutAttachmentBinding();
        }

        AZ::Vector4 BloomDownsamplePass::CalThresholdConstants()
        {
            // These constants will be used in shader to compute a soft knee based threshold 
            float x = m_threshold;
            float y = x * m_knee;
            float z = 2.0f * y;
            float w = 1.0f / (4.0f * y + 1e-5f);
            y -= x;

            return AZ::Vector4(x, y, z, w);
        }

        void BloomDownsamplePass::FrameBeginInternal(FramePrepareParams params)
        {
            RPI::Scene* scene = GetScene();
            PostProcessFeatureProcessor* fp = scene->GetFeatureProcessor<PostProcessFeatureProcessor>();
            RPI::ViewPtr view = m_pipeline->GetFirstView(GetPipelineViewTag());
            if (fp)
            {
                PostProcessSettings* postProcessSettings = fp->GetLevelSettingsFromView(view);
                if (postProcessSettings)
                {
                    BloomSettings* bloomSettings = postProcessSettings->GetBloomSettings();
                    if (bloomSettings)
                    {
                        m_threshold = bloomSettings->GetThreshold();
                        m_knee = bloomSettings->GetKnee();
                        m_shaderResourceGroup->SetConstant(m_thresholdConstantsInputIndex, CalThresholdConstants());
                    }
                }
            }

            RHI::Size sourceImageSize;
            RPI::PassAttachment* inputAttachment = GetInputBinding(0).GetAttachment().get();
            sourceImageSize = inputAttachment->m_descriptor.m_image.m_size;

            // Update shader constant
            m_shaderResourceGroup->SetConstant(m_sourceImageTexelSizeInputIndex, AZ::Vector2(1.0f / static_cast<float>(sourceImageSize.m_width), 1.0f / static_cast<float>(sourceImageSize.m_height)));

            ComputePass::FrameBeginInternal(params);
        }
    }   // namespace RPI
}   // namespace AZ
