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

#include <Atom/RPI.Public/Image/ImageSystemInterface.h>

namespace AZ::Render
{
    
    RPI::Ptr<TaaPass> TaaPass::Create(const RPI::PassDescriptor& descriptor)
    {
        RPI::Ptr<TaaPass> pass = aznew TaaPass(descriptor);
        return pass;
    }
    
    TaaPass::TaaPass(const RPI::PassDescriptor& descriptor)
        : RPI::ComputePass(descriptor)
    {
    }

    void TaaPass::CompileResources(const RHI::FrameGraphCompileContext& /*context*/)
    {
        // Set parameters on SRG here
        
        m_shaderResourceGroup->Compile();
    }
    
    void TaaPass::BuildCommandListInternal(const RHI::FrameGraphExecuteContext& /*context*/)
    {

    }

    void TaaPass::FrameBeginInternal(FramePrepareParams /*params*/)
    {
        m_lastFrameAccumulationBinding->SetAttachment(m_accumulationAttachments[m_accumulationOuptutIndex]);

        m_accumulationOuptutIndex ^= 1; // swap which attachment is the output and last frame

        UpdateAttachmentImage(m_accumulationAttachments[m_accumulationOuptutIndex]);
        m_outputColorBinding->SetAttachment(m_accumulationAttachments[m_accumulationOuptutIndex]);
    }
    
    void TaaPass::ResetInternal()
    {
        m_accumulationAttachments[0].reset();
        m_accumulationAttachments[1].reset();

        m_lastFrameAccumulationBinding = nullptr;
        m_outputColorBinding = nullptr;
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

        m_lastFrameAccumulationBinding = FindAttachmentBinding(Name("LastFrameAccumulation"));
        AZ_Error("TaaPass", m_lastFrameAccumulationBinding, "TaaPass requires a slot for LastFrameAccumulation.");
        m_outputColorBinding = FindAttachmentBinding(Name("OutputColor"));
        AZ_Error("TaaPass", m_lastFrameAccumulationBinding, "TaaPass requires a slot for OutputColor.");
    }
    
    void TaaPass::UpdateAttachmentImage(RPI::Ptr<RPI::PassAttachment>& attachment)
    {
        if (!attachment)
        {
            return;
        }

        // add field to attachment for manual update
        // update the image attachment descriptor to sync up size and format
        attachment->Update(true);
        attachment->m_lifetime = RHI::AttachmentLifetimeType::Imported;
        
        // set the bind flags
        RHI::ImageDescriptor& imageDesc = attachment->m_descriptor.m_image;
        imageDesc.m_bindFlags |= RHI::ImageBindFlags::Color | RHI::ImageBindFlags::ShaderReadWrite;
        
        Data::Instance<RPI::AttachmentImagePool> pool = RPI::ImageSystemInterface::Get()->GetSystemAttachmentPool();
        auto attachmentImage = RPI::AttachmentImage::Create(*pool.get(), imageDesc, Name(attachment->m_path.GetCStr()));
        
        attachment->m_path = attachmentImage->GetAttachmentId();
        attachment->m_importedResource = attachmentImage;
    }

} // namespace AZ::Render
