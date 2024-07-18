/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/TransientImageDescriptor.h>
#include <Atom/RHI/FrameAttachment.h>
#include <Atom/RHI/Image.h>
#include <Atom/RHI/ObjectCache.h>
#include <AzCore/Memory/PoolAllocator.h>

namespace AZ::RHI
{
    class ImageScopeAttachment;

    //! A specialization of Attachment for an image. Provides access to the image.
    class ImageFrameAttachment
        : public FrameAttachment
    {
    public:
        AZ_RTTI(ImageFrameAttachment, "{F620A6ED-C33A-4487-9BF7-12F652B8B1E3}", FrameAttachment);
        AZ_CLASS_ALLOCATOR(ImageFrameAttachment, SystemAllocator);
        virtual ~ImageFrameAttachment() override = default;

        //! Initialization for imported images.
        ImageFrameAttachment(const AttachmentId& attachmentId, Ptr<Image> image);

        //! Initialization for transient images.
        ImageFrameAttachment(const TransientImageDescriptor& descriptor);

        //! Returns the first scope attachment in the linked list.
        const ImageScopeAttachment* GetFirstScopeAttachment(int deviceIndex) const;
        ImageScopeAttachment* GetFirstScopeAttachment(int deviceIndex);

        //! Returns the last scope attachment in the linked list.
        const ImageScopeAttachment* GetLastScopeAttachment(int deviceIndex) const;
        ImageScopeAttachment* GetLastScopeAttachment(int deviceIndex);

        //! Returns the image assigned to this attachment. This is not guaranteed to exist
        //! until after frame graph compilation.
        const Image* GetImage() const;
        Image *GetImage();

        //! Returns the image descriptor for this attachment.
        const ImageDescriptor& GetImageDescriptor() const;

        //! Returns an optimized clear value for the image by traversing the usage chain.
        ClearValue GetOptimizedClearValue(int deviceIndex) const;

    private:
        ImageDescriptor m_imageDescriptor;

        /// TODO: Replace with optional. A user clear value in the case of a transient attachment.
        bool m_hasClearValueOverride = false;
        ClearValue m_clearValueOverride;
    };
}
