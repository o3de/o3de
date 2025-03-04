/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
 #pragma once

#include <Atom/RHI.Reflect/ImageViewDescriptor.h>
#include <Atom/RHI/Image.h>
#include <Atom/RHI/Resource.h>
#include <Atom/RHI/ResourceView.h>

namespace AZ::RHI
{
    class DeviceImage;

    //! A ImageView is a light-weight representation of a view onto a multi-device image.
    //! It holds a ConstPtr to a multi-device image as well as an ImageViewDescriptor
    //! Using both, single-device ImageViews can be retrieved
    class ImageView : public ResourceView
    {
    public:
        AZ_RTTI(ImageView, "{AB366B8F-F1B7-45C6-A0D8-475D4834FAD2}", ResourceView);
        virtual ~ImageView() = default;

        ImageView(const RHI::Image* image, ImageViewDescriptor descriptor, MultiDevice::DeviceMask deviceMask)
            : ResourceView{ image, deviceMask }
            , m_descriptor{ descriptor }
        {
        }

        //! Given a device index, return the corresponding DeviceImageView for the selected device
        const RHI::Ptr<RHI::DeviceImageView> GetDeviceImageView(int deviceIndex) const;

        //! Return the contained multi-device image
        const RHI::Image* GetImage() const
        {
            return static_cast<const RHI::Image*>(GetResource());
        }

        //! Return the contained ImageViewDescriptor
        const ImageViewDescriptor& GetDescriptor() const
        {
            return m_descriptor;
        }

        const DeviceResourceView* GetDeviceResourceView(int deviceIndex) const override
        {
            return GetDeviceImageView(deviceIndex).get();
        }

    private:
        //! The corresponding ImageViewDescriptor for this view.
        ImageViewDescriptor m_descriptor;
    };
}