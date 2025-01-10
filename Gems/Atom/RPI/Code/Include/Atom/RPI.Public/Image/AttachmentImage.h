/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RHI/ImagePool.h>
#include <Atom/RPI.Public/Configuration.h>
#include <Atom/RPI.Reflect/Image/Image.h>

namespace AZ
{
    namespace RPI
    {
        class AttachmentImageAsset;
        class AttachmentImagePool;
        class ResourcePoolAsset;

        struct CreateAttachmentImageRequest
        {
            //! The name of this image. It will be used as RHI object name (for gpu debug).
            Name m_imageName;
            //! If true, the AttachmentImage create function may fail if the attachment image with same name already exist. 
            //! The m_imageName will be used as the attachment id of the created attachment image.
            //! The attachment image can be found by its name (same as its attachment id).
            //! If false, a random generated uuid will be used for instance id as well as attachment id. 
            bool m_isUniqueName = false;
            //! The ImageDescriptor for this AttachmentImage
            RHI::ImageDescriptor m_imageDescriptor;
            //! The attachment image pool which the AttachmentImage is created from.
            //! A default system attachment image pool will be used if it's set to nullptr
           const AttachmentImagePool* m_imagePool = nullptr;

            // (Optional) Set the default clear value for this image
            const RHI::ClearValue* m_optimizedClearValue = nullptr;
            //! (Optional) The imageViewDescriptor for this image which overides the default ImageViewDescriptor.
            const RHI::ImageViewDescriptor* m_imageViewDescriptor = nullptr;
        };

        //! AttachmentImage is intended for use, primarily, as an attachment on a pass. Image data can
        //! be produced by the GPU or uploaded directly from CPU data. Use this class to represent
        //! color / depth stencil targets, read-write images, etc.
        class ATOM_RPI_PUBLIC_API AttachmentImage final
            : public Image
        {
            friend class ImageSystem;

        public:
            AZ_INSTANCE_DATA(AttachmentImage, "{85691099-5143-4C11-88B0-897DA9064FDF}", Image);
            AZ_CLASS_ALLOCATOR(AttachmentImage, AZ::SystemAllocator);

            ~AttachmentImage();

            //! Instantiates or returns an existing image instance using its paired asset.
            static Data::Instance<AttachmentImage> FindOrCreate(const Data::Asset<AttachmentImageAsset>& imageAsset);

            //! Creates an AttachmentImage
            //! @param imagePool The attachment image pool which the AttachmentImage is created from
            //! @param imageDescriptor The ImageDescriptor for this AttachmentImage
            //! @param imageName The name of this image. It will be used as RHI object name (for gpu debug).
            //! @param optimizedClearValue (Optional) set the default clear value for this image
            //! @param imageViewDescriptor (Optional) The imageViewDescriptor for this image which overides the default ImageViewDescriptor.
            static Data::Instance<AttachmentImage> Create(
                const AttachmentImagePool& imagePool,
                const RHI::ImageDescriptor& imageDescriptor,
                const Name& imageName,
                const RHI::ClearValue* optimizedClearValue = nullptr,
                const RHI::ImageViewDescriptor* imageViewDescriptor = nullptr);

            //! Creates an AttachmentImage
            static Data::Instance<AttachmentImage> Create(const CreateAttachmentImageRequest& createImageRequest);
            
            //! Finds an AttachmentImage by an unique attachment name
            static Data::Instance<AttachmentImage> FindByUniqueName(const Name& uniqueAttachmentName);

            //! Return an unique id which can be used as an attachment id in frame graph attachment database
            const RHI::AttachmentId& GetAttachmentId() const;

        private:
            AttachmentImage();

            //! Standard instance creation path.
            static Data::Instance<AttachmentImage> CreateInternal(AttachmentImageAsset& imageAsset);
            RHI::ResultCode Init(const AttachmentImageAsset& imageAsset);
            void Shutdown();

            Data::Instance<AttachmentImagePool> m_imagePool;

            RHI::AttachmentId m_attachmentId;

            // Need to keep a reference of the asset so the asset system won't release the asset after AttachmentImage was created
            Data::Asset<AttachmentImageAsset> m_imageAsset;
        };
    }
}
