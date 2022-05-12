/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/std/hash.h>
#include <RHI/Conversion.h>
#include <RHI/Device.h>
#include <RHI/Framebuffer.h>
#include <RHI/ImageView.h>
#include <RHI/RenderPass.h>

namespace AZ
{
    namespace Vulkan
    {
        size_t Framebuffer::Descriptor::GetHash() const
        {
            size_t seed = 0;
            AZStd::hash_combine(seed, m_device);
            AZStd::hash_combine(seed, m_renderPass);
            AZStd::hash_range(seed, m_attachmentImageViews.begin(), m_attachmentImageViews.end());
            return seed;
        }

        RHI::Ptr<Framebuffer> Framebuffer::Create()
        {
            return aznew Framebuffer();
        }

        RHI::ResultCode Framebuffer::Init(const Descriptor& descriptor)
        {
            AZ_Assert(descriptor.m_device, "Device is null.");
            AZ_Assert(descriptor.m_renderPass, "Renderpass is null");
            Base::Init(*descriptor.m_device);
            RHI::ResultCode result = RHI::ResultCode::Success;

            m_renderPass = descriptor.m_renderPass;
            AZ_Assert(!descriptor.m_attachmentImageViews.empty(), "Descriptor have no image view.");

            m_attachments.resize(descriptor.m_attachmentImageViews.size());          
           
            // An attachment (ImageView) become stale when the corresponding resource (Image) is updated,
            // and Framebuffer have to be updated then (after the update of the attachment).
            // ResourceInvalidateBus informs the update of a resource.
            // Since the ResourceEventPriority of ImageView is higher than one of Framebuffer,
            // ImageView will be updated before update of Framebuffer.
            bool attachmentIsStale = false;
            for (size_t index = 0; index < descriptor.m_attachmentImageViews.size(); ++index)
            {
                const ImageView* imageView = descriptor.m_attachmentImageViews[index];
                attachmentIsStale |= imageView->IsStale();
                RHI::ResourceInvalidateBus::MultiHandler::BusConnect(&imageView->GetImage());
                m_attachments[index] = imageView;
            }

            // In case an imageView is stale, native framebuffer will be createted by OnResourceInvalidate() soon later.
            if (!attachmentIsStale)
            {
                result = BuildNativeFramebuffer();
                RETURN_RESULT_IF_UNSUCCESSFUL(result);
            }

            SetName(GetName());
            return result;
        }

        VkFramebuffer Framebuffer::GetNativeFramebuffer() const
        {
            return m_nativeFramebuffer;
        }

        const RenderPass* Framebuffer::GetRenderPass() const
        {
            return m_renderPass.get();
        }

        const RHI::Size& Framebuffer::GetSize() const
        {
            static const RHI::Size safeSize;
            return m_attachments.size() ? m_attachments.front()->GetImage().GetDescriptor().m_size : safeSize;
        }

        void Framebuffer::SetNameInternal(const AZStd::string_view& name)
        {
            if (IsInitialized() && !name.empty())
            {
                Debug::SetNameToObject(reinterpret_cast<uint64_t>(m_nativeFramebuffer), name.data(), VK_OBJECT_TYPE_FRAMEBUFFER, static_cast<Device&>(GetDevice()));
            }
        }

        void Framebuffer::Shutdown()
        {
            for (const RHI::ConstPtr<ImageView>& imageView : m_attachments)
            {
                const RHI::Image& image = imageView->GetImage();
                RHI::ResourceInvalidateBus::MultiHandler::BusDisconnect(&image);
            }
            Invalidate();
            Base::Shutdown();
        }

        RHI::ResultCode Framebuffer::OnResourceInvalidate()
        {
            Invalidate();
            if (!AreResourcesReady())
            {
                // By updating of a resource, it might become non-ready for Framebuffer.
                // For example, resizing of SwapChain may cause a size-mismatch between a resource and Framebuffer.
                // On such a case, the update of the resource should be ignored.
                return RHI::ResultCode::Success;
            }
            
            const RHI::ResultCode result = BuildNativeFramebuffer();
            if (result == RHI::ResultCode::Success)
            {
                SetName(GetName());
            }
            return result;
        }

        RHI::ResultCode Framebuffer::BuildNativeFramebuffer()
        {
            AZ_Assert(m_renderPass, "Render pass is null.");
            AZ_Assert(!m_attachments.empty(), "Attachment image view is empty.");

            AZStd::vector<VkImageView> imageViews(m_attachments.size(), VK_NULL_HANDLE);
            for (size_t index = 0; index < imageViews.size(); ++index)
            {
                imageViews[index] = m_attachments[index]->GetNativeImageView();
            }

            VkFramebufferCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            createInfo.flags = 0;
            createInfo.renderPass = m_renderPass->GetNativeRenderPass();
            createInfo.attachmentCount = static_cast<uint32_t>(imageViews.size());
            createInfo.pAttachments = imageViews.data();
            createInfo.width = GetSize().m_width;
            createInfo.height = GetSize().m_height;
            createInfo.layers = 1;
            
            auto& device = static_cast<Device&>(GetDevice());
            const VkResult result = vkCreateFramebuffer(device.GetNativeDevice(), &createInfo, nullptr, &m_nativeFramebuffer);

            return ConvertResult(result);
        }

        bool Framebuffer::AreResourcesReady() const
        {
            const RHI::Size& size = GetSize();
            for (size_t index = 0; index < m_attachments.size(); ++index)
            {
                const RHI::ConstPtr<ImageView>& imageView = m_attachments[index];
                if (imageView->IsStale() || imageView->GetImage().GetDescriptor().m_size != size)
                {
                    return false;
                }
            }
            return true;
        }

        void Framebuffer::Invalidate()
        {
            if (m_nativeFramebuffer != VK_NULL_HANDLE)
            {
                auto& device = static_cast<Device&>(GetDevice());
                vkDestroyFramebuffer(device.GetNativeDevice(), m_nativeFramebuffer, nullptr);
                m_nativeFramebuffer = VK_NULL_HANDLE;
            }
        }
    }
}
