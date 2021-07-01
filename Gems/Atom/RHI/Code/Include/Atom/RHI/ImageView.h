/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/ImageViewDescriptor.h>
#include <Atom/RHI/ResourceView.h>

namespace AZ
{
    namespace RHI
    {
        class Image;

        /**
         * ImageView contains a platform-specific descriptor mapping to a sub-region of an image resource.
         * It associates 1-to-1 with a ImageViewDescriptor. Image views map to a subset of image sub-resources
         * (mip levels / array slices). They can additionally override the base format of the image.
         */
        class ImageView
            : public ResourceView
        {
        public:
            AZ_RTTI(ImageView, "{F2BDEE1F-DEFD-4443-9012-A28AED028D7B}", ResourceView);
            virtual ~ImageView() = default;

            /// Initializes the image view.
            ResultCode Init(const Image& image, const ImageViewDescriptor& viewDescriptor);

            /// Returns the view descriptor used at initialization time.
            const ImageViewDescriptor& GetDescriptor() const;

            /// Returns the image associated with this view.
            const Image& GetImage() const;

            /// Returns whether the view covers the entire image (i.e. isn't just a subset).
            bool IsFullView() const override final;

        private:
            bool ValidateForInit(const Image& image, const ImageViewDescriptor& viewDescriptor) const;

            /// The RHI descriptor for this view.
            ImageViewDescriptor m_descriptor;
        };
    }
}
