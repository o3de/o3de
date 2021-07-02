/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/ImagePoolDescriptor.h>
#include <Atom/RHI.Reflect/ClearValue.h>
#include <Atom/RHI/Image.h>
#include <Atom/RHI/ImagePoolBase.h>

namespace AZ
{
    namespace RHI
    {
        /**
         * @brief The data structure used to initialize an RHI::Image on an RHI::ImagePool.
         */
        struct ImageInitRequest
        {
            ImageInitRequest() = default;

            ImageInitRequest(
                Image& image,
                const ImageDescriptor& descriptor,
                const ClearValue* optimizedClearValue = nullptr);

            /// The image to initialize.
            Image* m_image = nullptr;

            /// The descriptor used to initialize the image.
            ImageDescriptor m_descriptor;

            /// An optional, optimized clear value for the image. Certain
            /// platforms may use this value to perform fast clears when this
            /// clear value is used.
            const ClearValue* m_optimizedClearValue = nullptr;
        };

        /**
         * @brief The data structure used to update contents of an RHI::Image on an RHI::ImagePool.
         */
        struct ImageUpdateRequest
        {
            ImageUpdateRequest() = default;

            /// A pointer to an initialized image, whose contents will be updated.
            Image* m_image = nullptr;

            /// The image subresource to update.
            ImageSubresource m_imageSubresource;

            /// The offset in pixels from the start of the sub-resource in the destination image.
            Origin m_imageSubresourcePixelOffset;

            /// The source data pointer
            const void* m_sourceData = nullptr;

            /// The source sub-resource layout.
            ImageSubresourceLayout m_sourceSubresourceLayout;
        };

        /**
         * ImagePool is a pool of images that will be bound as attachments to the frame scheduler.
         * As a result, they are intended to be produced and consumed by the GPU. Persistent Color / Depth Stencil / Image
         * attachments should be created from this pool. This pool is not designed for intra-frame aliasing.
         * If transient images are required, they can be created from the frame scheduler itself.
         */
        class ImagePool
            : public ImagePoolBase
        {
        public:
            AZ_RTTI(ImagePool, "{A5563DF9-191E-4DF7-86BA-CFF39BE07BDD}", ImagePoolBase);
            virtual ~ImagePool() = default;

            /// Initializes the pool. The pool must be initialized before images can be registered with it.
            ResultCode Init(Device& device, const ImagePoolDescriptor& descriptor);

            /// Initializes an image onto the pool. The pool provides backing GPU resources to the image.
            ResultCode InitImage(const ImageInitRequest& request);

            /// Updates image content from the CPU.
            ResultCode UpdateImageContents(const ImageUpdateRequest& request);

            /// Returns the descriptor used to initialize the pool.
            const ImagePoolDescriptor& GetDescriptor() const override final;

        protected:
            ImagePool() = default;

        private:
            using ResourcePool::Init;
            using ImagePoolBase::InitImage;

            bool ValidateUpdateRequest(const ImageUpdateRequest& updateRequest) const;

            //////////////////////////////////////////////////////////////////////////
            // Platform API

            /// Called when the pool is being initialized.
            virtual ResultCode InitInternal(Device& device, const ImagePoolDescriptor& descriptor) = 0;

            /// Called when an image contents are being updated.
            virtual ResultCode UpdateImageContentsInternal(const ImageUpdateRequest& request) = 0;

            /// Called when an image is being initialized on the pool.
            virtual ResultCode InitImageInternal(const ImageInitRequest& request) = 0;

            //////////////////////////////////////////////////////////////////////////

            ImagePoolDescriptor m_descriptor;
        };
    }
}
