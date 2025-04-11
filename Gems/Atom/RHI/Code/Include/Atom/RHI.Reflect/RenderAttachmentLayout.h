/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/AttachmentLoadStoreAction.h>
#include <Atom/RHI.Reflect/Format.h>
#include <Atom/RHI.Reflect/Interval.h>
#include <Atom/RHI.Reflect/Limits.h>
#include <AzCore/Utils/TypeHash.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/std/containers/span.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>

namespace AZ
{
    class ReflectContext;
}

namespace AZ::RHI
{
    static const uint32_t InvalidRenderAttachmentIndex = Limits::Pipeline::RenderAttachmentCountMax;
    //! Base class for extra data to be used when building a RenderAttachmentLayout
    struct RenderAttachmentExtras
    {
        AZ_RTTI(RenderAttachmentExtras, "{0A50E3A8-78B6-4B7A-86CD-D0AB0108743E}");
        virtual ~RenderAttachmentExtras() = default;
    };

    //! Describes one render attachment that is part of a layout.
    struct RenderAttachmentDescriptor
    {
        AZ_TYPE_INFO(RenderAttachmentDescriptor, "{2855E1D2-BDA1-45A8-ABB9-5D8FB1E78EF4}");
        static void Reflect(ReflectContext* context);
        bool IsValid() const;

        bool operator==(const RenderAttachmentDescriptor& other) const;
        bool operator!=(const RenderAttachmentDescriptor& other) const;

        //! Position of the attachment in the layout.
        uint32_t m_attachmentIndex = InvalidRenderAttachmentIndex;
        //! Position of the resolve attachment in layout (if it resolves).
        uint32_t m_resolveAttachmentIndex = InvalidRenderAttachmentIndex;
        //! Load and store action of the attachment.
        AttachmentLoadStoreAction m_loadStoreAction = AttachmentLoadStoreAction();
        //! The following two flags are only relevant when there are more than one subpasses
        //! that will be merged.
        //! The scope attachment access as defined in the pass template, which will be used
        //! to accurately define the subpass dependencies.
        AZ::RHI::ScopeAttachmentAccess m_scopeAttachmentAccess = AZ::RHI::ScopeAttachmentAccess::Unknown;
        //! The scope attachment stage as defined in the pass template, which will be used
        //! to accurately define the subpass dependencies.
        AZ::RHI::ScopeAttachmentStage m_scopeAttachmentStage = AZ::RHI::ScopeAttachmentStage::Uninitialized;
        //! Extra data that can be passed for platform specific operations.
        RenderAttachmentExtras* m_extras = nullptr;
    };

    //! Describes a subpass input attachment.
    struct SubpassInputDescriptor
    {
        AZ_TYPE_INFO(SubpassInputDescriptor, "{5E5B933D-8209-4722-8AC5-C3FEA1D75BB3}");
        static void Reflect(ReflectContext* context);

        bool operator==(const SubpassInputDescriptor& other) const;
        bool operator!=(const SubpassInputDescriptor& other) const;

        //! Attachment index that this subpass input references.
        uint32_t m_attachmentIndex = 0;
        //! Aspects that are used by the input (needed by some implementations, like Vulkan, when creating a renderpass with a subpass input)
        RHI::ImageAspectFlags m_aspectFlags = RHI::ImageAspectFlags::None;
        //! The following two flags are only relevant when there are more than one subpasses
        //! that will be merged.
        //! The scope attachment access as defined in the pass template, which will be used
        //! to accurately define the subpass dependencies.
        AZ::RHI::ScopeAttachmentAccess m_scopeAttachmentAccess = AZ::RHI::ScopeAttachmentAccess::Unknown;
        //! The scope attachment stage as defined in the pass template, which will be used
        //! to accurately define the subpass dependencies.
        AZ::RHI::ScopeAttachmentStage m_scopeAttachmentStage = AZ::RHI::ScopeAttachmentStage::Uninitialized;
        //! Load and store action of the attachment.
        AttachmentLoadStoreAction m_loadStoreAction = AttachmentLoadStoreAction();
        //! Extra data that can be passed for platform specific operations.
        RenderAttachmentExtras* m_extras = nullptr;
    };

