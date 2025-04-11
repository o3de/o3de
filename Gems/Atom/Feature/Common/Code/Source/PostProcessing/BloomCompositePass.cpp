/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <PostProcessing/BloomCompositePass.h>
#include <PostProcess/Bloom/BloomSettings.h>
#include <PostProcess/PostProcessFeatureProcessor.h>

#include <Atom/RHI/CommandList.h>
#include <Atom/RHI/Factory.h>
#include <Atom/RHI/FrameScheduler.h>

#include <Atom/RPI.Reflect/Pass/PassTemplate.h>

#include <Atom/RPI.Public/Pass/PassUtils.h>
#include <Atom/RPI.Public/Pass/PassAttachment.h>
#include <Atom/RPI.Public/RPIUtils.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/View.h>
#include <Atom/RPI.Public/RPISystemInterface.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>

#include <Atom/RPI.Reflect/Pass/ComputePassData.h>

namespace AZ
{
    namespace Render
    {
        RPI::Ptr<BloomCompositePass> BloomCompositePass::Create(const RPI::PassDescriptor& descriptor)
        {
            RPI::Ptr<BloomCompositePass> pass = aznew BloomCompositePass(descriptor);
            return AZStd::move(pass);
        }

        BloomCompositePass::BloomCompositePass(const RPI::PassDescriptor& descriptor)
            : ParentPass(descriptor)
        {
            // Load DownsampleMipChainPassData (shader asset)
            const RPI::DownsampleMipChainPassData* passData = RPI::PassUtils::GetPassData<RPI::DownsampleMipChainPassData>(descriptor);
            if (passData == nullptr)
            {
                AZ_Error("PassSystem", false, "[BloomCompositePass '%s']: Trying to construct without valid DownsampleMipChainPassData!",
                    GetPathName().GetCStr());
                return;
            }

            m_passData = *passData;
        }

        void BloomCompositePass::BuildInternal()
        {
            BuildChildPasses();
            ParentPass::BuildInternal();
        }

        void BloomCompositePass::FrameBeginInternal(FramePrepareParams params)
        {
            GetAttachmentInfo();
            UpdateParameters();
            UpdateChildren();

            ParentPass::FrameBeginInternal(params);
        }

        void BloomCompositePass::GetAttachmentInfo()
        {
            AZ_Assert(GetInputCount() > 0, "[BloomCompositePass '%s']: must have an input", GetPathName().GetCStr());

            // The Output attachment of composite pass is provided by downsample pass, because
            // composite pass as a parent pass is unable to bind attachment to slot by itself
            // in the pass file, which could result in errors during following passes' initialization
            AZ_Assert(GetInputOutputCount() > 0, "[BloomCompositePass '%s']: must have an output", GetPathName().GetCStr());

            RPI::PassAttachment* inAttachment = GetInputBinding(0).GetAttachment().get();
            RPI::PassAttachment* outAttachment = GetInputOutputBinding(0).GetAttachment().get();

            if (inAttachment != nullptr && outAttachment != nullptr)
            {
                m_inputWidth = inAttachment->m_descriptor.m_image.m_size.m_width;
                m_inputHeight = inAttachment->m_descriptor.m_image.m_size.m_height;

                m_outputWidth = outAttachment->m_descriptor.m_image.m_size.m_width;
                m_outputHeight = outAttachment->m_descriptor.m_image.m_size.m_height;
            }
        }

