/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <PostProcessing/BloomBlurPass.h>
#include <PostProcess/Bloom/BloomSettings.h>
#include <PostProcess/PostProcessFeatureProcessor.h>

#include <Atom/RHI/CommandList.h>
#include <Atom/RHI/Factory.h>
#include <Atom/RHI/FrameScheduler.h>

#include <Atom/RPI.Reflect/Pass/PassTemplate.h>

#include <Atom/RPI.Public/Pass/PassUtils.h>
#include <Atom/RPI.Public/Pass/PassAttachment.h>
#include <Atom/RPI.Public/Pass/PassDefines.h>
#include <Atom/RPI.Public/Pass/PassFactory.h>
#include <Atom/RPI.Public/Pass/PassSystemInterface.h>
#include <Atom/RPI.Public/RPIUtils.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/View.h>
#include <Atom/RPI.Public/RPISystemInterface.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>

#include <Atom/RPI.Public/Image/AttachmentImagePool.h>
#include <Atom/RPI.Public/Image/ImageSystemInterface.h>

#include <Atom/RPI.Reflect/Pass/ComputePassData.h>

#include <sstream>


namespace AZ
{
    namespace Render
    {
        namespace BloomBlurPassConstants
        {
            // Maximum smoothing kernel size is 257 x 257
            constexpr float BlurFilterMaxRadius = 128;

            // Minimum smoothing kernel size is 1 x 1
            constexpr float BlurFilterMinRadius = 0;
        }

        RPI::Ptr<BloomBlurPass> BloomBlurPass::Create(const RPI::PassDescriptor& descriptor)
        {
            RPI::Ptr<BloomBlurPass> pass = aznew BloomBlurPass(descriptor);
            return AZStd::move(pass);
        }

        BloomBlurPass::BloomBlurPass(const RPI::PassDescriptor& descriptor)
            : ParentPass(descriptor)
        {
            // Load DownsampleMipChainPassData (shader asset)
            const RPI::DownsampleMipChainPassData* passData = RPI::PassUtils::GetPassData<RPI::DownsampleMipChainPassData>(descriptor);
            if (passData == nullptr)
            {
                AZ_Error("PassSystem", false, "[BloomBlurPass '%s']: Trying to construct without valid DownsampleMipChainPassData!",
                    GetPathName().GetCStr());
                return;
            }

            m_passData = *passData;

            m_weightBuffer.resize(Render::Bloom::MaxStageCount);
            m_offsetBuffer.resize(Render::Bloom::MaxStageCount);
        }

        void BloomBlurPass::GetInputInfo()
        {
            AZ_Assert(GetInputOutputCount() > 0, "[BloomBlurPass '%s']: must have an input/output", GetPathName().GetCStr());
            RPI::PassAttachment* attachment = GetInputOutputBinding(0).GetAttachment().get();

            if (attachment != nullptr)
            {
                m_paramsUpdated |= (m_inputWidth != attachment->m_descriptor.m_image.m_size.m_width);
                m_paramsUpdated |= (m_inputHeight != attachment->m_descriptor.m_image.m_size.m_height);

                m_inputWidth = attachment->m_descriptor.m_image.m_size.m_width;
                m_inputHeight = attachment->m_descriptor.m_image.m_size.m_height;
            }
            else
            {
                AZ_Assert(GetInputOutputCount() > 0, "[BloomBlurPass '%s']: input/output image attachment not found", GetPathName().GetCStr());
            }
        }

