/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom_RHI_Vulkan_Platform.h>
#include <Atom/RHI/DeviceObject.h>
#include <Atom/RHI.Reflect/Limits.h>
#include <Atom/RHI.Reflect/ImageScopeAttachmentDescriptor.h>
#include <Atom/RHI.Reflect/Format.h>
#include <AzCore/std/containers/array.h>

namespace AZ
{
    namespace RHI
    {
        class PipelineStateDescriptorForDraw;
        struct ImageScopeAttachmentDescriptor;
    }

    namespace Vulkan
    {
        class Device;

        class RenderPass final
            : public RHI::DeviceObject
        {
            using Base = RHI::DeviceObject;

        public:
            AZ_CLASS_ALLOCATOR(RenderPass, AZ::SystemAllocator, 0);
            AZ_RTTI(RenderPass, "6F23B984-E6CF-40E2-9A8B-9605D82DFE27", Base);

            ~RenderPass() = default;
            static RHI::Ptr<RenderPass> Create();

            enum class AttachmentType : uint32_t
            {
                Color,              // Color render target attachment
                DepthStencil,       // Depth stencil attachment
                Resolve,            // Resolve attachment
                InputAttachment,    // An input attachment that is the output of a previous subpass.
                Preserve,           // An attachment that is not being used by the subpass but that it will be preserved.
                Count
            };

            static const uint32_t AttachmentTypeCount = static_cast<uint32_t>(AttachmentType::Count);

            //! Describes the load and store actions of an attachment. It's almost the same as the RHI version but without
            //! the clear color. We need to remove it so it doesn't affect the hash calculation.
            struct AttachmentLoadStoreAction
            {
                AttachmentLoadStoreAction() = default;
                explicit AttachmentLoadStoreAction(const RHI::AttachmentLoadStoreAction& loadStoreAction);
                AttachmentLoadStoreAction operator=(const RHI::AttachmentLoadStoreAction& loadStoreAction);

                RHI::AttachmentLoadAction m_loadAction = RHI::AttachmentLoadAction::Load;
                RHI::AttachmentStoreAction m_storeAction = RHI::AttachmentStoreAction::Store;
                RHI::AttachmentLoadAction m_loadActionStencil = RHI::AttachmentLoadAction::Load;
                RHI::AttachmentStoreAction m_storeActionStencil = RHI::AttachmentStoreAction::Store;
            };

            //! Information about one attachment.
            struct AttachmentBinding
            {
                RHI::Format m_format = RHI::Format::Unknown;
                AttachmentLoadStoreAction m_loadStoreAction;
                VkImageLayout m_initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                VkImageLayout m_finalLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                RHI::MultisampleState m_multisampleState;
            };

            //! Properties of one attachment in a subpass.
            struct SubpassAttachment
            {
                uint32_t m_attachmentIndex = RHI::InvalidRenderAttachmentIndex;
                VkImageLayout m_layout = VK_IMAGE_LAYOUT_UNDEFINED;

                bool IsValid() const { return m_attachmentIndex < RHI::Limits::Pipeline::AttachmentColorCountMax; }
            };

            //! Describes the used attachments in one subpass.
            struct SubpassDescriptor
            {
                uint32_t m_rendertargetCount = 0;
                uint32_t m_subpassInputCount = 0;
                uint32_t m_preserveAttachmentCount = 0;
                AZStd::array<SubpassAttachment, RHI::Limits::Pipeline::AttachmentColorCountMax> m_rendertargetAttachments;
                AZStd::array<SubpassAttachment, RHI::Limits::Pipeline::AttachmentColorCountMax> m_resolveAttachments;
                AZStd::array<SubpassAttachment, RHI::Limits::Pipeline::AttachmentColorCountMax> m_subpassInputAttachments;
                AZStd::array<uint32_t, RHI::Limits::Pipeline::AttachmentColorCountMax> m_preserveAttachments;
                SubpassAttachment m_depthStencilAttachment;
            };

            struct Descriptor
            {
                size_t GetHash() const;

                Device* m_device = nullptr;
                //! Number of attachments (rendertarget, resolve and depth/stencil)
                uint32_t m_attachmentCount = 0;
                //! Number of subpasses in the renderpass.
                uint32_t m_subpassCount = 0;
                //! Full list of attachments in the renderpass.
                AZStd::array<AttachmentBinding, RHI::Limits::Pipeline::RenderAttachmentCountMax> m_attachments;
                //! List of subpasses in the renderpass.
                AZStd::array<SubpassDescriptor, RHI::Limits::Pipeline::SubpassCountMax> m_subpassDescriptors;
                //! Dependencies of the resources between the subpasses.
                AZStd::vector<VkSubpassDependency> m_subpassDependencies;
            };

            RHI::ResultCode Init(const Descriptor& descriptor);

            VkRenderPass GetNativeRenderPass() const;
            const Descriptor& GetDescriptor() const;
            uint32_t GetAttachmentCount() const;

            static Descriptor ConvertRenderAttachmentLayout(const RHI::RenderAttachmentLayout& layout, const RHI::MultisampleState& multisampleState);

        private:
            RenderPass() = default;

            //////////////////////////////////////////////////////////////////////////
            // RHI::Object
            void SetNameInternal(const AZStd::string_view& name) override;
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // RHI::DeviceObject
            void Shutdown() override;
            //////////////////////////////////////////////////////////////////////////

            RHI::ResultCode BuildNativeRenderPass();

            struct SubpassReferences
            {
                AZStd::array<AZStd::vector<VkAttachmentReference>, AttachmentTypeCount> m_attachmentReferences;
                AZStd::vector<uint32_t> m_preserveAttachments;
            };

            template<AttachmentType type>
            void BuildAttachmentReferences(uint32_t subpassIndex, SubpassReferences& subpasReferences) const;
            void BuildAttachmentDescriptions(AZStd::vector<VkAttachmentDescription>& attachmentDescriptions) const;
            void BuildSubpassAttachmentReferences(AZStd::vector<SubpassReferences>& subpassReferences) const;
            void BuildSubpassDescriptions(const AZStd::vector<SubpassReferences>& subpassReferences, AZStd::vector<VkSubpassDescription>& subpassDescriptions) const;
            void BuildSubpassDependencies(AZStd::vector<VkSubpassDependency>& subpassDependencies) const;

            AZStd::span<const SubpassAttachment> GetSubpassAttachments(const uint32_t subpassIndex, const AttachmentType type) const;

            Descriptor m_descriptor;
            VkRenderPass m_nativeRenderPass = VK_NULL_HANDLE;

        };

