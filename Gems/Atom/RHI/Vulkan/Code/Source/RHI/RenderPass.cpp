/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <RHI/Conversion.h>
#include <RHI/Device.h>
#include <RHI/Framebuffer.h>
#include <RHI/RenderPass.h>
#include <AzCore/std/hash.h>
#include <memory>

namespace AZ
{
    namespace Vulkan
    {
        RenderPass::AttachmentLoadStoreAction::AttachmentLoadStoreAction(const RHI::AttachmentLoadStoreAction& loadStoreAction)
        {
            *this = loadStoreAction;
        }

        RenderPass::AttachmentLoadStoreAction RenderPass::AttachmentLoadStoreAction::operator=(const RHI::AttachmentLoadStoreAction& loadStoreAction)
        {
            m_loadAction = loadStoreAction.m_loadAction;
            m_storeAction = loadStoreAction.m_storeAction;
            m_loadActionStencil = loadStoreAction.m_loadActionStencil;
            m_storeActionStencil = loadStoreAction.m_storeActionStencil;
            return *this;
        }

        size_t RenderPass::Descriptor::GetHash() const
        {
            size_t hash = 0;
            size_t attachmentsHash = AZStd::hash_range(m_attachments.begin(), m_attachments.begin() + m_attachmentCount);
            size_t subpassHash = AZStd::hash_range(m_subpassDescriptors.begin(), m_subpassDescriptors.begin() + m_subpassCount);
            size_t subpassDependenciesHash = AZStd::hash_range(m_subpassDependencies.begin(), m_subpassDependencies.end());
            AZStd::hash_combine(hash, m_attachmentCount);
            AZStd::hash_combine(hash, m_subpassCount);
            AZStd::hash_combine(hash, attachmentsHash);
            AZStd::hash_combine(hash, subpassHash);
            AZStd::hash_combine(hash, subpassDependenciesHash);
            return hash;
        }

        RHI::Ptr<RenderPass> RenderPass::Create()
        {
            return aznew RenderPass();
        }

        RHI::ResultCode RenderPass::Init(const Descriptor& descriptor)
        {
            m_descriptor = descriptor;
            AZ_Assert(descriptor.m_device, "Device is null.");
            Base::Init(static_cast<RHI::Device&>(*descriptor.m_device));

            RHI::ResultCode result = BuildNativeRenderPass();
            RETURN_RESULT_IF_UNSUCCESSFUL(result);

            SetName(GetName());
            return result;
        }

        VkRenderPass RenderPass::GetNativeRenderPass() const
        {
            return m_nativeRenderPass;
        }

        const RenderPass::Descriptor& RenderPass::GetDescriptor() const
        {
            return m_descriptor;
        }

        uint32_t RenderPass::GetAttachmentCount() const
        {
            return m_descriptor.m_attachmentCount;
        }

        AZStd::span<const RenderPass::SubpassAttachment> RenderPass::GetSubpassAttachments(const uint32_t subpassIndex, const AttachmentType type) const
        {
            const SubpassDescriptor& descriptor = m_descriptor.m_subpassDescriptors[subpassIndex];
            switch (type)
            {
            case AttachmentType::Color:
                return AZStd::span<const SubpassAttachment>(descriptor.m_rendertargetAttachments.begin(), descriptor.m_rendertargetCount);
            case AttachmentType::DepthStencil:
                return descriptor.m_depthStencilAttachment.IsValid() ?
                    AZStd::span<const SubpassAttachment>(&descriptor.m_depthStencilAttachment, 1) : AZStd::span<const SubpassAttachment>();
            case AttachmentType::InputAttachment:
                return AZStd::span<const SubpassAttachment>(descriptor.m_subpassInputAttachments.begin(), descriptor.m_subpassInputCount);
            case AttachmentType::Resolve:
                return AZStd::span<const SubpassAttachment>(descriptor.m_resolveAttachments.begin(), descriptor.m_rendertargetCount);
            default:
                AZ_Assert(false, "Invalid attachment type %d", type);
                return {};
            }
        }

