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
#include <Atom/RHI/ResolveScopeAttachment.h>
#include <RHI/BufferView.h>
#include <Atom/RHI.Reflect/Vulkan/Conversion.h>
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
        
        RenderPassBuilder::RenderPassBuilder(Device& device, uint32_t subpassesCount)
            : m_device(device)
        {
            m_renderpassDesc.m_device = &m_device;
            m_framebufferDesc.m_device = &m_device;
            m_subpassCount = subpassesCount;
            m_usedAttachmentsPerSubpass.resize(m_subpassCount);
        }

        void RenderPassBuilder::AddScopeAttachments(const Scope& scope)
        {
            auto setAttachmentStoreActionFunc = [this](const uint32_t attachmentIndex, const RHI::AttachmentLoadStoreAction& loadStoreAction)
            {
                auto& attachmentLoadStoreAction = m_renderpassDesc.m_attachments[attachmentIndex].m_loadStoreAction;
                attachmentLoadStoreAction.m_storeAction = loadStoreAction.m_storeAction != RHI::AttachmentStoreAction::DontCare ?
                    loadStoreAction.m_storeAction : attachmentLoadStoreAction.m_storeAction;

                attachmentLoadStoreAction.m_storeActionStencil = loadStoreAction.m_storeActionStencil != RHI::AttachmentStoreAction::Store ?
                    loadStoreAction.m_storeActionStencil : attachmentLoadStoreAction.m_storeActionStencil;
            };

            // Add a new subpass

            const uint32_t subpassIndex = m_renderpassDesc.m_subpassCount++;
            auto& subpassDescriptor = m_renderpassDesc.m_subpassDescriptors[subpassIndex];
            AZStd::unordered_map<RHI::AttachmentId, RenderPass::SubpassAttachment> subpassResolveAttachmentsMap;
            AZStd::bitset<RHI::Limits::Pipeline::RenderAttachmentCountMax>& usedAttachments = m_usedAttachmentsPerSubpass[subpassIndex];
            struct RenderAttachment
            {
                const RHI::ImageScopeAttachment* m_scopeAttachment;
                RHI::ScopeAttachmentUsage m_usage;
            };

            AZStd::vector<RenderAttachment> renderAttachments;
            for (const auto* scopeAttachment : scope.GetImageAttachments())
            {
                for (const RHI::ScopeAttachmentUsageAndAccess& usageAndAccess : scopeAttachment->GetUsageAndAccess())
                {
                    renderAttachments.emplace_back(RenderAttachment{ scopeAttachment, usageAndAccess.m_usage });
                }
            }

            // This is the same order that the RHI::RenderAttachmentLayoutBuilder uses.
            auto getPriority = [](RHI::ScopeAttachmentUsage usage)
            {
                switch (usage)
                {
                case RHI::ScopeAttachmentUsage::Resolve:
                    return 0;
                case RHI::ScopeAttachmentUsage::RenderTarget:
                    return 1;
                case RHI::ScopeAttachmentUsage::DepthStencil:
                    return 2;
                case RHI::ScopeAttachmentUsage::SubpassInput:
                    return 4;
                default:
                    return ~0;
                }
            };

            // We sort it so it matches the order of the attachments in the renderpass declared in the PipelineState.
            AZStd::stable_sort(renderAttachments.begin(), renderAttachments.end(), [&](const auto& lhs, const auto& rhs)
            {
                return getPriority(lhs.m_usage) < getPriority(rhs.m_usage);
            });

            // Iterate through the image attachments and add the render attachments to the renderpass and framebuffer descriptors.
            for (const RenderAttachment& attachment : renderAttachments)
            {
                const RHI::ImageScopeAttachment* scopeAttachment = attachment.m_scopeAttachment;
                const RHI::ImageScopeAttachmentDescriptor& bindingDescriptor = scopeAttachment->GetDescriptor();
                const RHI::ImageFrameAttachment& imageFrameAttachment = scopeAttachment->GetFrameAttachment();
                const RHI::ImageDescriptor& imageDescriptor = imageFrameAttachment.GetImageDescriptor();
                const ImageView* attachmentImageView = static_cast<const ImageView*>(scopeAttachment->GetImageView());
                const RHI::Format imageViewFormat = attachmentImageView->GetFormat();
                auto scopeAttachmentId = scopeAttachment->GetDescriptor().m_attachmentId;
                AZ_Assert(imageViewFormat != RHI::Format::Unknown, "Invalid image view format.");
                // Add any subpass dependency from the use of this resource.
                AddResourceDependency<ImageView>(subpassIndex, scope, attachmentImageView);

                switch (attachment.m_usage)
                {
                case RHI::ScopeAttachmentUsage::RenderTarget:
                {
                    auto layout = GetImageAttachmentLayout(*scopeAttachment);
                    auto finalLayout = GetFinalLayout(scope, *scopeAttachment);
                    auto findIter = m_attachmentsMap.find(scopeAttachmentId);
                    uint32_t attachmentIndex = 0;
                    // Find if we already added this attachment
                    if (findIter == m_attachmentsMap.end())
                    {
                        attachmentIndex = m_renderpassDesc.m_attachmentCount++;
                        RenderPass::AttachmentBinding& attachmentBinding = m_renderpassDesc.m_attachments[attachmentIndex];
                        attachmentBinding.m_format = imageViewFormat;
                        attachmentBinding.m_loadStoreAction = bindingDescriptor.m_loadStoreAction;
                        attachmentBinding.m_initialLayout = GetInitialLayout(scope, *scopeAttachment);
                        attachmentBinding.m_finalLayout = finalLayout;
                        attachmentBinding.m_multisampleState = imageDescriptor.m_multisampleState;
                        m_framebufferDesc.m_attachmentImageViews.push_back(static_cast<const ImageView*>(scopeAttachment->GetImageView()));
                        m_clearValues.push_back(bindingDescriptor.m_loadStoreAction.m_clearValue);
                    }
                    else
                    {
                        attachmentIndex = findIter->second;
                    }

                    // Add the use of the attachment to the subpass.
                    uint32_t subpassRendertargetIndex = subpassDescriptor.m_rendertargetCount++;
                    subpassDescriptor.m_rendertargetAttachments[subpassRendertargetIndex] = RenderPass::SubpassAttachment{ attachmentIndex, layout };

                    // Check if we are resolving this render target.
                    auto findResolveIt = subpassResolveAttachmentsMap.find(scopeAttachmentId);
                    if (findResolveIt != subpassResolveAttachmentsMap.end())
                    {
                        // Add the resolve attachments of the subpass
                        subpassDescriptor.m_resolveAttachments[subpassRendertargetIndex] = findResolveIt->second;
                    }

                    m_lastSubpassResourceUse[attachmentImageView] = subpassIndex;
                    m_renderpassDesc.m_attachments[attachmentIndex].m_finalLayout = finalLayout;
                    setAttachmentStoreActionFunc(attachmentIndex, bindingDescriptor.m_loadStoreAction);
                    m_attachmentsMap[scopeAttachmentId] = attachmentIndex;
                    usedAttachments.set(attachmentIndex);
                    break;
                }
                case RHI::ScopeAttachmentUsage::DepthStencil:
                {
                    auto layout = GetImageAttachmentLayout(*scopeAttachment);
                    auto finalLayout = GetFinalLayout(scope, *scopeAttachment);
                    auto findIter = m_attachmentsMap.find(scopeAttachmentId);
                    uint32_t attachmentIndex = 0;
                    if (findIter == m_attachmentsMap.end())
                    {
                        attachmentIndex = m_renderpassDesc.m_attachmentCount++;
                        RenderPass::AttachmentBinding& attachmentBinding = m_renderpassDesc.m_attachments[attachmentIndex];
                        attachmentBinding.m_format = imageViewFormat;
                        attachmentBinding.m_loadStoreAction = bindingDescriptor.m_loadStoreAction;
                        attachmentBinding.m_initialLayout = GetInitialLayout(scope, *scopeAttachment);
                        attachmentBinding.m_finalLayout = finalLayout;
                        attachmentBinding.m_multisampleState = imageDescriptor.m_multisampleState;
                        m_framebufferDesc.m_attachmentImageViews.push_back(static_cast<const ImageView*>(scopeAttachment->GetImageView()));
                        m_clearValues.push_back(bindingDescriptor.m_loadStoreAction.m_clearValue);
                    }
                    else
                    {
                        attachmentIndex = findIter->second;
                    }

                    subpassDescriptor.m_depthStencilAttachment = RenderPass::SubpassAttachment{ attachmentIndex, layout };
                    m_renderpassDesc.m_attachments[attachmentIndex].m_finalLayout = finalLayout;
                    setAttachmentStoreActionFunc(attachmentIndex, bindingDescriptor.m_loadStoreAction);
                    m_lastSubpassResourceUse[attachmentImageView] = subpassIndex;
                    m_attachmentsMap[scopeAttachmentId] = attachmentIndex;
                    usedAttachments.set(attachmentIndex);
                    break;
                }
                case RHI::ScopeAttachmentUsage::Resolve:
                {
                    auto layout = GetImageAttachmentLayout(*scopeAttachment);
                    auto finalLayout = GetFinalLayout(scope, *scopeAttachment);
                    auto resolveScopeAttachment = static_cast<const RHI::ResolveScopeAttachment*>(scopeAttachment);

                    // This is the index of the resolve attachment in the renderpass.
                    auto findIter = m_attachmentsMap.find(scopeAttachmentId);
                    uint32_t attachmentIndex = 0;
                    if (findIter == m_attachmentsMap.end())
                    {
                        attachmentIndex = m_renderpassDesc.m_attachmentCount++;
                        RenderPass::AttachmentBinding& attachmentBinding = m_renderpassDesc.m_attachments[attachmentIndex];
                        attachmentBinding.m_format = imageViewFormat;
                        attachmentBinding.m_loadStoreAction = bindingDescriptor.m_loadStoreAction;
                        attachmentBinding.m_initialLayout = GetInitialLayout(scope, *scopeAttachment);
                        attachmentBinding.m_finalLayout = finalLayout;
                        attachmentBinding.m_multisampleState = imageDescriptor.m_multisampleState;

                        m_framebufferDesc.m_attachmentImageViews.push_back(static_cast<const ImageView*>(scopeAttachment->GetImageView()));
                        m_clearValues.push_back(bindingDescriptor.m_loadStoreAction.m_clearValue);
                    }
                    else
                    {
                        attachmentIndex = findIter->second;
                    }

                    // Add the SubPassAttchment information for the color attachment that will be resolved.
                    auto resolveAttachmentId = resolveScopeAttachment->GetDescriptor().m_resolveAttachmentId;
                    subpassResolveAttachmentsMap[resolveAttachmentId] = RenderPass::SubpassAttachment{ attachmentIndex, layout };
                    m_lastSubpassResourceUse[attachmentImageView] = subpassIndex;
                    m_renderpassDesc.m_attachments[attachmentIndex].m_finalLayout = finalLayout;
                    setAttachmentStoreActionFunc(attachmentIndex, bindingDescriptor.m_loadStoreAction);
                    m_attachmentsMap[scopeAttachmentId] = attachmentIndex;
                    usedAttachments.set(attachmentIndex);
                    break;
                }
                case RHI::ScopeAttachmentUsage::SubpassInput:
                {
                    auto layout = GetImageAttachmentLayout(*scopeAttachment);
                    auto finalLayout = GetFinalLayout(scope, *scopeAttachment);
                        auto scopeAttachmentId2 = scopeAttachment->GetDescriptor().m_attachmentId;
                        auto findIter = m_attachmentsMap.find(scopeAttachmentId2);
                        AZ_Assert(findIter != m_attachmentsMap.end(), "Could not find input attachment %s", scopeAttachmentId2.GetCStr());
                    const uint32_t attachmentIndex = findIter->second;
                    subpassDescriptor.m_subpassInputAttachments[subpassDescriptor.m_subpassInputCount++] = RenderPass::SubpassAttachment{ attachmentIndex , layout };
                    m_lastSubpassResourceUse[attachmentImageView] = subpassIndex;
                    m_renderpassDesc.m_attachments[attachmentIndex].m_finalLayout = finalLayout;
                    usedAttachments.set(attachmentIndex);
                    break;
                }
                case RHI::ScopeAttachmentUsage::Shader:
                case RHI::ScopeAttachmentUsage::Copy:
                    // do nothing
                    break;
                default:
                    AZ_Assert(false, "ScopeAttachmentUsage %d is invalid.", attachment.m_usage);
                }
            }            

            // Add the subpass dependencies from the buffer attachments.
            for (size_t index = 0; index < scope.GetBufferAttachments().size(); ++index)
            {
                const RHI::BufferScopeAttachment* scopeAttachment = scope.GetBufferAttachments()[index];
                const BufferView* bufferView = static_cast<const BufferView*>(scopeAttachment->GetBufferView());
                AddResourceDependency(subpassIndex, scope, bufferView);
            }
        }

        RHI::ResultCode RenderPassBuilder::End(RenderPassContext& builtContext)
        {
            // [GFX_TODO][ATOM-3948] Implement preserve attachments. For now preserve all attachments.
            for (uint32_t subpassIndex = 0; subpassIndex < m_subpassCount; ++subpassIndex)
            {
                const auto& usedAttachments = m_usedAttachmentsPerSubpass[subpassIndex];
                auto& subpassDescriptor = m_renderpassDesc.m_subpassDescriptors[subpassIndex];
                for (uint32_t attachmentIndex = 0; attachmentIndex < m_renderpassDesc.m_attachmentCount; ++attachmentIndex)
                {
                    if (!usedAttachments[attachmentIndex])
                    {
                        subpassDescriptor.m_preserveAttachments[subpassDescriptor.m_preserveAttachmentCount++] = attachmentIndex;
                    }
                }
            }      

            builtContext.m_renderPass = m_device.AcquireRenderPass(m_renderpassDesc);
            if (!builtContext.m_renderPass)
            {
                AZ_Assert(false, "Failed to create renderpass on RenderPassBuilder");
                return RHI::ResultCode::Fail;
            }
            
            m_framebufferDesc.m_renderPass = builtContext.m_renderPass.get();
            builtContext.m_framebuffer = m_device.AcquireFramebuffer(m_framebufferDesc);
            if (!builtContext.m_framebuffer)
            {
                AZ_Assert(false, "Failed to create framebuffer on RenderPassBuilder");
                return RHI::ResultCode::Fail;
            }           

            builtContext.m_clearValues = AZStd::move(m_clearValues);
            return RHI::ResultCode::Success;
        }

        void RenderPassBuilder::AddSubpassDependency(uint32_t srcSubpass, uint32_t dstSubpass, const Scope::Barrier& barrier)
        {          
            m_renderpassDesc.m_subpassDependencies.emplace_back();
            VkSubpassDependency& dependency = m_renderpassDesc.m_subpassDependencies.back();
            dependency = {};
            dependency.dependencyFlags = barrier.m_dependencyFlags;
            dependency.srcSubpass = srcSubpass;
            dependency.srcStageMask = barrier.m_srcStageMask;
            dependency.dstSubpass = dstSubpass;
            dependency.dstStageMask = barrier.m_dstStageMask;
            switch (barrier.m_type)
            {
            case VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER:
                dependency.srcAccessMask = barrier.m_imageBarrier.srcAccessMask;
                dependency.dstAccessMask = barrier.m_imageBarrier.dstAccessMask;
                break;
            case VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER:
                dependency.srcAccessMask = barrier.m_bufferBarrier.srcAccessMask;
                dependency.dstAccessMask = barrier.m_bufferBarrier.dstAccessMask;
                break;
            case VK_STRUCTURE_TYPE_MEMORY_BARRIER:
                dependency.srcAccessMask = barrier.m_memoryBarrier.srcAccessMask;
                dependency.dstAccessMask = barrier.m_memoryBarrier.dstAccessMask;
                break;
            default:
                AZ_Assert(false, "Invalid barrier type %d", barrier.m_type);
                break;
            }
        }

        VkImageLayout RenderPassBuilder::GetInitialLayout(const Scope& scope, const RHI::ImageScopeAttachment& attachment) const
        {
            // Calculate the initial layout of an image attachment from a list of barriers.
            // The initial layout is the old layout of the first barrier of the image (it's the layout
            // that the image will be before starting the renderpass).
            const ImageView* imageView = static_cast<const ImageView*>(attachment.GetImageView());
            auto& barriers = scope.m_subpassBarriers[static_cast<uint32_t>(Scope::BarrierSlot::Prologue)];

            auto findIt = AZStd::find_if(barriers.begin(), barriers.end(), [imageView](const Scope::Barrier& barrier)
            {
                return barrier.BlocksResource(*imageView, Scope::OverlapType::Complete);
            });

            // If we don't find any barrier for the image attachment, then the image will already be in the layout
            // it needs before starting the renderpass.
            return findIt != barriers.end() ? findIt->m_imageBarrier.oldLayout : GetImageAttachmentLayout(attachment);
        }

        VkImageLayout RenderPassBuilder::GetFinalLayout(const Scope& scope, const RHI::ImageScopeAttachment& attachment) const
        {
            // Calculate the final layout of an image attachment from a list of barriers.
            // The final layout is the new layout of the last barrier of the image (it's the layout
            // that the image will transition before ending the renderpass).
            const ImageView* imageView = static_cast<const ImageView*>(attachment.GetImageView());
            auto& barriers = scope.m_subpassBarriers[static_cast<uint32_t>(Scope::BarrierSlot::Epilogue)];

            auto findIt = AZStd::find_if(barriers.rbegin(), barriers.rend(), [imageView](const Scope::Barrier& barrier)
            {
                return barrier.BlocksResource(*imageView, Scope::OverlapType::Complete);
            });

            // If we don't find any barrier for the image attachment, then the image will stay in the same layout.
            return findIt != barriers.rend() ? findIt->m_imageBarrier.newLayout : GetImageAttachmentLayout(attachment);
        }
    }
}