        template<RenderPass::AttachmentType type>
        void RenderPass::BuildAttachmentReferences(uint32_t subpassIndex, SubpassReferences& subpasReferences) const
        {
            AZStd::span<const SubpassAttachment> subpassAttachmentList = GetSubpassAttachments(subpassIndex, type);
            AZStd::vector<VkAttachmentReference>& attachmentReferenceList = subpasReferences.m_attachmentReferences[static_cast<uint32_t>(type)];
            attachmentReferenceList.resize(subpassAttachmentList.size());
            for (uint32_t index = 0; index < subpassAttachmentList.size(); ++index)
            {
                if (subpassAttachmentList[index].IsValid())
                {
                    attachmentReferenceList[index].attachment = subpassAttachmentList[index].m_attachmentIndex;
                    attachmentReferenceList[index].layout = subpassAttachmentList[index].m_layout;
                }
                else
                {
                    attachmentReferenceList[index].attachment = VK_ATTACHMENT_UNUSED;
                }
            }
        }

        template<>
        void RenderPass::BuildAttachmentReferences<RenderPass::AttachmentType::Preserve>(uint32_t subpassIndex, SubpassReferences& subpasReferences) const;
    }
}

namespace AZStd
{
    template <>
    struct hash<AZ::Vulkan::RenderPass::AttachmentBinding>
    {
        using argument_type = AZ::Vulkan::RenderPass::AttachmentBinding;
        typedef AZStd::size_t result_type;

        result_type operator()(const argument_type& value) const
        {
            const char* buffer = reinterpret_cast<const char*>(&value);
            return AZStd::hash_range(buffer, buffer + sizeof(value));
        }
    };

    template <>
    struct hash<AZ::Vulkan::RenderPass::SubpassDescriptor>
    {
        using argument_type = AZ::Vulkan::RenderPass::SubpassDescriptor;
        typedef AZStd::size_t result_type;

        result_type operator()(const argument_type& value) const
        {
            const char* buffer = reinterpret_cast<const char*>(&value);
            return AZStd::hash_range(buffer, buffer + sizeof(value));
        }
    };

    template <>
    struct hash<VkSubpassDependency>
    {
        using argument_type = VkSubpassDependency;
        typedef AZStd::size_t result_type;

        result_type operator()(const argument_type& value) const
        {
            const char* buffer = reinterpret_cast<const char*>(&value);
            return AZStd::hash_range(buffer, buffer + sizeof(value));
        }
    };
}
