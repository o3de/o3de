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

    void MultiDeviceImage::InvalidateViews()
    {
        IterateObjects<Image>([]([[maybe_unused]] auto deviceIndex, auto deviceImage)
        {
            deviceImage->InvalidateViews();
        });
    }
} // namespace AZ::RHI
