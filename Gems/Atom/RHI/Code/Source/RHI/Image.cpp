/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/DeviceImageView.h>
#include <Atom/RHI/Image.h>
#include <Atom/RHI/ImageFrameAttachment.h>

namespace AZ::RHI
{
    void Image::SetDescriptor(const ImageDescriptor& descriptor)
    {
        m_descriptor = descriptor;
        m_aspectFlags = GetImageAspectFlags(descriptor.m_format);
    }

    const ImageDescriptor& Image::GetDescriptor() const
    {
        return m_descriptor;
    }

    void Image::GetSubresourceLayout(ImageSubresourceLayout& subresourceLayout, ImageAspectFlags aspectFlags) const
    {
        ImageSubresourceRange subresourceRange;
        subresourceRange.m_mipSliceMin = 0;
        subresourceRange.m_mipSliceMax = 0;
        subresourceRange.m_arraySliceMin = 0;
        subresourceRange.m_arraySliceMax = 0;
        subresourceRange.m_aspectFlags = aspectFlags;

        IterateObjects<DeviceImage>([&subresourceRange, &subresourceLayout](auto deviceIndex, auto deviceImage)
        {
            deviceImage->GetSubresourceLayouts(subresourceRange, &subresourceLayout.GetDeviceImageSubresource(deviceIndex), nullptr);
        });
    }

    const ImageFrameAttachment* Image::GetFrameAttachment() const
    {
        return static_cast<const ImageFrameAttachment*>(Resource::GetFrameAttachment());
    }

    Ptr<ImageView> Image::BuildImageView(const ImageViewDescriptor& imageViewDescriptor)
    {
        return aznew ImageView{ this, imageViewDescriptor, GetDeviceMask() };
    }

    uint32_t Image::GetResidentMipLevel() const
    {
        auto minLevel{AZStd::numeric_limits<uint32_t>::max()};
        IterateObjects<DeviceImage>([&minLevel]([[maybe_unused]] auto deviceIndex, auto deviceImage)
        {
            minLevel = AZStd::min(minLevel, deviceImage->GetResidentMipLevel());
        });
        return minLevel;
    }

    bool Image::IsStreamable() const
    {
        bool isStreamable{true};
        IterateObjects<DeviceImage>([&isStreamable]([[maybe_unused]] auto deviceIndex, auto deviceImage)
        {
            isStreamable &= deviceImage->IsStreamable();
        });
        return isStreamable;
    }

    ImageAspectFlags Image::GetAspectFlags() const
    {
        return m_aspectFlags;
    }

    const HashValue64 Image::GetHash() const
    {
        HashValue64 hash = HashValue64{ 0 };
        hash = m_descriptor.GetHash();
        hash = TypeHash64(m_supportedQueueMask, hash);
        hash = TypeHash64(m_aspectFlags, hash);
        return hash;
    }

    void Image::Shutdown()
    {
        Resource::Shutdown();
    }

    bool Image::IsInResourceCache(const ImageViewDescriptor& imageViewDescriptor)
    {
        bool isInResourceCache{true};
        IterateObjects<DeviceImage>([&isInResourceCache, &imageViewDescriptor]([[maybe_unused]] auto deviceIndex, auto deviceImage)
        {
            isInResourceCache &= deviceImage->IsInResourceCache(imageViewDescriptor);
        });
        return isInResourceCache;
    }

    //! Given a device index, return the corresponding DeviceBufferView for the selected device
    const RHI::Ptr<RHI::DeviceImageView> ImageView::GetDeviceImageView(int deviceIndex) const
    {
        AZStd::lock_guard lock(m_imageViewMutex);

        if (m_image->GetDeviceMask() != m_deviceMask)
        {
            m_deviceMask = m_image->GetDeviceMask();

            MultiDeviceObject::IterateDevices(
                m_deviceMask,
                [this](int deviceIndex)
                {
                    if (auto it{ m_cache.find(deviceIndex) }; it != m_cache.end())
                    {
                        m_cache.erase(it);
                    }
                    return true;
                });
        }

        auto iterator{ m_cache.find(deviceIndex) };
        if (iterator == m_cache.end())
        {
            //! Image view is not yet in the cache
            auto [new_iterator, inserted]{ m_cache.insert(
                AZStd::make_pair(deviceIndex, m_image->GetDeviceImage(deviceIndex)->GetImageView(m_descriptor))) };
            if (inserted)
            {
                return new_iterator->second;
            }
        }
        else if (&iterator->second->GetImage() != m_image->GetDeviceImage(deviceIndex).get())
        {
            iterator->second = m_image->GetDeviceImage(deviceIndex)->GetImageView(m_descriptor);
        }

        return iterator->second;
    }

    void ImageSubresourceLayout::Init(MultiDevice::DeviceMask deviceMask, const DeviceImageSubresourceLayout &deviceLayout)
    {
        MultiDeviceObject::IterateDevices(
            deviceMask,
            [this, deviceLayout](int deviceIndex)
            {
                m_deviceImageSubresourceLayout[deviceIndex] = deviceLayout;
                return true;
            });
    }
} // namespace AZ::RHI
