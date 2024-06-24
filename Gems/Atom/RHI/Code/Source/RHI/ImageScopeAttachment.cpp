/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/ImageScopeAttachment.h>
#include <Atom/RHI/ImageFrameAttachment.h>
#include <Atom/RHI/DeviceImageView.h>
#include <Atom/RHI/ResolveScopeAttachment.h>
#include <Atom/RHI/Scope.h>

namespace AZ::RHI
{
    ImageScopeAttachment::ImageScopeAttachment(
        Scope& scope,
        FrameAttachment& attachment,
        ScopeAttachmentUsage usage,
        ScopeAttachmentAccess access,
        ScopeAttachmentStage stage,
        const ImageScopeAttachmentDescriptor& descriptor)
        : ScopeAttachment(scope, attachment, usage, access, stage)
        , m_descriptor{ descriptor }
    {
        if (m_descriptor.m_loadStoreAction.m_loadAction == AttachmentLoadAction::Clear ||
            m_descriptor.m_loadStoreAction.m_loadActionStencil == AttachmentLoadAction::Clear)
        {
            AZ_Error("FrameScheduler", CheckBitsAny(access, ScopeAttachmentAccess::Write), "Attempting to clear an attachment that is read-only");
        }
    }

    const ImageScopeAttachmentDescriptor& ImageScopeAttachment::GetDescriptor() const
    {
        return m_descriptor;
    }

    const ImageView* ImageScopeAttachment::GetImageView() const
    {
        return static_cast<const ImageView*>(GetResourceView());
    }

    void ImageScopeAttachment::SetImageView(ConstPtr<ImageView> imageView)
    {
        SetResourceView(AZStd::move(imageView));
    }

    bool ImageScopeAttachment::IsBeingResolved() const
    {
        if(GetUsage() != ScopeAttachmentUsage::RenderTarget)
        {
            return false;
        }

        for (auto& resolveScope : GetScope().GetResolveAttachments())
        {
            if (resolveScope->GetDescriptor().m_resolveAttachmentId == m_descriptor.m_attachmentId)
            {
                return true;
            }
        }

        return false;
    }

    const ImageFrameAttachment& ImageScopeAttachment::GetFrameAttachment() const
    {
        return static_cast<const ImageFrameAttachment&>(ScopeAttachment::GetFrameAttachment());
    }

    ImageFrameAttachment& ImageScopeAttachment::GetFrameAttachment()
    {
        return static_cast<ImageFrameAttachment&>(ScopeAttachment::GetFrameAttachment());
    }

    const ImageScopeAttachment* ImageScopeAttachment::GetPrevious() const
    {
        return static_cast<const ImageScopeAttachment*>(ScopeAttachment::GetPrevious());
    }

    ImageScopeAttachment* ImageScopeAttachment::GetPrevious()
    {
        return static_cast<ImageScopeAttachment*>(ScopeAttachment::GetPrevious());
    }

    const ImageScopeAttachment* ImageScopeAttachment::GetNext() const
    {
        return static_cast<const ImageScopeAttachment*>(ScopeAttachment::GetNext());
    }

    ImageScopeAttachment* ImageScopeAttachment::GetNext()
    {
        return static_cast<ImageScopeAttachment*>(ScopeAttachment::GetNext());
    }

    const ScopeAttachmentDescriptor& ImageScopeAttachment::GetScopeAttachmentDescriptor() const
    {
        return GetDescriptor();
    }
}