        void BloomBlurPass::UpdateParameters()
        {
            auto UpdateIfChanged = [](float& local, float input)->bool
            {
                if (local != input)
                {
                    local = input;
                    return true;
                }
                else
                {
                    return false;
                }
            };

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
                        m_paramsUpdated |= UpdateIfChanged(m_kernelSizeScale,
                            bloomSettings->GetKernelSizeScale());
                        m_paramsUpdated |= UpdateIfChanged(m_kernelScreenPercents[0],
                            bloomSettings->GetKernelSizeStage0());
                        m_paramsUpdated |= UpdateIfChanged(m_kernelScreenPercents[1],
                            bloomSettings->GetKernelSizeStage1());
                        m_paramsUpdated |= UpdateIfChanged(m_kernelScreenPercents[2],
                            bloomSettings->GetKernelSizeStage2());
                        m_paramsUpdated |= UpdateIfChanged(m_kernelScreenPercents[3],
                            bloomSettings->GetKernelSizeStage3());
                        m_paramsUpdated |= UpdateIfChanged(m_kernelScreenPercents[4],
                            bloomSettings->GetKernelSizeStage4());
                    }
                }
            }
        }

        void BloomBlurPass::CreateBinding(BloomBlurChildPass* pass, uint32_t mipLevel, bool isHorizontalPass)
        {
            RPI::PassAttachmentBinding& parentInOutBinding = GetInputOutputBinding(0);
            const RPI::Ptr<RPI::PassAttachment>& parentInOutAttachment = parentInOutBinding.GetAttachment();

            RPI::PassAttachmentBinding& parentInBinding = GetInputBinding(0);
            const RPI::Ptr<RPI::PassAttachment>& parentWorkSpaceAttachment = parentInBinding.GetAttachment();

            // Create input binding, from downsampling pass
            RPI::PassAttachmentBinding inBinding;
            inBinding.m_name = "Input";
            inBinding.m_shaderInputName = "m_inputTexture";
            inBinding.m_slotType = RPI::PassSlotType::Input;
            inBinding.m_scopeAttachmentUsage = RHI::ScopeAttachmentUsage::Shader;
            inBinding.m_connectedBinding = isHorizontalPass ? &parentInOutBinding : &parentInBinding;

            RHI::ImageViewDescriptor viewDesc;
            viewDesc.m_mipSliceMin = static_cast<uint16_t>(mipLevel);
            viewDesc.m_mipSliceMax = static_cast<uint16_t>(mipLevel);
            inBinding.m_unifiedScopeDesc.SetAsImage(viewDesc);
            inBinding.SetAttachment(parentInOutAttachment);

            pass->AddAttachmentBinding(inBinding);

            // Create output binding, owned by current pass
            RPI::PassAttachmentBinding outBinding;
            outBinding.m_name = "Output";
            outBinding.m_shaderInputName = "m_outputTexture";
            outBinding.m_slotType = RPI::PassSlotType::Output;
            outBinding.m_scopeAttachmentUsage = RHI::ScopeAttachmentUsage::Shader;
            outBinding.m_connectedBinding = isHorizontalPass ? &parentInBinding : &parentInOutBinding;

            // Output to the same mip level as input downsampled texture
            outBinding.m_unifiedScopeDesc.SetAsImage(viewDesc);
            outBinding.SetAttachment(parentWorkSpaceAttachment);

            pass->AddAttachmentBinding(outBinding);
        }

        void BloomBlurPass::BuildChildPasses()
        {
            if (!m_children.empty())
            {
                // In this case children are still exists but attachment binding is flushed out, so we rebind them again
                uint32_t stageCount = Render::Bloom::MaxStageCount;
                for (uint32_t childIndex = 0; childIndex < m_children.size(); ++childIndex)
                {
                    uint32_t stageIndex = childIndex % stageCount;
                    bool isHorizontalPass = childIndex < stageCount;

                    BloomBlurChildPass* blurChild = static_cast<BloomBlurChildPass*>(m_children[childIndex].get());
                    CreateBinding(blurChild, stageIndex, isHorizontalPass);
                }
            }
            else
            {
                // Create children
                RPI::PassSystemInterface* passSystem = RPI::PassSystemInterface::Get();

                uint32_t stageCount = Render::Bloom::MaxStageCount;
                for (uint32_t childIndex = 0; childIndex < stageCount * 2; ++childIndex)
                {
                    uint32_t stageIndex = childIndex % stageCount;
                    bool isHorizontalPass = childIndex < stageCount;

                    RPI::PassDescriptor childDesc;
                    childDesc.m_passData = AZStd::make_shared<RPI::ComputePassData>();
                    RPI::ComputePassData* passData = static_cast<RPI::ComputePassData*>(childDesc.m_passData.get());
                    passData->m_shaderReference = m_passData.m_shaderReference;

                    if (isHorizontalPass)
                    {
                        childDesc.m_passName = Name{ AZStd::string::format("BloomBlurHorizontal%d", stageIndex) };
                    }
                    else
                    {
                        childDesc.m_passName = Name{ AZStd::string::format("BloomBlurVertical%d", stageIndex) };
                    }
                    RPI::Ptr<BloomBlurChildPass> childPass = passSystem->CreatePass<BloomBlurChildPass>(childDesc);

                    CreateBinding(childPass.get(), stageIndex, isHorizontalPass);
                    AddChild(childPass);
                }
            }
        }

        void BloomBlurPass::UpdateChildren()
        {
            uint32_t imageWidth, imageHeight;

            imageWidth = m_inputWidth;
            imageHeight = m_inputHeight;

            uint32_t stageCount = Render::Bloom::MaxStageCount;
            for (uint32_t childIdxH = 0, childIdxV = stageCount; childIdxH < stageCount; ++childIdxH, ++childIdxV)
            {
                // Horizontal
                BloomBlurChildPass* blurChild = static_cast<BloomBlurChildPass*>(m_children[childIdxH].get());
                blurChild->UpdateParameters(
                    m_offsetBuffer[childIdxH], m_weightBuffer[childIdxH], m_kernelRadiusData[childIdxH],
                    true, childIdxH, imageWidth, imageHeight);

                // Vertical
                blurChild = static_cast<BloomBlurChildPass*>(m_children[childIdxV].get());
                blurChild->UpdateParameters(
                    m_offsetBuffer[childIdxH], m_weightBuffer[childIdxH], m_kernelRadiusData[childIdxH],
                    false, childIdxH, imageWidth, imageHeight);

                imageWidth = imageWidth / 2;
                imageHeight = imageHeight / 2;
            }
        }

        void BloomBlurPass::BuildInternal()
        {
            BuildChildPasses();
            ParentPass::BuildInternal();
        }

        void BloomBlurPass::FrameBeginInternal(FramePrepareParams params)
        {
            GetInputInfo();
            UpdateParameters();

            if (m_paramsUpdated)
            {
                BuildKernelData();
                m_paramsUpdated = false;
            }
            UpdateChildren();

            ParentPass::FrameBeginInternal(params);
        }

        void BloomBlurPass::BuildKernelData()
        {
            m_weightData.clear();
            m_offsetData.clear();
            m_kernelRadiusData.clear();

            RPI::PassAttachment* inOutAttachment = GetInputOutputBinding(0).GetAttachment().get();
            uint32_t imageWidth = inOutAttachment->m_descriptor.m_image.m_size.m_width;

            // Horizontal & vertical pass shared the same kernel
            for (uint32_t i = 0; i < Render::Bloom::MaxStageCount; ++i)
            {
                // (Input screen width) * (Downscale factor based on mip level) * (ratio of kernel size and screen width) *
                //    0.5 to convert from diameter to radius (exclude center pixel)
                // Result is limited to a upper/lower bound to avoid extreme cases
                float radius = KernelRadiusClamp(imageWidth * (1.0f / static_cast<float>(exp2(i))) * AZStd::min(1.0f, m_kernelSizeScale * m_kernelScreenPercents[i]) * 0.5f);

                uint32_t kernelIntegerRadius = static_cast<uint32_t>(floor(radius + 0.5));

                // Define the width of kernel as 6*sigma (3 sigma each side) to achieve 99.7% confidence interval
                float sigma = radius / 3.0f;

                if (kernelIntegerRadius > 0)
                {
                    GenerateWeightOffset(sigma, kernelIntegerRadius);
                    PrepareBuffer(i);
                }
                else
                {
                    // If kernel radius is 0 skip kernel calculation and buffer preparation
                    m_weightData.emplace_back();
                    m_offsetData.emplace_back();
                    m_kernelRadiusData.push_back(0);
                }
            }
        }

        float BloomBlurPass::KernelRadiusClamp(float radius)
        {
            return fmaxf(fminf(radius, BloomBlurPassConstants::BlurFilterMaxRadius), BloomBlurPassConstants::BlurFilterMinRadius);
        }

        float BloomBlurPass::Gaussian1D(float x, float sigma)
        {
            return (1.0f / (sqrt(Constants::TwoPi) * sigma)) * exp(-(x * x) / (2.0f * sigma * sigma));
        }

        void BloomBlurPass::GenerateWeightOffset(float sigma, uint32_t kernelRadius)
        {
            float weightSum = 0.0;
            float weight = 0.0;

            // Gaussian kernel is radially symmetric, so we only store one wing of the 1d kernel
            AZStd::vector<float> weights;
            AZStd::vector<float> offsets;
            
            // Center pixel
            weight = Gaussian1D(0, sigma);
            weights.push_back(weight);
            offsets.push_back(0);
            weightSum += weight;
            for (uint32_t i = 1; i <= kernelRadius; i += 2)
            {
                float weight0 = Gaussian1D(static_cast<float>(i), sigma);
                float weight1 = 0.0;

                if (i != kernelRadius)
                {
                    weight1 = Gaussian1D(static_cast<float>(i + 1), sigma);
                }

                weight = weight0 + weight1;
                weights.push_back(weight);

                //    (i * weight0 + (i + 1) * weight1) / (weight0 + weight1)
                // => (i * (weight0 + weight1) + weight1) / (weight0 + weight1)
                // => i + weight1 / (weight0 + weight1)
                offsets.push_back(i + weight1 / weight);

                // Two symmetric weight on each side
                weightSum += weight * 2;
            }

            // Renormalize so the kenrel weight sum to 1
            float weightSumRcp = 1.0f / weightSum;
            for (uint32_t i = 0; i < weights.size(); ++i)
            {
                weights[i] *= weightSumRcp;
            }

            m_weightData.push_back(weights);
            m_offsetData.push_back(offsets);

            // Record reduced kernel's radius
            m_kernelRadiusData.push_back(static_cast<uint32_t>(weights.size()));
        }

        void BloomBlurPass::PrepareBuffer(uint32_t blurStageIndex)
        {
            uint32_t byteCount = sizeof(float) * static_cast<uint32_t>(m_weightData[blurStageIndex].size());

            // Prepare buffer, these two buffers shared the same size and layout so can be allocated together
            if (!(m_weightBuffer[blurStageIndex] || m_offsetBuffer[blurStageIndex]))
            {
                std::stringstream ss;
                ss << GetPathName().GetCStr() << ".WeightBuffer.Stage" << blurStageIndex;

                RPI::CommonBufferDescriptor desc;
                desc.m_poolType = RPI::CommonBufferPoolType::ReadOnly;
                desc.m_bufferName = ss.str().c_str();
                desc.m_elementSize = sizeof(float);
                desc.m_elementFormat = RHI::Format::R32_FLOAT;
                desc.m_byteCount = byteCount;
                desc.m_bufferData = static_cast<void*>(m_weightData[blurStageIndex].data());
                m_weightBuffer[blurStageIndex] = RPI::BufferSystemInterface::Get()->CreateBufferFromCommonPool(desc);

                ss.clear();
                ss << GetPathName().GetCStr() << "OffsetBuffer.Stage" << blurStageIndex;
                desc.m_bufferName = ss.str().c_str();
                desc.m_bufferData = static_cast<void*>(m_offsetData[blurStageIndex].data());
                m_offsetBuffer[blurStageIndex] = RPI::BufferSystemInterface::Get()->CreateBufferFromCommonPool(desc);
            }
            else
            {
                // Update buffer data and resize if necessary, weight and offset buffer always has same size
                // therefore no need to check twice
                if (byteCount != m_weightBuffer[blurStageIndex]->GetBufferSize())
                {
                    m_weightBuffer[blurStageIndex]->Resize(byteCount);
                    m_offsetBuffer[blurStageIndex]->Resize(byteCount);
                }
                m_weightBuffer[blurStageIndex]->UpdateData(m_weightData[blurStageIndex].data(), byteCount);
                m_offsetBuffer[blurStageIndex]->UpdateData(m_offsetData[blurStageIndex].data(), byteCount);
            }
        }

        // ============ Child pass's member function ============

        RPI::Ptr<BloomBlurChildPass> BloomBlurChildPass::Create(const RPI::PassDescriptor& descriptor)
        {
            RPI::Ptr<BloomBlurChildPass> pass = aznew BloomBlurChildPass(descriptor);
            return AZStd::move(pass);
        }

        BloomBlurChildPass::BloomBlurChildPass(const RPI::PassDescriptor& descriptor)
            : ComputePass(descriptor)
        { }

        void BloomBlurChildPass::UpdateParameters(
            Data::Instance<RPI::Buffer> offsetBuffer,
            Data::Instance<RPI::Buffer> weightBuffer,
            uint32_t radius,
            bool direction,
            uint32_t mipLevel,
            uint32_t imageWidth,
            uint32_t imageHeight)
        {
            // These quantities are stored locally because they need to be passed every frame
            // but the function is only invoked when parameters are updated
            m_offsetBuffer = offsetBuffer;
            m_weightBuffer = weightBuffer;

            m_sourceImageWidth = imageWidth;
            m_sourceImageHeight = imageHeight;

            m_shaderResourceGroup->SetConstant(m_kernelRadiusInputIndex, radius);
            m_shaderResourceGroup->SetConstant(m_directionInputIndex, direction);
            m_shaderResourceGroup->SetConstant(m_mipLevelInputIndex, mipLevel);

            float width = static_cast<float>(imageWidth);
            float height = static_cast<float>(imageHeight);
            m_shaderResourceGroup->SetConstant(m_sourceImageSizeInputIndex, AZ::Vector2(width, height));
            m_shaderResourceGroup->SetConstant(m_sourceImageTexelSizeInputIndex, AZ::Vector2(1.0f / width, 1.0f / height));
        }

        void BloomBlurChildPass::FrameBeginInternal(FramePrepareParams params)
        {
            if (m_offsetBuffer)
            {
                m_shaderResourceGroup->SetBufferView(
                    m_offsetsInputIndex, m_offsetBuffer->GetBufferView());
            }

            if (m_weightBuffer)
            {
                m_shaderResourceGroup->SetBufferView(
                    m_weightsInputIndex, m_weightBuffer->GetBufferView());
            }

            SetTargetThreadCounts(m_sourceImageWidth, m_sourceImageHeight, 1);

            ComputePass::FrameBeginInternal(params);
        }
    }   // namespace RPI
}   // namespace AZ
