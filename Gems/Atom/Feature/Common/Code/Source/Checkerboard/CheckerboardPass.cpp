/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Public/Image/AttachmentImagePool.h>
#include <Atom/RPI.Public/Image/ImageSystemInterface.h>

#include <Checkerboard/CheckerboardPass.h>

namespace AZ
{
    namespace Render
    {
        RPI::Ptr<Render::CheckerboardPass> CheckerboardPass::Create(const RPI::PassDescriptor& descriptor)
        {
            return aznew CheckerboardPass(descriptor);
        }

        CheckerboardPass::CheckerboardPass(const RPI::PassDescriptor& descriptor)
            : Base(descriptor)
        {
            // CheckerboardPass defines its own viewport and scissor
            m_overrideViewportState = true;
            m_overrideScissorSate = true;
        }

        void CheckerboardPass::FrameBeginInternal(FramePrepareParams params)
        {
            float width = (params.m_viewportState.m_maxX - params.m_viewportState.m_minX) * 0.5f;
            float height = (params.m_viewportState.m_maxY - params.m_viewportState.m_minY) * 0.5f;
            
            m_viewportState = params.m_viewportState;
            m_viewportState.m_minX += 0.5f * m_frameOffset;

            m_viewportState.m_maxX = m_viewportState.m_minX + width;
            m_viewportState.m_maxY = m_viewportState.m_minY + height;

            m_scissorState = params.m_scissorState;
            m_scissorState.m_maxX = m_scissorState.m_minX + (m_scissorState.m_maxX - m_scissorState.m_minX) / 2;
            m_scissorState.m_maxY = m_scissorState.m_minY + (m_scissorState.m_maxY - m_scissorState.m_minY) / 2;
            
            Base::FrameBeginInternal(params);
        }

        void CheckerboardPass::BuildInternal()
        {
            Data::Instance<RPI::AttachmentImagePool> pool = RPI::ImageSystemInterface::Get()->GetSystemAttachmentPool();

            // Create persistent attachment images to replace transient pass attachments
            // It's better to check if it connects to output slots too
            for (RPI::Ptr<RPI::PassAttachment>& attachment : m_ownedAttachments)
            {
                if (attachment->m_lifetime == RHI::AttachmentLifetimeType::Transient)
                {
                    if (attachment->m_descriptor.m_type == RHI::AttachmentType::Image)
                    {
                        // Force update image attachment descriptor to sync up size and format
                        attachment->Update();

                        attachment->m_lifetime = RHI::AttachmentLifetimeType::Imported;

                        // update image descriptor to new size and samples
                        RHI::ImageDescriptor &imageDesc = attachment->m_descriptor.m_image;

                        imageDesc.m_multisampleState.m_samples = 2;
                        imageDesc.m_size.m_width /= 2;
                        imageDesc.m_size.m_height /= 2;

                        // using hard coded clear value here until we can get the information from pass templated data
                        // [GFX TODO][ATOM-5406] We need a clear value for PassAttachmentDesc and PassAttachment
                        RHI::ClearValue clearValue = RHI::ClearValue::CreateVector4Float(0, 0, 0, 0);
                        
                        if (imageDesc.m_format == RHI::Format::D32_FLOAT_S8X24_UINT) //hacky condition check
                        {
                            imageDesc.m_bindFlags |= RHI::ImageBindFlags::DepthStencil;
                        }
                        else
                        {
                            imageDesc.m_bindFlags |= RHI::ImageBindFlags::Color;
                        }
                        
                        m_imageAttachments[attachment->m_name][0] = RPI::AttachmentImage::Create(*pool.get(), imageDesc,
                            Name(AZStd::string::format("%s_0", attachment->m_path.GetCStr())), &clearValue, nullptr);
                        m_imageAttachments[attachment->m_name][1] = RPI::AttachmentImage::Create(*pool.get(), imageDesc,
                            Name(AZStd::string::format("%s_1", attachment->m_path.GetCStr())), &clearValue, nullptr);

                        attachment->m_path = m_imageAttachments[attachment->m_name][0]->GetAttachmentId();
                        attachment->m_importedResource = m_imageAttachments[attachment->m_name][0];
                    }
                }
            }

            // reset frame offset to 0 since attachments are rebuilt
            m_frameOffset = 0;

            Base::BuildInternal();
        }


        Data::Instance<RPI::AttachmentImage> CheckerboardPass::GetAttachmentImage(Name attachmentName, uint8_t frameOffset)
        {
            if (m_imageAttachments.find(attachmentName) != m_imageAttachments.end())
            {
                return m_imageAttachments[attachmentName][frameOffset];
            }
            return nullptr;
        }

        void CheckerboardPass::FrameEndInternal()
        {
            m_frameOffset = 1 - m_frameOffset;
            // For next frame, we switch the internal attachment image for each owned pass attachments
            // So we can preserve the render target from current frame
            for (RPI::Ptr<RPI::PassAttachment>& attachment : m_ownedAttachments)
            {
                if (attachment->m_descriptor.m_type == RHI::AttachmentType::Image)
                {
                    Data::Instance<RPI::AttachmentImage> nextAttachment = GetAttachmentImage(attachment->m_name, m_frameOffset);
                    if (nextAttachment)
                    {
                        // update the attachment id and resource
                        attachment->m_path = nextAttachment->GetAttachmentId();
                        attachment->m_importedResource = nextAttachment;

                        // the attachment id saved in bindings need to be updated too. so the frame attachment will be attached properly.
                        for (auto& binding : m_attachmentBindings)
                        {
                            if (binding.m_attachment == attachment)
                            {
                                binding.m_unifiedScopeDesc.m_attachmentId = nextAttachment->GetAttachmentId();
                                break;
                            }
                        }
                    }
                }
            }
            Base::FrameEndInternal();
        }

      } // namespace Render
} // namespace AZ
