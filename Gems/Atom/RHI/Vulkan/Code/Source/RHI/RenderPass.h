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
            AZ_CLASS_ALLOCATOR(RenderPass, AZ::SystemAllocator);
            AZ_RTTI(RenderPass, "6F23B984-E6CF-40E2-9A8B-9605D82DFE27", Base);

            ~RenderPass() = default;
            static RHI::Ptr<RenderPass> Create();
             
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
                VkImageAspectFlags m_imageAspectFlags = VK_IMAGE_ASPECT_NONE;

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
                SubpassAttachment m_fragmentShadingRateAttachment;
            };

            struct Descriptor
            {
                size_t GetHash() const;

                Device* m_device = nullptr;
                //! Number of attachments (rendertarget, resolve, depth/stencil and shading rate)
                uint32_t m_attachmentCount = 0;
                //! Number of subpasses in the renderpass.
                uint32_t m_subpassCount = 0;
                //! Full list of attachments in the renderpass.
                AZStd::array<AttachmentBinding, RHI::Limits::Pipeline::RenderAttachmentCountMax> m_attachments;
                //! List of subpasses in the renderpass.
                AZStd::array<SubpassDescriptor, RHI::Limits::Pipeline::SubpassCountMax> m_subpassDescriptors;
                //! Dependencies of the resources between the subpasses.
                AZStd::vector<VkSubpassDependency> m_subpassDependencies;
                //! Necessary to avoid validation errors when Vulkan compares
                //! the VkRenderPass of the PipelineStateObject vs the VkRenderPass of the VkCmdBeginRenderPass.
                //! Even if the subpass dependencies are identical but they differ in order, it'd trigger validation errors.
                //! To make the order irrelevant, the solution is to merge the bitflags. 
                void MergeSubpassDependencies();
            };

            RHI::ResultCode Init(const Descriptor& descriptor);

            VkRenderPass GetNativeRenderPass() const;
            const Descriptor& GetDescriptor() const;
            uint32_t GetAttachmentCount() const;

            //! Typically the returned descriptor is only used to create a dummy VkRenderPass (cached and reusable)
            //! that will be associated with one or more PSOs. The PSO will use such VkRenderPass as a data source
            //! to better optimize the layout of the PSO. In the end the real VkRenderPass is built (cached and reusable)
            //! at runtime by the FrameGraph and used with VkCmdBeginRenderPass.
            //! This is possible because, per the Vulkan spec, it is only required that the PSO VkRenderPass and the VkCmdBeginRenderPass
            //! VkRenderPass to be "compatible", but they don't have to be the same object.
            static Descriptor ConvertRenderAttachmentLayout(
                Device& device,
                const RHI::RenderAttachmentLayout& layout,
                const RHI::MultisampleState& multisampleState);

            //! Contains the layout that the RenderAttachment will used on a subpass.
            //! This information is used when converting a RenderAttachmentLayout to a Renderpass::Descriptor (ConvertRenderAttachmentLayout)
            struct RenderAttachmentLayout : RHI::RenderAttachmentExtras
            {
                AZ_RTTI(RenderAttachmentLayout, "{EDFE4C66-9780-4752-96CD-CCCE81C029DC}");
                //! Layout of the attachment in a subpass.
                VkImageLayout m_layout = VK_IMAGE_LAYOUT_UNDEFINED;
            };

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

            template<typename T>
            RHI::ResultCode BuildNativeRenderPass();

            Descriptor m_descriptor;
            VkRenderPass m_nativeRenderPass = VK_NULL_HANDLE;
        };

        template<typename T>
        RHI::ResultCode RenderPass::BuildNativeRenderPass()
        {
            T builder(*m_descriptor.m_device);
            auto [vkResult, vkRenderPass] = builder.Build(m_descriptor);
            AssertSuccess(vkResult);
            m_nativeRenderPass = vkRenderPass;
            return ConvertResult(vkResult);
        }
    } // namespace Vulkan
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
