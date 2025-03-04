/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
 #pragma once

 #include <Atom/RHI.Reflect/ImageViewDescriptor.h>
 #include <Atom/RHI/Resource.h>
 #include <Atom/RHI/Image.h>

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
            : m_image{ image }
            , m_descriptor{ descriptor }
            , m_deviceMask{ deviceMask }
        {
        }

        //! Given a device index, return the corresponding DeviceImageView for the selected device
        const RHI::Ptr<RHI::DeviceImageView> GetDeviceImageView(int deviceIndex) const;

        //! Return the contained multi-device image
        const RHI::Image* GetImage() const
        {
            return m_image.get();
        }

        //! Return the contained ImageViewDescriptor
        const ImageViewDescriptor& GetDescriptor() const
        {
            return m_descriptor;
        }

        const Resource* GetResource() const override
        {
            return m_image.get();
        }

        const DeviceResourceView* GetDeviceResourceView(int deviceIndex) const override
        {
            return GetDeviceImageView(deviceIndex).get();
        }

        void Shutdown() final;

    private:
        //! Safe-guard access to DeviceImageView cache during parallel access
        mutable AZStd::mutex m_imageViewMutex;
        //! A raw pointer to a multi-device image
        ConstPtr<RHI::Image> m_image;
        //! The corresponding ImageViewDescriptor for this view.
        ImageViewDescriptor m_descriptor;
        //! The device mask of the image stored for comparison to figure out when cache entries need to be freed.
        mutable MultiDevice::DeviceMask m_deviceMask = MultiDevice::AllDevices;
        //! DeviceImageView cache
        //! This cache is necessary as the caller receives raw pointers from the ResourceCache,
        //! which now, with multi-device objects in use, need to be held in memory as long as
        //! the multi-device view is held.
        mutable AZStd::unordered_map<int, Ptr<RHI::DeviceImageView>> m_cache;
    };
}