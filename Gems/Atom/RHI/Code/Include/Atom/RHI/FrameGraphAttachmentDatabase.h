/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/AttachmentEnums.h>
#include <Atom/RHI.Reflect/BufferDescriptor.h>
#include <Atom/RHI.Reflect/ImageDescriptor.h>
#include <Atom/RHI.Reflect/ScopeId.h>
#include <Atom/RHI.Reflect/TransientImageDescriptor.h>
#include <Atom/RHI.Reflect/TransientBufferDescriptor.h>
#include <Atom/RHI/FrameAttachment.h>
#include <Atom/RHI/ScopeAttachment.h>
#include <Atom/RHI/Scope.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/unordered_map.h>

namespace AZ::RHI
{
    class SingleDeviceImage;
    class SingleDeviceBuffer;
    class ImageFrameAttachment;
    class SwapChainFrameAttachment;
    class BufferFrameAttachment;
    class ImageScopeAttachment;
    class BufferScopeAttachment;
    class SingleDeviceSwapChain;
    struct TransientImageDescriptor;
    struct TransientBufferDescriptor;
    struct ResolveScopeAttachmentDescriptor;

    class FrameGraphAttachmentDatabase
    {
    public:
        FrameGraphAttachmentDatabase() = default;

        //! Clears the database back to an empty state.
        void Clear();

        //! Imports an image into the database.
        ResultCode ImportImage(const AttachmentId& attachmentId, Ptr<SingleDeviceImage> image);

        //! Imports a swapchain into the database.
        ResultCode ImportSwapChain(const AttachmentId& attachmentId, Ptr<SingleDeviceSwapChain> swapChain);

        //! Imports a buffer into the database.
        ResultCode ImportBuffer(const AttachmentId& attachmentId, Ptr<SingleDeviceBuffer> buffer);

        //! Creates a transient image and inserts it into the database.
        ResultCode CreateTransientImage(const TransientImageDescriptor& descriptor);

        //! Creates a transient buffer and inserts it into the database.
        ResultCode CreateTransientBuffer(const TransientBufferDescriptor& descriptor);

        //! Finds the attachment associated with \param attachmentId and returns its image descriptor.
        ImageDescriptor GetImageDescriptor(const AttachmentId& attachmentId) const;

        //! Finds the attachment associated with \param attachmentId and returns its buffer descriptor.
        BufferDescriptor GetBufferDescriptor(const AttachmentId& attachmentId) const;

        //! Returns whether the attachment exists in the database.
        bool IsAttachmentValid(const AttachmentId& attachmentId) const;

        //! Finds an attachment associated with \param attachmentId.
        const FrameAttachment* FindAttachment(const AttachmentId& attachmentId) const;
        FrameAttachment* FindAttachment(const AttachmentId& attachmentId);

        //! Finds an attachment associated with \param attachmentId and attempts to cast
        //! to the requested type. Will return null if the type is not compatible, or the
        //! attachment was not found.
        template <typename AttachmentType>
        const AttachmentType* FindAttachment(const AttachmentId& attachmentId) const;

        template <typename AttachmentType>
        AttachmentType* FindAttachment(const AttachmentId& attachmentId);

        //! Returns the full list of attachments.
        const AZStd::vector<FrameAttachment*>& GetAttachments() const;

        //! Returns the full list of image attachments.
        const AZStd::vector<ImageFrameAttachment*>& GetImageAttachments() const;

        //! Returns the full list of buffer attachments.
        const AZStd::vector<BufferFrameAttachment*>& GetBufferAttachments() const;

        //! Returns the transient swap chain attachments registered in the graph.
        const AZStd::vector<SwapChainFrameAttachment*>& GetSwapChainAttachments() const;

        //! Returns the imported image attachments registered in the graph.
        const AZStd::vector<ImageFrameAttachment*>& GetImportedImageAttachments() const;

        //! Returns the imported buffer attachments registered in the graph.
        const AZStd::vector<BufferFrameAttachment*>& GetImportedBufferAttachments() const;

        //! Returns the transient image attachments registered in the graph.
        const AZStd::vector<ImageFrameAttachment*>& GetTransientImageAttachments() const;

        //! Returns the transient buffer attachments registered in the graph.
        const AZStd::vector<BufferFrameAttachment*>& GetTransientBufferAttachments() const;

        //! Finds the list of scope attachments used by a scope for the given attachment.
        const ScopeAttachmentPtrList* FindScopeAttachmentList(const ScopeId& scopeId, const AttachmentId& attachmentId) const;

        //! Finds the scope attachment used by a scope for the given attachment
        const ScopeAttachment* FindScopeAttachment(const ScopeId& scopeId, const AttachmentId& attachmentId) const;

        //! Finds the scope attachment used by a scope for the given attachment. If multiple scope image attachments are used for the
        //! same attachment, provide ScopeAttachmentUsage (in case attachments are merged) and
        //! ImageViewDescriptor (in case the attachments are different based on view, i.e different mips or aspect of a texture)  to ensure
        //! that the correct scope attachment is returned.
        const ScopeAttachment* FindScopeAttachment(
            const ScopeId& scopeId,
            const AttachmentId& attachmentId,
            const ImageViewDescriptor& imageViewDescriptor,
            const RHI::ScopeAttachmentUsage attachmentUsage) const;

