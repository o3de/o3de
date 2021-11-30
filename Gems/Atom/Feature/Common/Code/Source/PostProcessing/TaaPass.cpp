/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <PostProcessing/TaaPass.h>

#include <AzCore/Math/Random.h>
#include <Atom/RPI.Public/Image/AttachmentImagePool.h>
#include <Atom/RPI.Public/Image/ImageSystemInterface.h>
#include <Atom/RPI.Public/Pass/PassUtils.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/View.h>
#include <Atom/RPI.Reflect/Pass/PassName.h>

namespace AZ::Render
{
    
    RPI::Ptr<TaaPass> TaaPass::Create(const RPI::PassDescriptor& descriptor)
    {
        RPI::Ptr<TaaPass> pass = aznew TaaPass(descriptor);
        return pass;
    }
    
    TaaPass::TaaPass(const RPI::PassDescriptor& descriptor)
        : Base(descriptor)
    {
        uint32_t numJitterPositions = 8;

        const TaaPassData* taaPassData = RPI::PassUtils::GetPassData<TaaPassData>(descriptor);
        if (taaPassData)
        {
            numJitterPositions = taaPassData->m_numJitterPositions;
        }

        // The coprimes 2, 3 are commonly used for halton sequences because they have an even distribution even for
        // few samples. With larger primes you need to offset by some amount between each prime to have the same
        // effect. We could allow this to be configurable in the future.
        SetupSubPixelOffsets(2, 3, numJitterPositions);
    }

    void TaaPass::CompileResources(const RHI::FrameGraphCompileContext& context)
    {
        struct TaaConstants
        {
            AZStd::array<uint32_t, 2> m_size = { 1, 1 };
            AZStd::array<float, 2> m_rcpSize = { 0.0, 0.0 };
            
            AZStd::array<float, 4> m_weights1 = { 0.0 };
            AZStd::array<float, 4> m_weights2 = { 0.0 };
            AZStd::array<float, 4> m_weights3 = { 0.0 };
        };

        TaaConstants cb;
        RHI::Size inputSize = m_lastFrameAccumulationBinding->m_attachment->m_descriptor.m_image.m_size;
        cb.m_size[0] = inputSize.m_width;
        cb.m_size[1] = inputSize.m_height;
        cb.m_rcpSize[0] = 1.0f / inputSize.m_width;
        cb.m_rcpSize[1] = 1.0f / inputSize.m_height;
        
        Offset jitterOffset = m_subPixelOffsets.at(m_offsetIndex);
        GenerateFilterWeights(Vector2(jitterOffset.m_xOffset, jitterOffset.m_yOffset));
        cb.m_weights1 = { m_filterWeights[0], m_filterWeights[1], m_filterWeights[2], m_filterWeights[3] };
        cb.m_weights2 = { m_filterWeights[4], m_filterWeights[5], m_filterWeights[6], m_filterWeights[7] };
        cb.m_weights3 = { m_filterWeights[8], 0.0f, 0.0f, 0.0f };

        m_shaderResourceGroup->SetConstant(m_constantDataIndex, cb);


        Base::CompileResources(context);
    }
    
    void TaaPass::FrameBeginInternal(FramePrepareParams params)
    {
        RHI::Size inputSize = m_inputColorBinding->m_attachment->m_descriptor.m_image.m_size;
        Vector2 rcpInputSize = Vector2(1.0f / inputSize.m_width, 1.0f / inputSize.m_height);

        RPI::ViewPtr view = GetRenderPipeline()->GetDefaultView();
        m_offsetIndex = (m_offsetIndex + 1) % m_subPixelOffsets.size();
        Offset offset = m_subPixelOffsets.at(m_offsetIndex);
        view->SetClipSpaceOffset(offset.m_xOffset * rcpInputSize.GetX(), offset.m_yOffset * rcpInputSize.GetY());

        m_lastFrameAccumulationBinding->SetAttachment(m_accumulationAttachments[m_accumulationOuptutIndex]);
        m_accumulationOuptutIndex ^= 1; // swap which attachment is the output and last frame

        UpdateAttachmentImage(m_accumulationAttachments[m_accumulationOuptutIndex]);
        m_outputColorBinding->SetAttachment(m_accumulationAttachments[m_accumulationOuptutIndex]);
        
        Base::FrameBeginInternal(params);
    }
    
    void TaaPass::ResetInternal()
    {
        m_accumulationAttachments[0].reset();
        m_accumulationAttachments[1].reset();

        m_inputColorBinding = nullptr;
        m_lastFrameAccumulationBinding = nullptr;
        m_outputColorBinding = nullptr;

        Base::ResetInternal();
    }

    void TaaPass::BuildInternal()
    {
        m_accumulationAttachments[0] = FindAttachment(Name("Accumulation1"));
        m_accumulationAttachments[1] = FindAttachment(Name("Accumulation2"));

        bool hasAttachments = m_accumulationAttachments[0] || m_accumulationAttachments[1];
        AZ_Error("TaaPass", hasAttachments, "TaaPass must have Accumulation1 and Accumulation2 ImageAttachments defined.");

        if (hasAttachments)
        {
            // Make sure the attachments have images when the pass first loads.
            for (auto i : { 0, 1 })
            {
                if (!m_accumulationAttachments[i]->m_importedResource)
                {
                    UpdateAttachmentImage(m_accumulationAttachments[i]);
                }
            }
        }
        
        m_inputColorBinding = FindAttachmentBinding(Name("InputColor"));
        AZ_Error("TaaPass", m_inputColorBinding, "TaaPass requires a slot for InputColor.");
        m_lastFrameAccumulationBinding = FindAttachmentBinding(Name("LastFrameAccumulation"));
        AZ_Error("TaaPass", m_lastFrameAccumulationBinding, "TaaPass requires a slot for LastFrameAccumulation.");
        m_outputColorBinding = FindAttachmentBinding(Name("OutputColor"));
        AZ_Error("TaaPass", m_outputColorBinding, "TaaPass requires a slot for OutputColor.");

        // Set up the attachment for last frame accumulation and output color if it's never been done to
        // ensure SRG indices are set up correctly by the pass system.
        if (m_lastFrameAccumulationBinding->m_attachment == nullptr)
        {
            m_lastFrameAccumulationBinding->SetAttachment(m_accumulationAttachments[0]);
            m_outputColorBinding->SetAttachment(m_accumulationAttachments[1]);
        }

        Base::BuildInternal();
    }

