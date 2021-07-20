/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/ImageScopeAttachmentDescriptor.h>
#include <Atom/RHI/ScopeAttachment.h>
#include <AzCore/Memory/PoolAllocator.h>

namespace AZ
{
    namespace RHI
    {
        class ImageView;
        class ImageFrameAttachment;

        
        //! A specialization of a scope attachment for images. Provides
        //! access to the image view and image scope attachment descriptor.
        class ImageScopeAttachment
            : public ScopeAttachment
        {
        public:
            AZ_RTTI(ImageScopeAttachment, "{C2268A3B-BAED-4A63-BB49-E3FF762BA8F0}", ScopeAttachment);
            AZ_CLASS_ALLOCATOR(ImageScopeAttachment, AZ::PoolAllocator, 0);

            ImageScopeAttachment(
                Scope& scope,
                FrameAttachment& attachment,
                ScopeAttachmentUsage usage,
                ScopeAttachmentAccess access,
                const ImageScopeAttachmentDescriptor& descriptor);

            const ImageScopeAttachmentDescriptor& GetDescriptor() const;

            //! Returns the parent graph attachment referenced by this scope attachment.
            const ImageFrameAttachment& GetFrameAttachment() const;
            ImageFrameAttachment& GetFrameAttachment();

            //! Returns the previous scope attachment in the linked list.
            const ImageScopeAttachment* GetPrevious() const;
            ImageScopeAttachment* GetPrevious();

            //! Returns the next scope attachment in the linked list.
            const ImageScopeAttachment* GetNext() const;
            ImageScopeAttachment* GetNext();

            //! Returns the image view set on the scope attachment.
            const ImageView* GetImageView() const;

            //! Assigns an image view to the scope attachment.
            void SetImageView(ConstPtr<ImageView> imageView);

            bool IsBeingResolved() const;

        private:
            ImageScopeAttachmentDescriptor m_descriptor;
        };
    }
}
