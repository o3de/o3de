/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/ImageViewDescriptor.h>
#include <Atom/RHI/DeviceImageView.h>
#include <Atom/RHI/ResourceView.h>

namespace AZ
{
    namespace RHI
    {
        class Image;

        //! ImageView contains a platform-specific descriptor mapping to a sub-region of an image resource.
        //! It associates 1-to-1 with a ImageViewDescriptor. Image views map to a subset of image sub-resources
        //! (mip levels / array slices). They can additionally override the base format of the image
        class ImageView : public ResourceView
        {
        public:
            AZ_CLASS_ALLOCATOR(ImageView, AZ::SystemAllocator, 0);
            AZ_RTTI(ImageView, "{F2BDEE1F-DEFD-4443-9012-A28AED028D7B}", ResourceView);
            virtual ~ImageView() = default;

            //! Initializes the image view.
            ResultCode Init(const Image& image, const ImageViewDescriptor& viewDescriptor);

            //! Returns the view descriptor used at initialization time.
            const ImageViewDescriptor& GetDescriptor() const;

            //! Returns the image associated with this view.
            const Image& GetImage() const;

            inline Ptr<DeviceImageView> GetDeviceImageView(int deviceIndex) const
            {
                AZ_Assert(
                    m_deviceImageViews.find(deviceIndex) != m_deviceImageViews.end(),
                    "No DeviceImageView found for device index %d\n",
                    deviceIndex);
                return m_deviceImageViews.at(deviceIndex);
            }

            Ptr<DeviceResourceView> GetDeviceResourceView(int deviceIndex) const override;

            //! Returns whether the view covers the entire image (i.e. isn't just a subset).
            bool IsFullView() const override final;

            //! Returns the hash of the view.
            HashValue64 GetHash() const;

        protected:
            HashValue64 m_hash = HashValue64{ 0 };

        private:
            ResultCode InitInternal(const Resource& resource) override;
            void ShutdownInternal() override;
            ResultCode InvalidateInternal() override;

            bool ValidateForInit(const Image& image, const ImageViewDescriptor& viewDescriptor) const;

            // The RHI descriptor for this view.
            ImageViewDescriptor m_descriptor;

            AZStd::unordered_map<int, Ptr<DeviceImageView>> m_deviceImageViews;
        };
    }
}