    void TaaPass::UpdateAttachmentImage(RPI::Ptr<RPI::PassAttachment>& attachment)
    {
        if (!attachment)
        {
            return;
        }

        // update the image attachment descriptor to sync up size and format
        attachment->Update(true);
        RHI::ImageDescriptor& imageDesc = attachment->m_descriptor.m_image;
        RPI::AttachmentImage* currentImage = azrtti_cast<RPI::AttachmentImage*>(attachment->m_importedResource.get());

        if (attachment->m_importedResource && imageDesc.m_size == currentImage->GetDescriptor().m_size)
        {
            // If there's a resource already and the size didn't change, just keep using the old AttachmentImage.
            return;
        }
        
        Data::Instance<RPI::AttachmentImagePool> pool = RPI::ImageSystemInterface::Get()->GetSystemAttachmentPool();

        // set the bind flags
        imageDesc.m_bindFlags |= RHI::ImageBindFlags::Color | RHI::ImageBindFlags::ShaderReadWrite;
        
        // The ImageViewDescriptor must be specified to make sure the frame graph compiler doesn't treat this as a transient image.
        RHI::ImageViewDescriptor viewDesc = RHI::ImageViewDescriptor::Create(imageDesc.m_format, 0, 0);
        viewDesc.m_aspectFlags = RHI::ImageAspectFlags::Color;

        // The full path name is needed for the attachment image so it's not deduplicated from accumulation images in different pipelines.
        AZStd::string imageName = RPI::ConcatPassString(GetPathName(), attachment->m_path);
        auto attachmentImage = RPI::AttachmentImage::Create(*pool.get(), imageDesc, Name(imageName), nullptr, &viewDesc);

        if (attachmentImage)
        {
            attachment->m_path = attachmentImage->GetAttachmentId();
            attachment->m_importedResource = attachmentImage;
        }
        else
        {
            AZ_Error("TaaPass", false, "TaaPass disabled because it is unable to create an attachment image.")
            this->SetEnabled(false);
        }
    }

    void TaaPass::SetupSubPixelOffsets(uint32_t haltonX, uint32_t haltonY, uint32_t length)
    {
        m_subPixelOffsets.resize(length);
        HaltonSequence<2> sequence = HaltonSequence<2>({haltonX, haltonY});
        sequence.FillHaltonSequence(m_subPixelOffsets.begin(), m_subPixelOffsets.end());

        // Adjust to the -1.0 to 1.0 range. This is done because the view needs offsets in clip
        // space and is one less calculation that would need to be done in FrameBeginInternal()
        AZStd::for_each(m_subPixelOffsets.begin(), m_subPixelOffsets.end(),
            [](Offset& offset)
            {
                offset.m_xOffset = 2.0f * offset.m_xOffset - 1.0f;
                offset.m_yOffset = 2.0f * offset.m_yOffset - 1.0f;
            }
        );
    }

    // Approximation of a Blackman Harris window function of width 3.3.
    // https://en.wikipedia.org/wiki/Window_function#Blackman%E2%80%93Harris_window
    static float BlackmanHarris(AZ::Vector2 uv)
    {
        return expf(-2.29f * (uv.GetX() * uv.GetX() + uv.GetY() * uv.GetY()));
    }
    
    // Generates filter weights for the 3x3 neighborhood of a pixel. Since jitter positions are the
    // same for every pixel we can calculate this once here and upload to the SRG.
    // Jitter weights are based on a window function centered at the pixel center (we use Blackman-Harris).
    // As the jitter position moves around, some neighborhood locations decrease in weight, and others
    // increase in weight based on their distance from the center of the pixel.
    void TaaPass::GenerateFilterWeights(AZ::Vector2 jitterOffset)
    {
        static const AZStd::array<Vector2, 9> pixelOffsets =
        {
            // Center
            Vector2(0.0f, 0.0f),
            // Cross
            Vector2( 1.0f,  0.0f),
            Vector2( 0.0f,  1.0f),
            Vector2(-1.0f,  0.0f),
            Vector2( 0.0f, -1.0f),
            // Diagonals
            Vector2( 1.0f,  1.0f),
            Vector2( 1.0f, -1.0f),
            Vector2(-1.0f,  1.0f),
            Vector2(-1.0f, -1.0f),
        };

        float sum = 0.0f;
        for (uint32_t i = 0; i < 9; ++i)
        {
            m_filterWeights[i] = BlackmanHarris(pixelOffsets[i] + jitterOffset);
            sum += m_filterWeights[i];
        }

        // Normalize the weight so the sum of all weights is 1.0.
        float normalization = 1.0f / sum;
        for (uint32_t i = 0; i < 9; ++i)
        {
            m_filterWeights[i] *= normalization;
        }
    }

} // namespace AZ::Render
