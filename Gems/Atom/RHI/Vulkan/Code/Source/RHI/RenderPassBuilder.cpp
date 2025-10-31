/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI/BufferScopeAttachment.h>
#include <Atom/RHI/ImageFrameAttachment.h>
#include <Atom/RHI/ImageScopeAttachment.h>
#include <Atom/RHI/Buffer.h>
#include <Atom/RHI/ResolveScopeAttachment.h>
#include <RHI/BufferView.h>
#include <RHI/Conversion.h>
#include <RHI/RenderPassBuilder.h>
#include <RHI/Device.h>
#include <RHI/Framebuffer.h>
#include <RHI/RenderPass.h>
#include <RHI/Scope.h>

namespace AZ
{
    namespace Vulkan
    {

        bool RenderPassContext::IsValid() const
        {
            return m_framebuffer && m_renderPass;
        }

        void RenderPassContext::SetName(const AZ::Name& name)
        {
            if (m_renderPass)
            {
                m_renderPass->SetName(name);
            }

            if (m_framebuffer)
            {
                m_framebuffer->SetName(name);
            }
        }
        
        RenderPassBuilder::RenderPassBuilder(Device& device)
            : m_device(device)
        {
        }

        void RenderPassBuilder::AddScopeAttachments(Scope& scope)
        {
            if (!scope.UsesRenderpass())
            {
                return;
            }

            auto* subpassLayoutBuilder = m_layoutBuilder.AddSubpass();
            for (const RHI::ImageScopeAttachment* scopeAttachment : scope.GetImageAttachments())
            {
                const RHI::ImageScopeAttachmentDescriptor& bindingDescriptor = scopeAttachment->GetDescriptor();
                const ImageView* attachmentImageView =
                    static_cast<const ImageView*>(scopeAttachment->GetImageView()->GetDeviceImageView(scope.GetDeviceIndex()).get());
                RHI::Format imageViewFormat = attachmentImageView->GetFormat();
                auto scopeAttachmentId = scopeAttachment->GetDescriptor().m_attachmentId;

                auto subpassAttachmentLayout = aznew RenderPass::RenderAttachmentLayout;
                subpassAttachmentLayout->m_layout = GetImageAttachmentLayout(*scopeAttachment);
                m_subpassAttachmentsLayout.emplace_back(subpassAttachmentLayout);

                FramebufferInfo framebufferInfo;
                framebufferInfo.m_imageView = attachmentImageView;
                framebufferInfo.m_initialLayout = GetInitialLayout(scope, *scopeAttachment);
                framebufferInfo.m_finalLayout = GetFinalLayout(scope, *scopeAttachment);
                framebufferInfo.m_clearValue = bindingDescriptor.m_loadStoreAction.m_clearValue;
                framebufferInfo.m_lastSubpassUsage = subpassLayoutBuilder->GetSubpassIndex();

                switch (scopeAttachment->GetUsage())
                {
                case RHI::ScopeAttachmentUsage::RenderTarget:
                    m_multisampleState = scopeAttachment->GetFrameAttachment().GetImageDescriptor().m_multisampleState;
                    subpassLayoutBuilder->RenderTargetAttachment(
                        imageViewFormat,
                        scopeAttachmentId,
                        bindingDescriptor.m_loadStoreAction,
                        false /*resolve*/,
                        subpassAttachmentLayout);                
                    break;
                case RHI::ScopeAttachmentUsage::DepthStencil:
                    {
                        m_multisampleState = scopeAttachment->GetFrameAttachment().GetImageDescriptor().m_multisampleState;
                        auto findIter = m_framebufferAttachments.find(scopeAttachmentId);
                        if (findIter != m_framebufferAttachments.end() && findIter->second.m_lastSubpassUsage == framebufferInfo.m_lastSubpassUsage)
                        {
                            FramebufferInfo& currentFramebufferInfo = findIter->second;
                            const ImageView* depthImageView = currentFramebufferInfo.m_imageView;
                            // We filter the layout first to get the depth only or stencil only layout so we can combine them.
                            // It's not valid to use the depth only or stencil only layout for the renderpasses initial
                            // and final layout (when the image has a depth/stencil format), so we must filter it.
                            currentFramebufferInfo.m_initialLayout = CombineImageLayout(
                                FilterImageLayout(currentFramebufferInfo.m_initialLayout, depthImageView->GetDescriptor().m_aspectFlags),
                                FilterImageLayout(framebufferInfo.m_initialLayout, attachmentImageView->GetDescriptor().m_aspectFlags));
                            framebufferInfo.m_finalLayout = CombineImageLayout(
                                FilterImageLayout(currentFramebufferInfo.m_finalLayout, depthImageView->GetDescriptor().m_aspectFlags),
                                FilterImageLayout(framebufferInfo.m_finalLayout, attachmentImageView->GetDescriptor().m_aspectFlags));
                            // Check if the current depth/stencil image has both aspect masks
                            if (!RHI::CheckBitsAll(depthImageView->GetDescriptor().m_aspectFlags, RHI::ImageAspectFlags::DepthStencil))
                            {
                                // Handle the case with multiple ScopeAttachmentUsage::DepthStencil attachments.
                                // One for the Depth and another for the Stencil, with different access.
                                // Need to create a new ImageView that has both depth and stencil.
                                AZ_Assert(
                                    !RHI::CheckBitsAll(
                                        attachmentImageView->GetDescriptor().m_aspectFlags, RHI::ImageAspectFlags::DepthStencil),
                                    "Multiple DepthStencil attachments detected. ScopeAttachment %s in Scope %s",
                                    scopeAttachmentId.GetCStr(),
                                    scope.GetId().GetCStr());
                                RHI::ImageViewDescriptor descriptor = depthImageView->GetDescriptor();
                                descriptor.m_aspectFlags |= scopeAttachment->GetImageView()->GetDescriptor().m_aspectFlags;
                                auto fullDepthStencil = scope.GetDepthStencilFullView();
                                // Check if we need to create a new depth stencil image view or we can reuse the one saved in the scope
                                if (!fullDepthStencil || &fullDepthStencil->GetImage() != &depthImageView->GetImage() ||
                                    fullDepthStencil->GetDescriptor() != descriptor)
                                {
                                    RHI::Ptr<ImageView> fullDepthStencilPtr = ImageView::Create();
                                    fullDepthStencilPtr->Init(depthImageView->GetImage(), descriptor);
                                    scope.SetDepthStencilFullView(fullDepthStencilPtr);
                                    fullDepthStencil = fullDepthStencilPtr.get();
                                }
                                currentFramebufferInfo.m_imageView = static_cast<const ImageView*>(fullDepthStencil);
                                imageViewFormat = fullDepthStencil->GetFormat();
                            }
                        }
                        subpassLayoutBuilder->DepthStencilAttachment(
                            imageViewFormat,
                            scopeAttachmentId,
                            bindingDescriptor.m_loadStoreAction,
                            scopeAttachment->GetAccess(),
                            scopeAttachment->GetStage(),
                            subpassAttachmentLayout);
                        break;
                    }
                case RHI::ScopeAttachmentUsage::SubpassInput:
                    subpassLayoutBuilder->SubpassInputAttachment(
                        scopeAttachmentId,
                        attachmentImageView->GetAspectFlags(),
                        bindingDescriptor.m_loadStoreAction,
                        subpassAttachmentLayout);
                    break;
                case RHI::ScopeAttachmentUsage::ShadingRate:
                    subpassLayoutBuilder->ShadingRateAttachment(imageViewFormat, scopeAttachmentId, subpassAttachmentLayout);
                    break;
                case RHI::ScopeAttachmentUsage::Resolve:
                    {
                        const RHI::ResolveScopeAttachment* resolveScopeAttachment =
                            azrtti_cast<const RHI::ResolveScopeAttachment*>(scopeAttachment);
                        AZ_Assert(
                            resolveScopeAttachment,
                            "ScopeAttachment %s is not of type ResolveScopeAttachment",
                            scopeAttachmentId.GetCStr());
                        subpassLayoutBuilder->ResolveAttachment(
                            resolveScopeAttachment->GetDescriptor().m_resolveAttachmentId, scopeAttachmentId);
                        break;
                    }
                default:
                    // Image attachment is not a render attachment, so we do not add it to the m_framebufferAttachments list.
                    // Continue to the next attachment.
                    continue;
                }

                auto insertResult = m_framebufferAttachments.insert(AZStd::make_pair(scopeAttachmentId, framebufferInfo));
                // Update the final layout for the attachment
                insertResult.first->second.m_finalLayout = framebufferInfo.m_finalLayout;
                insertResult.first->second.m_lastSubpassUsage = framebufferInfo.m_lastSubpassUsage;
            }
        }