        void RenderPass::SetNameInternal(const AZStd::string_view& name)
        {
            if (IsInitialized() && !name.empty())
            {
                Debug::SetNameToObject(reinterpret_cast<uint64_t>(m_nativeRenderPass), name.data(), VK_OBJECT_TYPE_RENDER_PASS, static_cast<Device&>(GetDevice()));
            }
        }

        void RenderPass::Shutdown()
        {
            if (m_nativeRenderPass != VK_NULL_HANDLE)
            {
                auto& device = static_cast<Device&>(GetDevice());
                vkDestroyRenderPass(device.GetNativeDevice(), m_nativeRenderPass, nullptr);
                m_nativeRenderPass = VK_NULL_HANDLE;
            }
            Base::Shutdown();
        }

        RHI::ResultCode RenderPass::BuildNativeRenderPass()
        {
            AZStd::vector<VkAttachmentDescription> attachmentDescriptions;
            AZStd::vector<VkSubpassDescription> subpassDescriptions;
            AZStd::vector<SubpassReferences> subpassReferences;
            AZStd::vector<VkSubpassDependency> subpassDependencies;

            BuildAttachmentDescriptions(attachmentDescriptions);
            BuildSubpassAttachmentReferences(subpassReferences);
            BuildSubpassDescriptions(subpassReferences, subpassDescriptions);
            BuildSubpassDependencies(subpassDependencies);

            VkRenderPassCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
            createInfo.pNext = nullptr;
            createInfo.flags = 0;
            createInfo.attachmentCount = static_cast<uint32_t>(attachmentDescriptions.size());
            createInfo.pAttachments = attachmentDescriptions.empty() ? nullptr : attachmentDescriptions.data();
            createInfo.subpassCount = static_cast<uint32_t>(subpassDescriptions.size());
            createInfo.pSubpasses = subpassDescriptions.empty() ? nullptr : subpassDescriptions.data();
            createInfo.dependencyCount = static_cast<uint32_t>(subpassDependencies.size());
            createInfo.pDependencies = subpassDependencies.empty() ? nullptr : subpassDependencies.data();

            auto& device = static_cast<Device&>(GetDevice());

            const VkResult result = vkCreateRenderPass(device.GetNativeDevice(), &createInfo, nullptr, &m_nativeRenderPass);
            AssertSuccess(result);

            return ConvertResult(result);
        }

        void RenderPass::BuildAttachmentDescriptions(AZStd::vector<VkAttachmentDescription>& attachmentDescriptions) const
        {           
            for (uint32_t i = 0; i < m_descriptor.m_attachmentCount; ++i)
            {
                const AttachmentBinding& binding = m_descriptor.m_attachments[i];
                attachmentDescriptions.emplace_back(VkAttachmentDescription{});
                VkAttachmentDescription& desc = attachmentDescriptions.back();
                desc.format = ConvertFormat(binding.m_format);
                desc.samples = ConvertSampleCount(binding.m_multisampleState.m_samples);
                desc.loadOp = ConvertAttachmentLoadAction(binding.m_loadStoreAction.m_loadAction);
                desc.storeOp = ConvertAttachmentStoreAction(binding.m_loadStoreAction.m_storeAction);
                desc.stencilLoadOp = ConvertAttachmentLoadAction(binding.m_loadStoreAction.m_loadActionStencil);
                desc.stencilStoreOp = ConvertAttachmentStoreAction(binding.m_loadStoreAction.m_storeActionStencil);
                desc.initialLayout = binding.m_initialLayout;
                desc.finalLayout = binding.m_finalLayout;
            }
        }

        void RenderPass::BuildSubpassAttachmentReferences(AZStd::vector<SubpassReferences>& subpassReferences) const
        {
            subpassReferences.resize(m_descriptor.m_subpassCount);
            for (uint32_t i = 0; i < m_descriptor.m_subpassCount; ++i)
            {
                BuildAttachmentReferences<AttachmentType::Color>(i, subpassReferences[i]);
                BuildAttachmentReferences<AttachmentType::DepthStencil>(i, subpassReferences[i]);
                BuildAttachmentReferences<AttachmentType::InputAttachment>(i, subpassReferences[i]);
                BuildAttachmentReferences<AttachmentType::Resolve>(i, subpassReferences[i]);
                BuildAttachmentReferences<AttachmentType::Preserve>(i, subpassReferences[i]);
            }
        }

