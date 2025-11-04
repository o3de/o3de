/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>

#include <Atom/RHI.Reflect/UnifiedScopeAttachmentDescriptor.h>

namespace AZ::RHI
{
    UnifiedScopeAttachmentDescriptor::UnifiedScopeAttachmentDescriptor(
        const AttachmentId& attachmentId,
        const BufferViewDescriptor& bufferViewDescriptor,
        const AttachmentLoadStoreAction& loadStoreAction)
        : ScopeAttachmentDescriptor(attachmentId, loadStoreAction)
    {
        SetAsBuffer(bufferViewDescriptor);
    }

    UnifiedScopeAttachmentDescriptor::UnifiedScopeAttachmentDescriptor(
        const AttachmentId& attachmentId,
        const ImageViewDescriptor& imageViewDescriptor,
        const AttachmentLoadStoreAction& loadStoreAction)
        : ScopeAttachmentDescriptor(attachmentId, loadStoreAction)
    {
        SetAsImage(imageViewDescriptor);
    }

    UnifiedScopeAttachmentDescriptor::UnifiedScopeAttachmentDescriptor(
        const AttachmentId& attachmentId,
        const AttachmentId& resolveAttachmentId,
        const ImageViewDescriptor& imageViewDescriptor,
        const AttachmentLoadStoreAction& loadStoreAction)
        : ScopeAttachmentDescriptor(attachmentId, loadStoreAction)
    {
        SetAsResolve(imageViewDescriptor, resolveAttachmentId);
    }

    // Setters...

    void UnifiedScopeAttachmentDescriptor::SetAsBuffer(const BufferViewDescriptor& desc)
    {
        m_bufferViewDescriptor = desc;
        m_type = AttachmentType::Buffer;
    }

    void UnifiedScopeAttachmentDescriptor::SetAsImage(const ImageViewDescriptor& desc)
    {
        m_imageViewDescriptor = desc;
        m_type = AttachmentType::Image;
    }

    void UnifiedScopeAttachmentDescriptor::SetAsResolve(const ImageViewDescriptor& desc, AttachmentId resolveAttachmentId)
    {
        m_imageViewDescriptor = desc;
        m_resolveAttachmentId = resolveAttachmentId;
        m_type = AttachmentType::Resolve;
    }

    // Getters...

    BufferScopeAttachmentDescriptor UnifiedScopeAttachmentDescriptor::GetAsBuffer() const
    {
        AZ_Assert(m_type == AttachmentType::Buffer, "UnifiedScopeAttachmentDescriptor type mismatch during casting");

        return BufferScopeAttachmentDescriptor(m_attachmentId, m_bufferViewDescriptor, m_loadStoreAction);
    }

    ImageScopeAttachmentDescriptor UnifiedScopeAttachmentDescriptor::GetAsImage() const
    {
        AZ_Assert(m_type == AttachmentType::Image, "UnifiedScopeAttachmentDescriptor type mismatch during casting");

        return ImageScopeAttachmentDescriptor(m_attachmentId, m_imageViewDescriptor, m_loadStoreAction);
    }

    ResolveScopeAttachmentDescriptor UnifiedScopeAttachmentDescriptor::GetAsResolve() const
    {
        AZ_Assert(m_type == AttachmentType::Resolve, "UnifiedScopeAttachmentDescriptor type mismatch during casting");

        return ResolveScopeAttachmentDescriptor(m_attachmentId, m_resolveAttachmentId, m_imageViewDescriptor, m_loadStoreAction);
    }

    BufferViewDescriptor& UnifiedScopeAttachmentDescriptor::GetBufferViewDescriptor()
    {
        return m_bufferViewDescriptor;
    }

    ImageViewDescriptor& UnifiedScopeAttachmentDescriptor::GetImageViewDescriptor()
    {
        return m_imageViewDescriptor;
    }

    AttachmentType UnifiedScopeAttachmentDescriptor::GetType() const
    {
        return m_type;
    }
}