        RHI::ResultCode RenderPassBuilder::End(RenderPassContext& builtContext)
        {
            AZStd::array<AZ::Name, RHI::Limits::Pipeline::RenderAttachmentCountMax> attachmentNames;
            RHI::RenderAttachmentLayout builtRenderAttachmentLayout;
            RHI::ResultCode resultCode = m_layoutBuilder.End(builtRenderAttachmentLayout, &attachmentNames);
            if (resultCode != RHI::ResultCode::Success)
            {
                AZ_Assert(false, "Failed to create render pass descriptor");
                return resultCode;
            }

            RenderPass::Descriptor renderPassDesc =
                RenderPass::ConvertRenderAttachmentLayout(m_device, builtRenderAttachmentLayout, m_multisampleState);

            Framebuffer::Descriptor framebufferDesc;
            framebufferDesc.m_device = &m_device;

            // Set the clear values, image views and layouts.
            builtContext.m_clearValues.resize(renderPassDesc.m_attachmentCount);
            framebufferDesc.m_attachmentImageViews.resize(renderPassDesc.m_attachmentCount);
            for (uint32_t i = 0; i < renderPassDesc.m_attachmentCount; ++i)
            {
                auto findIter = m_framebufferAttachments.find(attachmentNames[i]);
                AZ_Assert(
                    findIter != m_framebufferAttachments.end(), "Failed to find attachment info for %s", attachmentNames[i].GetCStr());
                const FramebufferInfo& attachmentInfo = findIter->second;
                RenderPass::AttachmentBinding& attachmentBinding = renderPassDesc.m_attachments[i];
                attachmentBinding.m_initialLayout = attachmentInfo.m_initialLayout;
                attachmentBinding.m_finalLayout = attachmentInfo.m_finalLayout;
                builtContext.m_clearValues[i] = attachmentInfo.m_clearValue;
                framebufferDesc.m_attachmentImageViews[i] = attachmentInfo.m_imageView;
            }

            builtContext.m_renderPass = m_device.AcquireRenderPass(renderPassDesc);
            if (!builtContext.m_renderPass)
            {
                AZ_Assert(false, "Failed to create renderpass on RenderPassBuilder");
                return RHI::ResultCode::Fail;
            }        
            
            framebufferDesc.m_renderPass = builtContext.m_renderPass.get();
            builtContext.m_framebuffer = m_device.AcquireFramebuffer(framebufferDesc);
            if (!builtContext.m_framebuffer)
            {
                AZ_Assert(false, "Failed to create framebuffer on RenderPassBuilder");
                return RHI::ResultCode::Fail;
            }

            return RHI::ResultCode::Success;
        }