    //! Describes the attachments of one subpass as part of a render target layout.
    //! It include descriptions about the render targets, subpass inputs and depth/stencil attachment.
    struct SubpassRenderAttachmentLayout
    {
        AZ_TYPE_INFO(SubpassRenderAttachmentLayout, "{7AF04EC1-D835-4F97-8433-0D445C0D6F5B}");
        static void Reflect(ReflectContext* context);

        bool operator==(const SubpassRenderAttachmentLayout& other) const;
        bool operator!=(const SubpassRenderAttachmentLayout& other) const;

        //! Number of render targets in the subpass.
        uint32_t m_rendertargetCount = 0;
        //! Number of subpass input attachments in the subpass.
        uint32_t m_subpassInputCount = 0;
        //! List of render targets used by the subpass.
        AZStd::array<RenderAttachmentDescriptor, Limits::Pipeline::AttachmentColorCountMax> m_rendertargetDescriptors = { {} };
        //! List of subpass inputs used by the subpass.
        AZStd::array<SubpassInputDescriptor, Limits::Pipeline::AttachmentColorCountMax> m_subpassInputDescriptors = { {} };
        //! Descriptor of the depth/stencil attachment. If not used, the attachment index is InvalidRenderAttachmentIndex.
        RenderAttachmentDescriptor m_depthStencilDescriptor;
        //! Descriptor of the shading rate attachment. If not used, the attachment index is InvalidRenderAttachmentIndex.
        RenderAttachmentDescriptor m_shadingRateDescriptor;
    };

    //! A RenderAttachmentLayout consist of a description of one or more subpasses.
    //! Each subpass is a collection of render targets, subpass inputs and depth stencil attachments.
    //! Each subpass corresponds to a phase in the rendering of a Pipeline.
    //! Subpass outputs can be read by other subpasses as inputs.
    //! A RenderAttachmentLayout may be implemented by the platform using native functionality, achieving a
    //! performance gain for that specific platform. On other platforms, it may be emulated to achieve the same result
    //! but without the performance benefits. For example, Vulkan supports the concept of subpass natively.
    class RenderAttachmentLayout
    {
    public:
        AZ_TYPE_INFO(RenderAttachmentLayout, "{5ED96982-BFB0-4EFF-ABBE-1552CECE707D}");
        static void Reflect(ReflectContext* context);
  
        HashValue64 GetHash() const;

        bool operator==(const RenderAttachmentLayout& other) const;

        //! Number of total attachments in the list.
        uint32_t m_attachmentCount = 0;
        //! List with all attachments (renderAttachments, resolveAttachments and depth/stencil attachment).
        AZStd::array<Format, Limits::Pipeline::RenderAttachmentCountMax> m_attachmentFormats = { {} };

        //! Number of subpasses.
        uint32_t m_subpassCount = 0;
        //! List with the layout of each subpass.
        AZStd::array<SubpassRenderAttachmentLayout, Limits::Pipeline::SubpassCountMax> m_subpassLayouts;
    };

    //! Describes the layout of a collection of subpasses and it defines which of the subpasses this
    //! configuration will be using.
    struct RenderAttachmentConfiguration
    {
        AZ_TYPE_INFO(RenderAttachmentConfiguration, "{037F27A5-B568-439B-81D4-928DFA3A74F2}");
        static void Reflect(ReflectContext* context);

        HashValue64 GetHash() const;

        bool operator==(const RenderAttachmentConfiguration& other) const;

        //! Returns the format of a render target in the subpass being used.
        Format GetRenderTargetFormat(uint32_t index) const;
        //! Returns the format of a subpass input in the subpass being used.
        Format GetSubpassInputFormat(uint32_t index) const;
        //! Returns the format of resolve attachment in the subpass being used.
        Format GetRenderTargetResolveFormat(uint32_t index) const;
        //! Returns the format of a depth/stencil in the subpass being used.
        //! Will return Format::Unkwon if the depth/stencil is not being used.
        Format GetDepthStencilFormat() const;
        //! Returns the number of render targets in the subpass being used.
        uint32_t GetRenderTargetCount() const;
        //! Returns the number of subpass inputs in the subpass being used.
        uint32_t GetSubpassInputCount() const;
        //! Returns true if the render target is resolving in the subpass being used.
        bool DoesRenderTargetResolve(uint32_t index) const;

        //! Layout of the render target attachments.
        RenderAttachmentLayout m_renderAttachmentLayout;
        //! Index of the subpass being used.
        uint32_t m_subpassIndex = 0;
    };
}
