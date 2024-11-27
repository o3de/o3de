/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/ImageFrameAttachment.h>
#include <Atom/RHI/ImageView.h>
#include <Atom/RHI/MultiDeviceImage.h>

namespace AZ::RHI
{
    void MultiDeviceImage::SetDescriptor(const ImageDescriptor& descriptor)
    {
        m_descriptor = descriptor;
        m_aspectFlags = GetImageAspectFlags(descriptor.m_format);
    }

    const ImageDescriptor& MultiDeviceImage::GetDescriptor() const
    {
        return m_descriptor;
    }

    void MultiDeviceImage::GetSubresourceLayout(MultiDeviceImageSubresourceLayout& subresourceLayout, ImageAspectFlags aspectFlags) const
    {
        ImageSubresourceRange subresourceRange;
        subresourceRange.m_mipSliceMin = 0;
        subresourceRange.m_mipSliceMax = 0;
        subresourceRange.m_arraySliceMin = 0;
        subresourceRange.m_arraySliceMax = 0;
        subresourceRange.m_aspectFlags = aspectFlags;

        IterateObjects<Image>([&subresourceRange, &subresourceLayout](auto deviceIndex, auto deviceImage)
        {
            deviceImage->GetSubresourceLayouts(subresourceRange, &subresourceLayout.GetDeviceImageSubresource(deviceIndex), nullptr);
        });
    }

    const ImageFrameAttachment* MultiDeviceImage::GetFrameAttachment() const
    {
        return static_cast<const ImageFrameAttachment*>(MultiDeviceResource::GetFrameAttachment());
    }

    Ptr<MultiDeviceImageView> MultiDeviceImage::BuildImageView(const ImageViewDescriptor& imageViewDescriptor)
    {
        return aznew MultiDeviceImageView{ this, imageViewDescriptor };
    }

    uint32_t MultiDeviceImage::GetResidentMipLevel() const
    {
        auto minLevel{AZStd::numeric_limits<uint32_t>::max()};
        IterateObjects<Image>([&minLevel]([[maybe_unused]] auto deviceIndex, auto deviceImage)
        {
            minLevel = AZStd::min(minLevel, deviceImage->GetResidentMipLevel());
        });
        return minLevel;
    }

    bool MultiDeviceImage::IsStreamable() const
    {
        bool isStreamable{true};
        IterateObjects<Image>([&isStreamable]([[maybe_unused]] auto deviceIndex, auto deviceImage)
        {
            isStreamable &= deviceImage->IsStreamable();
        });
        return isStreamable;
    }

    ImageAspectFlags MultiDeviceImage::GetAspectFlags() const
    {
        return m_aspectFlags;
    }

    const HashValue64 MultiDeviceImage::GetHash() const
    {
        HashValue64 hash = HashValue64{ 0 };
        hash = m_descriptor.GetHash();
        hash = TypeHash64(m_supportedQueueMask, hash);
        hash = TypeHash64(m_aspectFlags, hash);
        return hash;
    }

    void MultiDeviceImage::Shutdown()
    {
        IterateObjects<Image>([]([[maybe_unused]] auto deviceIndex, auto deviceImage)
        {
            deviceImage->Shutdown();
        });

        MultiDeviceResource::Shutdown();
    }

    bool MultiDeviceImage::IsInResourceCache(const ImageViewDescriptor& imageViewDescriptor)
    {
        bool isInResourceCache{true};
        IterateObjects<Image>([&isInResourceCache, &imageViewDescriptor]([[maybe_unused]] auto deviceIndex, auto deviceImage)
        {
            isInResourceCache &= deviceImage->IsInResourceCache(imageViewDescriptor);
        });
        return isInResourceCache;
    }

    //! Given a device index, return the corresponding BufferView for the selected device
    const RHI::Ptr<RHI::ImageView> MultiDeviceImageView::GetDeviceImageView(int deviceIndex) const
    {
        AZStd::lock_guard lock(m_imageViewMutex);
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

        return iterator->second;
    }
} // namespace AZ::RHI