        bool RenderPassBuilder::CanBuild() const
        {
            return m_layoutBuilder.GetSubpassCount() > 0;
        }

        VkImageLayout RenderPassBuilder::GetInitialLayout(const Scope& scope, const RHI::ImageScopeAttachment& attachment) const
        {
            // Calculate the initial layout of an image attachment from a list of barriers.
            // The initial layout is a combination of the resource barriers.
            const ImageView* imageView = static_cast<const ImageView*>(attachment.GetImageView()->GetDeviceImageView(scope.GetDeviceIndex()).get());
            auto& barriers = scope.m_globalBarriers[static_cast<uint32_t>(Scope::BarrierSlot::Prologue)];
            auto isEqual = [imageView](const Scope::Barrier& barrier)
            {
                return barrier.Overlaps(*imageView, Scope::OverlapType::Partial);
            };

            // Combine all the layouts of all the barriers that are using the resource.
            auto it = AZStd::find_if(barriers.begin(), barriers.end(), isEqual);
            if (it != barriers.end())
            {
                VkImageLayout layout = it->m_imageBarrier.oldLayout;
                VkImageAspectFlags aspectFlags = it->m_imageBarrier.subresourceRange.aspectMask;
                for (it = AZStd::find_if(it + 1, barriers.end(), isEqual);
                     it != barriers.end();
                     it = AZStd::find_if(it + 1, barriers.end(), isEqual))
                {
                    // To properly combine them we first need to remove the layout of the unused aspects.
                    layout = CombineImageLayout(
                        FilterImageLayout(
                            layout,
                            ConvertImageAspectFlags(aspectFlags)),
                        FilterImageLayout(
                            it->m_imageBarrier.oldLayout,
                            ConvertImageAspectFlags(it->m_imageBarrier.subresourceRange.aspectMask)));
                    aspectFlags |= it->m_imageBarrier.subresourceRange.aspectMask;
                }

                return layout;
            }

            // If we don't find any barrier for the image attachment, then the image will already be in the layout
            // it needs before starting the renderpass.
            return GetImageAttachmentLayout(attachment);
        }

        VkImageLayout RenderPassBuilder::GetFinalLayout(const Scope& scope, const RHI::ImageScopeAttachment& attachment) const
        {
            // Calculate the final layout of an image attachment from a list of barriers.
            // The final layout is a combination of the resource barriers.
            const ImageView* imageView = static_cast<const ImageView*>(attachment.GetImageView()->GetDeviceImageView(scope.GetDeviceIndex()).get());
            auto& barriers = scope.m_globalBarriers[static_cast<uint32_t>(Scope::BarrierSlot::Epilogue)];

            auto isEqual = [imageView](const Scope::Barrier& barrier)
            {
                return barrier.Overlaps(*imageView, Scope::OverlapType::Partial);
            };

            // Combine all the layouts of all the barriers that are using the resource.
            auto it = AZStd::find_if(barriers.begin(), barriers.end(), isEqual);
            if (it != barriers.end())
            {
                VkImageLayout layout = it->m_imageBarrier.newLayout;
                VkImageAspectFlags aspectFlags = it->m_imageBarrier.subresourceRange.aspectMask;
                for (it = AZStd::find_if(it + 1, barriers.end(), isEqual); it != barriers.end();
                     it = AZStd::find_if(it + 1, barriers.end(), isEqual))
                {
                    // To properly combine them we first need to remove the layout of the unused aspects.
                    layout = CombineImageLayout(
                        FilterImageLayout(layout, ConvertImageAspectFlags(aspectFlags)),
                        FilterImageLayout(
                            it->m_imageBarrier.newLayout, ConvertImageAspectFlags(it->m_imageBarrier.subresourceRange.aspectMask)));
                    aspectFlags |= it->m_imageBarrier.subresourceRange.aspectMask;
                }

                return layout;
            }

            // If we don't find any barrier for the image attachment, then the image will stay in the same layout.
            return GetImageAttachmentLayout(attachment);
        }
    }
}
