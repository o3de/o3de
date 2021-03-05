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

#include <Atom/RHI/ImageScopeAttachment.h>
#include <Atom/RHI/ImageFrameAttachment.h>
#include <Atom/RHI/ImageView.h>
#include <Atom/RHI/ResolveScopeAttachment.h>
#include <Atom/RHI/Scope.h>
namespace AZ
{
    namespace RHI
    {
        ImageScopeAttachment::ImageScopeAttachment(
            Scope& scope,
            FrameAttachment& attachment,
            ScopeAttachmentUsage usage,
            ScopeAttachmentAccess access,
            const ImageScopeAttachmentDescriptor& descriptor)
            : ScopeAttachment(scope, attachment, usage, access)
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
            if(!HasUsage(ScopeAttachmentUsage::RenderTarget))
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
    }
}
