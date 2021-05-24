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

#include <Atom/RPI.Public/Image/AttachmentImagePool.h>
#include <Atom/RPI.Public/Image/ImageSystemInterface.h>
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
    }

    void TaaPass::CompileResources(const RHI::FrameGraphCompileContext& context)
    {
        // Set parameters on SRG here

        struct TaaConstants
        {
            AZStd::array<uint32_t, 2> m_size = { 1, 1 };
            AZStd::array<uint32_t, 2> m_padding = { 0, 0 };
        };

        TaaConstants cb;
        RHI::Size inputSize = m_lastFrameAccumulationBinding->m_attachment->m_descriptor.m_image.m_size;
        cb.m_size[0] = inputSize.m_width;
        cb.m_size[1] = inputSize.m_height;
        
        m_shaderResourceGroup->SetConstant(m_constantDataIndex, cb);

        Base::CompileResources(context);
    }
    
    void TaaPass::FrameBeginInternal(FramePrepareParams params)
    {
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

} // namespace AZ::Render