        void BloomCompositePass::UpdateParameters()
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
                        m_intensity = bloomSettings->GetIntensity();
                        m_enableBicubic = bloomSettings->GetBicubicEnabled();
                        m_tintData[0] = bloomSettings->GetTintStage0();
                        m_tintData[1] = bloomSettings->GetTintStage1();
                        m_tintData[2] = bloomSettings->GetTintStage2();
                        m_tintData[3] = bloomSettings->GetTintStage3();
                        m_tintData[4] = bloomSettings->GetTintStage4();
                    }
                }
            }
        }

        void BloomCompositePass::CreateBinding(BloomCompositeChildPass* pass, uint32_t mipLevel)
        {
            RPI::PassAttachmentBinding& parentInBinding = GetInputBinding(0);
            const RPI::Ptr<RPI::PassAttachment>& parentInAttachment = parentInBinding.GetAttachment();
            if (!parentInAttachment)
            {
                AZ_Error(
                    "PassSystem",
                    false,
                    "[BloomCompositePass '%s']: Slot '%s' has no attachment.",
                    GetPathName().GetCStr(),
                    parentInBinding.m_name.GetCStr());
                return;
            }

            RPI::PassAttachmentBinding& parentInOutBinding = GetInputOutputBinding(0);
            const RPI::Ptr<RPI::PassAttachment>& parentInOutAttachment = parentInOutBinding.GetAttachment();

            if (!parentInOutAttachment)
            {
                AZ_Error(
                    "PassSystem",
                    false,
                    "[BloomCompositePass '%s']: Slot '%s' has no attachment.",
                    GetPathName().GetCStr(),
                    parentInOutBinding.m_name.GetCStr());
                return;
            }

            // Create input binding, from downsampling pass
            RPI::PassAttachmentBinding inBinding;
            inBinding.m_name = "Input";
            inBinding.m_shaderInputName = "m_inputTexture";
            inBinding.m_slotType = RPI::PassSlotType::Input;
            inBinding.m_scopeAttachmentUsage = RHI::ScopeAttachmentUsage::Shader;
            inBinding.m_connectedBinding = &parentInBinding;

            RHI::ImageViewDescriptor inViewDesc;
            inViewDesc.m_mipSliceMin = static_cast<uint16_t>(mipLevel);
            inViewDesc.m_mipSliceMax = static_cast<uint16_t>(mipLevel);
            inBinding.m_unifiedScopeDesc.SetAsImage(inViewDesc);
            inBinding.SetAttachment(parentInAttachment);


            pass->AddAttachmentBinding(inBinding);

            // Create output binding, owned by current pass
            RPI::PassAttachmentBinding outBinding;
            outBinding.m_name = "Output";
            outBinding.m_shaderInputName = "m_outputTexture";
            outBinding.m_slotType = RPI::PassSlotType::Output;
            outBinding.m_scopeAttachmentUsage = RHI::ScopeAttachmentUsage::Shader;

            if (mipLevel == 0)
            {
                outBinding.m_connectedBinding = &parentInOutBinding;
                outBinding.SetAttachment(parentInOutAttachment);
            }
            else
            {
                outBinding.m_connectedBinding = &parentInBinding;

                RHI::ImageViewDescriptor outViewDesc;
                outViewDesc.m_mipSliceMin = static_cast<uint16_t>(mipLevel - 1);
                outViewDesc.m_mipSliceMax = static_cast<uint16_t>(mipLevel - 1);
                outBinding.m_unifiedScopeDesc.SetAsImage(outViewDesc);
                outBinding.SetAttachment(parentInAttachment);
            }
            
            pass->AddAttachmentBinding(outBinding);
        }

        void BloomCompositePass::BuildChildPasses()
        {
            if (!m_children.empty())
            {
                // In this case children are still exists but attachment binding is flushed out, so we rebind them again
                for (size_t childIndex = 0, mipLevel = m_children.size() - 1;
                     childIndex < m_children.size();
                     ++childIndex, --mipLevel)
                {
                    BloomCompositeChildPass* compositeChild = static_cast<BloomCompositeChildPass*>(m_children[childIndex].get());

                    CreateBinding(compositeChild, static_cast<uint32_t>(mipLevel));
                }
            }
            else
            {
                // Create children
                RPI::PassSystemInterface* passSystem = RPI::PassSystemInterface::Get();

                for (size_t childIndex = 0, mipLevel = Render::Bloom::MaxStageCount - 1;
                     childIndex < Render::Bloom::MaxStageCount;
                     ++childIndex, --mipLevel)
                {
                    RPI::PassDescriptor childDesc;
                    childDesc.m_passData = AZStd::make_shared<RPI::ComputePassData>();
                    RPI::ComputePassData* passData = static_cast<RPI::ComputePassData*>(childDesc.m_passData.get());
                    passData->m_shaderReference = m_passData.m_shaderReference;

                    childDesc.m_passName = Name{ AZStd::string::format("BloomComposite%d", static_cast<uint32_t>(childIndex)) };

                    RPI::Ptr<BloomCompositeChildPass> childPass = passSystem->CreatePass<BloomCompositeChildPass>(childDesc);

                    CreateBinding(childPass.get(), static_cast<uint32_t>(mipLevel));
                    AddChild(childPass);
                }
            }
        }

        void BloomCompositePass::UpdateChildren()
        {
            uint32_t sourceWidth, sourceHeight, targetWidth, targetHeight;

            targetWidth = m_inputWidth;
            targetHeight = m_inputHeight;

            sourceWidth = targetWidth / 2;
            sourceHeight = targetHeight / 2;

            BloomCompositeChildPass* compositeChild = static_cast<BloomCompositeChildPass*>(m_children[m_children.size() - 1].get());
            compositeChild->UpdateParameters(0, m_inputWidth, m_inputHeight, m_outputWidth, m_outputHeight, m_enableBicubic, m_tintData[0], m_intensity);

            for (int childIndex = static_cast<int>(m_children.size() - 2), mipLevel = 1;
                 childIndex >= 0;
                 --childIndex, ++mipLevel)
            {
                BloomCompositeChildPass* blurChild = static_cast<BloomCompositeChildPass*>(m_children[childIndex].get());
                blurChild->UpdateParameters(static_cast<uint32_t>(mipLevel), sourceWidth, sourceHeight, targetWidth, targetHeight, m_enableBicubic, m_tintData[m_children.size() - childIndex - 1], m_intensity);

                targetWidth = sourceWidth;
                targetHeight = sourceHeight;

                sourceWidth = targetWidth / 2;
                sourceHeight = targetHeight / 2;
            }
        }

        RPI::Ptr<BloomCompositeChildPass> BloomCompositeChildPass::Create(const RPI::PassDescriptor& descriptor)
        {
            RPI::Ptr<BloomCompositeChildPass> pass = aznew BloomCompositeChildPass(descriptor);
            return AZStd::move(pass);
        }

        BloomCompositeChildPass::BloomCompositeChildPass(const RPI::PassDescriptor& descriptor)
            : ComputePass(descriptor)
        { }

        void BloomCompositeChildPass::UpdateParameters(uint32_t sourceMip, uint32_t sourceImageWidth, uint32_t sourceImageHeight, uint32_t targetImageWidth, uint32_t targetImageHeight, bool enableBicubic, Vector3 tint, float intensity)
        {
            m_shaderResourceGroup->SetConstant(m_intensityInputIndex, intensity);

            float sourceWidth = static_cast<float>(sourceImageWidth);
            float sourceHeight = static_cast<float>(sourceImageHeight);
            m_shaderResourceGroup->SetConstant(m_sourceImageSizeInputIndex, AZ::Vector2(sourceWidth, sourceHeight));
            m_shaderResourceGroup->SetConstant(m_sourceImageTexelSizeInputIndex, AZ::Vector2(1.0f / sourceWidth, 1.0f / sourceHeight));

            m_shaderResourceGroup->SetConstant(m_sourceMipLevelInputIndex, sourceMip);
            m_shaderResourceGroup->SetConstant(m_enableBicubicInputIndex, enableBicubic);
            m_shaderResourceGroup->SetConstant(m_tintInputIndex, tint);

            m_targetImageWidth = targetImageWidth;
            m_targetImageHeight = targetImageHeight;
        }

        void BloomCompositeChildPass::FrameBeginInternal(FramePrepareParams params)
        {
            m_shaderResourceGroup->SetConstant(m_targetImageSizeInputIndex, AZ::Vector2(static_cast<float>(m_targetImageWidth), static_cast<float>(m_targetImageHeight)));

            SetTargetThreadCounts(m_targetImageWidth, m_targetImageHeight, 1);

            ComputePass::FrameBeginInternal(params);
        }

    }   // namespace RPI
}   // namespace AZ
