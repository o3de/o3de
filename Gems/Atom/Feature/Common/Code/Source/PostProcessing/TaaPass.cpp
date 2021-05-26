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

#include <PostProcessing/TaaPass.h>

#include <AzCore/Math/Random.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/View.h>
#include <Atom/RPI.Public/Image/AttachmentImagePool.h>
#include <Atom/RPI.Public/Image/ImageSystemInterface.h>
#include <Atom/RPI.Public/Pass/PassUtils.h>
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
        SetupSubPixelOffsets(2, 3, numJitterPositions);
    }

    void TaaPass::CompileResources(const RHI::FrameGraphCompileContext& context)
    {
        struct TaaConstants
        {
            AZStd::array<uint32_t, 2> m_size = { 1, 1 };
            AZStd::array<float, 2> m_rcpSize = { 0.0, 0.0 };
        };

        TaaConstants cb;
        RHI::Size inputSize = m_lastFrameAccumulationBinding->m_attachment->m_descriptor.m_image.m_size;
        cb.m_size[0] = inputSize.m_width;
        cb.m_size[1] = inputSize.m_height;
        cb.m_rcpSize[0] = 1.0f / inputSize.m_width;
        cb.m_rcpSize[1] = 1.0f / inputSize.m_height;
        
        m_shaderResourceGroup->SetConstant(m_constantDataIndex, cb);

        Base::CompileResources(context);
    }
    
    void TaaPass::FrameBeginInternal(FramePrepareParams params)
    {
        RHI::Size inputSize = m_inputColorBinding->m_attachment->m_descriptor.m_image.m_size;
        Vector2 rcpInputSize = Vector2(1.0 / inputSize.m_width, 1.0 / inputSize.m_height);

        RPI::ViewPtr view = GetRenderPipeline()->GetDefaultView();
        Offset offset = m_subPixelOffsets.at(m_offsetIndex);
        view->SetClipSpaceOffset(offset.m_xOffset * rcpInputSize.GetX(), offset.m_yOffset * rcpInputSize.GetY());
        m_offsetIndex = (m_offsetIndex + 1) % m_subPixelOffsets.size();

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

    void TaaPass::BuildAttachmentsInternal()
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

        Base::BuildAttachmentsInternal();
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
        viewDesc.m_overrideBindFlags = RHI::ImageBindFlags::ShaderReadWrite;

        // The full path name is needed for the attachment image so it's not deduplicated from accumulation images in different pipelines.
        AZStd::string imageName = RPI::ConcatPassString(GetPathName(), attachment->m_path);
        auto attachmentImage = RPI::AttachmentImage::Create(*pool.get(), imageDesc, Name(imageName), nullptr, &viewDesc);
        
        attachment->m_path = attachmentImage->GetAttachmentId();
        attachment->m_importedResource = attachmentImage;
    }

    void TaaPass::SetupSubPixelOffsets(uint32_t haltonX, uint32_t haltonY, uint32_t length)
    {
        m_subPixelOffsets.resize(length);
        HaltonSequence<2> sequence = HaltonSequence<2>({haltonX, haltonY});
        sequence.FillHaltonSequence(m_subPixelOffsets.begin(), m_subPixelOffsets.end());

        // Adjust to the -1.0 to 1.0 range.
        AZStd::for_each(m_subPixelOffsets.begin(), m_subPixelOffsets.end(),
            [](Offset& offset)
            {
                offset.m_xOffset = 2.0f * offset.m_xOffset - 1.0f;
                offset.m_yOffset = 2.0f * offset.m_yOffset - 1.0f;
            }
        );
    }

} // namespace AZ::Render
