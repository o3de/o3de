/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RHI/ImagePool.h>

#include <Atom/RPI.Reflect/Image/Image.h>


namespace AZ
{
    namespace RPI
    {
        class AttachmentImageAsset;
        class AttachmentImagePool;
        class ResourcePoolAsset;

        //! AttachmentImage is intended for use, primarily, as an attachment on a pass. Image data can
        //! be produced by the GPU or uploaded directly from CPU data. Use this class to represent
        //! color / depth stencil targets, read-write images, etc.
        class AttachmentImage final
            : public Image
        {
            friend class ImageSystem;

        public:
            AZ_INSTANCE_DATA(AttachmentImage, "{85691099-5143-4C11-88B0-897DA9064FDF}", Image);
            AZ_CLASS_ALLOCATOR(AttachmentImage, AZ::SystemAllocator, 0);

            ~AttachmentImage() = default;

            //! Instantiates or returns an existing image instance using its paired asset.
            static Data::Instance<AttachmentImage> FindOrCreate(const Data::Asset<AttachmentImageAsset>& imageAsset);

            //! Instantiates a unique instance with a random id, using data provided at runtime.
            static Data::Instance<AttachmentImage> Create(
                const AttachmentImagePool& imagePool,
                const RHI::ImageDescriptor& imageDescriptor,
                const Name& imageName,
                const RHI::ClearValue* optimizedClearValue = nullptr,
                const RHI::ImageViewDescriptor* imageViewDescriptor = nullptr);
            
            const RHI::AttachmentId& GetAttachmentId();

        private:
            AttachmentImage() = default;

            //! Standard instance creation path.
            static Data::Instance<AttachmentImage> CreateInternal(AttachmentImageAsset& imageAsset);
            RHI::ResultCode Init(const AttachmentImageAsset& imageAsset);

            Data::Instance<AttachmentImagePool> m_imagePool;

            RHI::AttachmentId m_attachmentId;
        };
    }
}