        void RenderPass::BuildSubpassDescriptions(const AZStd::vector<SubpassReferences>& subpassReferences, AZStd::vector<VkSubpassDescription>& subpassDescriptions) const
        {
            subpassDescriptions.resize(m_descriptor.m_subpassCount);
            for (uint32_t i = 0; i < m_descriptor.m_subpassCount; ++i)
            {
                const auto& attachmentReferencesPerType = subpassReferences[i].m_attachmentReferences;
                auto& inputAttachmentRefList = attachmentReferencesPerType[static_cast<uint32_t>(AttachmentType::InputAttachment)];
                auto& colorAttachmentRefList = attachmentReferencesPerType[static_cast<uint32_t>(AttachmentType::Color)];
                auto& depthAttachmentRefList = attachmentReferencesPerType[static_cast<uint32_t>(AttachmentType::DepthStencil)];
                auto& resolveAttachmentRefList = attachmentReferencesPerType[static_cast<uint32_t>(AttachmentType::Resolve)];
                auto& preserveAttachmentList = subpassReferences[i].m_preserveAttachments;

                VkSubpassDescription& desc = subpassDescriptions[i];
                desc = {};
                desc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
                desc.inputAttachmentCount = static_cast<uint32_t>(inputAttachmentRefList.size());
                desc.pInputAttachments = inputAttachmentRefList.empty() ? nullptr : inputAttachmentRefList.data();
                desc.colorAttachmentCount = static_cast<uint32_t>(colorAttachmentRefList.size());
                desc.pColorAttachments = colorAttachmentRefList.empty() ? nullptr : colorAttachmentRefList.data();
                desc.pResolveAttachments = resolveAttachmentRefList.empty() ? nullptr : resolveAttachmentRefList.data();
                desc.pDepthStencilAttachment = depthAttachmentRefList.empty() ? nullptr : depthAttachmentRefList.data();
                desc.preserveAttachmentCount = static_cast<uint32_t>(preserveAttachmentList.size());
                desc.pPreserveAttachments = preserveAttachmentList.empty() ? nullptr : preserveAttachmentList.data();
            }
        }

        void RenderPass::BuildSubpassDependencies(AZStd::vector<VkSubpassDependency>& subpassDependencies) const
        {
            VkPipelineStageFlags supportedStages = GetSupportedPipelineStages(RHI::PipelineStateType::Draw);

            subpassDependencies = m_descriptor.m_subpassDependencies;
            for (VkSubpassDependency& subpassDependency : subpassDependencies)
            {
                subpassDependency.srcStageMask = RHI::FilterBits(subpassDependency.srcStageMask, subpassDependency.srcSubpass == VK_SUBPASS_EXTERNAL ? ~0 : supportedStages);
                subpassDependency.dstStageMask = RHI::FilterBits(subpassDependency.dstStageMask, subpassDependency.dstSubpass == VK_SUBPASS_EXTERNAL ? ~0 : supportedStages);
                subpassDependency.srcAccessMask = RHI::FilterBits(subpassDependency.srcAccessMask, GetSupportedAccessFlags(subpassDependency.srcStageMask));
                subpassDependency.dstAccessMask = RHI::FilterBits(subpassDependency.dstAccessMask, GetSupportedAccessFlags(subpassDependency.dstStageMask));
            }
        }

        template<>
        void RenderPass::BuildAttachmentReferences<RenderPass::AttachmentType::Preserve>(uint32_t subpassIndex, SubpassReferences& subpasReferences) const
        {
            auto& subpassDescriptor = const_cast<SubpassDescriptor&>(m_descriptor.m_subpassDescriptors[subpassIndex]);
            AZStd::vector<uint32_t>& attachmentReferenceList = subpasReferences.m_preserveAttachments;
            attachmentReferenceList.insert(
                attachmentReferenceList.end(),
                subpassDescriptor.m_preserveAttachments.begin(),
                subpassDescriptor.m_preserveAttachments.begin() + subpassDescriptor.m_preserveAttachmentCount);
        }
    }
}
