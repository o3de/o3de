/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#include <Atom/RHI/FrameGraphCompileContext.h>
#include <Atom/RHI/FrameGraphAttachmentDatabase.h>
#include <Atom/RHI/Buffer.h>
#include <Atom/RHI/BufferView.h>
#include <Atom/RHI/BufferScopeAttachment.h>
#include <Atom/RHI/Image.h>
#include <Atom/RHI/ImageView.h>
#include <Atom/RHI/ImageScopeAttachment.h>

namespace AZ
{
    namespace RHI
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

        const BufferView* FrameGraphCompileContext::GetBufferView(const AttachmentId& attachmentId, size_t index) const
        {
            const ScopeAttachment* scopeAttacment = m_attachmentDatabase->FindScopeAttachment(m_scopeId, attachmentId, index);
            const BufferScopeAttachment* attachment = azrtti_cast<const BufferScopeAttachment*>(scopeAttacment);
            if (!attachment)
            {
                return nullptr;
            }
            return attachment->GetBufferView();
        }

        const Buffer* FrameGraphCompileContext::GetBuffer(const AttachmentId& attachmentId) const
        {
            const BufferView* bufferView = GetBufferView(attachmentId);
            if (bufferView)
            {
                return &bufferView->GetBuffer();
            }
            return nullptr;
        }

        const ImageView* FrameGraphCompileContext::GetImageView(const AttachmentId& attachmentId, size_t index) const
        {
            const ScopeAttachment* scopeAttacment = m_attachmentDatabase->FindScopeAttachment(m_scopeId, attachmentId, index);
            const ImageScopeAttachment* attachment = azrtti_cast<const ImageScopeAttachment*>(scopeAttacment);
            if (!attachment)
            {
                return nullptr;
            }
            return attachment->GetImageView();
        }

        const Image* FrameGraphCompileContext::GetImage(const AttachmentId& attachmentId) const
        {
            const ImageView* imageView = GetImageView(attachmentId);
            if (imageView)
            {
                return &imageView->GetImage();
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
}