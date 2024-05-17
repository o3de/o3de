/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI/FrameGraphCompileContext.h>
#include <Atom/RHI/FrameGraphAttachmentDatabase.h>
#include <Atom/RHI/Buffer.h>
#include <Atom/RHI/BufferScopeAttachment.h>
#include <Atom/RHI/Image.h>
#include <Atom/RHI/ImageScopeAttachment.h>

namespace AZ::RHI
{
    FrameGraphCompileContext::FrameGraphCompileContext(
        const ScopeId& scopeId,
        const FrameGraphAttachmentDatabase& attachmentDatabase)
        : m_scopeId{scopeId}
        , m_attachmentDatabase{&attachmentDatabase}
    {}

    bool FrameGraphCompileContext::IsAttachmentValid(const AttachmentId& attachmentId) const
    {
        return m_attachmentDatabase->FindAttachment(attachmentId) != nullptr;
    }

    const size_t FrameGraphCompileContext::GetScopeAttachmentCount(const AttachmentId& attachmentId) const
    {
        const ScopeAttachmentPtrList* scopeAttachmentList = m_attachmentDatabase->FindScopeAttachmentList(m_scopeId, attachmentId);
        if (scopeAttachmentList)
        {
            return scopeAttachmentList->size();
        }
        return 0;
    }

    const BufferView* FrameGraphCompileContext::GetBufferView(const ScopeAttachment* scopeAttacment) const
    {
        const BufferScopeAttachment* attachment = azrtti_cast<const BufferScopeAttachment*>(scopeAttacment);
        if (!attachment)
        {
            return nullptr;
        }
        return attachment->GetBufferView();
    }

    const BufferView* FrameGraphCompileContext::GetBufferView(const AttachmentId& attachmentId) const
    {
        const ScopeAttachment* scopeAttacment = m_attachmentDatabase->FindScopeAttachment(m_scopeId, attachmentId);
        return GetBufferView(scopeAttacment);
    }

    const BufferView* FrameGraphCompileContext::GetBufferView(const AttachmentId& attachmentId, const RHI::ScopeAttachmentUsage attachmentUsage) const
    {
        const ScopeAttachment* scopeAttacment = m_attachmentDatabase->FindScopeAttachment(m_scopeId, attachmentId, attachmentUsage);
        return GetBufferView(scopeAttacment);
    }

    const Buffer* FrameGraphCompileContext::GetBuffer(const AttachmentId& attachmentId) const
    {
        const BufferView* bufferView = GetBufferView(attachmentId);
        if (bufferView)
        {
            return bufferView->GetBuffer();
        }
        return nullptr;
    }

    const ImageView* FrameGraphCompileContext::GetImageView(const ScopeAttachment* scopeAttacment) const
    {
        const ImageScopeAttachment* attachment = azrtti_cast<const ImageScopeAttachment*>(scopeAttacment);
        if (!attachment)
        {
            return nullptr;
        }
        return attachment->GetImageView();
    }

    const ImageView* FrameGraphCompileContext::GetImageView(const AttachmentId& attachmentId, const ImageViewDescriptor& imageViewDescriptor, RHI::ScopeAttachmentUsage attachmentUsage) const
    {
        const ScopeAttachment* scopeAttacment = m_attachmentDatabase->FindScopeAttachment(m_scopeId, attachmentId, imageViewDescriptor, attachmentUsage);
        return GetImageView(scopeAttacment);
    }

    const ImageView* FrameGraphCompileContext::GetImageView(const AttachmentId& attachmentId) const
    {
        const ScopeAttachment* scopeAttacment = m_attachmentDatabase->FindScopeAttachment(m_scopeId, attachmentId);
        return GetImageView(scopeAttacment);
    }

    const Image* FrameGraphCompileContext::GetImage(const AttachmentId& attachmentId) const
    {
        const ImageView* imageView = GetImageView(attachmentId);
        if (imageView)
        {
            return imageView->GetImage();
        }
        return nullptr;
    }

    BufferDescriptor FrameGraphCompileContext::GetBufferDescriptor(const AttachmentId& attachmentId) const
    {
        return m_attachmentDatabase->GetBufferDescriptor(attachmentId);
    }

    ImageDescriptor FrameGraphCompileContext::GetImageDescriptor(const AttachmentId& attachmentId) const
    {
        return m_attachmentDatabase->GetImageDescriptor(attachmentId);
    }

    const ScopeId& FrameGraphCompileContext::GetScopeId() const
    {
        return m_scopeId;
    }
}