        //! Finds the scope attachment used by a scope for the given attachment. If multiple scope attachments are used for the same attachment 
        //! provide attachmentUsage to ensure that the correct scope attachment is returned
        const ScopeAttachment* FindScopeAttachment(
            const ScopeId& scopeId,
            const AttachmentId& attachmentId,
            const RHI::ScopeAttachmentUsage attachmentUsage) const;

        //! Returns the full list of scope attachments.
        const ScopeAttachmentPtrList& GetScopeAttachments() const;

        template <typename ScopeAttachmentType, typename... Args>
        ScopeAttachmentType* EmplaceScopeAttachment(
            Scope& scope,
            FrameAttachment& attachment,
            Args&&... arguments);

        //! Emplaces a use of a resource pool by a specific scope. Returns the ScopeId of the most recent use of the pool or en empty
        //! ScopeId if this is the first use.
        ScopeId EmplaceResourcePoolUse(SingleDeviceResourcePool& pool, ScopeId scopeId);

    private:
        bool ValidateAttachmentIsUnregistered(const AttachmentId& attachmentId) const;

        template <typename FrameAttachmentType, typename... Args>
        FrameAttachmentType* EmplaceFrameAttachment(Args&&... arguments);

        static size_t HashScopeAttachmentPair(const ScopeId& scopeId, const AttachmentId& attachmentId);

        ScopeAttachmentPtrList                                  m_scopeAttachments;

        // Key = hash of ScopeId and AttachmentId, see HashScopeAttachmentPair
        // Value is a list of pointers to all the ScopeAttachments used by the given scope for the given attachment
        // Scope can use multiple ScopeAttachments per attachments for reading/writing to different mips of an image
        AZStd::unordered_map<size_t, ScopeAttachmentPtrList>    m_scopeAttachmentLookup;
            
        AZStd::vector<FrameAttachment*>                         m_attachments;
        AZStd::unordered_map<AttachmentId, FrameAttachment*>    m_attachmentLookup;
        AZStd::vector<SwapChainFrameAttachment*>                m_swapChainAttachments;
        AZStd::vector<ImageFrameAttachment*>                    m_imageAttachments;
        AZStd::vector<ImageFrameAttachment*>                    m_importedImageAttachments;
        AZStd::vector<ImageFrameAttachment*>                    m_transientImageAttachments;
        AZStd::vector<BufferFrameAttachment*>                   m_bufferAttachments;
        AZStd::vector<BufferFrameAttachment*>                   m_importedBufferAttachments;
        AZStd::vector<BufferFrameAttachment*>                   m_transientBufferAttachments;
        AZStd::unordered_map<Ptr<SingleDeviceResourcePool>, ScopeId>        m_resourcePoolLastScopeUse;
    };

    template <typename FrameAttachmentType, typename... Args>
    FrameAttachmentType* FrameGraphAttachmentDatabase::EmplaceFrameAttachment(Args&&... arguments)
    {
        FrameAttachmentType* attachment = aznew FrameAttachmentType(AZStd::forward<Args&&>(arguments)...);
        m_attachments.emplace_back(attachment);
        m_attachmentLookup.emplace(attachment->GetId(), attachment);
        return attachment;
    }

    template <typename ScopeAttachmentType, typename... Args>
    ScopeAttachmentType* FrameGraphAttachmentDatabase::EmplaceScopeAttachment(
        Scope& scope,
        FrameAttachment& attachment,
        Args&&... arguments)
    {
        ScopeAttachmentType* scopeAttachment = aznew ScopeAttachmentType(scope, attachment, AZStd::forward<Args&&>(arguments)...);

        if (attachment.m_firstScopeAttachment == nullptr)
        {
            // First element in the linked list. Trivial assignment.
            attachment.m_firstScopeAttachment = scopeAttachment;
            attachment.m_firstScope = &scope;
        }
        else
        {
            // Link tail.next to node.
            attachment.m_lastScopeAttachment->m_next = scopeAttachment;

            // Link node.prev to tail.
            scopeAttachment->m_prev = attachment.m_lastScopeAttachment;
        }

        // Assign node to be the new tail.
        attachment.m_lastScopeAttachment = scopeAttachment;
        attachment.m_lastScope = &scope;

        const HardwareQueueClassMask queueMaskBit = GetHardwareQueueClassMask(scope.GetHardwareQueueClass());

        AZ_Warning(
            "FrameGraph",
            CheckBitsAny(attachment.GetSupportedQueueMask(), queueMaskBit),
            "Attachment '%s' does not support usage on the %s queue on scope '%s'. This may cause visual "
            "artifacts or even device removal and should be addressed.",
            attachment.GetId().GetCStr(),
            GetHardwareQueueClassName(scope.GetHardwareQueueClass()),
            scope.GetId().GetCStr());

        attachment.m_usedQueueMask |= queueMaskBit;

        m_scopeAttachments.push_back(scopeAttachment);
        size_t attachmentScopeHash = HashScopeAttachmentPair(scope.GetId(), attachment.GetId());
        m_scopeAttachmentLookup[attachmentScopeHash].push_back(scopeAttachment);
        return scopeAttachment;
    }

    template <typename AttachmentType>
    const AttachmentType* FrameGraphAttachmentDatabase::FindAttachment(const AttachmentId& attachmentId) const
    {
        return azrtti_cast<const AttachmentType*>(FindAttachment(attachmentId));
    }

    template <typename AttachmentType>
    AttachmentType* FrameGraphAttachmentDatabase::FindAttachment(const AttachmentId& attachmentId)
    {
        return azrtti_cast<AttachmentType*>(FindAttachment(attachmentId));
    }
}
